#include "config.h"

// ===== CONFIGURAÇÃO DA REMOTA =====
const int REMOTA_ID = 1;
const char* DEVICE_ID = "REMOTA_1";  // Alterar para REMOTA_2, REMOTA_3, etc.

// ===== CONFIGURAÇÃO MQTT - MOSQUITTO UMBREL =====
const char* MQTT_SERVER = "100.95.52.105";  // Tailscale IP do Umbrel
const int MQTT_PORT = 1883;                  // Porta não-SSL
const char* MQTT_CLIENT_ID = "ESP32_REMOTA_1";
const char* MQTT_USERNAME = "mqtt_user";
const char* MQTT_PASSWORD = "senha_segura";

// ===== SPARKPLUG B NAMESPACE =====
const char* SPARKPLUG_NAMESPACE = "spBv1.0";
const char* GROUP_ID = "ALIMENTADOR_PETS";
const char* EDGE_NODE_ID = "EON_CENTRAL";

// ===== TÓPICOS MQTT SPARKPLUG B =====
// Formato: spBv1.0/ALIMENTADOR_PETS/DBIRTH/EON_CENTRAL/REMOTA_1
const char* TOPIC_DBIRTH = "spBv1.0/ALIMENTADOR_PETS/DBIRTH/EON_CENTRAL/REMOTA_1";
const char* TOPIC_DDATA = "spBv1.0/ALIMENTADOR_PETS/DDATA/EON_CENTRAL/REMOTA_1";
const char* TOPIC_DDEATH = "spBv1.0/ALIMENTADOR_PETS/DDEATH/EON_CENTRAL/REMOTA_1";
const char* TOPIC_DCMD = "spBv1.0/ALIMENTADOR_PETS/DCMD/EON_CENTRAL/REMOTA_1";

// ===== CONFIGURAÇÃO DO HARDWARE =====
const int PINO_SERVO = 5;
const int PINO_BOTAO = 33;
const int PINO_LED_STATUS = 13;
const int PINO_HCSR04_TRIGGER = 2;
const int PINO_HCSR04_ECHO = 15;

// ===== CONFIGURAÇÕES DO SENSOR HCSR04 =====
const float DISTANCIA_LIMITE_RACAO = 5.0; // cm - quando ração está baixa
const unsigned long INTERVALO_MONITORAMENTO_RACAO = 300000; // 5 minutos (300 segundos)

// ===== CONFIGURAÇÕES DE HEARTBEAT =====
const unsigned long INTERVALO_HEARTBEAT = 5000; // 5 segundos (telemetria SparkplugB)

// ===== CONFIGURAÇÕES GERAIS =====
const unsigned long SERIAL_BAUD_RATE = 115200;
const unsigned long MAIN_LOOP_DELAY = 50; // ms
