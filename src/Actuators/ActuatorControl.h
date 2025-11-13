#pragma once
#include <Arduino.h>
#include "../Settings/settings.h"

namespace ActuatorControl {
    // Inicializa los pines de los actuadores según configuración
    void begin();
    
    // Aplica el estado de los relés digitales desde Settings::doc
    void applyDigitalStates();
    
    // Aplica el valor del dimmer análogo desde Settings::doc
    void applyAnalogValue();
    
    // Aplica todos los estados (digital + analog)
    void applyAllStates();
    
    // Configura un relay específico por nombre
    bool setRelayState(const char* name, bool state);
    
    // Configura el dimmer
    bool setDimmerValue(int percent);
}
