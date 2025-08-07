#include "gerenciador_wifi.h"
#include "gerenciador_telas.h"
#include <Preferences.h>

// Inicializar variáveis estáticas
String GerenciadorWifi::ssidSalvo = "";
String GerenciadorWifi::senhaSalva = "";
bool GerenciadorWifi::reconexaoAutomatica = true;
unsigned long GerenciadorWifi::ultimaTentativaReconexao = 0;
unsigned long GerenciadorWifi::ultimaVerificacaoStatus = 0;
bool GerenciadorWifi::estavaConcetado = false;
void (*GerenciadorWifi::callbackStatus)(bool, String, String) = nullptr;

int GerenciadorWifi::quantidadeRedes = 0;
String GerenciadorWifi::ssidsRedes[20];
int GerenciadorWifi::rssiRedes[20];
bool GerenciadorWifi::redesCriptografadas[20];

void GerenciadorWifi::inicializar() {
    Serial.println("=== Inicializando Gerenciador WiFi ===");
    
    // Configurar modo WiFi
    WiFi.mode(WIFI_STA);
    
    // Registrar eventos WiFi
    WiFi.onEvent(aoEventoWiFi);
    
    // Carregar credenciais salvas
    carregarCredenciais();
    
    // Se não há credenciais salvas, usar as padrão do config.h
    if (ssidSalvo.length() == 0 && strlen(DEFAULT_WIFI_SSID) > 0) {
        ssidSalvo = DEFAULT_WIFI_SSID;
        senhaSalva = DEFAULT_WIFI_PASSWORD;
        Serial.println("Usando credenciais padrão do config.h");
    }
    
    // Tentar conectar automaticamente se há credenciais
    if (ssidSalvo.length() > 0) {
        Serial.println("Tentando conectar automaticamente...");
        conectar(ssidSalvo, senhaSalva);
    }
    
    Serial.println("Gerenciador WiFi inicializado");
}

void GerenciadorWifi::conectar(String ssid, String senha) {
    Serial.println("Conectando ao WiFi: " + ssid);
    
    // Desconectar se já conectado
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        delay(100);
    }
    
    // Conectar
    WiFi.begin(ssid.c_str(), senha.c_str());
    
    // Salvar credenciais
    ssidSalvo = ssid;
    senhaSalva = senha;
    salvarCredenciais(ssid, senha);
    
    Serial.println("Tentativa de conexão iniciada...");
}

void GerenciadorWifi::desconectar() {
    Serial.println("Desconectando WiFi");
    reconexaoAutomatica = false;
    WiFi.disconnect();
    atualizarDadosSistema();
}

void GerenciadorWifi::escanearRedes() {
    Serial.println("Escaneando redes WiFi...");
    
    // Limpar lista anterior
    quantidadeRedes = 0;
    
    // Escanear
    int n = WiFi.scanNetworks();
    
    if (n == 0) {
        Serial.println("Nenhuma rede encontrada");
        return;
    }
    
    Serial.printf("Encontradas %d redes:\n", n);
    
    // Armazenar até 20 redes
    quantidadeRedes = min(n, 20);
    
    for (int i = 0; i < quantidadeRedes; i++) {
        ssidsRedes[i] = WiFi.SSID(i);
        rssiRedes[i] = WiFi.RSSI(i);
        redesCriptografadas[i] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        
        Serial.printf("%d: %s (%d dBm) %s\n", 
                     i, 
                     ssidsRedes[i].c_str(), 
                     rssiRedes[i],
                     redesCriptografadas[i] ? "[Protegida]" : "[Aberta]");
    }
    
    // Limpar memória do scan
    WiFi.scanDelete();
}

bool GerenciadorWifi::estaConectado() {
    return WiFi.status() == WL_CONNECTED;
}

String GerenciadorWifi::obterSSID() {
    if (estaConectado()) {
        return WiFi.SSID();
    }
    return "Desconectado";
}

