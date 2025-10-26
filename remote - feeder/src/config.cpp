#include "config.h"

// ===== CERTIFICADO SSL - HIVEMQ =====
const char* HIVEMQ_ROOT_CA = R"EOF(
"-----BEGIN CERTIFICATE-----\n" \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n" \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n" \
"b24gUm9otCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n" \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n" \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n" \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n" \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n" \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n" \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n" \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n" \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n" \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n" \
"rqXRfboQnoZsG4q5WTP468SQvvG5\n" \
"-----END CERTIFICATE-----\n";
)EOF";

// ===== CONFIGURAÇÃO DA REMOTA =====
const int REMOTA_ID = 1;

// ===== CONFIGURAÇÃO MQTT - HIVEMQ =====
const char* MQTT_SERVER = "56d05fe4fbc64e80964aa78d92456f22.s1.eu.hivemq.cloud";
const int MQTT_PORT = 8883;
const char* MQTT_CLIENT_ID = "ESP32_Remota_001";
const char* MQTT_USERNAME = "Remota_1";
const char* MQTT_PASSWORD = "Senha1234";
// ===== TÓPICOS MQTT (Otimizados para reduzir uso de memória) =====
const char* TOPIC_COMANDO = "a/r/cmd";      // alimentador/remota/comando -> a/r/cmd
const char* TOPIC_STATUS = "a/r/st";         // alimentador/remota/status -> a/r/st
const char* TOPIC_RESPOSTA = "a/r/rsp";      // alimentador/remota/resposta -> a/r/rsp
const char* TOPIC_HEARTBEAT = "a/r/hb";      // alimentador/remota/heartbeat -> a/r/hb
const char* TOPIC_ALERTA_RACAO = "a/r/alr";  // alimentador/remota/alerta_racao -> a/r/alr

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
const unsigned long INTERVALO_HEARTBEAT = 30000; // 10 segundos (latência reduzida)

// ===== CONFIGURAÇÕES GERAIS =====
const unsigned long SERIAL_BAUD_RATE = 115200;
const unsigned long MAIN_LOOP_DELAY = 50; // ms