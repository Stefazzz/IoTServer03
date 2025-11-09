#include "settings.h"

DynamicJsonDocument Settings::doc(Settings::kCapacity);


// CREAMOS UN JSON POR DEFECTO
static const char DEFAULT_JSON[] PROGMEM = R"json(
{
    "device_id": "ESP329A9EF0C8F42C",
    "device_name": "espaefcfc",
    "device_user": "admin",
    "device_password": "admin",
    "wifi": {
        "wifi_mode": true,
        "stations": [
            { "ssid": "Nicoll", "password": "38875133", "priority": 1, "ip_static": false },
            {
            "ssid": "Hotspot-Celular", "password": "MiClaveHotspot", "priority": 3,
            "ip_static": true, "ipv4": "192.168.43.50", "subnet": "255.255.255.0",
            "gateway": "192.168.43.1", "dns_primary": "8.8.8.8", "dns_secondary": "8.8.4.4"
        }
        ],
        "defaults": {
            "ip_static": false,
            "ipv4": "192.168.18.150",
            "subnet": "255.255.255.0",
            "gateway": "192.168.18.1",
            "dns_primary": "8.8.8.8",
            "dns_secondary": "8.8.4.4",
            "min_rssi_dbm": -80,
            "scan_interval_s": 30
        },
        "ring": {
            "enabled": true,
            "order": "priority",
            "retries_per_network": 3,
            "connect_timeout_ms": 10000,
            "backoff_ms": 2000,
            "round_robin_on_boot": false,
            "remember_last_success": true
        },
        "ap_ssid": "ESP329A9EF0C8F42C",
        "ap_password": "adminserver32",
        "ap_channel": 9,
        "ap_visibility": true,
        "ap_connect": 4
    },

    "mqtt": {
        "mqtt_enable": false,
        "mqtt_server": "62.171.140.128",
        "mqtt_port": 1883,
        "mqtt_retain": false,
        "mqtt_qos": 0,
        "mqtt_id": "ESP329A9EF0C8F42C",
        "mqtt_user": "emqx",
        "mqtt_password": "public",
        "mqtt_clean_session": true,
        "mqtt_willTopic": "emqx/ESP329A9EF0C8F42C/status",
        "mqtt_willMessage": { "connected": false },
        "mqtt_willQos": 0,
        "mqtt_willRetain": false,
        "mqtt_time_send": true,
        "mqtt_time_interval": 60000,
        "mqtt_status_send": true
    },

    "actuators": {
        "persist_states": true,
        "safe_defaults_on_boot": true,
        "digital": [
            { "name": "relay_1", "state": false, "inverted": true },
            { "name": "relay_2", "state": false, "inverted": true }
        ],
        "analog": {
            "name": "dimmer",
            "enabled": true,
            "value_percent": 0,
            "range": { "min": 0, "max": 100 },
            "smoothing_ms": 0,
            "gamma_correction": 1.0
        }
    },

    "file_version": "2.0.0"
}
)json";

// --- Helpers mínimos ---
static bool writeAll(const char *path, const char *data)
{
    File f = SPIFFS.open(path, "w");
    if (!f)
        return false;
    size_t n = f.print(data);
    f.close();
    return n > 0;
}

static bool loadFromFile(const char *path, DynamicJsonDocument &doc)
{
    File f = SPIFFS.open(path, "r");
    if (!f)
        return false;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    return !err;
}

// --- API simple ---
bool Settings::read()
{
    Logger::info("Settings::read()");
    if (!SPIFFS.exists(kPath))
    {
        Logger::info("No existe settings.json, creando por defecto");
        if (!writeAll(kPath, DEFAULT_JSON))
        {
            Logger::error("No se pudo crear settings.json");
            return false;
        }
    }
    if (!loadFromFile(kPath, doc))
    {
        Logger::error("deserializeJson falló; reiniciando a valores por defecto");
        return reset(true); // repone y persiste
    }
    return true;
}

bool Settings::save()
{
    Logger::info("Settings::save()");
    File f = SPIFFS.open(kPath, "w+");
    if (!f)
    {
        Logger::error(String("No se pudo abrir para escribir: ") + kPath);
        return false;
    }
    serializeJsonPretty(doc, f);
    f.close();
    Logger::info("settings.json guardado");
    return true;
}

bool Settings::reset(bool persist)
{
    Logger::info("Settings::reset()");
    doc.clear();
    DeserializationError err = deserializeJson(doc, DEFAULT_JSON);
    if (err)
    {
        Logger::error(String("deserializeJson(DEFAULT_JSON) falló: ") + err.c_str());
        doc.clear();
        return false;
    }
    return persist ? save() : true;
}

// HELPER DE VISUALIZACION
void Settings::printPrettySerial()
{
    // Imprime tal cual al puerto serial, con saltos de línea
    serializeJsonPretty(doc, Serial);
    Serial.println();
}

void Settings::printPrettyLogger()
{
    // Serializa a un String y lo envía línea por línea al Logger
    String buf;
    serializeJsonPretty(doc, buf);

    if (buf.isEmpty())
    {
        Logger::info("(Settings::doc vacío)");
        return;
    }

    // Recorremos por saltos de línea para que el logger no “aplane” el JSON
    int start = 0;
    while (start < (int)buf.length())
    {
        int nl = buf.indexOf('\n', start);
        if (nl < 0)
            nl = buf.length();
        String line = buf.substring(start, nl);
        Logger::info(line);
        start = nl + 1; // saltamos el '\n'
    }
}

