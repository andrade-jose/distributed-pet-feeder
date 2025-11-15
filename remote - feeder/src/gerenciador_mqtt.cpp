#include "gerenciador_mqtt.h"

// Inicializar ponteiro estático
MQTTManager* MQTTManager::instancia = nullptr;

MQTTManager::MQTTManager(WiFiManager* wifiMgr, const char* serverParam, int portParam,
                         const char* clientIdParam, const char* usernameParam,
                         const char* passwordParam)
    : wifiManager(wifiMgr), server(serverParam), port(portParam),
      clientId(clientIdParam), username(usernameParam), password(passwordParam),
      conectado(false), ultimaTentativa(0), reconnectDelay(INITIAL_RECONNECT_DELAY),
      callback(nullptr), topicoInscrito(""), bdSeq(0) {

    mqttClient.setClient(wifiClient);
    mqttClient.setServer(server, port);
    mqttClient.setCallback(onMQTTMessage);
    mqttClient.setKeepAlive(60);
    mqttClient.setBufferSize(2048);

    // Definir instância estática para callback
    instancia = this;
}

bool MQTTManager::configurarLWT() {
    // LWT será configurado no método connect()
    Serial.println("✅ LWT será configurado durante connect()");
    return true;
}


void MQTTManager::iniciar() {
    Serial.printf("📬 MQTT Manager iniciado - Mosquitto: %s:%d\n", server, port);
    Serial.printf("👤 Usuário: %s\n", username);
    Serial.printf("🔐 SSL: Desabilitado (porta 1883)\n");
}

bool MQTTManager::conectar() {
    if (!wifiManager->estaConectado()) {
        Serial.println("❌ WiFi não conectado - impossível conectar MQTT");
        return false;
    }

    if (estaConectado()) {
        return true;
    }

    Serial.printf("🔗 Conectando ao Mosquitto: %s:%d\n", server, port);
    Serial.printf("👤 Cliente: %s, Usuário: %s\n", clientId, username);

    // Configurar Last Will Testament
    extern const char* TOPIC_DDEATH;
    String deathPayload = "{\"timestamp\":" + String(millis()) +
                         ",\"bdSeq\":" + String(bdSeq) + "}";

    // PubSubClient: connect(id, user, pass, willTopic, willQos, willRetain, willMessage)
    bool success = mqttClient.connect(clientId, username, password,
                                       TOPIC_DDEATH, 1, false, deathPayload.c_str());

    if (success) {
        conectado = true;
        reconnectDelay = INITIAL_RECONNECT_DELAY;
        Serial.println("✅ MQTT conectado ao Mosquitto Umbrel!");
        Serial.println("✅ LWT (DDEATH) configurado");
        return true;
    } else {
        conectado = false;
        int estado = mqttClient.state();
        Serial.printf("❌ Falha MQTT. Estado: %d\n", estado);

        // Debug de estados comuns
        switch(estado) {
            case -4: Serial.println("❌ Timeout na conexão"); break;
            case -2: Serial.println("❌ Falha na conexão de rede"); break;
            case -1: Serial.println("❌ Servidor não encontrado"); break;
            case 1: Serial.println("❌ Protocolo não suportado"); break;
            case 2: Serial.println("❌ Client ID rejeitado"); break;
            case 3: Serial.println("❌ Servidor indisponível"); break;
            case 4: Serial.println("❌ Credenciais inválidas"); break;
            case 5: Serial.println("❌ Não autorizado"); break;
            default: Serial.println("❌ Erro desconhecido"); break;
        }

        return false;
    }
}

void MQTTManager::verificarConexao() {
    if (!mqttClient.connected()) {
        conectado = false;

        unsigned long agora = millis();
        if (agora - ultimaTentativa >= reconnectDelay) {
            ultimaTentativa = agora;

            Serial.printf("🔄 Tentando reconectar MQTT (delay: %lums)...\n", reconnectDelay);

            if (conectar()) {
                reconnectDelay = INITIAL_RECONNECT_DELAY; // Reset no sucesso

                // Re-subscribe aos tópicos após reconexão
                if (!topicoInscrito.isEmpty()) {
                    Serial.printf("🔄 Re-inscrevendo no tópico: %s\n", topicoInscrito.c_str());
                    mqttClient.subscribe(topicoInscrito.c_str());
                }
            } else {
                // Backoff exponencial
                reconnectDelay = min(reconnectDelay * 2, MAX_RECONNECT_DELAY);
                Serial.printf("⏰ Próxima tentativa em: %lums\n", reconnectDelay);
            }
        }
    } else {
        conectado = true;
    }
}

bool MQTTManager::estaConectado() {
    return mqttClient.connected() && conectado;
}

void MQTTManager::loop() {
    if (!mqttClient.loop()) {
        conectado = false;
    }
}

bool MQTTManager::publicar(const char* topic, const String& payload, int qos) {
    if (!estaConectado()) {
        Serial.println("❌ MQTT não conectado - impossível publicar");
        return false;
    }

    // QoS 0 = false, QoS 1 = true
    bool retained = false;
    bool sucesso = mqttClient.publish(topic, payload.c_str(), retained);

    if (sucesso) {
        Serial.printf("📤 MQTT enviado [QoS%d] [%s]: %s\n", qos, topic, payload.c_str());
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
        topicoInscrito = String(topic); // Armazena para re-subscribe
        Serial.printf("📥 Subscrito ao tópico: %s\n", topic);
    } else {
        Serial.printf("❌ Falha ao subscrever: %s\n", topic);
    }

    return sucesso;
}

void MQTTManager::definirCallback(MQTTCallback cb) {
    callback = cb;
    Serial.println("✅ Callback MQTT definido");
}

uint64_t MQTTManager::getBdSeq() {
    return bdSeq;
}

void MQTTManager::incrementBdSeq() {
    bdSeq++;
}

void MQTTManager::desconectar() {
    mqttClient.disconnect();
    conectado = false;
    reconnectDelay = INITIAL_RECONNECT_DELAY;
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
