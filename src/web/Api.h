#pragma once
#include <ESPAsyncWebServer.h>

// Llama esto desde setup() para registrar TODAS las rutas
void setupApi(AsyncWebServer& server);
