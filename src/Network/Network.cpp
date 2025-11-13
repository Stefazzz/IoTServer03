#include "Network.h"
#include "../core/Logger.h"
#include "../Settings/settings.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../actuators/ActuatorControl.h"
static String lastChangedPath = "";

const char *mqttServer = "62.171.140.128"; 
static String mqtt_server_static = "";

// MQTT cliente
WiFiClient espClient;
PubSubClient client(espClient);

// Último payload recibido (raw). Declarado extern en Network.h
String last_uplink = "";

// Handle del task que procesa uplinks recibidos por MQTT
static TaskHandle_t settingsTaskHandle = NULL;

bool updateJsonRecursive(JsonVariant dest, JsonVariantConst src, const String &path = "")
{
    bool changed = false;

    // Si src es objeto JSON
    if (src.is<JsonObjectConst>())
    {
        JsonObjectConst srcObj = src.as<JsonObjectConst>();
        JsonObject destObj = dest.as<JsonObject>();

        for (JsonPairConst kv : srcObj)
        {
            const char *key = kv.key().c_str();                       // clave actual
            String fullPath = path.length() ? path + "." + key : key; // ruta completa al campo
            //lastChangedPath = fullPath;
            if (!destObj.containsKey(key))
            {
                Logger::warn(String("CAMPO NO EXISTE"));
            }
            else
            {
                changed |= updateJsonRecursive(destObj[key], kv.value(), fullPath); // llamada recursiva
            }
        }
    }

    // Si src es array JSON
    else if (src.is<JsonArrayConst>())
    {
        JsonArrayConst srcArray = src.as<JsonArrayConst>();
        JsonArray destArray = dest.as<JsonArray>();

        size_t i = 0;
        for (JsonVariantConst v : srcArray)
        {
            if (i < destArray.size())
            {
                changed |= updateJsonRecursive(destArray[i], v, path + "[" + String(i) + "]");
            }
            else
            {
                Logger::warn(String("ÍNDICE FUERA DE RANGO: ") + path + "[" + String(i) + "]");
            }
            i++;
        }
    }

    // Si src es valor simple (número, string, bool, etc.)
    else if (src != dest)
    {
        dest.set(src);
        Logger::info(String("[SettingsTask] Actualizando campo: ") + path);
        changed = true;
    }

    return changed;
}

// --- TASK  ---
static void settingsTask(void *pvParameters) // procesa uplinks MQTT
{
    (void)pvParameters; // evitar advertencia de variable no usada
    String prev = "";

    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(30000)); // Espera notificación o timeout

        String current = "";
        if (Settings::doc.containsKey("last_uplink_raw"))
        {
            current = Settings::doc["last_uplink_raw"].as<const char *>();
        }

        if (current.length() == 0 || current == prev)
            continue;

        Logger::info(String("[SettingsTask] Nuevo uplink detectado, longitud=") + current.length());

        DynamicJsonDocument doc(4096);
        DeserializationError err = deserializeJson(doc, current);
        if (err)
        {
            Logger::error(String("[SettingsTask] Error parseando uplink: ") + err.c_str());
            continue;
        }

        bool changed = updateJsonRecursive(Settings::doc, doc);
        if (changed)
        {
            Logger::info("[SettingsTask] Guardando cambios en settings.json");
            //Limpiar campos temporales
            Settings::doc.remove("last_uplink_raw");
            Settings::doc.remove("last_uplink_json");
            
            if (Settings::save())
            {
                Logger::info("[SettingsTask] Cambios guardados correctamente");
                
                // Detectar qué cambió y aplicar acciones físicas
                JsonObject incoming = doc.as<JsonObject>();
                
                // Si cambiaron actuadores, aplicar estados físicos
                if (incoming.containsKey("actuators")) {
                    Logger::info("[SettingsTask] Detectado cambio en actuators, aplicando estados...");
                    // Aplicar cambios en los actuadores físicos
                    extern void applyActuatorChanges();
                    applyActuatorChanges();
                }
                
                // Si cambió WiFi, programar reconexión
                if (incoming.containsKey("wifi")) {
                    Logger::info("[SettingsTask] Detectado cambio en WiFi, aplicando reconexión...");
                    connectWiFi();
                    // Para aplicar cambios WiFi de forma segura, se requiere reinicio
                    // O implementar reconexión en tiempo real (más complejo)
                }
            }
            else
            {
                Logger::error("[SettingsTask] Error al guardar cambios");
            }
        }
        else
        {
            Logger::warn("[SettingsTask] No se detectaron cambios en campos conocidos");
        }

        prev = current;
    }
}

