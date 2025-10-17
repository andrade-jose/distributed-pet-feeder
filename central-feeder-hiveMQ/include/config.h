#ifndef CONFIG_H
#define CONFIG_H

// ===== CONFIGURAÃ‡Ã•ES DE HARDWARE =====

// Pinos dos botÃµes
#define UP_BUTTON_PIN 32
#define DOWN_BUTTON_PIN 33
#define ENTER_BUTTON_PIN 25

// ConfiguraÃ§Ã£o LCD I2C
#define LCD_I2C_ADDR 0x27
#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_SDA_PIN 21
#define LCD_SCL_PIN 22

// ConfiguraÃ§Ã£o RTC DS1307
#define RTC_I2C_ADDR 0x68
#define RTC_SDA_PIN 21 // Mesmo barramento I2C do LCD
#define RTC_SCL_PIN 22 // Mesmo barramento I2C do LCD

// ===== CONFIGURAÃ‡Ã•ES DE SISTEMA =====

// ConfiguraÃ§Ãµes de debounce
#define BUTTON_DEBOUNCE_MS 200

// Timeouts das telas (em milissegundos)
#define INFO_SCREEN_TIMEOUT 10000  // 10 segundos
#define WIFI_SCAN_TIMEOUT 15000    // 15 segundos
#define WIFI_CONNECT_TIMEOUT 30000 // 30 segundos

// Configurações de WiFi
#define WIFI_RECONNECT_INTERVAL 30000   // 30 segundos
#define WIFI_STATUS_CHECK_INTERVAL 5000 // 5 segundos
#define WIFI_MAX_NETWORKS 20            // Máximo de redes no scan

// WiFi padrão para desenvolvimento/teste (deixe vazio "" para usar apenas interface)
#define DEFAULT_WIFI_SSID "Coelhoandrade" // Coloque o nome da sua rede aqui
#define DEFAULT_WIFI_PASSWORD "190520jg"  // Coloque a senha aqui

// ===== CONFIGURAÇÕES MQTT HIVEMQ CLOUD =====

// Configurações do HiveMQ Cloud Broker
#define MQTT_BROKER_HOST "56d05fe4fbc64e80964aa78d92456f22.s1.eu.hivemq.cloud"  // Seu cluster HiveMQ
#define MQTT_BROKER_PORT 8883              // Porta TLS/SSL (use 8883 para conexão segura)
#define MQTT_USERNAME "seu_usuario"        // SUBSTITUA: Usuário do HiveMQ Cloud
#define MQTT_PASSWORD "sua_senha"          // SUBSTITUA: Senha do HiveMQ Cloud
#define MQTT_CLIENT_ID "ESP32_Central"     // ID único do cliente
#define MQTT_KEEP_ALIVE 60                 // Keep alive em segundos

// Configurações de reconexão
#define MQTT_RECONNECT_INTERVAL 5000       // 5 segundos entre tentativas
#define MQTT_MAX_RETRIES 3                 // Máximo de tentativas por ciclo

// ===== CONFIGURAÇÃO TLS/SSL =====
// Certificado raiz HiveMQ Cloud (Let's Encrypt ISRG Root X1)
const char* HIVEMQ_ROOT_CA = \
"-----BEGIN CERTIFICATE-----\n" \
"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\n" \
"-----END CERTIFICATE-----\n";

// ===== CONFIGURAÃ‡Ã•ES DE INTERFACE =====

// NÃºmero mÃ¡ximo de remotas
#define MAX_REMOTAS 6

// ConfiguraÃ§Ãµes de refeiÃ§Ãµes
#define REFEICOES_PER_REMOTA 3
#define MIN_QUANTIDADE_GRAMAS 100
#define MAX_QUANTIDADE_GRAMAS 1000
#define STEP_QUANTIDADE_GRAMAS 100

// HorÃ¡rios padrÃ£o das refeiÃ§Ãµes
#define REFEICAO_1_PADRAO "08:00"
#define REFEICAO_2_PADRAO "14:30"
#define REFEICAO_3_PADRAO "20:00"

// Quantidades padrÃ£o (em gramas)
#define QUANTIDADE_PADRAO 250

// ===== CONFIGURAÃ‡Ã•ES DE COMUNICAÃ‡ÃƒO =====
3
// Serial
#define SERIAL_BAUD_RATE 115200

// ===== TÃ“PICOS MQTT DA CENTRAL =====

// Novo tópico para alertas de ração baixa
#define MQTT_TOPIC_ALERTA_RACAO "alimentador/remota/alerta_racao"        // Alertas de ração baixa das remotas
#define MQTT_TOPIC_ALERTA_RACAO_RESOLVIDO "alimentador/remota/racao_ok"  // Quando ração é reabastecida

