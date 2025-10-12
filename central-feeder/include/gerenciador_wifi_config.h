#ifndef GERENCIADOR_WIFI_CONFIG_H
#define GERENCIADOR_WIFI_CONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "config.h"

// Configurações do Access Point
#define AP_SSID "AlimentadorAP"
#define AP_PASSWORD ""  // Sem senha para facilitar acesso
#define AP_CHANNEL 6
#define AP_MAX_CONNECTIONS 4
#define AP_IP IPAddress(192, 168, 4, 1)
#define AP_GATEWAY IPAddress(192, 168, 4, 1)
#define AP_SUBNET IPAddress(255, 255, 255, 0)

// Configurações do DNS (portal captive)
#define DNS_PORT 53

// Timeout para configuração
#define CONFIG_TIMEOUT 300000  // 5 minutos

class GerenciadorWiFiConfig {
public:
    static void inicializar();
    static void atualizar();
    static bool estaEmModoConfig();
    static void iniciarModoConfig();
    static void pararModoConfig();
    static bool wifiConfigurado();
    
private:
    static AsyncWebServer* configServer;
    static DNSServer* dnsServer;
    static bool modoConfig;
    static unsigned long inicioModoConfig;
    
    // Handlers das páginas
    static void handleRoot(AsyncWebServerRequest *request);
    static void handleScan(AsyncWebServerRequest *request);
    static void handleSave(AsyncWebServerRequest *request);
    static void handleSaveBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    static void handleStatus(AsyncWebServerRequest *request);
    
    // Funções auxiliares
    static void configurarRotas();
    static String gerarPaginaHTML();
    static String scanWiFiNetworks();
    static bool salvarCredenciais(const String& ssid, const String& password);
    static void tentarConectar(const String& ssid, const String& password);
    
    // Variável para body
    static String requestBody;
};

#endif // GERENCIADOR_WIFI_CONFIG_H
