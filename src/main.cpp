#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "core/Logger.h"
#include "Settings/settings.h"
#include "web/Api.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "Network/Network.h"
#include "Actuators/ActuatorControl.h"

AsyncWebServer server(80);

void setup()
{
  Logger::begin(115200);
  delay(200);

  // Red mínima (AP para pruebas)
  // WiFi.mode(WIFI_AP);
  // WiFi.softAP("ESP-IoTServer", "adminserver32");
  // Logger::info(String("AP IP: ") + WiFi.softAPIP().toString());

  // Settings
  if (!Settings::begin() || !Settings::read())
  {
    Logger::error("Settings not ready");
  }

  startSettingsTask();
  
    // CONECTIVIDAD WIFI
  connectWiFi();
  connectMQTT();

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
  

  // Inicializar control de actuadores
  Logger::info("=== Inicializando hardware de actuadores ===");
  ActuatorControl::begin();
  Logger::info("=== Hardware inicializado OK ===");
  
    // CONECTIVIDAD WIFI
  //connectWiFi();
  //connectMQTT();

  // Comentado temporalmente para evitar reinicio - aplicar después del setup completo
  // ActuatorControl::applyAllStates();
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