// Tópicos que a CENTRAL vai PUBLICAR (enviar comandos para as remotas)
#define MQTT_TOPIC_PREFIX "alimentador"
#define MQTT_TOPIC_COMANDO_REMOTA "alimentador/remota/%d/comando" // %d = ID da remota (1,2,3,4...)
#define MQTT_TOPIC_HORARIO_REMOTA "alimentador/remota/%d/horario" // %d = ID da remota
#define MQTT_TOPIC_TEMPO_MOVIMENTO "alimentador/remota/%d/tempo"  // %d = ID da remota

// Tópicos que a CENTRAL vai INSCREVER (receber status das remotas)
#define MQTT_TOPIC_STATUS_REMOTA "alimentador/remota/%d/status"     // %d = ID da remota
#define MQTT_TOPIC_VIDA_REMOTA "alimentador/remota/%d/vida"         // %d = ID da remota (heartbeat)
#define MQTT_TOPIC_RESPOSTA_REMOTA "alimentador/remota/%d/resposta" // %d = ID da remota
#define MQTT_TOPIC_HEARTBEAT_GERAL "alimentador/remota/heartbeat"   // Heartbeat geral das remotas

// Tópicos de compatibilidade com remota (sem ID especÃ­fico)
#define MQTT_TOPIC_COMANDO_GERAL "alimentador/remota/comando"       // Comando geral para remotas
#define MQTT_TOPIC_STATUS_GERAL "alimentador/remota/status"         // Status geral das remotas
#define MQTT_TOPIC_CONCLUIDO_GERAL "alimentador/remota/concluido"   // Resposta geral das remotas

// Tópicos da central

// Tópicos gerais da central
#define MQTT_TOPIC_CENTRAL_STATUS "alimentador/central/status"   // Status da prÃ³pria central
#define MQTT_TOPIC_CENTRAL_COMANDO "alimentador/central/comando" // Comandos para a central

// ===== COMANDOS MQTT =====

// Comandos que a central envia para as remotas
#define MQTT_CMD_INICIAR_MOVIMENTO "INICIAR" // Comando para iniciar alimentaÃ§Ã£o
#define MQTT_CMD_PARAR_MOVIMENTO "PARAR"     // Comando para parar
#define MQTT_CMD_STATUS "STATUS"             // Solicitar status
#define MQTT_CMD_HEARTBEAT "PING"            // Verificar se estÃ¡ viva

// Respostas esperadas das remotas
#define MQTT_RESP_MOVIMENTO_INICIADO "INICIADO"   // Remota confirmou inÃ­cio
#define MQTT_RESP_MOVIMENTO_CONCLUIDO "CONCLUIDO" // Remota finalizou movimento
#define MQTT_RESP_ERRO "ERRO"                     // Erro na remota
#define MQTT_RESP_ONLINE "ONLINE"                 // Remota estÃ¡ online
#define MQTT_RESP_OFFLINE "OFFLINE"               // Remota estÃ¡ offline

// ===== FORMATO DAS MENSAGENS MQTT =====

// Formato do comando de horÃ¡rio (JSON)
// {"hora": 8, "minuto": 30, "quantidade": 40}

// Formato do comando de tempo de movimento (JSON)
// {"tempo_movimento": 4}  // 4 segundos (40g Ã· 10g/s = 4s)

// Formato do comando de iniciar movimento (JSON)
// {"acao": "INICIAR", "tempo": 4, "timestamp": 1234567890}

// Formato da resposta de status (JSON)
// {"status": "ONLINE", "ultimo_movimento": "08:30", "timestamp": 1234567890}

// ===== CONFIGURAÇÕES DE ARMAZENAMENTO =====

// Namespaces para Preferences
#define PREFS_WIFI_NAMESPACE "wifi"
#define PREFS_SYSTEM_NAMESPACE "system"
#define PREFS_REMOTAS_NAMESPACE "remotas"

// Chaves para Preferences
#define PREFS_WIFI_SSID "ssid"
#define PREFS_WIFI_PASSWORD "password"
#define PREFS_LCD_BACKLIGHT "lcd_backlight"
#define PREFS_AUTO_RECONNECT "auto_reconnect"

// ===== CONFIGURAÇÕES DO SISTEMA DE ALERTA DE RAÇÃO =====

