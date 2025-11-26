// mqtt_service.cpp
#include "comm/mqtt_service.h"
#include "config.h"
#include "services/log_service.h"
#include "hardware/feeder_service.h"
#include "services/schedule_service.h"
#include "core/ClockService.h"

MQTTService mqttService;

MQTTService::MQTTService() :
    mqttClient(wifiClient),
    connected(false),
    lastReconnectAttempt(0),
    lastStatusPublish(0),
    lastDataPublish(0)
{}

bool MQTTService::begin(ClockService* clockSvc, LogService* logSvc) {
    this->clock = clockSvc;
    this->log = logSvc;

    setupWiFi();
    setupTLS();

    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->mqttCallback(topic, payload, length);
    });

    // Configurar timeout maior para TLS
    mqttClient.setSocketTimeout(30);
    mqttClient.setKeepAlive(60);

    LOG("✅ MQTT Service inicializado com TLS");
    return true;
}

void MQTTService::setupWiFi() {
    LOG_START("Conexão WiFi");
    LOG_KV("SSID", WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    LOG_INFO("Aguardando conexão WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        LOG_NO_NL(".");
        attempts++;
        if (attempts % 10 == 0) Serial.println();
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        LOG_SUCCESS("WiFi conectado");
        LOG_KV("IP", WiFi.localIP().toString());
        LOG_KV("RSSI", String(WiFi.RSSI()) + " dBm");
        LOG_KV("Tentativas", String(attempts));
    } else {
        LOG_ERROR("Falha na conexão WiFi após " + String(attempts) + " tentativas");
    }
}

void MQTTService::setupTLS() {
    LOG_START("Configuração TLS");

    #if MQTT_VALIDATE_CERT
        wifiClient.setCACert(MQTT_ROOT_CA);
        wifiClient.setTimeout(30);
        LOG_SUCCESS("TLS configurado com validação de certificado");
        LOG_KV("Modo", "Seguro (valida certificado)");
    #else
        wifiClient.setInsecure();  // Aceita qualquer certificado
        wifiClient.setTimeout(30);
        LOG_SUCCESS("TLS configurado sem validação de certificado");
        LOG_KV("Modo", "Inseguro (aceita qualquer certificado)");
    #endif

    LOG_KV("Timeout", "30s");
}

void MQTTService::mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Converter payload para string
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    LOG_MQTT_IN(topic, message);

    // Processar apenas comandos do tópico correto
    if (String(topic) == TOPIC_CMD) {
        handleCommand(topic, message);
    } else {
        LOG_WARN("Tópico ignorado: " + String(topic));
    }
}

