#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "gerenciador_servo.h"
#include "gerenciador_wifi.h"
#include "gerenciador_mqtt.h"
#include "gerenciador_hcsr04.h"
#include "gerenciador_alimentacao.h"
#include "gerenciador_sistema.h"
#include "gerenciador_comunicacao.h"
#include "processador_mqtt.h"

// ===== INSTÂNCIAS DOS COMPONENTES =====
ServoControl servo;
WiFiManager wifiManager(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
MQTTManager mqttManager(&wifiManager, MQTT_SERVER, MQTT_PORT, MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
GerenciadorHCSR04 sensorRacao(PINO_HCSR04_TRIGGER, PINO_HCSR04_ECHO, DISTANCIA_LIMITE_RACAO, nullptr, nullptr);
GerenciadorAlimentacao alimentacao(&servo);
GerenciadorSistema sistema(&servo, &wifiManager, &mqttManager, PINO_BOTAO, PINO_LED_STATUS);
ProcessadorMQTT processadorMQTT(&mqttManager, &alimentacao, &sistema);
GerenciadorComunicacao comunicacao(&mqttManager, &processadorMQTT);

// ===== VARIÁVEIS DE TIMING =====
unsigned long ultimoMonitoramentoRacao = 0;

// ===== CONFIGURAÇÃO INICIAL =====
void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);

    Serial.println("╔════════════════════════════════════════════╗");
    Serial.println("║  ESP32 REMOTA - SPARKPLUG B v1.0          ║");
    Serial.println("║  Alimentador de Pets Distribuído          ║");
    Serial.println("╚════════════════════════════════════════════╝");

    sistema.inicializar();
    sistema.exibirInformacoes();
    sistema.configurarHardware();
    sistema.inicializarComponentes(PINO_SERVO);

    sensorRacao.iniciar();
    Serial.println("✅ Sensor HCSR04 inicializado");

    // Conectar WiFi
    sistema.inicializarComunicacao();

    // Definir callback MQTT
    comunicacao.definirCallback();

    // Conectar MQTT e enviar DBIRTH
    if (mqttManager.estaConectado()) {
        // Subscrever ao tópico DCMD
        mqttManager.subscrever(TOPIC_DCMD);

        // Enviar DBIRTH (Birth Certificate)
        processadorMQTT.enviarDBIRTH();
    }

    sistema.testarComponentes();

    Serial.println("╔════════════════════════════════════════════╗");
    Serial.println("║  ESP32 REMOTA PRONTO!                     ║");
    Serial.println("╚════════════════════════════════════════════╝");
    Serial.printf("📡 Device ID: %s\n", DEVICE_ID);
    Serial.printf("📊 Telemetria DDATA: a cada %lu ms\n", INTERVALO_HEARTBEAT);
    Serial.printf("🔍 Monitoramento ração: a cada %lu ms\n", INTERVALO_MONITORAMENTO_RACAO);
}

// ===== LOOP PRINCIPAL =====
void loop()
{
    unsigned long agora = millis();

    // Verificar e manter conexões
    sistema.verificarConexoes();

    // Se MQTT reconectou, reenviar DBIRTH
    static bool mqttConectadoAntes = false;
    bool mqttConectadoAgora = mqttManager.estaConectado();

    if (mqttConectadoAgora && !mqttConectadoAntes) {
        // Reconectou - reenviar DBIRTH
        Serial.println("🔄 MQTT reconectado - reenviando DBIRTH...");
        mqttManager.subscrever(TOPIC_DCMD);
        processadorMQTT.enviarDBIRTH();
    }
    mqttConectadoAntes = mqttConectadoAgora;

    // Processar mensagens MQTT
    comunicacao.processarMensagens();

    // Processar botão físico
    sistema.processarBotao();

    // Atualizar LED de status
    sistema.atualizarLedStatus();

    // Executar ciclo de alimentação (se ativo)
    alimentacao.executarCiclo();

    // Enviar DDATA periodicamente (via ProcessadorMQTT)
    processadorMQTT.loop();

    // Monitorar nível de ração (a cada 5 minutos)
    if (agora - ultimoMonitoramentoRacao >= INTERVALO_MONITORAMENTO_RACAO) {
        Serial.println("🔍 Verificando nível de ração...");
        float distancia = sensorRacao.medirDistancia();
        bool racaoBaixa = (distancia >= sensorRacao.getDistanciaLimite());

        if (racaoBaixa) {
            Serial.printf("⚠️  RAÇÃO BAIXA! Distância: %.1f cm (limite: %.1f cm)\n",
                         distancia, sensorRacao.getDistanciaLimite());
        } else {
            Serial.printf("✅ Nível de ração OK: %.1f cm\n", distancia);
        }

        ultimoMonitoramentoRacao = agora;
    }

    // Monitorar sistema
    sistema.monitorarSistema();

    delay(MAIN_LOOP_DELAY);
}