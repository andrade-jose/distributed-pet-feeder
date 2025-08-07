#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "WiFiManager.h"

// Callback para receber mensagens MQTT
typedef void (*MqttCallback)(String topic, String payload);

class MQTTManager {
private:
    WiFiClient wifiClient;
    WiFiClientSecure wifiClientSecure;
    PubSubClient mqttClient;
    WiFiManager* wifiManager;
    
    const char* servidor;
    int porta;
    const char* clientId;
    const char* usuario;
    const char* senha;
    bool usarWebSocket;
    bool conectado;
    unsigned long ultimaTentativa;
    const unsigned long INTERVALO_RECONEXAO = 5000;
    
    MqttCallback callbackUsuario;

public:
    MQTTManager(WiFiManager* wifiMgr, const char* servidor, int porta, const char* clientId, const char* usuario = "", const char* senha = "");
    void iniciar();
    bool conectar();
    void verificarConexao();
    bool estaConectado();
    void loop();
    void desconectar();
    
    // Gerenciamento de mensagens
    void definirCallback(MqttCallback callback);
    bool publicar(const char* topico, const char* payload);
    bool publicar(const char* topico, String payload);
    bool subscrever(const char* topico);
    
    // Callback interno do PubSubClient
    static void callbackInterno(char* topic, byte* payload, unsigned int length);
    static MQTTManager* instancia; // Para acessar no callback estático
};

#endif
