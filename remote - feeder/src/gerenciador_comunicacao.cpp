#include "gerenciador_comunicacao.h"
#include <Arduino.h>

// Inicializar ponteiro estático
GerenciadorComunicacao* GerenciadorComunicacao::instancia = nullptr;

GerenciadorComunicacao::GerenciadorComunicacao(MQTTManager* mqttPtr, ProcessadorMQTT* processadorPtr)
    : mqttManager(mqttPtr), processadorMQTT(processadorPtr) {
    // Definir instância estática para callback
    instancia = this;
}

void GerenciadorComunicacao::definirCallback() {
    mqttManager->definirCallback(callbackMQTT);
}

void GerenciadorComunicacao::processarMensagens() {
    mqttManager->loop();
}

void GerenciadorComunicacao::processarDCMD(const String& payload) {
    // Delegar processamento para o ProcessadorMQTT
    processadorMQTT->processarDCMD(payload);
}

// Callback estático para MQTT
void GerenciadorComunicacao::callbackMQTT(String topic, String payload) {
    if (instancia) {
        Serial.printf("📥 MQTT recebido [%s]: %s\n", topic.c_str(), payload.c_str());

        // Verificar se é um comando (DCMD)
        if (topic.indexOf("/DCMD/") >= 0) {
            instancia->processarDCMD(payload);
        } else {
            Serial.println("ℹ️  Tópico não reconhecido (ignorado)");
        }
    }
}
