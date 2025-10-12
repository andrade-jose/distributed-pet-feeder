#include "gerenciador_mqtt.h"

// Inicializar ponteiro estático
MQTTManager* MQTTManager::instancia = nullptr;

MQTTManager::MQTTManager(WiFiManager* wifiMgr, const char* serverParam, int portParam,
                         const char* clientIdParam)
    : wifiManager(wifiMgr), server(serverParam), port(portParam),
      clientId(clientIdParam),
      conectado(false), ultimaTentativa(0), callback(nullptr) {

    // Configurar cliente MQTT (sem SSL)
    mqttClient.setClient(wifiClient);
    mqttClient.setServer(server, port);
    mqttClient.setCallback(onMQTTMessage);

    // Definir instância estática para callback
    instancia = this;
}

void MQTTManager::iniciar() {
    // Broker MQTT local - sem necessidade de SSL
    Serial.printf("📬 MQTT Manager iniciado - Broker Local: %s:%d\n", server, port);
}

bool MQTTManager::conectar() {
    if (!wifiManager->estaConectado()) {
        Serial.println("❌ WiFi não conectado - impossível conectar MQTT");
        return false;
    }
    
    if (estaConectado()) {
        return true;
    }
    
    Serial.printf("🔗 Conectando ao broker MQTT local: %s:%d\n", server, port);
    Serial.printf("👤 Cliente: %s\n", clientId);

    // Conectar SEM usuário/senha (broker local)
    if (mqttClient.connect(clientId)) {
        conectado = true;
        Serial.println("✅ MQTT conectado ao broker local!");
        return true;
    } else {
        conectado = false;
        Serial.printf("❌ Falha MQTT. Estado: %d\n", mqttClient.state());
        return false;
    }
}

void MQTTManager::verificarConexao() {
    if (!mqttClient.connected()) {
        conectado = false;
        
        // Tentar reconectar a cada INTERVALO_RECONEXAO
        unsigned long agora = millis();
        if (agora - ultimaTentativa >= INTERVALO_RECONEXAO) {
            ultimaTentativa = agora;
            Serial.println("🔄 Tentando reconectar MQTT...");
            conectar();
        }
    } else {
        conectado = true;
    }
}

bool MQTTManager::estaConectado() {
    return mqttClient.connected();
}

void MQTTManager::loop() {
    if (mqttClient.connected()) {
        mqttClient.loop();
    }
}

bool MQTTManager::publicar(const char* topic, const String& payload) {
    if (!estaConectado()) {
        Serial.println("❌ MQTT não conectado - impossível publicar");
        return false;
    }
    
    bool sucesso = mqttClient.publish(topic, payload.c_str());
    if (sucesso) {
        Serial.printf("📤 MQTT enviado [%s]: %s\n", topic, payload.c_str());
    } else {
        Serial.printf("❌ Falha ao enviar MQTT [%s]\n", topic);
    }
    
    return sucesso;
}

bool MQTTManager::subscrever(const char* topic) {
    if (!estaConectado()) {
        Serial.println("❌ MQTT não conectado - impossível subscrever");
        return false;
    }
    
    bool sucesso = mqttClient.subscribe(topic);
    if (sucesso) {
        Serial.printf("📥 Subscrito ao tópico: %s\n", topic);
    } else {
        Serial.printf("❌ Falha ao subscrever: %s\n", topic);
    }
    
    return sucesso;
}

void MQTTManager::definirCallback(MQTTCallback cb) {
    callback = cb;
}

void MQTTManager::desconectar() {
    mqttClient.disconnect();
    conectado = false;
    Serial.println("📬 MQTT desconectado");
}

// Callback estático que repassa para a instância
void MQTTManager::onMQTTMessage(char* topic, byte* payload, unsigned int length) {
    // Converter payload para String
    String payloadStr = "";
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }
    Serial.printf("📥 MQTT recebido [%s]: %s\n", topic, payloadStr.c_str());
    if (instancia && instancia->callback) {
        // Chamar callback da instância
        instancia->callback(String(topic), payloadStr);
    }
}