// Configurações de exibição do alerta
#define ALERTA_PISCADA_INTERVALO 500        // ms entre piscadas do texto de alerta
#define ALERTA_TIMEOUT_MAXIMO 300000        // 5 minutos máximo de alerta contínuo
#define ALERTA_PRIORIDADE_ALTA true         // Alerta substitui outras telas

// Mensagens de alerta
#define ALERTA_RACAO_TITULO "*** ATENCAO ***"
#define ALERTA_RACAO_LINHA1 "RACAO BAIXA!"
#define ALERTA_RACAO_LINHA2 "Remota %d"                    // %d será substituído pelo ID da remota
#define ALERTA_RACAO_LINHA3 "REABASTECER AGORA"
// ===== CONFIGURAÇÕES DE TIMING =====

// Loop principal
#define MAIN_LOOP_DELAY_MS 50

// AtualizaÃ§Ã£o de hora (NTP)
#define NTP_UPDATE_INTERVAL 7200000 // 2 horas (2 * 60 * 60 * 1000)
#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEZONE_OFFSET -3 // UTC-3 (BrasÃ­lia)
#define NTP_DAYLIGHT_OFFSET 0  // Sem horÃ¡rio de verÃ£o
#define ALERTA_VERIFICACAO_INTERVALO 2000   // Verificar alertas a cada 2 segundos
#define ALERTA_DEBOUNCE_MS 5000             // Evitar spam de alertas (5 segundos)

// ===== CONFIGURAÇÕES DE DEBUG =====

// Habilitar debug via Serial
#define DEBUG_ENABLED true
#define DEBUG_WIFI true
#define DEBUG_MQTT true
#define DEBUG_BUTTONS false
#define DEBUG_SCREEN_MANAGER false

// ===== VERSÃƒO DO SISTEMA =====

#define SYSTEM_VERSION "1.0.0"
#define SYSTEM_BUILD_DATE __DATE__
#define SYSTEM_BUILD_TIME __TIME__

// ===== MACROS UTILITARIAS =====

// Debug condicional
#if DEBUG_ENABLED
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

// Debug especí­fico para WiFi
#if DEBUG_WIFI && DEBUG_ENABLED
#define DEBUG_WIFI_PRINT(x)  \
    Serial.print("[WiFi] "); \
    Serial.print(x)
#define DEBUG_WIFI_PRINTLN(x) \
    Serial.print("[WiFi] ");  \
    Serial.println(x)
#else
#define DEBUG_WIFI_PRINT(x)
#define DEBUG_WIFI_PRINTLN(x)
#endif

// Debug específico para sistema de alerta
#if DEBUG_ENABLED
#define DEBUG_ALERTA_PRINT(x)  \
    Serial.print("[ALERTA] "); \
    Serial.print(x)
#define DEBUG_ALERTA_PRINTLN(x) \
    Serial.print("[ALERTA] ");  \
    Serial.println(x)
#define DEBUG_ALERTA_PRINTF(f, ...) \
    Serial.print("[ALERTA] ");      \
    Serial.printf(f, __VA_ARGS__)
#else
#define DEBUG_ALERTA_PRINT(x)
#define DEBUG_ALERTA_PRINTLN(x)
#define DEBUG_ALERTA_PRINTF(f, ...)
#endif

// Debug especí­fico para MQTT
#if DEBUG_MQTT && DEBUG_ENABLED
#define DEBUG_MQTT_PRINT(x)  \
    Serial.print("[MQTT] "); \
    Serial.print(x)
#define DEBUG_MQTT_PRINTLN(x) \
    Serial.print("[MQTT] ");  \
    Serial.println(x)
#define DEBUG_MQTT_PRINTF(f, ...) \
    Serial.print("[MQTT] ");      \
    Serial.printf(f, __VA_ARGS__)
#else
#define DEBUG_MQTT_PRINT(x)
#define DEBUG_MQTT_PRINTLN(x)
#define DEBUG_MQTT_PRINTF(f, ...)
#endif

// ValidaÃ§Ã£o de limites
#define CONSTRAIN_GRAMAS(x) constrain(x, MIN_QUANTIDADE_GRAMAS, MAX_QUANTIDADE_GRAMAS)

// ===== CONFIGURAÇÕES AVANÇADAS =====

// Watchdog (para futuro)
#define WATCHDOG_TIMEOUT_MS 30000 // 30 segundos

// Buffer sizes
#define WIFI_SSID_MAX_LENGTH 32
#define WIFI_PASSWORD_MAX_LENGTH 64
#define MESSAGE_BUFFER_SIZE 256

// Retry counts
#define WIFI_MAX_RETRIES 3
// MQTT_MAX_RETRIES já definido acima

#endif // CONFIG_H