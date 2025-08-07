#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"

class GerenciadorMQTT {
private:
    static WiFiClientSecure wifiClient;
    static PubSubClient mqttClient;
    static bool conectado;
    static unsigned long ultimoReconectar;
    static unsigned long ultimoHeartbeat;
    static int tentativasReconexao;
    
    // Callbacks
    static void (*callbackStatusRemota)(int idRemota, String status);
    static void (*callbackVidaRemota)(int idRemota, bool viva);
    static void (*callbackRespostaRemota)(int idRemota, String resposta);
    
    // Métodos privados
    static void onMensagemRecebida(char* topico, byte* payload, unsigned int comprimento);
    static bool reconectar();
    static void processarMensagemStatus(int idRemota, String payload);
    static void processarMensagemVida(int idRemota, String payload);
    static void processarMensagemResposta(int idRemota, String payload);
    static void processarHeartbeatGeral(String payload);
    static int extrairIdRemotaDoTopico(String topico);
    static String construirTopico(const char* template_topico, int idRemota);
    
public:
    // Inicialização
    static bool inicializar();
    static void atualizar();
    
    // Conexão
    static bool conectar();
    static void desconectar();
    static bool estaConectado();
    
    // Publicação de comandos para remotas
    static bool enviarComandoRemota(int idRemota, String acao, int tempo = 0);
    static bool enviarComandoGeral(String acao, int tempo = 0, int idRemota = 1); // Novo: comando geral
    static bool configurarHorarioRemota(int idRemota, int hora, int minuto, int quantidade);
    static bool configurarTempoMovimento(int idRemota, int tempo);
    static bool solicitarStatusRemota(int idRemota);
    static bool enviarPingRemota(int idRemota);
    
    // Inscrição em tópicos
    static bool inscreverStatusRemotas();
    static bool inscreverVidaRemotas();
    static bool inscreverRespostasRemotas();
    
    // Status da central
    static bool publicarStatusCentral(String status);
    
    // Callbacks
    static void definirCallbackStatusRemota(void (*callback)(int, String));
    static void definirCallbackVidaRemota(void (*callback)(int, bool));
    static void definirCallbackRespostaRemota(void (*callback)(int, String));
    
    // Utilitários
    static String obterStatusConexao();
    static String obterDescricaoErroMQTT(int codigo);
    static unsigned long obterUltimoHeartbeat();
    static void resetarTentativasReconexao();
};
