#include "gerenciador_mqtt.h"
#include "sistema_estado.h"
#include "processador_mqtt.h"   

GerenciadorMQTT* gerenciadorMQTT = nullptr;

GerenciadorMQTT::GerenciadorMQTT() 
    : wifiClient(nullptr),
      mqttClient(nullptr),
      conectado(false),
      tentouInicializar(false),
      ultimo_pacote(0),
      ultimo_reconectar(0),
      pacotes_perdidos(0),
      tentativas_reconexao(0) {
}


GerenciadorMQTT::~GerenciadorMQTT() {
    if (mqttClient) {
        delete mqttClient;
    }
    if (wifiClient) {
        delete wifiClient;
    }
}

GerenciadorMQTT* GerenciadorMQTT::obterInstancia() {
    return gerenciadorMQTT;
}

void GerenciadorMQTT::inicializar() {
    if (!gerenciadorMQTT) {
        gerenciadorMQTT = new GerenciadorMQTT();
        gerenciadorMQTT->init();
    }
}

void GerenciadorMQTT::atualizar() {
    if (!gerenciadorMQTT) return;
    
    if (!gerenciadorMQTT->conectado && WiFi.status() == WL_CONNECTED && !gerenciadorMQTT->tentouInicializar) {
        Serial.println("WiFi conectado, tentando conectar MQTT...");
        gerenciadorMQTT->init();
    }
    
    gerenciadorMQTT->loop();
}


bool GerenciadorMQTT::init() {
    DEBUG_PRINTLN("=== Inicializando Cliente MQTT HiveMQ ===");

    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("WiFi não conectado!");
        return false;
    }

    // Criar cliente WiFi seguro
    if (!wifiClient) {
        wifiClient = new WiFiClientSecure();
    }

    // ⚡ TLS sem certificado
    wifiClient->setInsecure();

    // Criar cliente MQTT
    if (!mqttClient) {
        mqttClient = new PubSubClient(*wifiClient);
    }

    // Configurar servidor MQTT
    mqttClient->setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    mqttClient->setCallback(callbackMQTT);
    mqttClient->setKeepAlive(MQTT_KEEP_ALIVE);

    // Aumentar buffer size para mensagens maiores
    mqttClient->setBufferSize(512);

    DEBUG_PRINTF("Servidor: %s:%d\n", MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    DEBUG_PRINTLN("TLS/SSL: Habilitado (modo inseguro)");

    // Conectar ao broker
    return conectar();
}


// Conectar ao broker MQTT
bool GerenciadorMQTT::conectar() {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("WiFi não conectado - impossível conectar MQTT");
        return false;
    }

    if (!mqttClient) {
        DEBUG_PRINTLN("Cliente MQTT não inicializado");
        return false;
    }

    // Já conectado
    if (mqttClient->connected()) {
        conectado = true;
        return true;
    }

    DEBUG_PRINTLN("Conectando ao HiveMQ Cloud...");
    DEBUG_PRINTF("Cliente ID: %s\n", MQTT_CLIENT_ID);

    // Tentar conectar com usuário e senha
    bool sucesso = mqttClient->connect(
        MQTT_CLIENT_ID,
        MQTT_USERNAME,
        MQTT_PASSWORD
    );

    if (sucesso) {
        estadoSistema.atualizarStatusMqtt(true);
        DEBUG_PRINTLN("✓ CONECTADO AO HIVEMQ CLOUD!");
        conectado = true;
        tentativas_reconexao = 0;
        ultimo_pacote = millis();

        // Inscrever em tópicos
        inscreverTopicos();

        // Publicar status da central
        publicarStatusCentral("ONLINE");

        return true;
    } else {
        estadoSistema.atualizarStatusMqtt(false);
        int estado = mqttClient->state();
        DEBUG_PRINTF("✗ Falha na conexão! Estado: %d (%s)\n",
                         estado, obterDescricaoErroMQTT(estado).c_str());
        conectado = false;
        tentativas_reconexao++;
        return false;
    }
}

// Desconectar
void GerenciadorMQTT::desconectar() {
    estadoSistema.atualizarStatusMqtt(false);
    if (mqttClient && mqttClient->connected()) {
        DEBUG_PRINTLN("Desconectando do HiveMQ Cloud...");
        publicarStatusCentral("OFFLINE");
        mqttClient->disconnect();
        conectado = false;
        DEBUG_PRINTLN("Desconectado");
    }
}