// --- Función de callback (cuando llega un mensaje) ---
void callback(char *topic, byte *payload, unsigned int length)
{
    Logger::info("\n📡 Tópico: ");
    Logger::info(topic);
    Serial.print("📦 Datos: ");
    // Construir el string del payload (reemplazamos el anterior)
    last_uplink = "";
    for (unsigned int i = 0; i < length; i++)
    {
        last_uplink += (char)payload[i];
    }
    Serial.println(last_uplink);

    // Volcar el valor raw al documento global de Settings en RAM para que
    // otros módulos (ej. /api/settings) lo puedan leer sin persistir.
    Settings::doc["last_uplink_raw"] = last_uplink;

    // Intentar parsear JSON del payload y, si es válido, guardar también
    // una representación JSON en Settings::doc["last_uplink_json"]. No
    // persistimos automáticamente para evitar desgaste de SPIFFS.
    DynamicJsonDocument tmp(1024);
    DeserializationError err = deserializeJson(tmp, last_uplink);
    if (!err)
    {
        Settings::doc["last_uplink_json"].set(tmp.as<JsonVariant>());
    }

    // Notificar al task que procesa cambios (si fue creado)
    if (settingsTaskHandle != NULL)
    {
        // Notificar desde contexto no-ISR: usar xTaskNotifyGive
        xTaskNotifyGive(settingsTaskHandle);
    }
}

void connectMQTT()
{
    mqtt_server_static = Settings::doc["mqtt"]["mqtt_server"].as<String>();
    client.setServer(mqtt_server_static.c_str(), mqttPort);
    client.setCallback(callback);
}

void connectWiFi()
{
    // CONECTIVIDAD WIFI
    // Limpia estado previo y pon modo estación (cliente)
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect(true, true);
    delay(100);

    Logger::info(String("Conectando a WiFi: ") + WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Espera (bloqueante) hasta conectar o hasta el timeout
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_TIMEOUT_MS)
    {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("[WiFi] ¡Conectado! IP: %s\n", WiFi.localIP().toString().c_str());
    }
    else
    {
        Serial.println("[WiFi] No se pudo conectar (timeout).");
        // Levantar AP de emergencia con configuración desde settings
        if(wifi_AP){
            Logger::info("[WiFi] Iniciando modo AP...");
            
            // Parsear las IPs desde strings a IPAddress
            IPAddress apIP, apGW, apMask;
            
            // Usar valores por defecto si el parseo falla
            if (!apIP.fromString(ipv4_static)) {
                apIP = IPAddress(192, 168, 4, 1);
                Logger::warn("[WiFi] No se pudo parsear ap_ipv4, usando 192.168.4.1");
            }
            if (!apGW.fromString(gateway_static)) {
                apGW = IPAddress(192, 168, 4, 1);
                Logger::warn("[WiFi] No se pudo parsear ap_gateway, usando 192.168.4.1");
            }
            if (!apMask.fromString(subnet_static)) {
                apMask = IPAddress(255, 255, 255, 0);
                Logger::warn("[WiFi] No se pudo parsear ap_subnet, usando 255.255.255.0");
            }
            
            // Configurar IP estática del AP
            WiFi.mode(WIFI_MODE_AP);
            WiFi.softAPConfig(apIP, apGW, apMask);
            WiFi.softAP(AP_SSID, AP_PASS);
            
            Logger::info(String("✅ [WiFi] AP iniciado. SSID: ") + AP_SSID);
            Logger::info(String("[WiFi] IP del AP: ") + WiFi.softAPIP().toString());
            Logger::info(String("[WiFi] Gateway: ") + apGW.toString());
            Logger::info(String("[WiFi] Subnet: ") + apMask.toString());
        }
    }
}

// --- Reconexión MQTT ---
void reconnectMQTT()
{
    while (!client.connected())
    {
        Logger::info("Conectando a TTN MQTT...");
        if (client.connect("ESP32Client", mqttUser, mqttPassword))
        {
            Logger::info("✅ Conectado a TTN!");
            client.subscribe(topic_sub);
            Logger::info("📡 Suscrito a: ");
            Logger::info(topic_sub);
        }
        else
        {
            Logger::error("❌ Falló (rc=");
            Serial.print(client.state());
            Logger::info("), reintentando...");
            delay(5000);
        }
    }
}

void startSettingsTask()
{
    if (settingsTaskHandle != NULL)
        return; // ya iniciado
    BaseType_t res = xTaskCreatePinnedToCore(
        settingsTask,
        "SettingsTask",
        4096, // stack size en bytes
        NULL, // parámetros
        1,    // prioridad
        &settingsTaskHandle,
        1 // core 1
    );
    if (res == pdPASS)
    {
        Logger::info("SettingsTask iniciado");
    }
    else
    {
        Logger::error("No se pudo crear SettingsTask");
        settingsTaskHandle = NULL;
    }
}
// publicacion de datos (2 datos )
void publishData(float temperature, float humidity)
{
    String payload = String("{ \"TEMP\":" + String(temperature) + ", \"HUM\":" + String(humidity) + "}");

    String topic = String("FincaVA/SistemaPiscina/Actuadores"); // o "v3/tu_app_id@ttn/devices/tu_device_id/up"

    Logger::info("Publicando mensaje: ");
    Logger::info(payload);

    if (client.publish(topic.c_str(), payload.c_str()))
    {
        Logger::info("Mensaje publicado con éxito");
    }
    else
    {
        Logger::error("Error publicando el mensaje");
    }
}
void Network_loop()
{
    if (!client.connected())
    {
        reconnectMQTT();
    }
    client.loop();
}
// Función auxiliar para aplicar cambios en actuadores
void applyActuatorChanges() {
    ActuatorControl::applyAllStates();
}