#include "core/ConfigManager.h"

const char* ConfigManager::NAMESPACE = "petfeeder";
const char* ConfigManager::KEY_WIFI_SSID = "wifi_ssid";
const char* ConfigManager::KEY_WIFI_PASS = "wifi_pass";
const char* ConfigManager::KEY_MQTT_HOST = "mqtt_host";
const char* ConfigManager::KEY_MQTT_PORT = "mqtt_port";
const char* ConfigManager::KEY_MQTT_USER = "mqtt_user";
const char* ConfigManager::KEY_MQTT_PASS = "mqtt_pass";

ConfigManager::ConfigManager(RemoteManager* rm) : remoteManager(rm) {
}

bool ConfigManager::init() {
    Serial.println("[ConfigManager] Inicializando...");

    if (!prefs.begin(NAMESPACE, false)) {
        Serial.println("[ConfigManager] ERRO: Falha ao abrir Preferences!");
        return false;
    }

    Serial.println("[ConfigManager] Preferences inicializado");
    return true;
}

// ========== WiFi ==========

String ConfigManager::getWiFiSSID() {
    return prefs.getString(KEY_WIFI_SSID, "");
}

String ConfigManager::getWiFiPassword() {
    return prefs.getString(KEY_WIFI_PASS, "");
}

void ConfigManager::setWiFiCredentials(const String& ssid, const String& password) {
    prefs.putString(KEY_WIFI_SSID, ssid);
    prefs.putString(KEY_WIFI_PASS, password);
    Serial.printf("[ConfigManager] WiFi salvo: %s\n", ssid.c_str());
}

// ========== MQTT ==========

String ConfigManager::getMQTTHost() {
    return prefs.getString(KEY_MQTT_HOST, "");
}

int ConfigManager::getMQTTPort() {
    return prefs.getInt(KEY_MQTT_PORT, 8883);  // Porta TLS padrão
}

String ConfigManager::getMQTTUser() {
    return prefs.getString(KEY_MQTT_USER, "");
}

String ConfigManager::getMQTTPassword() {
    return prefs.getString(KEY_MQTT_PASS, "");
}

void ConfigManager::setMQTTConfig(const String& host, int port, const String& user, const String& password) {
    prefs.putString(KEY_MQTT_HOST, host);
    prefs.putInt(KEY_MQTT_PORT, port);
    prefs.putString(KEY_MQTT_USER, user);
    prefs.putString(KEY_MQTT_PASS, password);
    Serial.printf("[ConfigManager] MQTT salvo: %s:%d\n", host.c_str(), port);
}

// ========== Persistência de Refeições ==========

bool ConfigManager::saveRemoteConfig(int remoteId) {
    if (!remoteManager) return false;

    RemoteState* remote = remoteManager->getRemote(remoteId);
    if (!remote) {
        Serial.printf("[ConfigManager] Remota %d não encontrada!\n", remoteId);
        return false;
    }

    char key[32];
    for (int i = 0; i < 3; i++) {
        snprintf(key, sizeof(key), "r%d_m%d_h", remoteId, i);
        prefs.putInt(key, remote->meals[i].hour);

        snprintf(key, sizeof(key), "r%d_m%d_m", remoteId, i);
        prefs.putInt(key, remote->meals[i].minute);

        snprintf(key, sizeof(key), "r%d_m%d_q", remoteId, i);
        prefs.putInt(key, remote->meals[i].quantity);

        snprintf(key, sizeof(key), "r%d_m%d_e", remoteId, i);
        prefs.putBool(key, remote->meals[i].enabled);
    }

    Serial.printf("[ConfigManager] Configuração da Remota %d salva\n", remoteId);
    return true;
}

bool ConfigManager::loadRemoteConfig(int remoteId) {
    if (!remoteManager) return false;

    RemoteState* remote = remoteManager->getRemote(remoteId);
    if (!remote) {
        Serial.printf("[ConfigManager] Remota %d não encontrada!\n", remoteId);
        return false;
    }

    char key[32];
    for (int i = 0; i < 3; i++) {
        snprintf(key, sizeof(key), "r%d_m%d_h", remoteId, i);
        remote->meals[i].hour = prefs.getInt(key, 0);

        snprintf(key, sizeof(key), "r%d_m%d_m", remoteId, i);
        remote->meals[i].minute = prefs.getInt(key, 0);

        snprintf(key, sizeof(key), "r%d_m%d_q", remoteId, i);
        remote->meals[i].quantity = prefs.getInt(key, 0);

        snprintf(key, sizeof(key), "r%d_m%d_e", remoteId, i);
        remote->meals[i].enabled = prefs.getBool(key, false);
    }

    Serial.printf("[ConfigManager] Configuração da Remota %d carregada\n", remoteId);
    return true;
}

bool ConfigManager::saveAllRemotes() {
    if (!remoteManager) return false;

    for (int i = 0; i < remoteManager->getRemoteCount(); i++) {
        RemoteState* remote = remoteManager->getRemoteByIndex(i);
        if (remote) {
            saveRemoteConfig(remote->id);
        }
    }
    return true;
}

bool ConfigManager::loadAllRemotes() {
    if (!remoteManager) return false;

    for (int i = 0; i < remoteManager->getRemoteCount(); i++) {
        RemoteState* remote = remoteManager->getRemoteByIndex(i);
        if (remote) {
            loadRemoteConfig(remote->id);
        }
    }
    return true;
}

void ConfigManager::clearAll() {
    prefs.clear();
    Serial.println("[ConfigManager] Todas as configurações foram apagadas!");
}

void ConfigManager::printConfig() {
    Serial.println("\n=== CONFIGURAÇÕES ATUAIS ===");
    Serial.printf("WiFi SSID: %s\n", getWiFiSSID().c_str());
    Serial.printf("MQTT Host: %s:%d\n", getMQTTHost().c_str(), getMQTTPort());
    Serial.printf("MQTT User: %s\n", getMQTTUser().c_str());
    Serial.println("============================\n");
}
