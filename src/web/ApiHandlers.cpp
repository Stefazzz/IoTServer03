#include "ApiHandlers.h"
#include "../Settings/settings.h"
#include "../core/Logger.h"

namespace ApiHandlers
{
    // ========== Helpers comunes ==========

    void sendJson(AsyncWebServerRequest *req, int code, const String &body)
    {
        auto *res = req->beginResponse(code, "application/json", body);
        res->addHeader("Cache-Control", "no-cache");
        req->send(res);
    }

    void sendError(AsyncWebServerRequest *req, int code, const String &msg)
    {
        // Respuesta de error uniforme
        String s;
        s.reserve(64 + msg.length());
        s  = "{ \"ok\": false, \"error\": \"";
        s += msg;
        s += "\" }";
        sendJson(req, code, s);
    }

    // Envoltorio estándar: { ok:true, code:<n>, data:<payload> }
    void sendJsonEnvelope(AsyncWebServerRequest *req, int httpCode, const JsonDocument &payload, int code)
    {
        // Usar el mismo tamaño que Settings::kCapacity más overhead para el envelope
        size_t cap = Settings::kCapacity + JSON_OBJECT_SIZE(3) + 256;
        DynamicJsonDocument out(cap);
        out["ok"]   = true;
        out["code"] = code;
        out["data"].set(payload.as<JsonVariantConst>());

        String s;
        size_t len = measureJson(out);  // Medir el tamaño exacto necesario
        s.reserve(len + 1);  // +1 para el terminador nulo
        serializeJson(out, s);
        
        sendJson(req, httpCode, s);
    }

    // ========== SETTINGS ==========

    // GET /api/settings  → envuelto con ok/code/data
    void handleGetSettings(AsyncWebServerRequest *req)
    {
        // code = 1: lectura OK
        sendJsonEnvelope(req, 200, Settings::doc, 1);
    }

    // GET /api/last_uplink -> devuelve el último uplink recibido (raw + parsed JSON si existe)
    void handleLastUplink(AsyncWebServerRequest *req)
    {
        DynamicJsonDocument outDoc(2048);

        if (Settings::doc.containsKey("last_uplink_raw")) {
            outDoc["raw"] = Settings::doc["last_uplink_raw"].as<const char*>();
        } else {
            outDoc["raw"] = "";
        }

        if (Settings::doc.containsKey("last_uplink_json")) {
            outDoc["json"].set(Settings::doc["last_uplink_json"].as<JsonVariantConst>());
        } else {
            outDoc["json"] = nullptr;
        }

        // Envolver con la convención { ok, code, data }
        sendJsonEnvelope(req, 200, outDoc, 3);
    }

    // GET /api/settings/download  → attachment “puro” (sin ok/code)
    void handleDownloadSettings(AsyncWebServerRequest *req)
    {
        const char* path = Settings::kPath; // "/settings.json" en tu proyecto
        if (!SPIFFS.exists(path)) {
            sendError(req, 404, "settings_not_found");
            return;
        }

        Logger::info("Descarga del archivo settings.json");
        

        AsyncWebServerResponse *response = req->beginResponse(
            SPIFFS,    
            "/settings.json", 
            "application/json", 
            true
        );
        req->send(response);
    }

    // POST /api/settings (Body: raw JSON)
    // Reemplaza el settings en RAM por el recibido y guarda en SPIFFS.
    void handlePostSettings(AsyncWebServerRequest *req, const String &bodyJson)
    {
        Logger::info(String("POST /api/settings body bytes: ") + bodyJson.length());

        // 1) Parsear el JSON entrante
        DynamicJsonDocument tmp(Settings::kCapacity);
        DeserializationError err = deserializeJson(tmp, bodyJson);
        if (err)
        {
            Logger::error(String("deserializeJson failed: ") + err.c_str());
            sendError(req, 400, "invalid_json");
            return;
        }

        // 2) Validaciones mínimas recomendadas (opcional)
        if (!tmp.containsKey("device_id") || !tmp.containsKey("wifi"))
        {
            sendError(req, 422, "missing_required_fields");
            return;
        }

        // 3) Reemplazar el documento global en RAM
        Settings::doc.clear();
        Settings::doc.set(tmp.as<JsonVariant>());

        // 4) Guardar a SPIFFS
        if (!Settings::save())
        {
            Logger::error("Settings::save() failed");
            sendError(req, 500, "save_failed");
            return;
        }

        Logger::info("settings.json actualizado y guardado correctamente");

        // code = 2: guardado OK
        sendJson(req, 200, "{ \"ok\": true, \"code\": 2 }");
    }

} // namespace ApiHandlers
