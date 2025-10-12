#include "config.h"

// ===== CONFIGURAÇÃO DA REMOTA =====
// IMPORTANTE: Defina um ID único para cada remota (1, 2, 3, 4)
const int REMOTA_ID = 1;

// ===== CONFIGURAÇÃO MQTT - BROKER LOCAL =====
// IMPORTANTE: Substitua pelo IP da sua Central ESP32
// O IP será mostrado no LCD da Central após conectar ao WiFi
const char* MQTT_SERVER = "192.168.15.26";  // IP da Central ESP32
const int MQTT_PORT = 1883;                  // Porta padrão MQTT (sem SSL)
const char* MQTT_CLIENT_ID = "ESP32_Remota_001"; // Pode manter o mesmo
// NÃO É NECESSÁRIO USUÁRIO E SENHA NO BROKER LOCAL

// ===== TÓPICOS MQTT =====
const char* TOPIC_COMANDO = "alimentador/remota/comando";
const char* TOPIC_STATUS = "alimentador/remota/status";
const char* TOPIC_RESPOSTA = "alimentador/remota/resposta";
const char* TOPIC_HEARTBEAT = "alimentador/remota/heartbeat";
const char* TOPIC_ALERTA_RACAO = "alimentador/remota/alerta_racao";

// ===== CONFIGURAÇÃO DO HARDWARE =====
const int PINO_SERVO = 5;
const int PINO_HALL = 4;
const int PINO_BOTAO = 18;
const int PINO_LED_STATUS = 13;
const int PINO_HCSR04_TRIGGER = 2;
const int PINO_HCSR04_ECHO = 15;

// ===== CONFIGURAÇÕES DO SENSOR HCSR04 =====
const float DISTANCIA_LIMITE_RACAO = 5.0; // cm - quando ração está baixa
const unsigned long INTERVALO_MONITORAMENTO_RACAO = 10000; // ms - 10 segundos

// ===== CONFIGURAÇÕES GERAIS =====
const unsigned long SERIAL_BAUD_RATE = 115200;
const unsigned long MAIN_LOOP_DELAY = 50; // ms
