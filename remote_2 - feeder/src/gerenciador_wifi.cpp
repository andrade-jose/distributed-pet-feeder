#include "gerenciador_wifi.h"

WiFiManager::WiFiManager(const char* ssidParam, const char* passwordParam) 
    : ssid(ssidParam), password(passwordParam), conectado(false), ultimaTentativa(0) {
}

void WiFiManager::iniciar() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    
    Serial.println("📡 WiFi Manager iniciado");
}

bool WiFiManager::conectar() {
    if (estaConectado()) {
        return true;
    }
    
    Serial.printf("🔗 Conectando ao WiFi: %s\n", ssid);
    WiFi.begin(ssid, password);
    
    unsigned long inicio = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - inicio) < 15000) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        conectado = true;
        Serial.printf("\n✅ WiFi conectado! IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("📶 RSSI: %d dBm\n", WiFi.RSSI());
        return true;
    } else {
        conectado = false;
        Serial.println("\n❌ Falha na conexão WiFi");
        return false;
    }
}

void WiFiManager::verificarConexao() {
    if (WiFi.status() != WL_CONNECTED) {
        conectado = false;
        
        // Tentar reconectar a cada INTERVALO_RECONEXAO
        unsigned long agora = millis();
        if (agora - ultimaTentativa >= INTERVALO_RECONEXAO) {
            ultimaTentativa = agora;
            Serial.println("🔄 Tentando reconectar WiFi...");
            conectar();
        }
    } else {
        conectado = true;
    }
}

bool WiFiManager::estaConectado() {
    return (WiFi.status() == WL_CONNECTED);
}

String WiFiManager::obterIP() {
    return WiFi.localIP().toString();
}

int WiFiManager::obterRSSI() {
    return WiFi.RSSI();
}

void WiFiManager::desconectar() {
    WiFi.disconnect();
    conectado = false;
    Serial.println("📡 WiFi desconectado");
}