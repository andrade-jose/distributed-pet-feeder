#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "core/RemoteManager.h"

class ConfigManager {
private:
    Preferences prefs;
    RemoteManager* remoteManager;

    static const char* NAMESPACE;
    static const char* KEY_WIFI_SSID;
    static const char* KEY_WIFI_PASS;
    static const char* KEY_MQTT_HOST;
    static const char* KEY_MQTT_PORT;
    static const char* KEY_MQTT_USER;
    static const char* KEY_MQTT_PASS;

public:
    ConfigManager(RemoteManager* rm);

    // Inicialização
    bool init();

    // WiFi
    String getWiFiSSID();
    String getWiFiPassword();
    void setWiFiCredentials(const String& ssid, const String& password);

    // MQTT
    String getMQTTHost();
    int getMQTTPort();
    String getMQTTUser();
    String getMQTTPassword();
    void setMQTTConfig(const String& host, int port, const String& user, const String& password);

    // Persistência de refeições
    bool saveRemoteConfig(int remoteId);
    bool loadRemoteConfig(int remoteId);
    bool saveAllRemotes();
    bool loadAllRemotes();

    // Utilidades
    void clearAll();
    void printConfig();
};
