// gerenciador_wifi.cpp
#include "gerenciador_wifi.h"
#include <Preferences.h>
#include "sistema_estado.h"  // ✅ ADICIONAR este include

// ✅ ADICIONAR: Declarar a instância global do EstadoSistema
extern EstadoSistema estadoSistema;

// ✅ CORRIGIR: Usar GerenciadorWiFi (i maiúsculo) em todas as definições
String GerenciadorWiFi::ssidSalvo = "";
String GerenciadorWiFi::senhaSalva = "";
bool GerenciadorWiFi::reconexaoAutomatica = true;
unsigned long GerenciadorWiFi::ultimaTentativaReconexao = 0;
unsigned long GerenciadorWiFi::ultimaVerificacaoStatus = 0;
bool GerenciadorWiFi::estavaConectado = false;
void (*GerenciadorWiFi::callbackStatus)(bool, String, String) = nullptr;

int GerenciadorWiFi::quantidadeRedes = 0;
String GerenciadorWiFi::ssidsRedes[20];
int GerenciadorWiFi::rssiRedes[20];
bool GerenciadorWiFi::redesCriptografadas[20];

void GerenciadorWiFi::inicializar() {
    Serial.println("=== Inicializando Gerenciador WiFi ===");

    // Configurar modo WiFi
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    // Registrar evento
    WiFi.onEvent(GerenciadorWiFi::aoEventoWiFi);

    // ✅ FORÇAR uso das credenciais do config.h
    Serial.println("Limpando credenciais antigas...");
    GerenciadorWiFi::limparCredenciais();

    Serial.println("Usando credenciais do config.h");
    Serial.printf("SSID: %s\n", DEFAULT_WIFI_SSID);
    GerenciadorWiFi::ssidSalvo = DEFAULT_WIFI_SSID;
    GerenciadorWiFi::senhaSalva = DEFAULT_WIFI_PASSWORD;
    GerenciadorWiFi::salvarCredenciais(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);

    // Tentar conectar automaticamente
    if (GerenciadorWiFi::reconexaoAutomatica &&
        !GerenciadorWiFi::ssidSalvo.isEmpty() &&
        !GerenciadorWiFi::senhaSalva.isEmpty()) {

        Serial.println("Tentando conectar automaticamente...");
        Serial.println("Conectando ao WiFi: " + GerenciadorWiFi::ssidSalvo);
        GerenciadorWiFi::conectar(GerenciadorWiFi::ssidSalvo, GerenciadorWiFi::senhaSalva);
    }

    Serial.println("Gerenciador WiFi inicializado");
}

void GerenciadorWiFi::conectar(String ssid, String senha) {
    Serial.println("Conectando ao WiFi: " + ssid);
    
    if (ssid.isEmpty()) {
        Serial.println("ERRO: SSID vazio!");
        return;
    }
    
    // Salvar credenciais
    GerenciadorWiFi::ssidSalvo = ssid;
    GerenciadorWiFi::senhaSalva = senha;
    GerenciadorWiFi::salvarCredenciais(ssid, senha);
    
    // Iniciar conexão
    WiFi.begin(ssid.c_str(), senha.c_str());
    GerenciadorWiFi::ultimaTentativaReconexao = millis();
    
    Serial.println("Tentativa de conexão iniciada...");
}

void GerenciadorWiFi::desconectar() {
    WiFi.disconnect(true);
    GerenciadorWiFi::atualizarDadosSistema();
    Serial.println("WiFi desconectado");
}

