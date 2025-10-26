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

    // Registrar callback para quando o LCD alterar uma refeição
    GerenciadorTelas::definirCallbackAtualizacaoRefeicao([](int idRemota, int indiceRefeicao, int hora, int minuto, int quantidade) {
        Serial.printf("[CALLBACK] LCD alterou: Remota %d, Refeição %d → %02d:%02d (%dg)\n",
                     idRemota, indiceRefeicao, hora, minuto, quantidade);

        GerenciadorMQTT* mqtt = GerenciadorMQTT::obterInstancia();
        if (mqtt && mqtt->estaConectado()) {
            // Enviar para a remota
            mqtt->configurarHorarioRefeicao(idRemota, indiceRefeicao, hora, minuto, quantidade);

            // Notificar dashboards da mudança (origem = "l" de LCD)
            mqtt->notificarMudancaConfig(idRemota, indiceRefeicao, hora, minuto, quantidade, "l");

            // Publicar estado completo atualizado (com retain)
            mqtt->publicarEstadoCompleto(idRemota);

            Serial.println("[CALLBACK] Configuração publicada via MQTT");
        } else {
            Serial.println("[CALLBACK] MQTT não conectado, configuração não publicada");
        }
    });

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