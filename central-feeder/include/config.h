#ifndef CONFIG_H
#define CONFIG_H

// ===== CONFIGURAÇÕES DE HARDWARE =====

// Pinos dos botões
#define UP_BUTTON_PIN 32
#define DOWN_BUTTON_PIN 33
#define ENTER_BUTTON_PIN 25

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

// Configurações de debounce
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

// ===== CONFIGURAÇÕES DE INTERFACE =====

// Número máximo de remotas
#define MAX_REMOTAS 6
#define REMOTAS_PER_PAGE 3

// Configurações de refeições
#define REFEICOES_PER_REMOTA 3
#define MIN_QUANTIDADE_GRAMAS 5
#define MAX_QUANTIDADE_GRAMAS 200
#define STEP_QUANTIDADE_GRAMAS 5

// Horários padrão das refeições
#define REFEICAO_1_PADRAO "08:00"
#define REFEICAO_2_PADRAO "14:30"
#define REFEICAO_3_PADRAO "20:00"

// Quantidades padrão (em gramas)
#define QUANTIDADE_PADRAO 40

// ===== CONFIGURAÇÕES DE COMUNICAÇÃO =====

// Serial
#define SERIAL_BAUD_RATE 115200

// UDP para comunicação com remotas (backup)
#define UDP_LOCAL_PORT 4210
#define UDP_REMOTE_PORT 4211
#define UDP_TIMEOUT_MS 5000

// ===== CONFIGURAÇÃO MQTT HIVEMQ CLOUD - CENTRAL =====

// Configurações do servidor HiveMQ Cloud
#define MQTT_SERVER "9aa85a8cfb4a4ba896f2289aa408ba5a.s1.eu.hivemq.cloud" // Substitua pela URL do seu cluster
#define MQTT_PORT 8883                                                    // Porta WebSocket SSL
#define MQTT_USE_SSL true                                                 // WebSocket SSL habilitado

// Credenciais específicas da CENTRAL
#define MQTT_CLIENT_ID "ESP32_Central_001" // ID único da central
#define MQTT_USERNAME "Central"            // Usuário específico da central
#define MQTT_PASSWORD "Senha1234"          // Senha específica da central

// Configurações de conexão
#define MQTT_KEEP_ALIVE 60
#define MQTT_RECONNECT_INTERVAL 10000 // 10 segundos (aumentado para dar tempo ao DNS)
#define MQTT_MAX_RETRIES 3            // Reduzido para não sobrecarregar
#define MQTT_CONN_TIMEOUT 15000       // 15 segundos de timeout de conexão (renomeado para evitar conflito)
#define MQTT_DNS_TIMEOUT 10000        // 10 segundos para resolução DNS

// ===== TÓPICOS MQTT DA CENTRAL =====

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

// Tópicos de compatibilidade com remota (sem ID específico)
#define MQTT_TOPIC_COMANDO_GERAL "alimentador/remota/comando"       // Comando geral para remotas
#define MQTT_TOPIC_STATUS_GERAL "alimentador/remota/status"         // Status geral das remotas
#define MQTT_TOPIC_CONCLUIDO_GERAL "alimentador/remota/concluido"   // Resposta geral das remotas

// Tópicos da central

// Tópicos gerais da central
#define MQTT_TOPIC_CENTRAL_STATUS "alimentador/central/status"   // Status da própria central
#define MQTT_TOPIC_CENTRAL_COMANDO "alimentador/central/comando" // Comandos para a central

// ===== COMANDOS MQTT =====

// Comandos que a central envia para as remotas
#define MQTT_CMD_INICIAR_MOVIMENTO "INICIAR" // Comando para iniciar alimentação
#define MQTT_CMD_PARAR_MOVIMENTO "PARAR"     // Comando para parar
#define MQTT_CMD_STATUS "STATUS"             // Solicitar status
#define MQTT_CMD_HEARTBEAT "PING"            // Verificar se está viva

// Respostas esperadas das remotas
#define MQTT_RESP_MOVIMENTO_INICIADO "INICIADO"   // Remota confirmou início
#define MQTT_RESP_MOVIMENTO_CONCLUIDO "CONCLUIDO" // Remota finalizou movimento
#define MQTT_RESP_ERRO "ERRO"                     // Erro na remota
#define MQTT_RESP_ONLINE "ONLINE"                 // Remota está online
#define MQTT_RESP_OFFLINE "OFFLINE"               // Remota está offline

// ===== FORMATO DAS MENSAGENS MQTT =====

// Formato do comando de horário (JSON)
// {"hora": 8, "minuto": 30, "quantidade": 40}

// Formato do comando de tempo de movimento (JSON)
// {"tempo_movimento": 4}  // 4 segundos (40g ÷ 10g/s = 4s)

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

// ===== CONFIGURAÇÕES DE TIMING =====

// Loop principal
#define MAIN_LOOP_DELAY_MS 50

// Atualização de hora (NTP)
#define NTP_UPDATE_INTERVAL 7200000 // 2 horas (2 * 60 * 60 * 1000)
#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEZONE_OFFSET -3 // UTC-3 (Brasília)
#define NTP_DAYLIGHT_OFFSET 0  // Sem horário de verão

// ===== CONFIGURAÇÕES DE DEBUG =====

// Habilitar debug via Serial
#define DEBUG_ENABLED true
#define DEBUG_WIFI true
#define DEBUG_MQTT true
#define DEBUG_BUTTONS false
#define DEBUG_SCREEN_MANAGER false

// ===== VERSÃO DO SISTEMA =====

#define SYSTEM_VERSION "1.0.0"
#define SYSTEM_BUILD_DATE __DATE__
#define SYSTEM_BUILD_TIME __TIME__

// ===== MACROS UTILITÁRIAS =====

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

// Debug específico para WiFi
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

// Debug específico para MQTT
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

// Validação de limites
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
#define UDP_MAX_RETRIES 3
// MQTT_MAX_RETRIES já definido acima

// ===== FUNÇÕES DE CACHE E HISTÓRICO =====
// Limpa dados armazenados em Preferences (cache)
void limparCache();

// Limpa histórico do sistema (operações de limpeza iguais ao cache por enquanto)
void limparHistorico();

#endif // CONFIG_H
