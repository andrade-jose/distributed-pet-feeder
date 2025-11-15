// sistema_estado.h - APENAS DECLARAÇÕES
#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

struct EstadoSistema {
    // WiFi
    String wifiSSID = "Desconectado";
    bool wifiConectado = false;
    String qualidadeWifi = "Sem sinal";
    String ipWifi = "";
    
    // MQTT
    bool mqttConectado = false;
    
    // Callbacks para eventos
    void (*onWifiMudou)(bool conectado) = nullptr;
    void (*onMqttMudou)(bool conectado) = nullptr;
    void (*onRemotaMudou)(int id, bool conectada) = nullptr;
    
    // Remotas
    struct EstadoRemota {
        int id;
        bool conectada;
        bool online;
        unsigned long ultimoHeartbeat;
        String nivelRacao;
        String nome;
        
        // Refeições
        struct Refeicao {
            int hora = 8;
            int minuto = 0;
            int quantidade = 40;
            String ultimaExecucao = "Nunca";
        };
        Refeicao refeicoes[3]; // REFEICOES_PER_REMOTA
    };
    
    EstadoRemota remotas[MAX_REMOTAS];
    int numRemotas = 0;
    
    // Flag para sincronização de refeições
    bool refeicoesModificadas = false;
    
    // ===== MÉTODOS - APENAS DECLARAÇÕES =====
    
    void atualizarStatusWifi(String ssid, bool conectado, String qualidade, String ip);
    void atualizarStatusMqtt(bool conectado);
    void atualizarRemota(int id, bool conectada, bool online, String nivelRacao = "");
    void adicionarRemota(int id);
    bool remotaConectada(int id);
    bool remotaAtivaRecente(int id); // Verifica se teve sinal nos últimos 10 minutos
    void verificarTimeouts();
    void marcarRefeicoesModificadas();
    bool verificarELimparFlagRefeicoes();
    void salvarConfiguracao();
    void carregarConfiguracao();
    
    // ✅ ADICIONAR: Métodos para gerenciar refeições específicas
    void definirRefeicao(int idRemota, int indiceRefeicao, int hora, int minuto, int quantidade);
    void atualizarUltimaExecucao(int idRemota, int indiceRefeicao, String tempo);
};

// Declaração da instância global
extern EstadoSistema estadoSistema;