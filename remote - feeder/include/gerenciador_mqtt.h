#ifndef GERENCIADOR_MQTT_H
#define GERENCIADOR_MQTT_H

#include <WiFiClient.h>
#include <PubSubClient.h>
#include "gerenciador_wifi.h"
#include "config.h"

// Callback function type
typedef void (*MQTTCallback)(String topic, String payload);

class MQTTManager {
private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    WiFiManager* wifiManager;

    const char* server;
    int port;
    const char* clientId;
    const char* username;
    const char* password;

    bool conectado;
    unsigned long ultimaTentativa;
    unsigned long reconnectDelay;
    MQTTCallback callback;

    // Armazenar tópico para re-subscribe automático
    String topicoInscrito;

    // SparkplugB Birth/Death Sequence
    uint64_t bdSeq;

    // Constantes para reconexão
    static const unsigned long INTERVALO_RECONEXAO = 5000;
    static const unsigned long MAX_RECONNECT_DELAY = 60000;
    static const unsigned long INITIAL_RECONNECT_DELAY = 1000;

    // Callback estático para PubSubClient
    static void onMQTTMessage(char* topic, byte* payload, unsigned int length);
    static MQTTManager* instancia;

    // Last Will Testament (DDEATH)
    bool configurarLWT();

public:
    MQTTManager(WiFiManager* wifiMgr, const char* server, int port,
                const char* clientId, const char* username, const char* password);

    void iniciar();
    bool conectar();
    void verificarConexao();
    bool estaConectado();
    void loop();

    // Publicar com QoS (0 ou 1)
    bool publicar(const char* topic, const String& payload, int qos = 1);
    bool subscrever(const char* topic);
    void definirCallback(MQTTCallback cb);

    // SparkplugB Birth/Death Sequence
    uint64_t getBdSeq();
    void incrementBdSeq();

    void desconectar();
};

#endif