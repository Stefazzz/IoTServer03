#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

// Solo helpers y handlers de SETTINGS por ahora
namespace ApiHandlers
{
    // helpers comunes
    void sendJson(AsyncWebServerRequest *req, int code, const String &body);
    void sendError(AsyncWebServerRequest *req, int code, const String &msg);

    // SETTINGS
    void handleGetSettings(AsyncWebServerRequest *req);
    void handleDownloadSettings(AsyncWebServerRequest *req);
    void handlePostSettings(AsyncWebServerRequest *req, const String &bodyJson);

    
    void sendJsonEnvelope(AsyncWebServerRequest *req, int httpCode, const JsonDocument &payload, int code);
}