// Verificar se está conectado
bool GerenciadorMQTT::estaConectado() const {
    return conectado && mqttClient && mqttClient->connected() && (WiFi.status() == WL_CONNECTED);
}

bool GerenciadorMQTT::publicar(const String& topico, const String& payload) {
    if (!estaConectado()) {
        Serial.println("MQTT não conectado, não é possível publicar");
        return false;
    }
    
    bool sucesso = mqttClient->publish(topico.c_str(), payload.c_str());
    
    if (sucesso) {
        DEBUG_PRINTF("MQTT publicado: %s -> %s\n", topico.c_str(), payload.c_str());
        ultimo_pacote = millis();
    } else {
        DEBUG_PRINTF("Falha ao publicar MQTT: %s\n", topico.c_str());
        pacotes_perdidos++;
    }
    
    return sucesso;
}

void GerenciadorMQTT::callbackMQTT(char* topic, byte* payload, unsigned int length) {
    if (!gerenciadorMQTT) return;
    String payloadStr;
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }
    gerenciadorMQTT->processarMensagem(String(topic), payloadStr);
}

// Loop para manter conexão e processar mensagens
void GerenciadorMQTT::loop() {
    if (!mqttClient) {
        return;
    }
        // Verificar se o estado MQTT mudou
    bool estavaConectado = estadoSistema.mqttConectado;
    bool agoraConectado = estaConectado();
    
    if (estavaConectado != agoraConectado) {
        estadoSistema.atualizarStatusMqtt(agoraConectado);
        Serial.printf("[MQTT] Estado mudou: %d -> %d\n", estavaConectado, agoraConectado);
    }

    // Se não estiver conectado, tentar reconectar
    if (!mqttClient->connected()) {
        conectado = false;

        unsigned long agora = millis();
        if (agora - ultimo_reconectar > MQTT_RECONNECT_INTERVAL) {
            ultimo_reconectar = agora;

            if (tentativas_reconexao < MQTT_MAX_RETRIES) {
                DEBUG_PRINTLN("Tentando reconectar...");
                conectar();
            } else {
                // Resetar contador após um tempo maior
                if (agora - ultimo_reconectar > MQTT_RECONNECT_INTERVAL * 5) {
                    tentativas_reconexao = 0;
                }
            }
        }
    } else {
        // Processar mensagens MQTT
        mqttClient->loop();
    }
}

// Enviar comando para remota específica
bool GerenciadorMQTT::enviarComandoRemota(int idRemota, const String& acao, int tempo) {
    if (!estaConectado()) {
        DEBUG_PRINTLN("MQTT não conectado - comando não enviado");
        return false;
    }

    String topico = construirTopico(MQTT_TOPIC_COMANDO_REMOTA, idRemota);

    // Criar JSON do comando
    StaticJsonDocument<200> doc;
    doc["acao"] = acao;
    doc["timestamp"] = millis();

    if (tempo > 0) {
        doc["tempo"] = tempo;
    }

    String payload;
    serializeJson(doc, payload);

    DEBUG_PRINTF("Enviando comando para Remota %d...\n", idRemota);
    DEBUG_PRINTF("   Tópico: %s\n", topico.c_str());
    DEBUG_PRINTF("   Payload: %s\n", payload.c_str());

    // Publicar
    bool sucesso = mqttClient->publish(topico.c_str(), payload.c_str());

    if (sucesso) {
        DEBUG_PRINTF("✓ Comando enviado para Remota %d\n", idRemota);
        ultimo_pacote = millis();
    } else {
        DEBUG_PRINTF("✗ Falha ao enviar comando para Remota %d\n", idRemota);
    }

    return sucesso;
}

// Enviar comando geral
bool GerenciadorMQTT::enviarComandoGeral(const String& acao, int tempo, int idRemota) {
    if (!estaConectado()) {
        DEBUG_PRINTLN("MQTT não conectado - comando geral não enviado");
        return false;
    }

    // Criar JSON do comando com ID da remota incluído
    StaticJsonDocument<200> doc;
    doc["acao"] = acao;
    doc["remota_id"] = idRemota;
    doc["timestamp"] = millis();

    if (tempo > 0) {
        doc["tempo"] = tempo;
    }

    String payload;
    serializeJson(doc, payload);

    DEBUG_PRINTF("Enviando comando GERAL para Remota %d...\n", idRemota);
    DEBUG_PRINTF("  Tópico: %s\n", MQTT_TOPIC_COMANDO_GERAL);
    DEBUG_PRINTF("  Payload: %s\n", payload.c_str());

    bool sucesso = mqttClient->publish(MQTT_TOPIC_COMANDO_GERAL, payload.c_str());

    if (sucesso) {
        DEBUG_PRINTF("✓ Comando geral enviado para Remota %d\n", idRemota);
        ultimo_pacote = millis();
    } else {
        DEBUG_PRINTF("✗ Falha ao enviar comando geral\n");
    }

    return sucesso;
}

