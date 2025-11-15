#include "comm/MQTTClient.h"

MQTTClient* MQTTClient::instance = nullptr;

MQTTClient::MQTTClient()
    : mqttClient(nullptr),
      connected(false),
      lastReconnectAttempt(0),
      reconnectAttempts(0),
      messageCallback(nullptr),
      brokerPort(1883),
      clientId("ESP32_PetFeeder") {

    instance = this;
}

MQTTClient::~MQTTClient() {
    if (mqttClient) {
        delete mqttClient;
    }
    instance = nullptr;
}

void MQTTClient::configure(const String& host, int port, const String& user, const String& pass, const String& clientID) {
    brokerHost = host;
    brokerPort = port;
    username = user;
    password = pass;
    clientId = clientID;

    Serial.println("[MQTTClient] Configurado:");
    Serial.printf("  Host: %s:%d\n", host.c_str(), port);
    Serial.printf("  User: %s\n", user.c_str());
    Serial.printf("  ClientID: %s\n", clientID.c_str());
}

void MQTTClient::setMessageCallback(MQTTMessageCallback callback) {
    messageCallback = callback;
}

void MQTTClient::setTLSCertificate(const char* caCert) {
    if (caCert && strlen(caCert) > 0) {
        wifiClient.setCACert(caCert);
        Serial.println("[MQTTClient] Certificado TLS configurado");
    } else {
        wifiClient.setInsecure();  // Aceitar qualquer certificado
        Serial.println("[MQTTClient] Modo TLS inseguro (sem validação de certificado)");
    }
}

bool MQTTClient::connect() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[MQTTClient] WiFi não conectado!");
        return false;
    }

    if (!mqttClient) {
        mqttClient = new PubSubClient(wifiClient);
        mqttClient->setServer(brokerHost.c_str(), brokerPort);
        mqttClient->setCallback(staticMQTTCallback);
        mqttClient->setBufferSize(512);
        mqttClient->setKeepAlive(60);
    }

    if (mqttClient->connected()) {
        connected = true;
        return true;
    }

    Serial.println("[MQTTClient] Conectando ao broker MQTT...");

    bool success = false;
    if (username.length() > 0) {
        success = mqttClient->connect(clientId.c_str(), username.c_str(), password.c_str());
    } else {
        success = mqttClient->connect(clientId.c_str());
    }

    if (success) {
        Serial.println("[MQTTClient] ✅ Conectado ao broker!");
        connected = true;
        reconnectAttempts = 0;
        return true;
    } else {
        int state = mqttClient->state();
        Serial.printf("[MQTTClient] ❌ Falha na conexão! Estado: %d\n", state);
        connected = false;
        reconnectAttempts++;
        return false;
    }
}

void MQTTClient::disconnect() {
    if (mqttClient && mqttClient->connected()) {
        mqttClient->disconnect();
        Serial.println("[MQTTClient] Desconectado");
    }
    connected = false;
}

bool MQTTClient::isConnected() {
    return connected && mqttClient && mqttClient->connected();
}

void MQTTClient::loop() {
    if (!mqttClient) return;

    if (mqttClient->connected()) {
        mqttClient->loop();
        connected = true;
    } else {
        connected = false;

        // Tentar reconectar
        unsigned long now = millis();
        if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;

            if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                Serial.println("[MQTTClient] Tentando reconectar...");
                connect();
            }
        }
    }
}

bool MQTTClient::publish(const String& topic, const String& payload, bool retain) {
    if (!isConnected()) {
        Serial.println("[MQTTClient] Não conectado - publicação falhou");
        return false;
    }

    bool success = mqttClient->publish(topic.c_str(), payload.c_str(), retain);

    if (success) {
        Serial.printf("[MQTT→] %s: %s\n", topic.c_str(), payload.c_str());
    } else {
        Serial.printf("[MQTT✗] Falha ao publicar em %s\n", topic.c_str());
    }

    return success;
}

bool MQTTClient::subscribe(const String& topic) {
    if (!isConnected()) {
        Serial.println("[MQTTClient] Não conectado - inscrição falhou");
        return false;
    }

    bool success = mqttClient->subscribe(topic.c_str());

    if (success) {
        Serial.printf("[MQTT] Inscrito em: %s\n", topic.c_str());
    } else {
        Serial.printf("[MQTT] Falha ao inscrever em: %s\n", topic.c_str());
    }

    return success;
}

bool MQTTClient::unsubscribe(const String& topic) {
    if (!isConnected()) return false;

    return mqttClient->unsubscribe(topic.c_str());
}

String MQTTClient::getStatusString() {
    if (isConnected()) {
        return "Conectado";
    } else if (reconnectAttempts > 0) {
        return "Reconectando... (" + String(reconnectAttempts) + ")";
    } else {
        return "Desconectado";
    }
}

void MQTTClient::staticMQTTCallback(char* topic, byte* payload, unsigned int length) {
    if (!instance || !instance->messageCallback) return;

    String payloadStr;
    payloadStr.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }

    Serial.printf("[MQTT←] %s: %s\n", topic, payloadStr.c_str());

    instance->messageCallback(String(topic), payloadStr);
}