void GerenciadorWiFi::escanearRedes() {
    Serial.println("Escaneando redes WiFi...");
    
    GerenciadorWiFi::quantidadeRedes = 0;
    int numRedes = WiFi.scanNetworks();
    
    if (numRedes == 0) {
        Serial.println("Nenhuma rede encontrada");
        return;
    }
    
    GerenciadorWiFi::quantidadeRedes = min(numRedes, 20);
    
    for (int i = 0; i < GerenciadorWiFi::quantidadeRedes; i++) {
        GerenciadorWiFi::ssidsRedes[i] = WiFi.SSID(i);
        GerenciadorWiFi::rssiRedes[i] = WiFi.RSSI(i);
        GerenciadorWiFi::redesCriptografadas[i] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
    
    Serial.printf("Encontradas %d redes\n", GerenciadorWiFi::quantidadeRedes);
    WiFi.scanDelete();
}

bool GerenciadorWiFi::estaConectado() {
    return WiFi.status() == WL_CONNECTED;
}

String GerenciadorWiFi::obterSSID() {
    return WiFi.SSID();
}

String GerenciadorWiFi::obterIP() {
    return WiFi.localIP().toString();
}

int GerenciadorWiFi::obterRSSI() {
    return WiFi.RSSI();
}

String GerenciadorWiFi::obterQualidadeRSSI() {
    int rssi = GerenciadorWiFi::obterRSSI();
    
    if (rssi >= -50) return "Excelente";
    else if (rssi >= -60) return "Boa";
    else if (rssi >= -70) return "Regular";
    else return "Fraca";
}

void GerenciadorWiFi::atualizar() {
    unsigned long agora = millis();
    
    // Atualizar status a cada 5 segundos
    if (agora - GerenciadorWiFi::ultimaVerificacaoStatus >= 5000) {
        GerenciadorWiFi::ultimaVerificacaoStatus = agora;
        GerenciadorWiFi::atualizarDadosSistema();
    }
    
    // Tentar reconectar se necessário
    if (GerenciadorWiFi::reconexaoAutomatica && 
        !GerenciadorWiFi::estaConectado() && 
        !GerenciadorWiFi::ssidSalvo.isEmpty()) {
        
        if (agora - GerenciadorWiFi::ultimaTentativaReconexao >= 30000) {
            GerenciadorWiFi::ultimaTentativaReconexao = agora;
            GerenciadorWiFi::tentarReconectar();
        }
    }
}

void GerenciadorWiFi::atualizarDadosSistema() {
    bool conectadoAgora = GerenciadorWiFi::estaConectado();
    
    if (conectadoAgora != GerenciadorWiFi::estavaConectado) {
        GerenciadorWiFi::estavaConectado = conectadoAgora;
        
        if (conectadoAgora) {
            Serial.println("WiFi conectado! Iniciando MQTT...");
        } else {
            Serial.println("WiFi desconectado!");
        }
        
        // Chamar callback se definido
        if (GerenciadorWiFi::callbackStatus) {
            GerenciadorWiFi::callbackStatus(
                conectadoAgora, 
                GerenciadorWiFi::obterSSID(), 
                GerenciadorWiFi::obterIP()
            );
        }
        
        // ✅ CORRIGIDO: estadoSistema agora está disponível
        estadoSistema.atualizarStatusWifi(
            GerenciadorWiFi::obterSSID(),
            conectadoAgora,
            GerenciadorWiFi::obterQualidadeRSSI(),
            GerenciadorWiFi::obterIP()
        );
    }
}

void GerenciadorWiFi::salvarCredenciais(String ssid, String senha) {
    Preferences prefs;
    prefs.begin("wifi", false);
    
    prefs.putString("ssid", ssid);
    prefs.putString("senha", senha);
    
    prefs.end();
    Serial.println("Credenciais WiFi salvas");
}

bool GerenciadorWiFi::carregarCredenciais() {
    Preferences prefs;
    prefs.begin("wifi", true);
    
    GerenciadorWiFi::ssidSalvo = prefs.getString("ssid", "");
    GerenciadorWiFi::senhaSalva = prefs.getString("senha", "");
    
    prefs.end();
    
    if (!GerenciadorWiFi::ssidSalvo.isEmpty()) {
        Serial.println("Credenciais carregadas: " + GerenciadorWiFi::ssidSalvo);
        return true;
    }
    
    return false;
}

void GerenciadorWiFi::limparCredenciais() {
    Preferences prefs;
    prefs.begin("wifi", false);
    
    prefs.remove("ssid");
    prefs.remove("senha");
    
    prefs.end();
    
    GerenciadorWiFi::ssidSalvo = "";
    GerenciadorWiFi::senhaSalva = "";
    
    Serial.println("Credenciais WiFi limpas");
}

void GerenciadorWiFi::definirCallbackStatus(void (*callback)(bool, String, String)) {
    GerenciadorWiFi::callbackStatus = callback;
}

int GerenciadorWiFi::obterQuantidadeRedes() {
    return GerenciadorWiFi::quantidadeRedes;
}

String GerenciadorWiFi::obterSSIDRede(int indice) {
    if (indice >= 0 && indice < GerenciadorWiFi::quantidadeRedes) {
        return GerenciadorWiFi::ssidsRedes[indice];
    }
    return "";
}

int GerenciadorWiFi::obterRSSIRede(int indice) {
    if (indice >= 0 && indice < GerenciadorWiFi::quantidadeRedes) {
        return GerenciadorWiFi::rssiRedes[indice];
    }
    return 0;
}

bool GerenciadorWiFi::obterRedeCriptografada(int indice) {
    if (indice >= 0 && indice < GerenciadorWiFi::quantidadeRedes) {
        return GerenciadorWiFi::redesCriptografadas[indice];
    }
    return true;
}

void GerenciadorWiFi::aoEventoWiFi(WiFiEvent_t evento, WiFiEventInfo_t info) {
    switch (evento) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("Evento: WiFi conectado");
            break;
            
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.println("Evento: IP obtido - " + WiFi.localIP().toString());
            GerenciadorWiFi::atualizarDadosSistema();
            break;
            
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("Evento: WiFi desconectado");
            GerenciadorWiFi::atualizarDadosSistema();
            break;
            
        default:
            break;
    }
}

void GerenciadorWiFi::tentarReconectar() {
    if (!GerenciadorWiFi::ssidSalvo.isEmpty() && !GerenciadorWiFi::senhaSalva.isEmpty()) {
        Serial.println("Tentando reconectar ao WiFi: " + GerenciadorWiFi::ssidSalvo);
        WiFi.begin(GerenciadorWiFi::ssidSalvo.c_str(), GerenciadorWiFi::senhaSalva.c_str());
    }
}