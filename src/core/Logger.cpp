#include "Logger.h"

void Logger::begin(unsigned long baud) {
    Serial.begin(baud);
    while (!Serial) delay(10);
}

void Logger::info(const String &msg) {
    Serial.println("[INFO] " + msg);
}

void Logger::error(const String &msg) {
    Serial.println("[ERROR] " + msg);
}

void Logger::warn(const String &msg) {
    Serial.println("[WARN] " + msg);
}