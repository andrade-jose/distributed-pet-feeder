#pragma once

// ========== VERSÃO DO SISTEMA ==========
#define SYSTEM_VERSION "2.0.0-refactored"

// ========== CONFIGURAÇÕES DE WIFI ==========
#define WIFI_SSID "SEU_WIFI"
#define WIFI_PASSWORD "SUA_SENHA"

// ========== CONFIGURAÇÕES DE MQTT ==========
#define MQTT_BROKER_HOST "192.168.1.100"  // IP do seu servidor Mosquitto
#define MQTT_BROKER_PORT 8883              // 8883 para TLS, 1883 para sem TLS
#define MQTT_USERNAME "petfeeder"
#define MQTT_PASSWORD "senha_mqtt"
#define MQTT_CLIENT_ID "central_gateway"

// ========== TÓPICOS MQTT ==========
#define MQTT_TOPIC_PREFIX "petfeeder"

// Tópicos da central
#define MQTT_TOPIC_CENTRAL_STATUS MQTT_TOPIC_PREFIX "/central/status"
#define MQTT_TOPIC_CENTRAL_CMD MQTT_TOPIC_PREFIX "/central/cmd"

// Tópicos das remotas (usar com sprintf para incluir ID)
// Exemplo: petfeeder/remote/1/status
#define MQTT_TOPIC_REMOTE_STATUS MQTT_TOPIC_PREFIX "/remote/%d/status"
#define MQTT_TOPIC_REMOTE_CMD MQTT_TOPIC_PREFIX "/remote/%d/cmd"
#define MQTT_TOPIC_REMOTE_DATA MQTT_TOPIC_PREFIX "/remote/%d/data"

// ========== CONFIGURAÇÕES DE HARDWARE ==========
// Pinos dos botões
#define BTN_UP_PIN 34
#define BTN_DOWN_PIN 35
#define BTN_OK_PIN 32
#define BTN_BACK_PIN 33

// Configurações LCD I2C
#define LCD_ADDRESS 0x27
#define LCD_COLS 20
#define LCD_ROWS 4

// ========== CONFIGURAÇÕES DO SISTEMA ==========
#define MAX_REMOTAS 4
#define MAX_MEALS_PER_REMOTE 3

// Timeouts (em milisegundos)
#define REMOTE_TIMEOUT 600000          // 10 minutos
#define MQTT_RECONNECT_INTERVAL 5000   // 5 segundos
#define SCREEN_UPDATE_INTERVAL 100     // 100ms

// ========== DEBUG ==========
#define DEBUG_ENABLED true

#if DEBUG_ENABLED
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif