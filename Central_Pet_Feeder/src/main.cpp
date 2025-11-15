#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "mqtt_cert.h"  // Certificado TLS do Mosquitto

// Core
#include "core/RemoteManager.h"
#include "core/ClockService.h"
#include "core/ConfigManager.h"

// Communication
#include "comm/MQTTClient.h"
#include "comm/PayloadBuilder.h"

// UI
#include "ui/LCDRenderer.h"
#include "ui/MenuController.h"

// HAL
#include "hal/Buttons.h"
#include "hal/ScreenBacklight.h"

// ========== INSTÂNCIAS GLOBAIS ==========

// Core
RemoteManager remoteManager;
ClockService clockService;
ConfigManager configManager(&remoteManager);

// Communication
MQTTClient mqttClient;

// UI
LCDRenderer lcdRenderer;
Buttons buttons;
MenuController menuController(&lcdRenderer, &remoteManager, &clockService, &buttons);
ScreenBacklight screenBacklight(lcdRenderer.getLCD());

// ========== VARIÁVEIS DE CONTROLE ==========
unsigned long lastClockUpdate = 0;
unsigned long lastMQTTStatusPublish = 0;
unsigned long lastScreenUpdate = 0;

const unsigned long CLOCK_UPDATE_INTERVAL = 1000;      // 1 segundo
const unsigned long MQTT_STATUS_INTERVAL = 30000;      // 30 segundos

// ========== FUNÇÕES AUXILIARES ==========

// Publica estado completo da central para o Dashboard
void publishCentralStateToDA() {
    JsonDocument doc;

    doc["timestamp"] = millis();
    doc["status"] = "ONLINE";
    doc["uptime"] = millis();
    doc["remotes_count"] = remoteManager.getRemoteCount();
    doc["remotes_online"] = remoteManager.getOnlineCount();

    // Array de remotas com todas as informações
    JsonArray remotesArray = doc["remotes"].to<JsonArray>();

    for (int i = 0; i < remoteManager.getRemoteCount(); i++) {
        RemoteState* remote = remoteManager.getRemoteByIndex(i);
        if (!remote) continue;

        JsonObject remoteObj = remotesArray.add<JsonObject>();
        remoteObj["id"] = remote->id;
        remoteObj["name"] = remote->name;
        remoteObj["online"] = remoteManager.isRemoteActive(remote->id);
        remoteObj["feed_level"] = remote->feedLevel;
        remoteObj["last_seen"] = remote->lastSeen;

        // Array de refeições
        JsonArray mealsArray = remoteObj["meals"].to<JsonArray>();
        for (int j = 0; j < 3; j++) {
            JsonObject mealObj = mealsArray.add<JsonObject>();
            mealObj["hour"] = remote->meals[j].hour;
            mealObj["minute"] = remote->meals[j].minute;
            mealObj["quantity"] = remote->meals[j].quantity;
            mealObj["enabled"] = remote->meals[j].enabled;
        }
    }

    String payload;
    serializeJson(doc, payload);

    mqttClient.publish(MQTT_TOPIC_CENTRAL_STATUS, payload, true);  // com retain
    Serial.println("[DASHBOARD] Estado completo publicado");
}

// ========== CALLBACK MQTT ==========

