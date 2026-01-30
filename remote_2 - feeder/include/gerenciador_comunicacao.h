#ifndef GERENCIADOR_COMUNICACAO_H
#define GERENCIADOR_COMUNICACAO_H

#include "gerenciador_mqtt.h"
#include "gerenciador_alimentacao.h"
#include "gerenciador_sistema.h"

class GerenciadorComunicacao {
private:
    MQTTManager* mqttManager;
    GerenciadorAlimentacao* alimentacao;
    GerenciadorSistema* sistema;
    
    unsigned long ultimoHeartbeat;
    static const unsigned long INTERVALO_HEARTBEAT = 30000;

public:
    GerenciadorComunicacao(MQTTManager* mqttPtr, GerenciadorAlimentacao* alimentacaoPtr, 
                          GerenciadorSistema* sistemaPtr);
    
    void definirCallback();
    void processarMensagens();
    void enviarHeartbeat(const char* topicHeartbeat);
    void enviarStatusMQTT(const char* topicStatus, const String& status);
    void enviarConclusaoMQTT(const char* topicResposta, int tempoDecorrido, const String& comandoId);
    
    void processarComandoCentral(const String& payload);
    static void callbackMQTT(String topic, String payload);
    
    // Instância estática para callback
    static GerenciadorComunicacao* instancia;
};

#endif