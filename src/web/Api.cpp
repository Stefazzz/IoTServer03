#include "Api.h"
#include "ApiHandlers.h"
#include "../core/Logger.h"
#include "../Settings/settings.h"
#include <SPIFFS.h>

using namespace ApiHandlers;

String GetBodyContent(uint8_t *data, size_t len)
{
    String body;
    body.reserve(len);
    for (size_t i = 0; i < len; i++)
    {
        body += static_cast<char>(data[i]);
    }
    return body;
}

static void enableCORS()
{
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

// Acumula el body (ESPAsyncWebServer lo entrega por chunks)
static void handleBodyUpload(AsyncWebServerRequest *req,
                             uint8_t *data, size_t len, size_t index, size_t total)
{
    String Body = GetBodyContent(data, len);
    StaticJsonDocument<768> info_doc;
    DeserializationError error = deserializeJson(info_doc, Body);
    if (error)
    {
        // LOG_ERROR("API: Invalid JSON in body");
        sendError(req, 400, "invalid_json");
        return;
    }

    Settings::doc["device_id"] = info_doc["device_id"];
    if (Settings::save())
    {
        req->send(200, "application/json", "{\"save\": true}");
    }
    else
    {
        sendError(req, 500, "save_failed");
    }
    Settings::printPrettyLogger();

    /*Settings::doc["device_password"] = info_doc["device_password"] ;
    if(Settings::save()){
        req ->send(200, "application/json","{\"save\": true}");
    } else {
        sendError(req, 500, "save_failed");
    }
    Settings::printPrettyLogger();
*/
}

void setupApi(AsyncWebServer &server)
{
    enableCORS();

    // Servir archivos estáticos (CSS, JS, img) desde SPIFFS
    // Esto permite que peticiones como /assets/js/header.js devuelvan el archivo
    server.serveStatic("/assets", SPIFFS, "/assets");
    // Servir el root desde SPIFFS (index.html por defecto)
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // Responder preflight CORS / not found
    server.onNotFound([](AsyncWebServerRequest *req)
                    {
    if (req->method() == HTTP_OPTIONS) { req->send(204); return; }
    sendError(req, 404, "not_found"); });
    // -------- Pagina principal --------
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                {
        if (SPIFFS.exists("/index.html")) {
            request->send(SPIFFS, "/index.html", "text/html");
        } else {
            request->send(200, "text/html",
                "<h1>IoT Server</h1><p>/data/index.html no encontrado</p>");
        } });
    // -------- HEALTH CHECK --------
    server.on("/api/health", HTTP_GET, [](AsyncWebServerRequest *req)
              {
                  String json = String("{\"ok\":true,\"fw\":\"") +  + "\"}";
                  AsyncWebServerResponse *response = req->beginResponse(200, "application/json", json);
                  response->addHeader("Access-Control-Allow-Origin", "*");
                  req->send(response); });
    // -------- SETTINGS --------
    // GET /api/settings
    server.on("/api/settings", HTTP_GET, handleGetSettings);

    // GET /api/last_uplink
    server.on("/api/last_uplink", HTTP_GET, handleLastUplink);

    // GET /api/settings_download
    server.on("/api/settings_download", HTTP_GET, handleDownloadSettings);

    // POST /api/settings  (Body: raw JSON)
    server.on(
        "/api/settings",

        HTTP_POST,
        [](AsyncWebServerRequest *req) {},
        nullptr,
        handleBodyUpload);
}