// Configurar horário da remota
bool GerenciadorMQTT::configurarHorarioRemota(int idRemota, int hora, int minuto, int quantidade) {
    if (!estaConectado()) {
        DEBUG_PRINTLN("MQTT não conectado - configuração de horário não enviada");
        return false;
    }

    String topico = construirTopico(MQTT_TOPIC_HORARIO_REMOTA, idRemota);

    // Criar JSON da configuração
    StaticJsonDocument<200> doc;
    doc["hora"] = hora;
    doc["minuto"] = minuto;
    doc["quantidade"] = quantidade;
    doc["timestamp"] = millis();

    String payload;
    serializeJson(doc, payload);

    DEBUG_PRINTF("Configurando horário para Remota %d...\n", idRemota);
    DEBUG_PRINTF("  Tópico: %s\n", topico.c_str());
    DEBUG_PRINTF("  Payload: %s\n", payload.c_str());

    bool sucesso = mqttClient->publish(topico.c_str(), payload.c_str());

    if (sucesso) {
        DEBUG_PRINTF("✓ Horário configurado: Remota %d às %02d:%02d (%dg)\n",
                          idRemota, hora, minuto, quantidade);
        ultimo_pacote = millis();
    }

    return sucesso;
}

// Configurar tempo de movimento
bool GerenciadorMQTT::configurarTempoMovimento(int idRemota, int tempo) {
    if (!estaConectado()) {
        return false;
    }

    String topico = construirTopico(MQTT_TOPIC_TEMPO_MOVIMENTO, idRemota);

    // Criar JSON do tempo
    StaticJsonDocument<100> doc;
    doc["tempo_movimento"] = tempo;
    doc["timestamp"] = millis();

    String payload;
    serializeJson(doc, payload);

    bool sucesso = mqttClient->publish(topico.c_str(), payload.c_str());

    if (sucesso) {
        DEBUG_PRINTF("✓ Tempo configurado para Remota %d: %d segundos\n", idRemota, tempo);
        ultimo_pacote = millis();
    }

    return sucesso;
}

void GerenciadorMQTT::processarMensagem(const String& topico, const String& payload) {
    // Apenas delegar para ProcessadorMQTT
    ProcessadorMQTT::processarMensagem(topico, payload);
    ultimo_pacote = millis();
}

// Solicitar status da remota
bool GerenciadorMQTT::solicitarStatusRemota(int idRemota) {
    return enviarComandoRemota(idRemota, MQTT_CMD_STATUS);
}

