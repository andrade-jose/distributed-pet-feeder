#pragma once

// ========== VERSÃO DO SISTEMA ==========
#define SYSTEM_VERSION "2.0.0-refactored"

// ========== CONFIGURAÇÕES DE WIFI ==========
#define WIFI_SSID "Coloque seu usuario"
#define WIFI_PASSWORD "Coloque sua senha"

// ========== CONFIGURAÇÕES DE NTP ==========
#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEZONE_OFFSET -3  // GMT-3 (Brasília)
#define NTP_DAYLIGHT_OFFSET 0   // Horário de verão (0 = desabilitado)
#define NTP_UPDATE_INTERVAL 3600000  // Atualizar a cada 1 hora (em ms)

// ========== CONFIGURAÇÕES DE MQTT ==========
#define MQTT_BROKER_HOST "179.98.138.194"  // IP do seu servidor Mosquitto
#define MQTT_BROKER_PORT 8883              // 8883 para TLS, 1883 para sem TLS
#define MQTT_USERNAME "usuario"
#define MQTT_PASSWORD "senha"
#define MQTT_CLIENT_ID "central_gateway"

// TLS Configuration
#define MQTT_USE_TLS true                  // true = usar TLS, false = sem TLS
#define MQTT_VALIDATE_CERT false           // true = validar certificado, false = aceitar qualquer cert (INSEGURO!)
// NOTA: Servidor está pedindo certificado de cliente (mTLS).
// Modo inseguro temporariamente até configurar certificado de cliente.

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

// Tópico de logs offline (remotas enviam histórico quando voltam online)
#define MQTT_TOPIC_LOGS MQTT_TOPIC_PREFIX "/logs"

// ========== CONFIGURAÇÕES DE HARDWARE ==========
// Pinos dos botões
#define BTN_UP_PIN 3300
#define BTN_DOWN_PIN 25
#define BTN_OK_PIN 32

// Configurações LCD I2C
#define LCD_ADDRESS 0x27
#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_SDA_PIN 4  // Pino SDA do I2C
#define LCD_SCL_PIN 15  // Pino SCL do I2C

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
