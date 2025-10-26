// gerenciador_mqtt.h
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"

class GerenciadorMQTT {
private:
    WiFiClientSecure* wifiClient;
    PubSubClient* mqttClient;

    bool conectado;
    bool tentouInicializar;
    unsigned long ultimo_pacote;
    unsigned long ultimo_reconectar;
    int pacotes_perdidos;
    int tentativas_reconexao;

    String construirTopico(const char* template_topico, int idRemota);
    int extrairIdRemotaDoTopico(String topico);
    String obterDescricaoErroMQTT(int codigo);
    static void callbackMQTT(char* topic, byte* payload, unsigned int length);

public:
    // ✅ CORREÇÃO: Remover instance estático, usar métodos estáticos para gerenciar instância única
    GerenciadorMQTT();
    ~GerenciadorMQTT();

    // Inicialização e atualização
    static void inicializar();
    static void atualizar();
    
    // ✅ ADICIONAR: Método para obter a instância
    static GerenciadorMQTT* obterInstancia();

    bool init();
    bool conectar();
    void desconectar();
    bool estaConectado() const;
    void loop();
    void processarMensagem(const String& topico, const String& payload);

    // Getters
    unsigned long getUltimoPacote() const;
    int getPacotesPerdidos() const;

    // Comandos para remotas
    bool publicar(const String& topico, const String& payload);
    bool publicarComRetain(const String& topico, const String& payload); // NOVO: publish com retain
    bool enviarComandoRemota(int idRemota, const String& acao, int tempo = 0);
    bool enviarComandoGeral(const String& acao, int tempo = 0, int idRemota = 1);
    bool configurarHorarioRemota(int idRemota, int hora, int minuto, int quantidade);
    bool configurarHorarioRefeicao(int idRemota, int indiceRefeicao, int hora, int minuto, int quantidade); // NOVO
    bool configurarTempoMovimento(int idRemota, int tempo);
    bool solicitarStatusRemota(int idRemota);

    // Configuração via Dashboard (NOVO)
    bool publicarEstadoCompleto(int idRemota); // Publica estado completo com retain
    bool notificarMudancaConfig(int idRemota, int indiceRefeicao, int hora, int minuto, int quantidade, const char* origem); // Notifica mudança

    // Inscrições em tópicos
    bool inscreverTopicos();

    // Utilidades
    String obterStatusConexao() const;
    bool publicarStatusCentral(String status);
    void resetarTentativas();
};

// ✅ CORREÇÃO: Declarar como pointer para instância única
extern GerenciadorMQTT* gerenciadorMQTT;