void MQTTService::handleCommand(const char* topic, const char* payload) {
    LOG_SEPARATOR();
    LOG_INFO("Processando comando MQTT");

    // Parse JSON usando ArduinoJson
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        LOG_ERROR("Falha ao parsear JSON: " + String(error.c_str()));
        publishStatus(true);  // Confirma online mas houve erro
        LOG_SEPARATOR();
        return;
    }

    // Extrair comando
    String cmd = doc["cmd"] | "";

    if (cmd.isEmpty()) {
        LOG_ERROR("Campo 'cmd' não encontrado no JSON");
        LOG_SEPARATOR();
        return;
    }

    LOG_KV("Comando", cmd);

    // ========== COMANDO: FEED (Alimentação Manual) ==========
    if (cmd == "FEED") {
        int quantity = doc["quantity"] | 100;
        LOG_KV("Quantidade", String(quantity) + "g");

        // Validar quantidade
        if (!validateFeedQuantity(quantity)) {
            LOG_ERROR("Quantidade inválida: " + String(quantity) + "g (min: " +
                     String(MIN_FEED_QUANTITY) + "g, max: " + String(MAX_FEED_QUANTITY) + "g)");
            publishStatus(true);
            LOG_SEPARATOR();
            return;
        }

        LOG_START("Alimentação manual");

        // Executar alimentação
        bool success = feederService.dispense(quantity);

        // Publicar status de confirmação
        publishStatus(true);

        if (success) {
            LOG_COMPLETE("Alimentação manual");
        } else {
            LOG_WARN("Alimentação com possíveis problemas");
        }
        LOG_SEPARATOR();
    }

    // ========== COMANDO: CONFIG_MEAL (Configurar Refeição) ==========
    else if (cmd == "CONFIG_MEAL") {
        int meal = doc["meal"] | 0;
        int hour = doc["hour"] | 0;
        int minute = doc["minute"] | 0;
        int quantity = doc["quantity"] | 0;

        LOG_KV("Refeição", String(meal));
        LOG_KV("Horário", String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute));
        LOG_KV("Quantidade", String(quantity) + "g");

        // Validações
        if (!validateMealIndex(meal)) {
            LOG_ERROR("Índice de refeição inválido: " + String(meal) + " (válido: 0-" + String(MAX_MEALS-1) + ")");
            publishStatus(true);
            LOG_SEPARATOR();
            return;
        }

        if (!validateTime(hour, minute)) {
            LOG_ERROR("Horário inválido: " + String(hour) + ":" + String(minute));
            publishStatus(true);
            LOG_SEPARATOR();
            return;
        }

        if (!validateFeedQuantity(quantity)) {
            LOG_ERROR("Quantidade inválida: " + String(quantity) + "g");
            publishStatus(true);
            LOG_SEPARATOR();
            return;
        }

        LOG_START("Configuração de refeição");

        // Configurar refeição no ScheduleService
        scheduleService.setMeal(meal, hour, minute, quantity, true);  // enabled = true por padrão

        // Confirmar execução
        publishStatus(true);
        LOG_COMPLETE("Refeição configurada");
        LOG_SEPARATOR();
    }

    // ========== COMANDO: SYNC (Sincronizar Logs) ==========
    else if (cmd == "SYNC") {
        LOG_START("Sincronização de logs");
        publishStatus(true);

        int pendingLogs = logService.getPendingLogsCount();
        LOG_KV("Logs pendentes", String(pendingLogs));

        if (pendingLogs > 0) {
            logService.sendPendingLogsMQTT();
            LOG_COMPLETE("Sincronização de logs");
        } else {
            LOG_INFO("Nenhum log pendente para enviar");
        }
        LOG_SEPARATOR();
    }

    // ========== COMANDO: STATUS (Solicitar Status) ==========
    else if (cmd == "STATUS") {
        LOG_START("Envio de status");
        publishStatus(true);
        publishData("OK");  // Publicar telemetria também
        LOG_COMPLETE("Status enviado");
        LOG_SEPARATOR();
    }

    // ========== COMANDO DESCONHECIDO ==========
    else {
        LOG_ERROR("Comando desconhecido: " + cmd);
        publishStatus(true);
        LOG_SEPARATOR();
    }
}

// ========== VALIDAÇÕES ==========

bool MQTTService::validateFeedQuantity(int qty) {
    return (qty >= MIN_FEED_QUANTITY && qty <= MAX_FEED_QUANTITY);
}

bool MQTTService::validateMealIndex(int meal) {
    return (meal >= 0 && meal < MAX_MEALS);
}

bool MQTTService::validateTime(int hour, int minute) {
    return (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59);
}

// ========== LOOP PRINCIPAL ==========

void MQTTService::loop() {
    unsigned long now = millis();

    // Reconnect se necessário
    if (!mqttClient.connected()) {
        reconnect();
    } else {
        mqttClient.loop();

        // Publicar status periodicamente
        if (now - lastStatusPublish >= STATUS_PUBLISH_INTERVAL) {
            lastStatusPublish = now;
            publishStatus(true);
        }

        // Publicar telemetria periodicamente
        if (now - lastDataPublish >= DATA_PUBLISH_INTERVAL) {
            lastDataPublish = now;
            // TODO: Integrar com sensor HC-SR04 real
            publishData("OK");  // Por enquanto sempre OK
        }
    }
}

bool MQTTService::isConnected() {
    return connected && mqttClient.connected();
}

// ========== RECONNECT ==========

