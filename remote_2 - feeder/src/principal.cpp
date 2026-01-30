#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "gerenciador_servo.h"
#include "gerenciador_sensorhall.h"
#include "gerenciador_wifi.h"
#include "gerenciador_mqtt.h"
#include "gerenciador_hcsr04.h"
#include "gerenciador_alimentacao.h"
#include "gerenciador_sistema.h"
#include "gerenciador_comunicacao.h"

// ===== INSTÂNCIAS DOS COMPONENTES =====
ServoControl servo;
SensorHall sensorHall;
WiFiManager wifiManager(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
MQTTManager mqttManager(&wifiManager, MQTT_SERVER, MQTT_PORT, MQTT_CLIENT_ID);
GerenciadorHCSR04 sensorRacao(PINO_HCSR04_TRIGGER, PINO_HCSR04_ECHO, DISTANCIA_LIMITE_RACAO, &mqttManager, TOPIC_ALERTA_RACAO);
GerenciadorAlimentacao alimentacao(&servo, &sensorHall);
GerenciadorSistema sistema(&servo, &sensorHall, &wifiManager, &mqttManager, PINO_BOTAO, PINO_LED_STATUS);
GerenciadorComunicacao comunicacao(&mqttManager, &alimentacao, &sistema);

// ===== CONFIGURAÇÃO INICIAL =====
void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    
    sistema.inicializar();
    sistema.exibirInformacoes();
    sistema.configurarHardware();
    sistema.inicializarComponentes(PINO_SERVO, PINO_HALL);
    
    sensorRacao.iniciar();
    DEBUG_PRINTLN("Sensor HCSR04 inicializado - Monitoramento de racao ativado");
    
    sistema.inicializarComunicacao();
    comunicacao.definirCallback();
    sistema.conectarServicos(TOPIC_COMANDO);
    sistema.testarComponentes();
    
    DEBUG_PRINTLN("ESP32 REMOTA pronto para receber comandos do CENTRAL!");
    DEBUG_PRINTF("Monitoramento de racao: Distancia limite = %.1f cm\n", DISTANCIA_LIMITE_RACAO);
}

// ===== LOOP PRINCIPAL =====
void loop()
{
    sistema.verificarConexoes();
    comunicacao.processarMensagens();
    sistema.processarBotao();
    sistema.atualizarLedStatus();
    alimentacao.executarCiclo();
    
    // Monitorar nivel de racao
    static unsigned long ultimoMonitoramentoRacao = 0;
    unsigned long agora = millis();
    if (agora - ultimoMonitoramentoRacao >= INTERVALO_MONITORAMENTO_RACAO) {
        sensorRacao.monitorar();
        ultimoMonitoramentoRacao = agora;
    }
    
    sistema.monitorarSistema();
    comunicacao.enviarHeartbeat(TOPIC_HEARTBEAT);
    
    delay(MAIN_LOOP_DELAY);
}