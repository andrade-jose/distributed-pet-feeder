#ifndef GERENCIADOR_MQTT_H
#define GERENCIADOR_MQTT_H

#include <WiFiClient.h>
#include <PubSubClient.h>
#include "gerenciador_wifi.h"

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
    
    bool conectado;
    unsigned long ultimaTentativa;
    MQTTCallback callback;
    
    static const unsigned long INTERVALO_RECONEXAO = 10000; // 10 segundos
    
    // Callback estático para PubSubClient
    static void onMQTTMessage(char* topic, byte* payload, unsigned int length);
    static MQTTManager* instancia; // Para callback estático

public:
    MQTTManager(WiFiManager* wifiMgr, const char* server, int port,
                const char* clientId);
    
    void iniciar();
    bool conectar();
    void verificarConexao();
    bool estaConectado();
    void loop();
    
    bool publicar(const char* topic, const String& payload);
    bool subscrever(const char* topic);
    void definirCallback(MQTTCallback cb);
    
    void desconectar();
};

#endif