String GerenciadorWifi::obterIP() {
    if (estaConectado()) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

int GerenciadorWifi::obterRSSI() {
    if (estaConectado()) {
        return WiFi.RSSI();
    }
    return -100;
}

String GerenciadorWifi::obterQualidadeRSSI() {
    if (!estaConectado()) {
        return "Sem sinal";
    }
    
    int rssi = obterRSSI();
    
    if (rssi >= -50) return "Excelente";
    else if (rssi >= -60) return "Bom";
    else if (rssi >= -70) return "Regular";
    else if (rssi >= -80) return "Fraco";
    else return "Muito fraco";
}

void GerenciadorWifi::atualizar() {
    unsigned long agora = millis();
    
    // Verificar status a cada 5 segundos
    if (agora - ultimaVerificacaoStatus > 5000) {
        ultimaVerificacaoStatus = agora;
        
        bool atualmenteConectado = estaConectado();
        
        // Se mudou o status, atualizar
        if (atualmenteConectado != estavaConcetado) {
            estavaConcetado = atualmenteConectado;
            atualizarDadosSistema();
            
            if (callbackStatus) {
                callbackStatus(atualmenteConectado, obterSSID(), obterIP());
            }
            
            if (atualmenteConectado) {
                Serial.println("WiFi conectado: " + obterSSID() + " (" + obterIP() + ")");
            } else {
                Serial.println("WiFi desconectado");
            }
        }
        
        // Tentar reconectar se necessário
        if (!atualmenteConectado && reconexaoAutomatica && ssidSalvo.length() > 0) {
            if (agora - ultimaTentativaReconexao > 30000) { // Tentar a cada 30s
                tentarReconectar();
            }
        }
    }
}

void GerenciadorWifi::salvarCredenciais(String ssid, String senha) {
    Preferences prefs;
    prefs.begin("wifi", false);
    
    prefs.putString("ssid", ssid);
    prefs.putString("password", senha);
    
    prefs.end();
    
    Serial.println("Credenciais WiFi salvas");
}

bool GerenciadorWifi::carregarCredenciais() {
    Preferences prefs;
    prefs.begin("wifi", true);
    
    ssidSalvo = prefs.getString("ssid", "");
    senhaSalva = prefs.getString("password", "");
    
    prefs.end();
    
    if (ssidSalvo.length() > 0) {
        Serial.println("Credenciais carregadas: " + ssidSalvo);
        return true;
    }
    
    Serial.println("Nenhuma credencial salva");
    return false;
}

void GerenciadorWifi::limparCredenciais() {
    Preferences prefs;
    prefs.begin("wifi", false);
    
    prefs.clear();
    prefs.end();
    
    ssidSalvo = "";
    senhaSalva = "";
    
    Serial.println("Credenciais WiFi removidas");
}

void GerenciadorWifi::definirCallbackStatus(void (*callback)(bool, String, String)) {
    callbackStatus = callback;
}

int GerenciadorWifi::obterQuantidadeRedes() {
    return quantidadeRedes;
}

String GerenciadorWifi::obterSSIDRede(int indice) {
    if (indice >= 0 && indice < quantidadeRedes) {
        return ssidsRedes[indice];
    }
    return "";
}

int GerenciadorWifi::obterRSSIRede(int indice) {
    if (indice >= 0 && indice < quantidadeRedes) {
        return rssiRedes[indice];
    }
    return -100;
}

bool GerenciadorWifi::obterRedeCriptografada(int indice) {
    if (indice >= 0 && indice < quantidadeRedes) {
        return redesCriptografadas[indice];
    }
    return true;
}

void GerenciadorWifi::aoEventoWiFi(WiFiEvent_t evento, WiFiEventInfo_t info) {
    switch (evento) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("Evento: WiFi conectado");
            break;
            
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.println("Evento: IP obtido - " + WiFi.localIP().toString());
            reconexaoAutomatica = true;
            break;
            
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("Evento: WiFi desconectado");
            break;
            
        default:
            break;
    }
}

void GerenciadorWifi::tentarReconectar() {
    ultimaTentativaReconexao = millis();
    
    if (ssidSalvo.length() > 0) {
        Serial.println("Tentando reconectar ao WiFi...");
        WiFi.begin(ssidSalvo.c_str(), senhaSalva.c_str());
    }
}

void GerenciadorWifi::atualizarDadosSistema() {
    // Atualizar dados do sistema no GerenciadorTelas
    GerenciadorTelas::atualizarStatusWifi(
        obterSSID(),
        estaConectado(),
        obterQualidadeRSSI()
    );
}
