#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// Callback para mensagens recebidas
typedef void (*MQTTMessageCallback)(const String& topic, const String& payload);

class MQTTClient {
private:
    WiFiClientSecure wifiClient;
    PubSubClient* mqttClient;

    String brokerHost;
    int brokerPort;
    String username;
    String password;
    String clientId;

    bool connected;
    unsigned long lastReconnectAttempt;
    int reconnectAttempts;

    MQTTMessageCallback messageCallback;

    static MQTTClient* instance;
    static void staticMQTTCallback(char* topic, byte* payload, unsigned int length);

    static const unsigned long RECONNECT_INTERVAL = 5000;  // 5 segundos
    static const int MAX_RECONNECT_ATTEMPTS = 10;

public:
    MQTTClient();
    ~MQTTClient();

    // Configuração
    void configure(const String& host, int port, const String& user, const String& pass, const String& clientID);
    void setMessageCallback(MQTTMessageCallback callback);

    // TLS (opcional - deixar certificado vazio para conexão sem TLS)
    void setTLSCertificate(const char* caCert);

    // Conexão
    bool connect();
    void disconnect();
    bool isConnected();
    void loop();

    // Publicação
    bool publish(const String& topic, const String& payload, bool retain = false);

    // Inscrição
    bool subscribe(const String& topic);
    bool unsubscribe(const String& topic);

    // Estado
    String getStatusString();
    int getReconnectAttempts() { return reconnectAttempts; }
};
