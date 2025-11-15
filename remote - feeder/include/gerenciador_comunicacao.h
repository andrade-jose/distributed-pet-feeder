#ifndef GERENCIADOR_COMUNICACAO_H
#define GERENCIADOR_COMUNICACAO_H

#include "gerenciador_mqtt.h"
#include "processador_mqtt.h"

class GerenciadorComunicacao {
private:
    MQTTManager* mqttManager;
    ProcessadorMQTT* processadorMQTT;

public:
    GerenciadorComunicacao(MQTTManager* mqttPtr, ProcessadorMQTT* processadorPtr);

    void definirCallback();
    void processarMensagens();

    void processarDCMD(const String& payload);
    static void callbackMQTT(String topic, String payload);

    // Instância estática para callback
    static GerenciadorComunicacao* instancia;
};

#endif