#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"

class ClockService;
class LogService;

class MQTTService {
public:
    MQTTService();

    // Novo begin com ClockService e LogService
    bool begin(ClockService* clock, LogService* log);

    void loop();
    bool isConnected();

    void publishStatus(bool online);
    void publishData(const char* feedLevel);
    void publishLog(const char* logData);
    void publishFeedAck(uint16_t quantity, bool success, const char* source);

    void reconnect();

private:
    WiFiClientSecure wifiClient;
    PubSubClient mqttClient;

    ClockService* clock;
    LogService* log;

    bool connected;
    unsigned long lastReconnectAttempt;
    unsigned long lastStatusPublish;
    unsigned long lastDataPublish;

    void setupWiFi();
    void setupTLS();
    void mqttCallback(char* topic, byte* payload, unsigned int length);
    void handleCommand(const char* topic, const char* payload);

    bool validateFeedQuantity(int qty);
    bool validateMealIndex(int meal);
    bool validateTime(int hour, int minute);
};

extern MQTTService mqttService;

#endif
