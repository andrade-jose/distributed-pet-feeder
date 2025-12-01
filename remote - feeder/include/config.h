// config.h
#ifndef CONFIG_H
#define CONFIG_H

// =============================================
// CONFIGURAÇÕES DE DEBUG
// =============================================
#define DEBUG 1
#define USE_RTC_MOCK 1

// Sistema de Logging Aprimorado
#if DEBUG
    // Macros básicas
    #define LOG(x) Serial.println(x)
    #define LOG_NO_NL(x) Serial.print(x)

    // Macros com prefixos coloridos e formatados
    #define LOG_INFO(msg) Serial.printf("[INFO ] %s\n", String(msg).c_str())
    #define LOG_SUCCESS(msg) Serial.printf("[OK   ] ✓ %s\n", String(msg).c_str())
    #define LOG_ERROR(msg) Serial.printf("[ERROR] ✗ %s\n", String(msg).c_str())
    #define LOG_WARN(msg) Serial.printf("[WARN ] ⚠ %s\n", String(msg).c_str())
    #define LOG_DEBUG(msg) Serial.printf("[DEBUG] %s\n", String(msg).c_str())

    // Macros para seções
    #define LOG_SECTION(title) do { \
        Serial.println(); \
        Serial.println("=================================================="); \
        Serial.println("  " + String(title)); \
        Serial.println("=================================================="); \
    } while(0)

    #define LOG_SUBSECTION(title) do { \
        Serial.println(); \
        Serial.println("----------------------------------------"); \
        Serial.println("  " + String(title)); \
        Serial.println("----------------------------------------"); \
    } while(0)

    // Macros para operações
    #define LOG_START(operation) Serial.printf("⏳ Iniciando: %s...\n", String(operation).c_str())
    #define LOG_COMPLETE(operation) Serial.printf("✅ Concluído: %s\n", String(operation).c_str())
    #define LOG_FAILED(operation) Serial.printf("❌ Falhou: %s\n", String(operation).c_str())

    // Macros para valores-chave
    #define LOG_KV(key, value) Serial.printf("   • %s: %s\n", String(key).c_str(), String(value).c_str())
    #define LOG_MQTT_IN(topic, msg) Serial.printf("📥 MQTT IN  [%s]: %s\n", String(topic).c_str(), String(msg).c_str())
    #define LOG_MQTT_OUT(topic, msg) Serial.printf("📤 MQTT OUT [%s]: %s\n", String(topic).c_str(), String(msg).c_str())

    // Separadores visuais
    #define LOG_SEPARATOR() Serial.println("------------------------------------------------------------")
    #define LOG_SEPARATOR_DOUBLE() Serial.println("============================================================")

#else
    #define LOG(x)
    #define LOG_NO_NL(x)
    #define LOG_INFO(msg)
    #define LOG_SUCCESS(msg)
    #define LOG_ERROR(msg)
    #define LOG_WARN(msg)
    #define LOG_DEBUG(msg)
    #define LOG_SECTION(title)
    #define LOG_SUBSECTION(title)
    #define LOG_START(operation)
    #define LOG_COMPLETE(operation)
    #define LOG_FAILED(operation)
    #define LOG_KV(key, value)
    #define LOG_MQTT_IN(topic, msg)
    #define LOG_MQTT_OUT(topic, msg)
    #define LOG_SEPARATOR()
    #define LOG_SEPARATOR_DOUBLE()
#endif

// =============================================
// CONFIGURAÇÕES DE HARDWARE - ESP32-S3
// =============================================
#define SERVO_PIN 5
#define SENSOR_PIN 15
#define RTC_SDA_PIN 8
#define RTC_SCL_PIN 10  // Movido para pino 10 (caso use RTC físico no futuro)
// =============================================
//  CONFIGURAÇÕES DE NTP 
// =============================================
#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEZONE_OFFSET -3  // GMT-3 (Brasília)
#define NTP_DAYLIGHT_OFFSET 0   // Horário de verão (0 = desabilitado)
#define NTP_UPDATE_INTERVAL 3600000  // Atualizar a cada 1 hora (em ms)
// =============================================
// CONFIGURAÇÕES DE REDE
// =============================================
// WiFi
#define WIFI_SSID "Coloque seu usuario"
#define WIFI_PASSWORD "Coloque sua senha"

// MQTT com TLS
#define MQTT_BROKER "179.98.138.194"  // Ou IP
#define MQTT_PORT 8883                // Porta TLS
#define MQTT_USER "usuario"
#define MQTT_PASSWORD "senha"
#define MQTT_VALIDATE_CERT false      // false = aceita qualquer certificado (inseguro)

