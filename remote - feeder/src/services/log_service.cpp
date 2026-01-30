// log_service.cpp
#include "services/log_service.h"
#include "config.h"
#include "core/ClockService.h"
#include "comm/mqtt_service.h"
#include <ArduinoJson.h>

LogService logService;

LogService::LogService() :
    logCount(0),
    hasPendingLogs(false),
    clock(nullptr) {}

bool LogService::begin(ClockService* clock) {
    this->clock = clock;

    loadLogs();

    LOG("✅ Log Service inicializado");
    LOG("📊 Logs pendentes: " + String(logCount));
    return true;
}

void LogService::addLog(uint32_t timestamp, uint16_t qty, bool delivered, const char* source) {
    if (timestamp == 0) {
        LOG("⚠️ Clock sem horário válido — log registrado com ts=0");
    }

    if (logCount >= MAX_LOGS) {
        LOG("⚠️ Buffer de logs cheio! Sobrescrevendo log mais antigo.");
        for (int i = 0; i < MAX_LOGS - 1; i++) {
            logs[i] = logs[i + 1];
        }
        logCount = MAX_LOGS - 1;
    }

    logs[logCount].timestamp = timestamp;
    logs[logCount].qty = qty;
    logs[logCount].delivered = delivered;
    strncpy(logs[logCount].source, source, sizeof(logs[logCount].source) - 1);
    logs[logCount].source[sizeof(logs[logCount].source) - 1] = '\0';

    logCount++;
    hasPendingLogs = true;

    LOG("📝 Log adicionado: " +
        String(qty) + "g - " +
        (delivered ? "✓ OK" : "✗ FAIL") +
        " - src=" + String(source));

    saveLogs();
}

void LogService::saveLogs() {
    if (!prefs.begin(NVS_LOGS_NAMESPACE, false)) {
        LOG("❌ Erro ao abrir NVS para salvar logs");
        return;
    }

    prefs.putInt("logCount", logCount);

    for (int i = 0; i < logCount; i++) {
        String prefix = "log" + String(i);
        prefs.putUInt((prefix + "_timestamp").c_str(), logs[i].timestamp);
        prefs.putUShort((prefix + "_qty").c_str(), logs[i].qty);
        prefs.putBool((prefix + "_delivered").c_str(), logs[i].delivered);
        prefs.putString((prefix + "_source").c_str(), logs[i].source);
    }

    prefs.end();
    LOG("💾 Logs salvos: " + String(logCount));
}

void LogService::loadLogs() {
    if (!prefs.begin(NVS_LOGS_NAMESPACE, true)) {
        LOG("❌ Erro ao abrir NVS para carregar logs");
        return;
    }

    logCount = prefs.getInt("logCount", 0);
    logCount = constrain(logCount, 0, MAX_LOGS);

    for (int i = 0; i < logCount; i++) {
        String prefix = "log" + String(i);
        logs[i].timestamp = prefs.getUInt((prefix + "_timestamp").c_str(), 0);
        logs[i].qty = prefs.getUShort((prefix + "_qty").c_str(), 0);
        logs[i].delivered = prefs.getBool((prefix + "_delivered").c_str(), false);
        String source = prefs.getString((prefix + "_source").c_str(), "unknown");
        strncpy(logs[i].source, source.c_str(), sizeof(logs[i].source) - 1);
        logs[i].source[sizeof(logs[i].source) - 1] = '\0';
    }

    hasPendingLogs = (logCount > 0);
    prefs.end();

    LOG("📂 Logs carregados: " + String(logCount));
}

void LogService::sendPendingLogsMQTT() {
    if (logCount == 0) {
        hasPendingLogs = false;
        LOG("📊 Nenhum log pendente para enviar");
        return;
    }

    if (!mqttService.isConnected()) {
        LOG("⚠️ MQTT desconectado, aguardando para enviar logs...");
        return;
    }

    LOG("📤 Enviando " + String(logCount) + " logs via MQTT...");

    for (int i = 0; i < logCount; i++) {
        JsonDocument doc;
        doc["deviceId"] = DEVICE_ID;
        doc["timestamp"] = logs[i].timestamp;
        doc["qty"] = logs[i].qty;
        doc["delivered"] = logs[i].delivered;
        doc["source"] = logs[i].source;

        String payload;
        serializeJson(doc, payload);

        mqttService.publishLog(payload.c_str());

        LOG("  Log " + String(i + 1) +
            " | ts=" + String(logs[i].timestamp) +
            " | qty=" + String(logs[i].qty) +
            " | ok=" + String(logs[i].delivered) +
            " | src=" + logs[i].source);

        delay(80);
    }

    LOG("✅ Todos os logs enviados");
    clearLogs();
}

void LogService::clearLogs() {
    logCount = 0;
    hasPendingLogs = false;

    if (!prefs.begin(NVS_LOGS_NAMESPACE, false)) {
        LOG("❌ Erro ao abrir NVS para limpar logs");
        return;
    }

    prefs.clear();
    prefs.end();

    LOG("🧹 Logs apagados");
}

void LogService::loop() {
    static uint32_t lastTry = 0;

    if (!hasPendingLogs) return;

    if (millis() - lastTry < LOG_SEND_RETRY_INTERVAL) return;

    lastTry = millis();

    if (!mqttService.isConnected()) {
        LOG("⏳ MQTT off — aguardando para enviar logs...");
        return;
    }

    sendPendingLogsMQTT();
}

int LogService::getPendingLogsCount() {
    return logCount;
}
