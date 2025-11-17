#include "ActuatorControl.h"
#include "../core/Logger.h"
#include "../../include/app_pins.h"

namespace ActuatorControl {

// Estructura para mapear nombres a pines
struct RelayPin {
    const char* name;
    int pin;
};

// Mapeo de relés a pines GPIO (ajusta según tu hardware)
// Nota: En ESP32-S3, evita usar GPIO 26-32 (USB), GPIO 19-20 (USB), GPIO 0 (boot)
static const RelayPin relayPins[] = {
    {"Motor", PIN_RELAY_1},  // GPIO 21
    {"Led_Jacuzzi", PIN_RELAY_2},  // GPIO 14
    {"Led_Piscina", PIN_RELAY_3}   // GPIO 47 
};

// Mapeo de válvulas a pines GPIO
static const RelayPin valvePins[] = {
    {"chorros", PIN_VALVE_CHORROS},
    {"retorno_jacuzzi", PIN_VALVE_RETORNO_JACUZZI},
    {"desnatador", PIN_VALVE_DESNATADOR},
    {"fondo", PIN_VALVE_FONDO},
    {"cascada", PIN_VALVE_CASCADA}
};

void begin() {
    Logger::info("[ActuatorControl] Inicializando actuadores...");
    
    // Configurar pines de relés (tamaño del array)
    size_t numRelays = sizeof(relayPins) / sizeof(relayPins[0]);
    for (size_t i = 0; i < numRelays; i++) {
        pinMode(relayPins[i].pin, OUTPUT);
        digitalWrite(relayPins[i].pin, LOW);  // Estado inicial apagado
        Logger::info(String("[ActuatorControl] Pin ") + relayPins[i].pin + " configurado para " + relayPins[i].name);
    }
    
    // Configurar pines de válvulas
    size_t numValves = sizeof(valvePins) / sizeof(valvePins[0]);
    for (size_t i = 0; i < numValves; i++) {
        pinMode(valvePins[i].pin, OUTPUT);
        digitalWrite(valvePins[i].pin, LOW);  // Estado inicial cerrado
        Logger::info(String("[ActuatorControl] Pin ") + valvePins[i].pin + " configurado para válvula " + valvePins[i].name);
    }
    
    Logger::info("[ActuatorControl] Actuadores inicializados OK");
}

void applyDigitalStates() {
    if (!Settings::doc.containsKey("actuators")) {
        Logger::warn("[ActuatorControl] No hay sección 'actuators' en settings");
        return;
    }
    if (!Settings::doc["actuators"].containsKey("digital")) {
        Logger::warn("[ActuatorControl] No hay sección 'digital' en actuators");
        return;
    }
    
    JsonArray digitalArray = Settings::doc["actuators"]["digital"].as<JsonArray>();
    
    for (JsonVariant relay : digitalArray) {
        if (!relay.containsKey("name") || !relay.containsKey("state")) continue;
        
        const char* name = relay["name"].as<const char*>();
        bool state = relay["state"].as<bool>();
        bool inverted = relay.containsKey("inverted") ? relay["inverted"].as<bool>() : false;
        
        // Buscar el pin correspondiente
        size_t numRelays = sizeof(relayPins) / sizeof(relayPins[0]);
        for (size_t i = 0; i < numRelays; i++) {
            if (strcmp(relayPins[i].name, name) == 0) {
                bool physicalState = inverted ? !state : state;
                digitalWrite(relayPins[i].pin, physicalState ? HIGH : LOW);
                Logger::info(String("[ActuatorControl] ") + name + " -> " + (state ? "ON" : "OFF") + 
                           " (GPIO " + String(relayPins[i].pin) + " = " + (physicalState ? "HIGH" : "LOW") + ")");
                break;
            }
        }
    }
}

void applyAnalogValue() {
 /*   if (!Settings::doc.containsKey("actuators")) return;
    if (!Settings::doc["actuators"].containsKey("analog")) return;
    
    JsonObject analog = Settings::doc["actuators"]["analog"].as<JsonObject>();
    
    if (!analog.containsKey("enabled") || !analog["enabled"].as<bool>()) {
        ledcWrite(PWM_CHANNEL, 0);
        Logger::info("[ActuatorControl] Dimmer deshabilitado");
        return;
    }
    
    if (!analog.containsKey("value_percent")) return;
    
    int percent = analog["value_percent"].as<int>();
    // Limitar a rango válido
    percent = constrain(percent, 0, 100);
    
    // Convertir porcentaje a valor PWM (0-255)
    int pwmValue = map(percent, 0, 100, 0, 255);
    
    // Aplicar corrección gamma si existe
    if (analog.containsKey("gamma_correction")) {
        float gamma = analog["gamma_correction"].as<float>();
        if (gamma > 0) {
            float normalized = pwmValue / 255.0;
            pwmValue = (int)(pow(normalized, gamma) * 255);
        }
    }
    
    ledcWrite(PWM_CHANNEL, pwmValue);
    Logger::info(String("[ActuatorControl] Dimmer -> ") + percent + "% (PWM=" + pwmValue + ")");*/
}

void applyAllStates() {
    Logger::info("[ActuatorControl] Aplicando todos los estados...");
    applyDigitalStates();
    applyAnalogValue();
}

bool setRelayState(const char* name, bool state) {
    if (!Settings::doc.containsKey("actuators")) return false;
    if (!Settings::doc["actuators"].containsKey("digital")) return false;
    
    JsonArray digitalArray = Settings::doc["actuators"]["digital"].as<JsonArray>();
    
    for (JsonVariant relay : digitalArray) {
        if (!relay.containsKey("name")) continue;
        
        if (strcmp(relay["name"].as<const char*>(), name) == 0) {
            relay["state"] = state;
            
            // Aplicar físicamente
            bool inverted = relay.containsKey("inverted") ? relay["inverted"].as<bool>() : false;
            
            for (const auto& rp : relayPins) {
                if (strcmp(rp.name, name) == 0) {
                    bool physicalState = inverted ? !state : state;
                    digitalWrite(rp.pin, physicalState ? HIGH : LOW);
                    Logger::info(String("[ActuatorControl] Relay ") + name + " -> " + (state ? "ON" : "OFF"));
                    return true;
                }
            }
        }
    }
    
    return false;
}
/*
bool setDimmerValue(int percent) {
    if (!Settings::doc.containsKey("actuators")) return false;
    if (!Settings::doc["actuators"].containsKey("analog")) return false;
    
    percent = constrain(percent, 0, 100);
    Settings::doc["actuators"]["analog"]["value_percent"] = percent;
    
    applyAnalogValue();
    return true;
}
*/

void applyValveStates() {
    if (!Settings::doc.containsKey("actuators")) return;
    if (!Settings::doc["actuators"].containsKey("valves")) return;
    
    JsonArray valvesArray = Settings::doc["actuators"]["valves"].as<JsonArray>();
    
    for (JsonVariant valve : valvesArray) {
        if (!valve.containsKey("name") || !valve.containsKey("state")) continue;
        
        const char* name = valve["name"].as<const char*>();
        bool state = valve["state"].as<bool>();
        
        // Buscar el pin correspondiente
        size_t numValves = sizeof(valvePins) / sizeof(valvePins[0]);
        for (size_t i = 0; i < numValves; i++) {
            if (strcmp(valvePins[i].name, name) == 0) {
                digitalWrite(valvePins[i].pin, state ? HIGH : LOW);
                Logger::info(String("[ActuatorControl] Válvula ") + name + " -> " + (state ? "ABIERTA" : "CERRADA"));
                break;
            }
        }
    }
}

bool setValveState(const char* name, bool state) {
    if (!Settings::doc.containsKey("actuators")) return false;
    if (!Settings::doc["actuators"].containsKey("valves")) return false;
    
    JsonArray valvesArray = Settings::doc["actuators"]["valves"].as<JsonArray>();
    
    for (JsonVariant valve : valvesArray) {
        if (!valve.containsKey("name")) continue;
        
        if (strcmp(valve["name"].as<const char*>(), name) == 0) {
            valve["state"] = state;
            
            // Aplicar físicamente
            for (const auto& vp : valvePins) {
                if (strcmp(vp.name, name) == 0) {
                    digitalWrite(vp.pin, state ? HIGH : LOW);
                    Logger::info(String("[ActuatorControl] Válvula ") + name + " -> " + (state ? "ABIERTA" : "CERRADA"));
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool setPoolMode(const char* modeName) {
    if (!Settings::doc.containsKey("actuators")) return false;
    if (!Settings::doc["actuators"].containsKey("piscina_modes")) return false;
    if (!Settings::doc["actuators"].containsKey("valves")) return false;
    
    JsonObject modes = Settings::doc["actuators"]["piscina_modes"].as<JsonObject>();
    
    if (!modes.containsKey(modeName)) {
        Logger::error(String("[ActuatorControl] Modo desconocido: ") + modeName);
        return false;
    }
    
    JsonObject mode = modes[modeName].as<JsonObject>();
    
    // Actualizar active_pool_mode
    Settings::doc["actuators"]["active_pool_mode"] = modeName;
    
    // Actualizar estados de válvulas según el modo
    JsonArray valvesArray = Settings::doc["actuators"]["valves"].as<JsonArray>();
    
    for (JsonVariant valve : valvesArray) {
        const char* valveName = valve["name"].as<const char*>();
        if (mode.containsKey(valveName)) {
            bool newState = mode[valveName].as<bool>();
            valve["state"] = newState;
        }
    }
    
    // Aplicar físicamente
    applyValveStates();
    
    Logger::info(String("[ActuatorControl] Modo piscina cambiado a: ") + modeName);
    return true;
}

} // namespace ActuatorControl
