#pragma once
#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "../core/Logger.h"
#include "../Network/Network.h"
namespace Settings
{
    // Ruta y tamaño del documento (ajústalo si tu JSON crece)
    static constexpr const char *kPath = "/settings.json";
    static constexpr size_t kCapacity = 24 * 1024;

    // Documento global en RAM para que todo el proyecto lo use
    extern DynamicJsonDocument doc;

    // (Opcional) Monta SPIFFS y asegura que exista el archivo
    inline bool begin(bool formatOnFail = true)
    {
        if (!SPIFFS.begin(formatOnFail))
        {
            Logger::error("SPIFFS.begin() falló");
            return false;
        }
        return true;
    }

    // 1) Leer settings.json -> doc (si no existe/corrupto, crea por defecto y guarda)
    bool read();

    // 2) Guardar el doc actual -> settings.json
    bool save();

    // 3) Reiniciar a valores por defecto (RAM). Si persist==true, también guarda.
    bool reset(bool persist = true);

    // HELPERS DE VISUALIZACION
    //  Imprime el JSON (RAM) en formato pretty hacia Serial directamente
    void printPrettySerial();

    // Imprime el JSON (RAM) en formato pretty pero línea por línea con Logger::info
    void printPrettyLogger();
}
