#include "config.h"

// ===== CONFIGURAÇÃO MQTT - BROKER LOCAL =====
// IMPORTANTE: Substitua pelo IP da sua Central ESP32
// O IP será mostrado no LCD da Central após conectar ao WiFi
const char* MQTT_SERVER = "192.168.15.26";  // IP da Central ESP32
const int MQTT_PORT = 1883;                  // Porta padrão MQTT (sem SSL)
const char* MQTT_CLIENT_ID = "ESP32_Remota_002"; // Remota 2 (ESP32-S3)
// NÃO É NECESSÁRIO USUÁRIO E SENHA NO BROKER LOCAL

// ===== TÓPICOS MQTT =====
const char* TOPIC_COMANDO = "alimentador/remota/comando";
const char* TOPIC_STATUS = "alimentador/remota/status";
const char* TOPIC_RESPOSTA = "alimentador/remota/resposta";
const char* TOPIC_HEARTBEAT = "alimentador/remota/heartbeat";
const char* TOPIC_ALERTA_RACAO = "alimentador/remota/alerta_racao";

// ===== CONFIGURAÇÃO DO HARDWARE ESP32-S3 =====
// Pinos ajustados para ESP32-S3 DevKit
const int PINO_SERVO = 9;          // GPIO9 - Servo Motor
const int PINO_HALL = 10;          // GPIO10 - Sensor Hall
const int PINO_BOTAO = 0;          // GPIO0 - Botão (BOOT button)
const int PINO_LED_STATUS = 48;    // GPIO48 - LED RGB onboard (ou GPIO38 para LED simples)
const int PINO_HCSR04_TRIGGER = 11; // GPIO11 - HC-SR04 Trigger
const int PINO_HCSR04_ECHO = 12;    // GPIO12 - HC-SR04 Echo

// ===== CONFIGURAÇÕES DO SENSOR HCSR04 =====
const float DISTANCIA_LIMITE_RACAO = 5.0; // cm - quando ração está baixa
const unsigned long INTERVALO_MONITORAMENTO_RACAO = 10000; // ms - 10 segundos

// ===== CONFIGURAÇÕES GERAIS =====
const unsigned long SERIAL_BAUD_RATE = 115200;
const unsigned long MAIN_LOOP_DELAY = 50; // ms
