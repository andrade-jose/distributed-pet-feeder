#include "comm/PayloadBuilder.h"

String PayloadBuilder::buildCommand(const String& command, int value) {
    JsonDocument doc;
    doc["cmd"] = command;
    if (value > 0) {
        doc["value"] = value;
    }
    doc["timestamp"] = millis();

    String output;
    serializeJson(doc, output);
    return output;
}

String PayloadBuilder::buildFeedCommand(int quantity) {
    JsonDocument doc;
    doc["cmd"] = "FEED";
    doc["quantity"] = quantity;
    doc["timestamp"] = millis();

    String output;
    serializeJson(doc, output);
    return output;
}

String PayloadBuilder::buildMealConfig(int mealIndex, int hour, int minute, int quantity) {
    JsonDocument doc;
    doc["cmd"] = "CONFIG_MEAL";
    doc["meal"] = mealIndex;
    doc["hour"] = hour;
    doc["minute"] = minute;
    doc["quantity"] = quantity;
    doc["timestamp"] = millis();

    String output;
    serializeJson(doc, output);
    return output;
}

String PayloadBuilder::buildCentralStatus(bool online, int remotesOnline, int remotesTotal) {
    JsonDocument doc;
    doc["status"] = online ? "ONLINE" : "OFFLINE";
    doc["remotes_online"] = remotesOnline;
    doc["remotes_total"] = remotesTotal;
    doc["uptime"] = millis();
    doc["timestamp"] = millis();

    String output;
    serializeJson(doc, output);
    return output;
}

String PayloadBuilder::buildSimpleMessage(const String& key, const String& value) {
    JsonDocument doc;
    doc[key] = value;
    doc["timestamp"] = millis();

    String output;
    serializeJson(doc, output);
    return output;
}

String PayloadBuilder::buildSimpleMessage(const String& key, int value) {
    JsonDocument doc;
    doc[key] = value;
    doc["timestamp"] = millis();

    String output;
    serializeJson(doc, output);
    return output;
}