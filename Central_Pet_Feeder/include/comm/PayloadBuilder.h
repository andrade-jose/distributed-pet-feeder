#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

class PayloadBuilder {
public:
    // Comandos para remotas
    static String buildCommand(const String& command, int value = 0);
    static String buildFeedCommand(int quantity);
    static String buildMealConfig(int mealIndex, int hour, int minute, int quantity);

    // Status da central
    static String buildCentralStatus(bool online, int remotesOnline, int remotesTotal);

    // Utilidades
    static String buildSimpleMessage(const String& key, const String& value);
    static String buildSimpleMessage(const String& key, int value);
};