void onMQTTMessage(const String& topic, const String& payload) {
    Serial.printf("[MQTT] Mensagem recebida: %s -> %s\n", topic.c_str(), payload.c_str());

    // Parse do payload JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.printf("[MQTT] Erro ao parsear JSON: %s\n", error.c_str());
        return;
    }

    // ========== COMANDOS DO DASHBOARD ==========
    // Tópico: petfeeder/central/cmd
    if (topic == MQTT_TOPIC_CENTRAL_CMD) {
        String cmd = doc["cmd"] | "";

        if (cmd == "CONFIG_MEAL") {
            // Dashboard enviou configuração de refeição
            int remoteId = doc["remote_id"] | 0;
            int mealIndex = doc["meal"] | 0;
            int hour = doc["hour"] | 0;
            int minute = doc["minute"] | 0;
            int quantity = doc["quantity"] | 0;

            Serial.printf("[DASHBOARD] Configurar refeição: Remota %d, R%d = %02d:%02d (%dg)\n",
                          remoteId, mealIndex + 1, hour, minute, quantity);

            // Atualizar na central
            remoteManager.setMealSchedule(remoteId, mealIndex, hour, minute, quantity);
            configManager.saveRemoteConfig(remoteId);

            // Enviar para a remota
            String remoteCmdPayload = PayloadBuilder::buildMealConfig(mealIndex, hour, minute, quantity);
            char remoteTopic[64];
            snprintf(remoteTopic, sizeof(remoteTopic), MQTT_TOPIC_REMOTE_CMD, remoteId);
            mqttClient.publish(remoteTopic, remoteCmdPayload);

            // Publicar estado atualizado de volta para o Dashboard
            publishCentralStateToDA();
        }
        else if (cmd == "FEED_NOW") {
            // Dashboard solicitou alimentação manual
            int remoteId = doc["remote_id"] | 0;
            int quantity = doc["quantity"] | 100;

            Serial.printf("[DASHBOARD] Alimentação manual: Remota %d (%dg)\n", remoteId, quantity);

            String feedPayload = PayloadBuilder::buildFeedCommand(quantity);
            char remoteTopic[64];
            snprintf(remoteTopic, sizeof(remoteTopic), MQTT_TOPIC_REMOTE_CMD, remoteId);
            mqttClient.publish(remoteTopic, feedPayload);
        }
        else if (cmd == "GET_STATE") {
            // Dashboard solicitou estado completo
            Serial.println("[DASHBOARD] Solicitação de estado completo");
            publishCentralStateToDA();
        }

        return;
    }

    // ========== MENSAGENS DAS REMOTAS ==========
    // Parse do tópico para identificar a remota
    int remoteId = -1;
    if (topic.indexOf("/remote/") >= 0) {
        int start = topic.indexOf("/remote/") + 8;
        int end = topic.indexOf("/", start);
        if (end > start) {
            String idStr = topic.substring(start, end);
            remoteId = idStr.toInt();
        }
    }

    if (remoteId <= 0) {
        Serial.println("[MQTT] ID de remota inválido no tópico");
        return;
    }

    // Processar dados da remota
    if (topic.endsWith("/status")) {
        bool online = doc["online"] | false;
        remoteManager.updateRemoteStatus(remoteId, online);
        remoteManager.updateLastSeen(remoteId);

        // Notificar Dashboard sobre mudança
        publishCentralStateToDA();
    }
    else if (topic.endsWith("/data")) {
        // Atualizar dados da remota
        if (doc["feed_level"].is<String>()) {
            String feedLevel = doc["feed_level"].as<String>();
            remoteManager.updateFeedLevel(remoteId, feedLevel);
        }

        remoteManager.updateRemoteStatus(remoteId, true);
        remoteManager.updateLastSeen(remoteId);

        // Notificar Dashboard sobre mudança
        publishCentralStateToDA();
    }
}

// ========== CALLBACK DE CONFIGURAÇÃO DE REFEIÇÃO ==========

void onMealConfigChanged(int remoteId, int mealIndex, int hour, int minute, int quantity) {
    Serial.printf("[LCD] Refeição alterada: Remota %d, Refeição %d = %02d:%02d (%dg)\n",
                  remoteId, mealIndex, hour, minute, quantity);

    // Salvar na configuração
    configManager.saveRemoteConfig(remoteId);

    // Enviar via MQTT para a remota
    String payload = PayloadBuilder::buildMealConfig(mealIndex, hour, minute, quantity);

    char topic[64];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_REMOTE_CMD, remoteId);

    mqttClient.publish(topic, payload);

    // Notificar Dashboard sobre mudança
    publishCentralStateToDA();

    Serial.println("[LCD] Configuração enviada via MQTT e Dashboard atualizado");
}

// ========== INICIALIZAÇÃO DO WIFI ==========

void initWiFi() {
    Serial.println("\n========== INICIALIZANDO WIFI ==========");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.printf("Conectando a %s", WIFI_SSID);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi conectado!");
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    } else {
        Serial.println("\n❌ Falha ao conectar WiFi!");
    }

    Serial.println("========================================\n");
}

