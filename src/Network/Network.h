#pragma once
#include <WiFi.h>
#include <PubSubClient.h>

// --- Configuración WiFi ---
#define WIFI_SSID Settings::doc["wifi"]["stations"][0]["ssid"].as<String>().c_str()
#define WIFI_PASS Settings::doc["wifi"]["stations"][0]["password"].as<String>().c_str()
// Tiempo máximo de conexión (ms)
#define WIFI_TIMEOUT_MS Settings::doc["wifi"]["wifi_timeout_ms"].as<unsigned long>()

// --- Configuración MQTT ---
extern const char *mqttServer;
#define mqttPort Settings::doc["mqtt"]["mqtt_port"].as<int>()
#define mqttUser Settings::doc["mqtt"]["mqtt_user"].as<String>().c_str()
#define mqttPassword Settings::doc["mqtt"]["mqtt_password"].as<String>().c_str()
#define topic_sub Settings::doc["mqtt"]["mqtt_willTopic"].as<String>().c_str()
// Habilitar modo AP si no se conecta a WiFi
#define AP_SSID Settings::doc["wifi"]["ap_ssid"].as<String>().c_str()
#define AP_PASS Settings::doc["wifi"]["ap_password"].as<String>().c_str()
#define wifi_AP Settings::doc["wifi"]["wifi_mode"].as<bool>()
#define ipv4_static Settings::doc["wifi"]["ipv4"].as<String>().c_str()
#define subnet_static Settings::doc["wifi"]["subnet"].as<String>().c_str()
#define gateway_static Settings::doc["wifi"]["gateway"].as<String>().c_str()
// --- Variables globales ---
extern WiFiClient espClient;
extern PubSubClient client;

// Último payload recibido (raw)
extern String last_uplink;

void callback(char *topic, byte *payload, unsigned int length);
void reconnectMQTT();
void publishData(float temperature, float humidity, uint16_t distance_mm);
void connectWiFi();
void connectMQTT();
void Network_loop();
// Inicia el task que procesa cambios en Settings (uplinks MQTT)
void startSettingsTask();