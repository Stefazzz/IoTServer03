#pragma once
#include <WiFi.h>
#include <PubSubClient.h>

// --- Configuración WiFi ---
extern const char *WIFI_SSID;
extern const char *WIFI_PASS;
extern const unsigned long WIFI_TIMEOUT_MS;

// --- Configuración MQTT ---
extern const char *mqttServer;
extern const int mqttPort;
extern const char *mqttUser;
extern const char *mqttPassword;
extern const char *topic_sub;

// --- Variables globales ---
extern WiFiClient espClient;
extern PubSubClient client;

// Último payload recibido (raw)
extern String last_uplink;

void callback(char *topic, byte *payload, unsigned int length);
void reconnectMQTT();
void publishData(float temperature, float humidity);
void connectWiFi();
void connectMQTT();
void Network_loop();