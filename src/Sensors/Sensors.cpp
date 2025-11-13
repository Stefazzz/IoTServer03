#include "Sensors.h"
#include <DHT.h>
#include <Wire.h>
#include <VL53L0X.h>
#include "../Settings/settings.h"
#include "../core/Logger.h"
#include "../Actuators/ActuatorControl.h"
#include "../../include/app_pins.h"

// DHT11 setup
static DHT dht(PIN_DHT, DHT11);

// VL53L0X setup (Pololu library)
static VL53L0X lox;
static bool vl53_ok = false;

// Task handle
static TaskHandle_t sensorsTaskHandle = NULL;

static void sensorsTask(void* pv) {
  (void)pv;
  
  // Esperar a que el hardware se estabilice
  vTaskDelay(pdMS_TO_TICKS(2000));
  Logger::info("[SensorsTask] Iniciando lecturas...");
  
  unsigned long lastDht = 0;
  unsigned long lastTof = 0;
  const uint32_t dhtInterval = 10000; // 10s
  const uint32_t tofInterval = 500;   // 500ms (menos frecuente)

  for(;;) {
    unsigned long now = millis();

    // DHT11
    if (now - lastDht >= dhtInterval) {
      lastDht = now;
      float h = 92;
      float t = 12;
      //float h = dht.readHumidity();
      //float t = dht.readTemperature();
      if (true) { //!isnan(h) && !isnan(t))
        // Guardar en Settings::doc (compatibilidad con estructura actual)
        Settings::doc["actuators"]["analog"]["humidity"] = h;
        Settings::doc["actuators"]["analog"]["temp"] = t;
        // También en un bloque sensors nuevo
        Settings::doc["sensors"]["dht11"]["humidity"] = h;
        Settings::doc["sensors"]["dht11"]["temperature"] = t;
        Settings::doc["sensors"]["dht11"]["ts"] = (uint32_t) now;
        Logger::info(String("[DHT11] T=") + t + "C H=" + h + "%");
      } else {
        Logger::warn("[DHT11] Lectura inválida");
      }
    }

    // VL53L0X (Pololu library)
    if (vl53_ok && now - lastTof >= tofInterval) {
      lastTof = now;
      uint16_t dist = lox.readRangeSingleMillimeters();
      if (!lox.timeoutOccurred()) {
        Settings::doc["sensors"]["vl53l0x"]["distance_mm"] = (int)dist;
        Settings::doc["sensors"]["vl53l0x"]["ts"] = (uint32_t) now;
        // Opcional: también exponer en actuators.analog
        Settings::doc["actuators"]["analog"]["distance_mm"] = (int)dist;
        // Logger reducido para no spamear
        // Logger::info(String("[VL53L0X] ") + dist + " mm");
      } else {
        //Logger::warn("[VL53L0X] Timeout");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

namespace Sensors {

void begin() {
  Logger::info("[Sensors] Iniciando módulo de sensores...");
  
  // DHT11
  dht.begin();
  Logger::info(String("[DHT11] Configurado en pin ") + PIN_DHT);
  delay(100);

  // VL53L0X - I2C (Pololu library)
  Logger::info(String("[I2C] Iniciando en SDA=") + PIN_SDA + ", SCL=" + PIN_SCL);
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000); // 100kHz para estabilidad
  delay(100);
  
  lox.setTimeout(500);
  if (!lox.init()) {
    Logger::warn("[VL53L0X] No detectado en I2C (dirección 0x29)");
    Logger::warn("[VL53L0X] Verifica conexiones: SDA->GPIO4, SCL->GPIO5");
    vl53_ok = false;
  } else {
    vl53_ok = true;
    lox.startContinuous();
    Logger::info("[VL53L0X] ✓ Iniciado OK en modo continuo");
  }

  // Crear tarea de lectura
  if (sensorsTaskHandle == NULL) {
    BaseType_t res = xTaskCreatePinnedToCore(
      sensorsTask, 
      "SensorsTask", 
      8192,  // Stack más grande para seguridad
      NULL, 
      1,     // Prioridad baja
      &sensorsTaskHandle, 
      1      // Core 1
    );
    if (res == pdPASS) {
      Logger::info("[Sensors] ✓ SensorsTask creada");
    } else {
      Logger::error("[Sensors] ✗ No se pudo crear SensorsTask");
    }
  }
}

} // namespace Sensors