// Certificado TLS (formato PEM)
static const char MQTT_ROOT_CA[] = R"(-----BEGIN CERTIFICATE-----
MIIDgzCCAmugAwIBAgIUR8VrpyjAUYLfFlqgvpA3NCLOw8AwDQYJKoZIhvcNAQEL
BQAwUTELMAkGA1UEBhMCQlIxCzAJBgNVBAgMAlNQMRIwEAYDVQQHDAlTYW8gUGF1
bG8xDzANBgNVBAoMBlVtYnJlbDEQMA4GA1UEAwwHTVFUVCBDQTAeFw0yNTExMTUx
NjE0NDdaFw0zNTExMTMxNjE0NDdaMFExCzAJBgNVBAYTAkJSMQswCQYDVQQIDAJT
UDESMBAGA1UEBwwJU2FvIFBhdWxvMQ8wDQYDVQQKDAZVbWJyZWwxEDAOBgNVBAMM
B01RVFQgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCioB5ApI5q
pdtIMHKEfaiFvxJ3EFY38AF3ShX4WP5EVspVVcLpOJfKFqtDySMfN9b606zxikZd
W/e8wHUESbqjW5FIkUh1TKIwfrk14gpeIQdeYVNlLGwYsDoYTVq63DE+b77xb57a
c27uRBabCcOA+eTnHeG50ZJMgV7NA4XEqTPm+63N6OZR+Kvy0QfTMtbkhiWInWTi
egGjGAmGnc6SZ8YTB2F1k62LIQRDrumdQy/8icWki+gif6KSVrN8d1n49QLMZ4cN
hxDvre4MyBvhdHBanQA8Y49D+a0Ve9/Y4v1L6xL3u0VhdJ1baEEVSxcLQBd++V/X
60D8goU44RQrAgMBAAGjUzBRMB0GA1UdDgQWBBQdpCHq26TlavGXnHoBVww+cBfK
KDAfBgNVHSMEGDAWgBQdpCHq26TlavGXnHoBVww+cBfKKDAPBgNVHRMBAf8EBTAD
AQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAPfabyLOvmHnPeYMfqscGI1Ge4bgE08BGX
I1xR0t/GZcoEWJQzOiXc/OwfsTAwWkrF4Ubc45D2wcxKw0lz84kHUoqQFFkeIOMY
4I0CMwkXl62qCBUmvfPW2If+A+KEda8ubFVgddWfiXeDjfyjEJbAhoycvr9zFmv1
gKiynqEJPnNnsPfV2bTRFI+wp3coxAsCOBlGRIIzmSl1XHzgefVmnZfYzELl+g3D
BL4H2yA7Uq6vWpW4VOIyPCLAGuL6HotBkqjb3VO5BoqADKPbr5dsS/cg9I91rZZt
gCtDmwltHwVs+x7taXl1Gda+MSFulvJCClE8iEJ6T916MzH9OO+w
-----END CERTIFICATE-----
)";

// =============================================
// CONFIGURAÇÕES DE IDENTIFICAÇÃO
// =============================================
// ID numérico da remota (1-4) - CONFIGURAR ANTES DE COMPILAR
#define REMOTE_ID 1

// ID string para logs (compatível com protocolo da Central)
#define DEVICE_ID "remote1"

// Tópicos MQTT (compatíveis com Central Pet Feeder)
#define TOPIC_CMD "petfeeder/remote/1/cmd"           // Recebe comandos da Central
#define TOPIC_STATUS "petfeeder/remote/1/status"     // Envia status online/offline
#define TOPIC_DATA "petfeeder/remote/1/data"         // Envia telemetria (feed_level)
#define TOPIC_LOGS "petfeeder/logs"                  // Envia logs offline
#define TOPIC_FEED_ACK "petfeeder/remote/1/feed_ack" // Confirmação de alimentação

// =============================================
// CONFIGURAÇÕES DE ARMAZENAMENTO
// =============================================
#define NVS_SCHEDULE_NAMESPACE "schedule"
#define NVS_LOGS_NAMESPACE "logs"

// =============================================
// LIMITES DO SISTEMA
// =============================================
#define MAX_LOGS 50
#define FEED_TIMEOUT_MS 10000

// Limites de validação (segurança)
#define MIN_FEED_QUANTITY 10      // Mínimo 10g
#define MAX_FEED_QUANTITY 500     // Máximo 500g
#define MAX_MEALS 3               // 3 refeições por dia

// Intervalos de envio
#define STATUS_PUBLISH_INTERVAL 60000      // Status a cada 60s
#define DATA_PUBLISH_INTERVAL 300000       // Telemetria a cada 5min
#define LOG_SEND_RETRY_INTERVAL 120000     // Retry de logs a cada 2min


#endif
