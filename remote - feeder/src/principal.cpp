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

// ===== INSTÂNCIAS DOS COMPONENTES =====
ServoControl servo;
WiFiManager wifiManager(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
MQTTManager mqttManager(&wifiManager, MQTT_SERVER, MQTT_PORT, MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
GerenciadorHCSR04 sensorRacao(PINO_HCSR04_TRIGGER, PINO_HCSR04_ECHO, DISTANCIA_LIMITE_RACAO, &mqttManager, TOPIC_ALERTA_RACAO);
GerenciadorAlimentacao alimentacao(&servo);
GerenciadorSistema sistema(&servo, &wifiManager, &mqttManager, PINO_BOTAO, PINO_LED_STATUS);
GerenciadorComunicacao comunicacao(&mqttManager, &alimentacao, &sistema);

// ===== VARIÁVEIS DE TIMING =====
unsigned long ultimoHeartbeat = 0;
unsigned long ultimoMonitoramentoRacao = 0;

// ===== CONFIGURAÇÃO INICIAL =====
void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    
    sistema.inicializar();
    sistema.exibirInformacoes();
    sistema.configurarHardware();
    sistema.inicializarComponentes(PINO_SERVO);
    
    sensorRacao.iniciar();
    DEBUG_PRINTLN("Sensor HCSR04 inicializado - Monitoramento de racao ativado");
    
    sistema.inicializarComunicacao();
    comunicacao.definirCallback();
    sistema.conectarServicos(TOPIC_COMANDO);
    sistema.testarComponentes();
    
    DEBUG_PRINTLN("ESP32 REMOTA pronto para receber comandos do CENTRAL!");
    DEBUG_PRINTF("Monitoramento de racao: Distancia limite = %.1f cm\n", DISTANCIA_LIMITE_RACAO);
    DEBUG_PRINTF("Heartbeat configurado para: %lu ms (5 minutos)\n", INTERVALO_HEARTBEAT);
    DEBUG_PRINTF("Monitoramento racao configurado para: %lu ms (5 minutos)\n", INTERVALO_MONITORAMENTO_RACAO);
}

// ===== LOOP PRINCIPAL =====
void loop()
{
    unsigned long agora = millis();
    
    sistema.verificarConexoes();
    comunicacao.processarMensagens();
    sistema.processarBotao();
    sistema.atualizarLedStatus();
    alimentacao.executarCiclo();
    
    // Monitorar nivel de racao (a cada 5 minutos)
    if (agora - ultimoMonitoramentoRacao >= INTERVALO_MONITORAMENTO_RACAO) {
        DEBUG_PRINTLN("🔍 Verificando nível de ração...");
        sensorRacao.monitorar();
        ultimoMonitoramentoRacao = agora;
    }
    
    // Heartbeat a cada 5 minutos
    if (agora - ultimoHeartbeat >= INTERVALO_HEARTBEAT) {
        comunicacao.enviarHeartbeat(TOPIC_HEARTBEAT);
        ultimoHeartbeat = agora;
        
        // Debug para verificar timing
        DEBUG_PRINTLN("💓 Heartbeat enviado - Próximo em 5 minutos");
    }
    
    sistema.monitorarSistema();
    
    delay(MAIN_LOOP_DELAY);
}