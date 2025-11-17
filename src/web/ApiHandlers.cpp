#include "ApiHandlers.h"
#include "../Settings/settings.h"
#include "../core/Logger.h"
#include "../Actuators/ActuatorControl.h"

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

    // ========== ACTUATORS ==========
    // POST /api/relay  (Body JSON: {"name":"relay_1", "state":true} | {"name":"relay_1", "toggle":true})
    void handleRelayBody(AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total)
    {
        String body;
        body.reserve(len);
        for (size_t i = 0; i < len; i++) body += static_cast<char>(data[i]);

        StaticJsonDocument<256> in;
        DeserializationError err = deserializeJson(in, body);
        if (err) { sendError(req, 400, "invalid_json"); return; }
        if (!in.containsKey("name")) { sendError(req, 422, "missing_name"); return; }

        const char* name = in["name"].as<const char*>();
        bool hasState = in.containsKey("state");
        bool wantToggle = in.containsKey("toggle") ? in["toggle"].as<bool>() : (!hasState);

        bool newState = false;
        if (hasState) {
            newState = in["state"].as<bool>();
        } else if (wantToggle) {
            // Look up current state to invert
            newState = false; // default
            if (Settings::doc.containsKey("actuators") && Settings::doc["actuators"].containsKey("digital")) {
                for (JsonVariant r : Settings::doc["actuators"]["digital"].as<JsonArray>()) {
                    if (r.containsKey("name") && strcmp(r["name"].as<const char*>(), name) == 0) {
                        bool cur = r["state"].as<bool>();
                        newState = !cur;
                        break;
                    }
                }
            }
        }

        bool ok = ActuatorControl::setRelayState(name, newState);
        if (!ok) { sendError(req, 404, "relay_not_found"); return; }

        // Persist if configured
        bool persist = Settings::doc["actuators"]["persist_states"].as<bool>();
        if (persist) Settings::save();

        // Response payload
        StaticJsonDocument<192> out;
        out["name"] = name;
        out["state"] = newState;
        sendJsonEnvelope(req, 200, out, 10); // code=10: relay updated
    }

} // namespace ApiHandlers
