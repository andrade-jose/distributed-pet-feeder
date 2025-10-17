#include <Arduino.h>
#include "config.h"
#include "gerenciador_wifi.h"
#include "gerenciador_tempo.h"
#include "gerenciador_mqtt.h"

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  DEBUG_PRINTLN("=== Inicializando Sistema Cliente MQTT ===");
  DEBUG_PRINTF("Versão: %s\n", SYSTEM_VERSION);
  DEBUG_PRINTF("Build: %s %s\n", SYSTEM_BUILD_DATE, SYSTEM_BUILD_TIME);

  // Inicializar componentes essenciais
  GerenciadorTempo::inicializar();
  GerenciadorWifi::inicializar();
  GerenciadorMQTT::inicializar();

  DEBUG_PRINTLN("Sistema pronto!");
  DEBUG_PRINTLN("Cliente MQTT conectado ao HiveMQ Cloud");
}

void loop()
{
  // Atualizar GerenciadorTempo (RTC + sincronização NTP)
  GerenciadorTempo::atualizar();

  // Atualizar Gerenciador WiFi
  GerenciadorWifi::atualizar();

  // Atualizar cliente MQTT
  GerenciadorMQTT::atualizar();

  delay(MAIN_LOOP_DELAY_MS);
}
