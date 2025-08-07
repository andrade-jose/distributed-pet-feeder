#include "WiFiManager.h"

WiFiManager::WiFiManager(const char* ssid, const char* password) {
    this->ssid = ssid;
    this->password = password;
    this->conectado = false;
    this->ultimaTentativa = 0;
}

void WiFiManager::iniciar() {
    WiFi.mode(WIFI_STA);
    Serial.println("📡 WiFiManager inicializado");
}

bool WiFiManager::conectar() {
    Serial.println("📡 Conectando ao WiFi...");
    Serial.printf("   SSID: %s\n", ssid);
    
    WiFi.begin(ssid, password);
    
    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
        delay(500);
        Serial.print(".");
        tentativas++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        conectado = true;
        Serial.println("");
        Serial.printf("✅ WiFi conectado!\n");
        Serial.printf("   IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("   RSSI: %d dBm\n", WiFi.RSSI());
        return true;
    } else {
        conectado = false;
        Serial.println("");
        Serial.println("❌ Falha ao conectar WiFi!");
        return false;
    }
}

void WiFiManager::verificarConexao() {
    unsigned long agora = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
        conectado = false;
        
        if (agora - ultimaTentativa >= INTERVALO_RECONEXAO) {
            Serial.println("🔄 WiFi desconectado. Tentando reconectar...");
            conectar();
            ultimaTentativa = agora;
        }
    } else {
        conectado = true;
    }
}

bool WiFiManager::estaConectado() {
    return conectado && (WiFi.status() == WL_CONNECTED);
}

String WiFiManager::obterIP() {
    if (estaConectado()) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

void WiFiManager::desconectar() {
    WiFi.disconnect();
    conectado = false;
    Serial.println("📡 WiFi desconectado");
}
