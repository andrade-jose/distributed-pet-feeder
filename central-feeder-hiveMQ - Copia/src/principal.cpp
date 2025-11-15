#include <WiFi.h>
#include <WireGuard-ESP32.h>
#include <PubSubClient.h>

const char* ssid = "SEU_WIFI";
const char* password = "SENHA_WIFI";

// MQTT (dentro da VPN)
const char* mqttServer = "10.8.0.1"; // IP do broker dentro da rede WireGuard
const int mqttPort = 1883;

// --- Configuração WireGuard (.conf convertido)
const char* privateKey = "GIWQpwolXZWPdNzdb2rQRgdLOE65UpWbhcwIhJ3PQEA=";
const char* peerPublicKey = "9HHOHLHoYfIC9ixiTZ6whj6Zhn9saPzBXTjpNP0DEwQ=";
const char* presharedKey = "ju6bZca6sq8JpIJyu024tRM/bSpospe1yWwZXbkTYhI=";
const char* peerEndpoint = "192.168.15.36"; // pode usar IP público se resolver mal DNS
const uint16_t peerPort = 51820;
const char* allowedIPs = "0.0.0.0/0";
IPAddress local_ip(10, 8, 0, 2);
IPAddress subnet_mask(255, 255, 255, 0);

WireGuard wg;
WiFiClient wgClient;
PubSubClient mqtt(wgClient);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nConectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");

  // Inicializa WireGuard
  Serial.println("Iniciando túnel WireGuard...");
  wg.begin(
    local_ip,
    privateKey,
    peerPublicKey,
    peerEndpoint,
    peerPort,
    allowedIPs,
    subnet_mask,
    presharedKey
  );
  Serial.println("WireGuard ativo! Conectado à VPN.");

  // Testa conexão MQTT dentro da VPN
  mqtt.setServer(mqttServer, mqttPort);
  Serial.print("Conectando ao broker MQTT dentro da VPN...");
  if (mqtt.connect("ESP32-WG")) {
    Serial.println(" conectado!");
    mqtt.publish("vpn/teste", "WireGuard + MQTT ativo!");
  } else {
    Serial.println(" falha!");
  }
}

void loop() {
  mqtt.loop();
}
