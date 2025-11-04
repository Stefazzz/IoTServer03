#include "Network.h"
#include "../core/Logger.h"
#include "../Settings/settings.h"
// FreeRTOS (task / notifications)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// --- Configuración MQTT TTN ---
const char *mqttServer = "62.171.140.128"; // o us1, au1 según tu región
const int mqttPort = 1883;
const char *mqttUser = "GRUPO1";
const char *mqttPassword = "GRUPO1";

// --- Tópico TTN para recibir todos los uplinks ---
const char *topic_sub = "FincaVA/SistemaPiscina/Actuadores"; // o "v3/tu_app_id@ttn/devices/+/up"

// === CAMBIA ESTO POR TU RED ===
const char *WIFI_SSID = "Nicoll";
const char *WIFI_PASS = "38875133";

// Tiempo máximo de conexión (ms)
const unsigned long WIFI_TIMEOUT_MS = 15000;

// MQTT cliente
WiFiClient espClient;
PubSubClient client(espClient);

// Último payload recibido (raw). Declarado extern en Network.h
String last_uplink = "";

// Handle del task que procesa uplinks recibidos por MQTT
static TaskHandle_t settingsTaskHandle = NULL;

// Task que espera notificaciones cuando llega un nuevo uplink.
static void settingsTask(void *pvParameters) {
    (void)pvParameters;
    String prev = "";
    for (;;) {
        // Espera notificación (viene desde callback) o timeout ocasional
        // Se despierta inmediatamente cuando xTaskNotifyGive es llamado.
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(30000)); // 30s timeout

        // Leer el último uplink desde Settings::doc
        String current = "";
        if (Settings::doc.containsKey("last_uplink_raw")) {
            current = Settings::doc["last_uplink_raw"].as<const char*>();
        }

        if (current.length() == 0) {
            // nada que procesar
            continue;
        }

        if (current == prev) {
            // mismo payload que antes, nada nuevo
            continue;
        }

        // Nuevo payload: procesar
        Logger::info(String("[SettingsTask] Nuevo uplink detectado, longitud=") + current.length());

        // Intentar parsear JSON y volcar una representación procesada en Settings::doc
        DynamicJsonDocument doc(2048);
        DeserializationError err = deserializeJson(doc, current);
        if (!err) {
            bool changed = false;
            // Iterar sobre cada campo del JSON recibido
            JsonObject root = doc.as<JsonObject>();
            for (JsonPair kv : root) {
                const char* key = kv.key().c_str();
                // Si el campo existe en Settings::doc, actualizarlo
                if (Settings::doc.containsKey(key)) {
                    // Comparar valores antes de actualizar
                    String oldValue;
                    String newValue;
                    serializeJson(Settings::doc[key], oldValue);
                    serializeJson(kv.value(), newValue);
                    
                    if (oldValue != newValue) {
                        Logger::info(String("[SettingsTask] Actualizando ") + key + 
                                   " de '" + oldValue + "' a '" + newValue + "'");
                        Settings::doc[key] = kv.value();
                        changed = true;
                    }
                }
            }
            
            // Si hubo cambios, guardar en SPIFFS
            if (changed) {
                Logger::info("[SettingsTask] Guardando cambios en settings.json");
                if (Settings::save()) {
                    Logger::info("[SettingsTask] Cambios guardados correctamente");
                } else {
                    Logger::error("[SettingsTask] Error al guardar cambios");
                }
            } else {
                Logger::info("[SettingsTask] No se detectaron cambios en campos conocidos");
            }
        } else {
            Logger::error(String("[SettingsTask] Falló deserializeJson: ") + err.c_str());
        }

        // Actualizamos prev
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
    if (settingsTaskHandle != NULL) {
        // Notificar desde contexto no-ISR: usar xTaskNotifyGive
        xTaskNotifyGive(settingsTaskHandle);
    }
}

void connectMQTT()
{
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);
}

void startSettingsTask() {
    if (settingsTaskHandle != NULL) return; // ya iniciado
    BaseType_t res = xTaskCreatePinnedToCore(
        settingsTask,
        "SettingsTask",
        4096,           // stack size en bytes
        NULL,
        1,              // prioridad
        &settingsTaskHandle,
        1               // core 1
    );
    if (res == pdPASS) {
        Logger::info("SettingsTask iniciado");
    } else {
        Logger::error("No se pudo crear SettingsTask");
        settingsTaskHandle = NULL;
    }
}

void connectWiFi()
{
    // CONECTIVIDAD WIFI
    //  Limpia estado previo y pon modo estación (cliente)
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
        // Si quieres, aquí podrías levantar un AP de emergencia
        // WiFi.mode(WIFI_MODE_AP);
        // WiFi.softAP("ESP32_AP", "adminserver32");
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

void Network_loop() {
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop();
}
