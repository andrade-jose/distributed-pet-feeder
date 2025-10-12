#include <Arduino.h>
#include "config.h"
#include "botoes.h"
#include "display.h"
#include "gerenciador_telas.h"
#include "gerenciador_wifi.h"
#include "gerenciador_tempo.h"
#include "gerenciador_mqtt.h"
#include "gerenciador_web.h"
#include "gerenciador_wifi_config.h"

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  DEBUG_PRINTLN("=== Inicializando Sistema ===");
  DEBUG_PRINTF("Versão: %s\n", SYSTEM_VERSION);
  DEBUG_PRINTF("Build: %s %s\n", SYSTEM_BUILD_DATE, SYSTEM_BUILD_TIME);

  // Inicializar componentes
  Display::init();
  Botoes::inicializar();
  GerenciadorTempo::inicializar();
  // Verificar configuração WiFi (portal captive se necessário)
  GerenciadorWiFiConfig::inicializar();
  GerenciadorWifi::inicializar();
  GerenciadorMQTT::inicializar();  // Inicializar broker MQTT local
  GerenciadorWeb::inicializar();
  GerenciadorTelas::inicializar();

  DEBUG_PRINTLN("Sistema pronto!");
  DEBUG_PRINTLN("Use os botões para navegar");
}

void loop()
{
  // Atualizar GerenciadorTempo (RTC + sincronização NTP)
  GerenciadorTempo::atualizar();

  // Atualizar portal captive (se ativo)
  GerenciadorWiFiConfig::atualizar();

  // Atualizar Gerenciador WiFi
  GerenciadorWifi::atualizar();

  // Atualizar broker MQTT local
  GerenciadorMQTT::atualizar();

  // Atualizar o Gerenciador Web
  GerenciadorWeb::atualizar();

  // Atualizar gerenciador de telas (navegação + renderização)
  GerenciadorTelas::atualizar();

  delay(MAIN_LOOP_DELAY_MS);
}