// ========== INICIALIZAÇÃO DO MQTT ==========

void initMQTT() {
    Serial.println("\n========== INICIALIZANDO MQTT ==========");

    mqttClient.configure(MQTT_BROKER_HOST, MQTT_BROKER_PORT,
                         MQTT_USERNAME, MQTT_PASSWORD, MQTT_CLIENT_ID);

    // Configurar TLS com certificado CA
    mqttClient.setTLSCertificate(MQTT_CA_CERT);

    mqttClient.setMessageCallback(onMQTTMessage);

    if (mqttClient.connect()) {
        Serial.println("✅ MQTT conectado!");

        // Inscrever nos tópicos das remotas
        for (int i = 0; i < remoteManager.getRemoteCount(); i++) {
            RemoteState* remote = remoteManager.getRemoteByIndex(i);
            if (remote) {
                char topic[64];

                snprintf(topic, sizeof(topic), MQTT_TOPIC_REMOTE_STATUS, remote->id);
                mqttClient.subscribe(topic);

                snprintf(topic, sizeof(topic), MQTT_TOPIC_REMOTE_DATA, remote->id);
                mqttClient.subscribe(topic);
            }
        }

        // Inscrever em comandos para a central (do Dashboard)
        mqttClient.subscribe(MQTT_TOPIC_CENTRAL_CMD);

        // Publicar estado completo inicial para o Dashboard
        publishCentralStateToDA();

    } else {
        Serial.println("❌ Falha ao conectar MQTT");
    }

    Serial.println("========================================\n");
}

// ========== SETUP ==========

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n");
    Serial.println("╔══════════════════════════════════════╗");
    Serial.println("║   CENTRAL PET FEEDER - REFACTORED   ║");
    Serial.println("║          Version " SYSTEM_VERSION "          ║");
    Serial.println("╚══════════════════════════════════════╝");
    Serial.println();

    // ===== INICIALIZAR CORE =====

    Serial.println("[CORE] Inicializando RemoteManager...");
    remoteManager.addRemote(1);
    remoteManager.addRemote(2);
    remoteManager.addRemote(3);
    remoteManager.addRemote(4);

    Serial.println("[CORE] Inicializando ClockService...");
    if (!clockService.init()) {
        Serial.println("[CORE] ⚠️ RTC não inicializado, continuando sem hora");
    }

    Serial.println("[CORE] Inicializando ConfigManager...");
    configManager.init();
    configManager.loadAllRemotes();

    // ===== INICIALIZAR HAL =====

    Serial.println("[HAL] Inicializando Buttons...");
    buttons.init();

    Serial.println("[HAL] Inicializando LCD...");
    lcdRenderer.init();
    screenBacklight.on();

    // ===== INICIALIZAR UI =====

    Serial.println("[UI] Inicializando MenuController...");
    menuController.init();
    menuController.setMealConfigCallback(onMealConfigChanged);

    // ===== INICIALIZAR NETWORK =====

    initWiFi();
    if (WiFi.status() == WL_CONNECTED) {
        initMQTT();
    }

    Serial.println("\n✅ SISTEMA INICIADO COM SUCESSO!\n");
}

// ========== LOOP ==========

void loop() {
    unsigned long now = millis();

    // Atualizar relógio (1x por segundo)
    if (now - lastClockUpdate >= CLOCK_UPDATE_INTERVAL) {
        lastClockUpdate = now;
        clockService.update();
    }

    // Atualizar UI (100ms)
    if (now - lastScreenUpdate >= SCREEN_UPDATE_INTERVAL) {
        lastScreenUpdate = now;
        menuController.update();
    }

    // Loop MQTT
    mqttClient.loop();

    // Publicar estado completo da central periodicamente (30s) para Dashboard
    if (mqttClient.isConnected() && (now - lastMQTTStatusPublish >= MQTT_STATUS_INTERVAL)) {
        lastMQTTStatusPublish = now;
        publishCentralStateToDA();
    }

    // Reconectar WiFi/MQTT se necessário
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WIFI] Conexão perdida, tentando reconectar...");
        WiFi.reconnect();
        delay(5000);
    }
}