#ifndef PROCESSADOR_MQTT_H
#define PROCESSADOR_MQTT_H

#include <Arduino.h>
#include "gerenciador_mqtt.h"
#include "gerenciador_alimentacao.h"
#include "gerenciador_sistema.h"
#include "gerenciador_hcsr04.h"
#include "sparkplugb.h"
#include "config.h"

class ProcessadorMQTT {
private:
    MQTTManager* mqttManager;
    GerenciadorAlimentacao* alimentacao;
    GerenciadorSistema* sistema;

    // Sequência de mensagens DDATA
    uint64_t seq;

    // Flag de Birth enviado
    bool birthEnviado;

    // Timestamp do último DDATA
    unsigned long ultimoDDATA;

public:
    ProcessadorMQTT(MQTTManager* mqtt,
                    GerenciadorAlimentacao* alim,
                    GerenciadorSistema* sist);

    // ===== SPARKPLUG B MESSAGES =====

    // DBIRTH - Birth Certificate (enviado ao conectar)
    bool enviarDBIRTH();

    // DDATA - Telemetria (enviado periodicamente)
    bool enviarDDATA(float distanciaRacao, bool racaoBaixa);

    // DDEATH - Death Certificate (enviado ao desconectar voluntariamente)
    bool enviarDDEATH();

    // DCMD - Processar comandos recebidos
    void processarDCMD(const String& payload);

    // ===== UTILITÁRIOS =====

    // Obter sequência atual
    uint64_t getSeq();

    // Incrementar sequência
    void incrementSeq();

    // Verificar se birth foi enviado
    bool getBirthEnviado();

    // Loop para envio periódico de DDATA
    void loop();
};

#endif
