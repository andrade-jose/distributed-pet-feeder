#include "MQTTManager.h"

// Instância estática para callback
MQTTManager* MQTTManager::instancia = nullptr;

MQTTManager::MQTTManager(WiFiManager* wifiMgr, const char* servidor, int porta, const char* clientId, const char* usuario, const char* senha)
    : mqttClient(wifiClient) {
    this->wifiManager = wifiMgr;
    this->servidor = servidor;
    this->porta = porta;
    this->clientId = clientId;
    this->usuario = usuario;
    this->senha = senha;
    this->conectado = false;
    this->ultimaTentativa = 0;
    this->callbackUsuario = nullptr;
    
    // Detectar se deve usar SSL baseado na porta
    this->usarWebSocket = (porta == 8883 || porta == 8884);
    
    // Configurar o cliente apropriado
    if (usarWebSocket) {
        // Configurar SSL/TLS para HiveMQ Cloud
        wifiClientSecure.setInsecure(); // Para HiveMQ Cloud público
        mqttClient.setClient(wifiClientSecure);
    } else {
        mqttClient.setClient(wifiClient);
    }
    
    // Definir instância estática
    MQTTManager::instancia = this;
}

void MQTTManager::iniciar() {
    mqttClient.setServer(servidor, porta);
    mqttClient.setCallback(MQTTManager::callbackInterno);
    Serial.println("📡 MQTTManager inicializado");
    Serial.printf("   Servidor: %s:%d\n", servidor, porta);
    Serial.printf("   Client ID: %s\n", clientId);
    Serial.printf("   SSL/TLS: %s\n", usarWebSocket ? "Ativado" : "Desativado");
    if (strlen(usuario) > 0) {
        Serial.printf("   Usuário: %s\n", usuario);
        Serial.println("   Autenticação: Ativada");
    } else {
        Serial.println("   Autenticação: Desativada");
    }
}

bool MQTTManager::conectar() {
    if (!wifiManager->estaConectado()) {
        Serial.println("❌ WiFi não conectado. Não é possível conectar MQTT.");
        return false;
    }
    
    Serial.println("📡 Conectando ao MQTT...");
    
    if (usarWebSocket) {
        Serial.println("    Conectando via SSL/TLS...");
        Serial.printf("   🌐 HiveMQ Cloud: %s\n", servidor);
    }
    
    bool sucesso = false;
    
    // Conectar com ou sem autenticação
    if (strlen(usuario) > 0 && strlen(senha) > 0) {
        // Conectar com usuário e senha
        Serial.println("   🔐 Conectando com autenticação...");
        sucesso = mqttClient.connect(clientId, usuario, senha);
    } else {
        // Conectar sem autenticação
        Serial.println("   🔓 Conectando sem autenticação...");
        sucesso = mqttClient.connect(clientId);
    }
    
    if (sucesso) {
        conectado = true;
        Serial.println("✅ MQTT conectado!");
        return true;
    } else {
        conectado = false;
        Serial.printf("❌ Falha MQTT! Código: %d\n", mqttClient.state());
        Serial.println("   Códigos de erro:");
        Serial.println("   -4: MQTT_CONNECTION_TIMEOUT");
        Serial.println("   -3: MQTT_CONNECTION_LOST");
        Serial.println("   -2: MQTT_CONNECT_FAILED");
        Serial.println("   -1: MQTT_DISCONNECTED");
        Serial.println("    1: MQTT_CONNECT_BAD_PROTOCOL");
        Serial.println("    2: MQTT_CONNECT_BAD_CLIENT_ID");
        Serial.println("    3: MQTT_CONNECT_UNAVAILABLE");
        Serial.println("    4: MQTT_CONNECT_BAD_CREDENTIALS");
        Serial.println("    5: MQTT_CONNECT_UNAUTHORIZED");
        return false;
    }
}

void MQTTManager::verificarConexao() {
    unsigned long agora = millis();
    
    if (!wifiManager->estaConectado()) {
        conectado = false;
        return;
    }
    
    if (!mqttClient.connected()) {
        conectado = false;
        
        if (agora - ultimaTentativa >= INTERVALO_RECONEXAO) {
            Serial.println("🔄 MQTT desconectado. Tentando reconectar...");
            conectar();
            ultimaTentativa = agora;
        }
    } else {
        conectado = true;
    }
}

bool MQTTManager::estaConectado() {
    return conectado && mqttClient.connected();
}

void MQTTManager::loop() {
    if (estaConectado()) {
        mqttClient.loop();
    }
}

void MQTTManager::desconectar() {
    mqttClient.disconnect();
    conectado = false;
    Serial.println("📡 MQTT desconectado");
}

void MQTTManager::definirCallback(MqttCallback callback) {
    this->callbackUsuario = callback;
}

bool MQTTManager::publicar(const char* topico, const char* payload) {
    if (estaConectado()) {
        bool sucesso = mqttClient.publish(topico, payload);
        if (sucesso) {
            Serial.printf("📤 MQTT enviado [%s]: %s\n", topico, payload);
        } else {
            Serial.printf("❌ Falha ao enviar MQTT [%s]\n", topico);
        }
        return sucesso;
    } else {
        Serial.println("❌ MQTT não conectado. Não é possível publicar.");
        return false;
    }
}

bool MQTTManager::publicar(const char* topico, String payload) {
    return publicar(topico, payload.c_str());
}

bool MQTTManager::subscrever(const char* topico) {
    if (estaConectado()) {
        bool sucesso = mqttClient.subscribe(topico);
        if (sucesso) {
            Serial.printf("📥 Subscrito ao tópico: %s\n", topico);
        } else {
            Serial.printf("❌ Falha ao subscrever: %s\n", topico);
        }
        return sucesso;
    } else {
        Serial.println("❌ MQTT não conectado. Não é possível subscrever.");
        return false;
    }
}

void MQTTManager::callbackInterno(char* topic, byte* payload, unsigned int length) {
    if (MQTTManager::instancia && MQTTManager::instancia->callbackUsuario) {
        String topico = String(topic);
        String mensagem = "";
        
        for (unsigned int i = 0; i < length; i++) {
            mensagem += (char)payload[i];
        }
        
        // Chamar callback do usuário
        MQTTManager::instancia->callbackUsuario(topico, mensagem);
    }
}
