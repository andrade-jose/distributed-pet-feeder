#ifndef CONFIG_H
#define CONFIG_H

// ===== CONFIGURAÇÕES DE HARDWARE =====
#define UP_BUTTON_PIN 32
#define DOWN_BUTTON_PIN 33
#define ENTER_BUTTON_PIN 25
#define DEBUG_BOTOES 1

// Configuração LCD I2C
#define LCD_I2C_ADDR 0x27
#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_SDA_PIN 21
#define LCD_SCL_PIN 22


// Configuração RTC DS1307
#define RTC_I2C_ADDR 0x68
#define RTC_SDA_PIN 21 // Mesmo barramento I2C do LCD
#define RTC_SCL_PIN 22 // Mesmo barramento I2C do LCD

// ===== CONFIGURAÇÕES DE SISTEMA =====
#define BUTTON_DEBOUNCE_MS 200
#define INFO_SCREEN_TIMEOUT 10000
#define WIFI_RECONNECT_INTERVAL 30000
#define WIFI_STATUS_CHECK_INTERVAL 5000

// WiFi padrão
#define DEFAULT_WIFI_SSID "Coelhoandrade"
#define DEFAULT_WIFI_PASSWORD "190520jg"

// ===== CONFIGURAÇÕES WIREGUARD =====
#define WIREGUARD_SERVER "100.95.52.105"      // IP do seu servidor
#define WIREGUARD_PORT 51820
#define WIREGUARD_LOCAL_IP "10.8.0.3"         // ESP32 na VPN
#define WIREGUARD_SERVER_IP "10.8.0.1"        // Servidor na VPN

// ===== CONFIGURAÇÕES MQTT VIA VPN =====
#define MQTT_BROKER_HOST "10.8.0.1"           // ✅ Broker via VPN
#define MQTT_BROKER_PORT 1883
#define MQTT_USERNAME "mqtt_user"
#define MQTT_PASSWORD "mqtt_123"
#define MQTT_CLIENT_ID "ESP32_Central"
#define MQTT_KEEP_ALIVE 60
#define MQTT_RECONNECT_INTERVAL 5000
#define MQTT_MAX_RETRIES 3

// ===== SPARKPLUGB CONFIGURATION =====
#define SPARKPLUG_NAMESPACE "spBv1.0"
#define SPARKPLUG_GROUP "ALIMENTADOR_PETS"
#define SPARKPLUG_NODE_ID "EON_CENTRAL"
#define SPARKPLUG_DEVICE_PREFIX "REMOTA_"

// ===== TÓPICOS SPARKPLUGB =====
// Node (Central) - Tópicos que a CENTRAL vai usar
#define MQTT_TOPIC_NBIRTH "spBv1.0/ALIMENTADOR_PETS/NBIRTH/EON_CENTRAL"
#define MQTT_TOPIC_NDEATH "spBv1.0/ALIMENTADOR_PETS/NDEATH/EON_CENTRAL"
#define MQTT_TOPIC_NDATA "spBv1.0/ALIMENTADOR_PETS/NDATA/EON_CENTRAL"
#define MQTT_TOPIC_NCMD "spBv1.0/ALIMENTADOR_PETS/NCMD/EON_CENTRAL"

// Devices (Remotas) - Wildcards para subscrever
#define MQTT_TOPIC_DBIRTH_ALL "spBv1.0/ALIMENTADOR_PETS/DBIRTH/EON_CENTRAL/+"
#define MQTT_TOPIC_DDATA_ALL "spBv1.0/ALIMENTADOR_PETS/DDATA/EON_CENTRAL/+"
#define MQTT_TOPIC_DDEATH_ALL "spBv1.0/ALIMENTADOR_PETS/DDEATH/EON_CENTRAL/+"

// Device específico - Template para publicar comandos (usar com snprintf)
#define MQTT_TOPIC_DCMD_TEMPLATE "spBv1.0/ALIMENTADOR_PETS/DCMD/EON_CENTRAL/REMOTA_%d"

// ===== CONFIGURAÇÕES DE TIMEOUT (Otimizado para heartbeat de 10s) =====
#define TIMEOUT_HEARTBEAT_REMOTA 30000  // 30 segundos sem heartbeat (antes: 60s)
#define TIMEOUT_CONEXAO_REMOTA 60000    // 1 minuto sem qualquer sinal (antes: 5min)
#define TIMEOUT_REMOTA_ATIVA 120000     // 2 minutos para considerar ativa (antes: 10min)
#define INTERVALO_VERIFICACAO_TIMEOUT 5000 // Verificar a cada 5s

