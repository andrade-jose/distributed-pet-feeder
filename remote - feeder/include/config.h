#ifndef CONFIG_H
#define CONFIG_H

// ===== CONFIGURAÇÃO WIFI =====
const char* WIFI_SSID = "Coelhoandrade";        // Altere para o nome da sua rede WiFi
const char* WIFI_PASSWORD = "190520jg";         // Altere para a senha da sua rede WiFi

// ===== CONFIGURAÇÃO MQTT =====
// HiveMQ Cloud - SSL/TLS obrigatório na porta 8883
const char* MQTT_SERVER = "9aa85a8cfb4a4ba896f2289aa408ba5a.s1.eu.hivemq.cloud";  // URL do seu cluster HiveMQ
const int MQTT_PORT = 8883;                     // Porta SSL/TLS (8883) - VOLTANDO PARA SSL TRADICIONAL
const char* MQTT_CLIENT_ID = "ESP32_Remota_001"; // ID único do cliente
const char* MQTT_USERNAME = "Romota1";          // Usuário criado no HiveMQ Cloud
const char* MQTT_PASSWORD = "Senha1234";        // Senha do usuário HiveMQ Cloud

// ===== TÓPICOS MQTT =====
const char* TOPIC_COMANDO = "alimentador/remota/comando";           // Recebe comandos do CENTRAL
const char* TOPIC_STATUS = "alimentador/remota/status";             // Envia status para CENTRAL
const char* TOPIC_CONCLUIDO = "alimentador/remota/concluido";       // Envia quando concluído para CENTRAL
const char* TOPIC_HEARTBEAT = "alimentador/remota/heartbeat";       // Sinal de vida

// ===== CONFIGURAÇÃO DO HARDWARE =====
const int PINO_SERVO = 5;      // Pino do servo
const int PINO_HALL = 4;       // Pino do sensor Hall
const int PINO_BOTAO = 18;      // Pino do botão
const int PINO_LED_STATUS = 13; // Pino do LED de status

#endif
