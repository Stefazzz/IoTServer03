#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "core/Logger.h"
#include "Settings/settings.h"
#include "web/Api.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "Network/Network.h"

AsyncWebServer server(80);

void setup()
{
  Logger::begin(115200);
  delay(200);

  // Red mínima (AP para pruebas)
  // WiFi.mode(WIFI_AP);
  // WiFi.softAP("ESP-IoTServer", "adminserver32");
  // Logger::info(String("AP IP: ") + WiFi.softAPIP().toString());

  // CONECTIVIDAD WIFI
  connectWiFi();
  connectMQTT();
  // Settings
  if (!Settings::begin() || !Settings::read())
  {
    Logger::error("Settings not ready");
  }

  startSettingsTask();
  
  // API (solo settings por ahora)
  setupApi(server);
  server.begin();
  Logger::info("WebServer ASYNC iniciado");

  // INICIAMOS LA LECTURA DE ARCHIVOS
  if (!Settings::begin())
    return;

  if(!Settings::read())
  {
    Logger::error("No se pudo cargar settings.json");
    return;
  }

  Settings::printPrettyLogger();
}

void loop()
{
  Network_loop();

  /*
  // Simulación de datos de los sensores
  float temperature = 200;
  float humidity = 200;

  //Publicar datos cada 15 segundos
  publishData(temperature, humidity);
  delay(15000);
*/
}
