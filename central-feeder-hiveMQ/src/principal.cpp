// principal.cpp
#include <Arduino.h>
#include "config.h"
#include "botoes.h"
#include "display.h"
#include "gerenciador_tempo.h"
#include "gerenciador_wifi.h"
#include "gerenciador_mqtt.h"
#include "gerenciador_telas.h"
#include "controlador_alimentacao.h"
#include "sistema_estado.h"

#define VERSAO SYSTEM_VERSION

// ✅ CORREÇÃO: Usar pointer (declaração consistente)
extern GerenciadorMQTT* gerenciadorMQTT;
extern EstadoSistema estadoSistema;

void setup() 
{
    Serial.begin(115200);
    delay(2000);  // Aguarda 2 segundos para estabilizar
    
    Serial.println("\n\n========================================");
    Serial.println("ESP32 INICIADO COM SUCESSO!");
    Serial.println("========================================");
    Serial.print("Versao do Firmware: ");
    Serial.println(__DATE__ " " __TIME__);
    Serial.println("Iniciando setup...");
    
    // Resto do seu código setup aqui...
    
    Serial.println("=== Inicializando Sistema Cliente MQTT ===");
    Serial.printf("Versão: %s\n", VERSAO);
    Serial.printf("Build: %s %s\n", __DATE__, __TIME__);

    Botoes::inicializar();
    Display::init();
    GerenciadorTempo::inicializar();
    GerenciadorWiFi::inicializar();
    GerenciadorMQTT::inicializar();  // ✅ Isso cria a instância
    GerenciadorTelas::inicializar();
    ControladorAlimentacao::inicializar();
    
    Serial.println("Sistema pronto!");
}

void loop() {
    GerenciadorTempo::atualizar();
    GerenciadorWiFi::atualizar();
    GerenciadorMQTT::atualizar();
    estadoSistema.verificarTimeouts();
    GerenciadorTelas::atualizar();
    ControladorAlimentacao::atualizar();

    delay(100);
}