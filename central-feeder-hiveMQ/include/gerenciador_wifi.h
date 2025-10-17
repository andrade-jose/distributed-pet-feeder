#ifndef GERENCIADOR_WIFI_H
#define GERENCIADOR_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

class GerenciadorWifi {
public:
    static void inicializar();
    static void conectar(String ssid, String senha);
    static void desconectar();
    static void escanearRedes();
    static bool estaConectado();
    static String obterSSID();
    static String obterIP();
    static int obterRSSI();
    static String obterQualidadeRSSI();
    static void atualizar();
    
    // Configuração
    static void salvarCredenciais(String ssid, String senha);
    static bool carregarCredenciais();
    static void limparCredenciais();
    
    // Callback para status
    static void definirCallbackStatus(void (*callback)(bool conectado, String ssid, String ip));
    
    // Lista de redes escaneadas
    static int obterQuantidadeRedes();
    static String obterSSIDRede(int indice);
    static int obterRSSIRede(int indice);
    static bool obterRedeCriptografada(int indice);
    
private:
    static String ssidSalvo;
    static String senhaSalva;
    static bool reconexaoAutomatica;
    static unsigned long ultimaTentativaReconexao;
    static unsigned long ultimaVerificacaoStatus;
    static bool estavaConcetado;
    static void (*callbackStatus)(bool, String, String);
    
    // Redes escaneadas
    static int quantidadeRedes;
    static String ssidsRedes[WIFI_MAX_NETWORKS];
    static int rssiRedes[WIFI_MAX_NETWORKS];
    static bool redesCriptografadas[WIFI_MAX_NETWORKS];
    
    static void aoEventoWiFi(WiFiEvent_t evento, WiFiEventInfo_t info);
    static void tentarReconectar();
    static void atualizarDadosSistema();
};

#endif
