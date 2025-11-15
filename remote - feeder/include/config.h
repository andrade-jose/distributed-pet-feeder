#ifndef CONFIG_H
#define CONFIG_H

// ===== CONFIGURAÇÃO WIFI =====
#define DEFAULT_WIFI_SSID "Coelhoandrade"
#define DEFAULT_WIFI_PASSWORD "190520jg"

// ===== CONFIGURAÇÃO DA REMOTA =====
extern const int REMOTA_ID;
extern const char* DEVICE_ID;  // "REMOTA_1", "REMOTA_2", etc.

// ===== CONFIGURAÇÃO MQTT - MOSQUITTO UMBREL =====
extern const char* MQTT_SERVER;
extern const int MQTT_PORT;
extern const char* MQTT_CLIENT_ID;
extern const char* MQTT_USERNAME;
extern const char* MQTT_PASSWORD;

// ===== SPARKPLUG B NAMESPACE =====
extern const char* SPARKPLUG_NAMESPACE;
extern const char* GROUP_ID;
extern const char* EDGE_NODE_ID;

// ===== TÓPICOS MQTT SPARKPLUG B =====
extern const char* TOPIC_DBIRTH;
extern const char* TOPIC_DDATA;
extern const char* TOPIC_DDEATH;
extern const char* TOPIC_DCMD;

// ===== CONFIGURAÇÃO DO HARDWARE =====
extern const int PINO_SERVO;
extern const int PINO_BOTAO;
extern const int PINO_LED_STATUS;
extern const int PINO_HCSR04_TRIGGER;
extern const int PINO_HCSR04_ECHO;

// ===== CONFIGURAÇÕES DO SENSOR HCSR04 =====
extern const float DISTANCIA_LIMITE_RACAO;
extern const unsigned long INTERVALO_MONITORAMENTO_RACAO;

// ===== CONFIGURAÇÕES GERAIS =====
extern const unsigned long SERIAL_BAUD_RATE;
extern const unsigned long MAIN_LOOP_DELAY;

// ===== CONFIGURAÇÕES DE HEARTBEAT =====
extern const unsigned long INTERVALO_HEARTBEAT;

// ===== CONFIGURAÇÕES DE DEBUG =====
#define DEBUG_ENABLED true
#define DEBUG_WIFI true
#define DEBUG_MQTT true
#define DEBUG_SENSORS true

// Macros de debug
#if DEBUG_ENABLED
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(x, ...) Serial.printf(x, __VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(x, ...)
#endif

#endif