// ===== CONFIGURAÇÕES DE INTERFACE =====
#define MAX_REMOTAS 6
#define REFEICOES_PER_REMOTA 3
#define MIN_QUANTIDADE_GRAMAS 100
#define MAX_QUANTIDADE_GRAMAS 1000
#define STEP_QUANTIDADE_GRAMAS 100

// Horários padrão das refeições
#define REFEICAO_1_PADRAO "08:00"
#define REFEICAO_2_PADRAO "14:30"
#define REFEICAO_3_PADRAO "20:00"

// Quantidades padrão (em gramas)
#define QUANTIDADE_PADRAO 250

// ===== CONFIGURAÇÕES DE COMUNICAÇÃO =====
#define SERIAL_BAUD_RATE 115200

// ===== TÓPICOS ANTIGOS (DESABILITADOS - Migrado para SparkplugB) =====
// Os tópicos customizados foram substituídos por SparkplugB
// Manter comentado para referência:
/*
#define MQTT_TOPIC_ALERTA_RACAO "a/r/al"
#define MQTT_TOPIC_COMANDO_GERAL "a/r/c"
#define MQTT_TOPIC_STATUS_GERAL "a/r/st"
#define MQTT_TOPIC_RESPOSTA_GERAL "a/r/rsp"
#define MQTT_TOPIC_HEARTBEAT_GERAL "a/r/hb"
#define MQTT_TOPIC_COMANDO_REMOTA "a/r/%d/c"
#define MQTT_TOPIC_HORARIO_REMOTA "a/r/%d/h"
#define MQTT_TOPIC_TEMPO_MOVIMENTO "a/r/%d/tmp"
#define MQTT_TOPIC_STATUS_REMOTA "a/r/%d/st"
#define MQTT_TOPIC_VIDA_REMOTA "a/r/%d/hb"
#define MQTT_TOPIC_RESPOSTA_REMOTA "a/r/%d/rsp"
#define MQTT_TOPIC_CONCLUIDO_GERAL "a/r/done"
#define MQTT_TOPIC_CENTRAL_STATUS "a/c/st"
#define MQTT_TOPIC_CONFIG_QUERY "a/c/cq"
#define MQTT_TOPIC_CONFIG_SET "a/c/cs"
#define MQTT_TOPIC_CONFIG_UPDATE "a/c/cu"
#define MQTT_TOPIC_STATE "a/c/s/%d"
*/

// ===== COMANDOS MQTT =====
#define MQTT_CMD_INICIAR_MOVIMENTO "INICIAR"
#define MQTT_CMD_PARAR_MOVIMENTO "PARAR"
#define MQTT_CMD_STATUS "STATUS"
#define MQTT_CMD_HEARTBEAT "PING"

// ===== CONFIGURAÇÕES DE ARMAZENAMENTO =====
#define PREFS_WIFI_NAMESPACE "wifi"
#define PREFS_SYSTEM_NAMESPACE "system"
#define PREFS_REMOTAS_NAMESPACE "remotas"

// ===== CONFIGURAÇÕES DO SISTEMA DE ALERTA DE RAÇÃO =====
#define ALERTA_PISCADA_INTERVALO 500
#define ALERTA_TIMEOUT_MAXIMO 300000
#define ALERTA_RACAO_TITULO "*** ATENCAO ***"
#define ALERTA_RACAO_LINHA1 "RACAO BAIXA!"
#define ALERTA_RACAO_LINHA2 "Remota %d"
#define ALERTA_RACAO_LINHA3 "REABASTECER AGORA"

// ===== CONFIGURAÇÕES DE TIMING =====
#define MAIN_LOOP_DELAY_MS 50

// Configurações NTP
#define NTP_SERVER "pool.ntp.org"           // Servidor NTP
#define NTP_TIMEZONE_OFFSET -3              // Fuso horário (Brasília = -3)
#define NTP_DAYLIGHT_OFFSET 0               // Horário de verão
#define NTP_SYNC_INTERVAL (2 * 60 * 60 * 1000) // Sincronizar a cada 2 horas (em ms)
#define NTP_TIMEOUT 10000                   // Timeout da sincronização (10 segundos)

// ===== CONFIGURAÇÕES DE DEBUG =====
#define DEBUG_ENABLED true

// ===== VERSÃO DO SISTEMA =====
#define SYSTEM_VERSION "1.0.0"
#define SYSTEM_BUILD_DATE __DATE__
#define SYSTEM_BUILD_TIME __TIME__

// ===== MACROS UTILITARIAS =====
#if DEBUG_ENABLED
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

#endif // CONFIG_H