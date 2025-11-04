#pragma once
#include <Arduino.h>

class Logger
{
public:
    static void begin(unsigned long baud);
    static void info(const String &msg);
    static void error(const String &msg);
    static void warn(const String &msg);
};