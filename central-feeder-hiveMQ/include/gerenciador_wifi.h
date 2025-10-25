// gerenciador_wifi.h
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

class GerenciadorWiFi {  // ✅ Nome correto com "WiFi" (i maiúsculo)
private:
    static String ssidSalvo;
    static String senhaSalva;
    static bool reconexaoAutomatica;
    static unsigned long ultimaTentativaReconexao;
    static unsigned long ultimaVerificacaoStatus;
    static bool estavaConectado;
    static void (*callbackStatus)(bool, String, String);
    
    // Para escaneamento de redes
    static int quantidadeRedes;
    static String ssidsRedes[20];
    static int rssiRedes[20];
    static bool redesCriptografadas[20];
    
    static void aoEventoWiFi(WiFiEvent_t evento, WiFiEventInfo_t info);
    static void tentarReconectar();
    static void atualizarDadosSistema();
    static void salvarCredenciais(String ssid, String senha);
    static bool carregarCredenciais();
    static void limparCredenciais();

public:
    static void inicializar();
    static void atualizar();
    static void conectar(String ssid, String senha);
    static void desconectar();
    static void escanearRedes();
    
    static bool estaConectado();
    static String obterSSID();
    static String obterIP();
    static int obterRSSI();
    static String obterQualidadeRSSI();
    
    static void definirCallbackStatus(void (*callback)(bool, String, String));
    
    // Métodos para redes escaneadas
    static int obterQuantidadeRedes();
    static String obterSSIDRede(int indice);
    static int obterRSSIRede(int indice);
    static bool obterRedeCriptografada(int indice);
};