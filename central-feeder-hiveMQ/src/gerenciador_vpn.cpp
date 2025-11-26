#include "gerenciador_vpn.h"
#include "config.h"

GerenciadorVPN gerenciadorVPN;

GerenciadorVPN::GerenciadorVPN() 
    : conectado(false), ultimaTentativa(0) {
}

bool GerenciadorVPN::inicializar() {
    Serial.println("\n=== 🔐 INICIALIZANDO WIREGUARD ===");
    return conectarWireGuard();
}

bool GerenciadorVPN::conectarWireGuard() {
    // ✅ SUA CONFIGURAÇÃO WIREGUARD (substitua pelos seus dados)
    const char* privateKey = "YOUR_ESP32_PRIVATE_KEY"; // Chave privada do ESP32
    const char* serverPublicKey = "YOUR_SERVER_PUBLIC_KEY"; // Chave pública do servidor
    const char* presharedKey = ""; // Opcional
    const char* serverEndpoint = "100.95.52.105"; // IP do seu servidor Umbrel
    uint16_t serverPort = 51820;
    
    // IPs na rede VPN
    IPAddress localIp(10, 8, 0, 3);  // ESP32 na VPN
    IPAddress gateway(10, 8, 0, 1);  // Servidor na VPN
    IPAddress subnet(255, 255, 255, 0);
    
    Serial.print("🔄 Conectando WireGuard... ");
    Serial.print("Local: "); Serial.print(localIp);
    Serial.print(" → Servidor: "); Serial.println(serverEndpoint);
    
    if (wg.begin(localIp, gateway, subnet, privateKey, serverPublicKey, presharedKey, serverEndpoint, serverPort)) {
        conectado = true;
        Serial.println("✅ WIREGUARD CONECTADO!");
        Serial.print("📡 IP VPN: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        conectado = false;
        Serial.println("❌ Falha WireGuard");
        return false;
    }
}

bool GerenciadorVPN::estaConectado() {
    return conectado && WiFi.status() == WL_CONNECTED;
}

void GerenciadorVPN::atualizar() {
    // Manter conexão ativa
    if (!estaConectado() && millis() - ultimaTentativa > 30000) {
        ultimaTentativa = millis();
        Serial.println("🔄 Reconectando WireGuard...");
        conectarWireGuard();
    }
}