// Inscrever em tópicos
bool GerenciadorMQTT::inscreverTopicos() {
    if (!mqttClient || !mqttClient->connected()) {
        DEBUG_PRINTLN("Cliente não conectado - não é possível inscrever");
        return false;
    }

    DEBUG_PRINTLN("=== INSCREVENDO EM TÓPICOS ===");

    bool sucesso = true;

    // Status das remotas (com wildcard)
    String topico1 = "alimentador/remota/+/status";
    if (mqttClient->subscribe(topico1.c_str())) {
        DEBUG_PRINTF("✓ Inscrito: %s\n", topico1.c_str());
    } else {
        DEBUG_PRINTF("✗ Falha: %s\n", topico1.c_str());
        sucesso = false;
    }

    // Status geral (compatibilidade)
    String topico2 = "alimentador/remota/status";
    if (mqttClient->subscribe(topico2.c_str())) {
        DEBUG_PRINTF("✓ Inscrito: %s\n", topico2.c_str());
    } else {
        DEBUG_PRINTF("✗ Falha: %s\n", topico2.c_str());
        sucesso = false;
    }

    // Vida das remotas
    String topico3 = "alimentador/remota/+/vida";
    if (mqttClient->subscribe(topico3.c_str())) {
        DEBUG_PRINTF("✓ Inscrito: %s\n", topico3.c_str());
    } else {
        DEBUG_PRINTF("✗ Falha: %s\n", topico3.c_str());
        sucesso = false;
    }

    // Heartbeat geral
    if (mqttClient->subscribe(MQTT_TOPIC_HEARTBEAT_GERAL)) {
        DEBUG_PRINTF("✓ Inscrito: %s\n", MQTT_TOPIC_HEARTBEAT_GERAL);
    } else {
        DEBUG_PRINTF("✗ Falha: %s\n", MQTT_TOPIC_HEARTBEAT_GERAL);
        sucesso = false;
    }

    // Respostas das remotas
    String topico5 = "alimentador/remota/+/resposta";
    if (mqttClient->subscribe(topico5.c_str())) {
        DEBUG_PRINTF("✓ Inscrito: %s\n", topico5.c_str());
    } else {
        DEBUG_PRINTF("✗ Falha: %s\n", topico5.c_str());
        sucesso = false;
    }

    // Respostas gerais (compatibilidade)
    if (mqttClient->subscribe(MQTT_TOPIC_CONCLUIDO_GERAL)) {
        DEBUG_PRINTF("✓ Inscrito: %s\n", MQTT_TOPIC_CONCLUIDO_GERAL);
    } else {
        DEBUG_PRINTF("✗ Falha: %s\n", MQTT_TOPIC_CONCLUIDO_GERAL);
        sucesso = false;
    }

    // Alertas de ração
    if (mqttClient->subscribe(MQTT_TOPIC_ALERTA_RACAO)) {
        DEBUG_PRINTF("✓ Inscrito: %s\n", MQTT_TOPIC_ALERTA_RACAO);
    } else {
        DEBUG_PRINTF("✗ Falha: %s\n", MQTT_TOPIC_ALERTA_RACAO);
        sucesso = false;
    }

    DEBUG_PRINTLN("===============================");

    return sucesso;
}

// Publicar status da central
bool GerenciadorMQTT::publicarStatusCentral(String status) {
    if (!estaConectado()) {
        return false;
    }

    StaticJsonDocument<200> doc;
    doc["status"] = status;
    doc["timestamp"] = millis();
    doc["client_id"] = MQTT_CLIENT_ID;

    String payload;
    serializeJson(doc, payload);

    return mqttClient->publish(MQTT_TOPIC_CENTRAL_STATUS, payload.c_str());
}

// Extrair ID da remota do tópico
int GerenciadorMQTT::extrairIdRemotaDoTopico(String topico) {
    int inicio = topico.indexOf("/remota/") + 8;
    int fim = topico.indexOf("/", inicio);

    if (inicio > 7 && fim > inicio) {
        String idStr = topico.substring(inicio, fim);
        return idStr.toInt();
    }

    return 1;
}

// Construir tópico com ID da remota
String GerenciadorMQTT::construirTopico(const char* template_topico, int idRemota) {
    char buffer[100];
    snprintf(buffer, sizeof(buffer), template_topico, idRemota);
    return String(buffer);
}

// Obter status da conexão
String GerenciadorMQTT::obterStatusConexao() const {
    if (conectado && mqttClient && mqttClient->connected()) {
        return "Conectado";
    } else if (tentativas_reconexao > 0) {
        return "Reconectando...";
    } else {
        return "Desconectado";
    }
}

// Obter descrição do erro MQTT
String GerenciadorMQTT::obterDescricaoErroMQTT(int codigo) {
    switch (codigo) {
        case -4: return "MQTT_CONNECTION_TIMEOUT";
        case -3: return "MQTT_CONNECTION_LOST";
        case -2: return "MQTT_CONNECT_FAILED";
        case -1: return "MQTT_DISCONNECTED";
        case 0:  return "MQTT_CONNECTED";
        case 1:  return "MQTT_CONNECT_BAD_PROTOCOL";
        case 2:  return "MQTT_CONNECT_BAD_CLIENT_ID";
        case 3:  return "MQTT_CONNECT_UNAVAILABLE";
        case 4:  return "MQTT_CONNECT_BAD_CREDENTIALS";
        case 5:  return "MQTT_CONNECT_UNAUTHORIZED";
        default: return "ERRO_DESCONHECIDO";
    }
}

// Getters
unsigned long GerenciadorMQTT::getUltimoPacote() const {
    return ultimo_pacote;
}

int GerenciadorMQTT::getPacotesPerdidos() const {
    return pacotes_perdidos;
}

// Reset de tentativas
void GerenciadorMQTT::resetarTentativas() {
    tentativas_reconexao = 0;
}
