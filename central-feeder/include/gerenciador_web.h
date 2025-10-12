#ifndef GERENCIADOR_WEB_H
#define GERENCIADOR_WEB_H

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "config.h"

class GerenciadorWeb {
public:
    static void inicializar();
    static void atualizar();
    static bool estaIniciado();
    
private:
    static AsyncWebServer server;
    static bool iniciado;
    
    // Handlers para as rotas
    static void handleRoot(AsyncWebServerRequest *request);
    static void handleAPI_Status(AsyncWebServerRequest *request);
    static void handleAPI_Feed(AsyncWebServerRequest *request);
    static void handleAPI_StatusRemota(AsyncWebServerRequest *request);
    static void handleAPI_Configurar(AsyncWebServerRequest *request);
    static void handleAPI_ConfigurarBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    static void handleAPI_ObterConfig(AsyncWebServerRequest *request);
    static void handleAPI_Config(AsyncWebServerRequest *request);
    static void handleAPI_ListarRemotas(AsyncWebServerRequest *request);
    
    // Variável para armazenar body da requisição
    static String requestBody;
    
    // Funções auxiliares
    static String obterStatusSistema();
    static String obterStatusRemotas();
    static void configurarRotas();
};

#endif