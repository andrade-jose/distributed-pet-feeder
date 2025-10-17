#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include <map>

class GerenciadorMQTT {
private:
    // Cliente WiFi seguro (TLS/SSL)
    static WiFiClientSecure* wifiClient;

    // Cliente MQTT
    static PubSubClient* mqttClient;

    bool conectado;
    unsigned long ultimo_pacote;
    unsigned long ultimo_reconectar;
    int pacotes_perdidos;
    int tentativas_reconexao;

    // Rastreamento de remotas ativas
    struct InfoRemota {
        bool conectada;
        unsigned long ultimo_heartbeat;
        int rssi;
        unsigned long uptime;
    };
    std::map<int, InfoRemota> remotas_ativas;
    static const unsigned long TIMEOUT_REMOTA = 60000; // 60 segundos sem heartbeat = desconectada

    // Callbacks
    void (*callbackStatusRemota)(int, const String&);
    void (*callbackVidaRemota)(int, const String&);
    void (*callbackRespostaRemota)(int, const String&);
    void (*callbackAlertaRacao)(int, const String&);

    String construirTopico(const char* template_topico, int idRemota);

    // Métodos de processamento
    void processarMensagemStatus(int idRemota, String payload);
    void processarMensagemVida(int idRemota, String payload);
    void processarMensagemResposta(int idRemota, String payload);
    void processarHeartbeatGeral(String payload);
    void processarAlertaRacao(String payload);
    int extrairIdRemotaDoTopico(String topico);
    String obterDescricaoErroMQTT(int codigo);

    // Callback estático para PubSubClient
    static void callbackMQTT(char* topic, byte* payload, unsigned int length);

public:
    // Instância singleton (público para acesso em gerenciador_web.cpp)
    static GerenciadorMQTT* instance;

    GerenciadorMQTT();
    ~GerenciadorMQTT();

    // Inicialização e atualização
    static void inicializar();
    static void atualizar();

    bool init();
    bool conectar();
    void desconectar();
    bool estaConectado() const;

    void loop();  // Método para atualizar cliente MQTT

    void processarMensagem(const String& topico, const String& payload);

    // Getters
    unsigned long getUltimoPacote() const;
    int getPacotesPerdidos() const;

    // Métodos de rastreamento de remotas
    bool remotaEstaConectada(int idRemota) const;
    void atualizarStatusRemota(int idRemota, int rssi = 0, unsigned long uptime = 0);
    void verificarTimeoutRemotas();

    // Comandos para remotas
    bool enviarComandoRemota(int idRemota, const String& acao, int tempo = 0);
    bool enviarComandoGeral(const String& acao, int tempo = 0, int idRemota = 1);
    bool configurarHorarioRemota(int idRemota, int hora, int minuto, int quantidade);
    bool configurarTempoMovimento(int idRemota, int tempo);
    bool solicitarStatusRemota(int idRemota);

    // Definir callbacks
    void definirCallbackStatusRemota(void (*callback)(int, const String&));
    void definirCallbackVidaRemota(void (*callback)(int, const String&));
    void definirCallbackRespostaRemota(void (*callback)(int, const String&));
    void definirCallbackAlertaRacao(void (*callback)(int, const String&));

    // Inscrições em tópicos
    bool inscreverTopicos();

    // Utilidades
    String obterStatusConexao() const;
    bool publicarStatusCentral(String status);
    void resetarTentativas();
};