void MQTTService::reconnect() {
    unsigned long now = millis();
    if (now - lastReconnectAttempt < 15000) {
        return;
    }
    lastReconnectAttempt = now;

    if (WiFi.status() != WL_CONNECTED) {
        LOG_WARN("WiFi desconectado, tentando reconectar");
        setupWiFi();
        return;
    }

    LOG_SEPARATOR();
    LOG_START("Conexão MQTT");
    LOG_KV("Broker", String(MQTT_BROKER) + ":" + String(MQTT_PORT));
    LOG_KV("Usuário", MQTT_USER);

    String clientId = "ESP32-S3-" + String(DEVICE_ID) + "-" + String(random(0xffff), HEX);
    LOG_KV("Client ID", clientId);

    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
        connected = true;
        LOG_SUCCESS("MQTT conectado com TLS!");

        // Inscrever no tópico de comandos
        mqttClient.subscribe(TOPIC_CMD);
        LOG_KV("Inscrito", TOPIC_CMD);

        // Publicar status online
        publishStatus(true);

        // Enviar logs pendentes
        int pendingLogs = logService.getPendingLogsCount();
        if (pendingLogs > 0) {
            LOG_INFO("Enviando " + String(pendingLogs) + " logs pendentes");
            logService.sendPendingLogsMQTT();
        } else {
            LOG_DEBUG("Nenhum log pendente");
        }
        LOG_SEPARATOR();

    } else {
        connected = false;
        int state = mqttClient.state();
        LOG_ERROR("Falha na conexão MQTT");
        LOG_KV("Código de erro", String(state));

        String errorMsg = "";
        switch(state) {
            case -4: errorMsg = "Problema com TLS/Certificado"; break;
            case -2: errorMsg = "Problema de autenticação"; break;
            case -1: errorMsg = "Não conseguiu conectar no broker"; break;
            case 1: errorMsg = "Protocolo incorreto"; break;
            case 2: errorMsg = "Client ID rejeitado"; break;
            case 4: errorMsg = "Usuário/senha inválidos"; break;
            case 5: errorMsg = "Não autorizado"; break;
            default: errorMsg = "Erro desconhecido";
        }
        LOG_KV("Diagnóstico", errorMsg);
        LOG_SEPARATOR();
    }
}

// ========== PUBLICAÇÕES (COMPATÍVEL COM PROTOCOLO DA CENTRAL) ==========

void MQTTService::publishStatus(bool online) {
    if (!mqttClient.connected()) return;

    // Formato esperado pela Central: {"online": true/false, "timestamp": 12345}
    JsonDocument doc;
    doc["online"] = online;
    doc["timestamp"] = clock ? clock->getTimestamp() : 0;

    String payload;
    serializeJson(doc, payload);

    if (mqttClient.publish(TOPIC_STATUS, payload.c_str())) {
        LOG_MQTT_OUT("STATUS", String(online ? "ONLINE" : "OFFLINE"));
    } else {
        LOG_ERROR("Falha ao publicar status");
    }
}

void MQTTService::publishData(const char* feedLevel) {
    if (!mqttClient.connected()) return;

    // Formato esperado pela Central: {"feed_level": "OK/LOW/EMPTY", "timestamp": 12345}
    JsonDocument doc;
    doc["feed_level"] = feedLevel;
    doc["timestamp"] = clock ? clock->getTimestamp() : 0;

    String payload;
    serializeJson(doc, payload);

    if (mqttClient.publish(TOPIC_DATA, payload.c_str())) {
        LOG_MQTT_OUT("DATA", "feed_level=" + String(feedLevel));
    } else {
        LOG_ERROR("Falha ao publicar telemetria");
    }
}

void MQTTService::publishLog(const char* logData) {
    if (!mqttClient.connected()) return;

    // LogData já vem no formato JSON correto do LogService
    if (mqttClient.publish(TOPIC_LOGS, logData)) {
        LOG_MQTT_OUT("LOGS", "Log enviado");
    } else {
        LOG_ERROR("Falha ao publicar log");
    }
}

void MQTTService::publishFeedAck(uint16_t quantity, bool success, const char* source) {
    if (!mqttClient.connected()) return;

    // Formato: {"device_id": "remote1", "quantity": 100, "success": true, "source": "manual/schedule", "timestamp": 12345}
    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["quantity"] = quantity;
    doc["success"] = success;
    doc["source"] = source;
    doc["timestamp"] = clock ? clock->getTimestamp() : 0;

    String payload;
    serializeJson(doc, payload);

    if (mqttClient.publish(TOPIC_FEED_ACK, payload.c_str())) {
        LOG_MQTT_OUT("FEED_ACK", "qty=" + String(quantity) + "g, " + (success ? "✓" : "✗") + ", src=" + String(source));
    } else {
        LOG_ERROR("Falha ao publicar confirmação de alimentação");
    }
}