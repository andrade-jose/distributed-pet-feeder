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
      tentativas_reconexao(0),
      bdSeq(0) {  // ✅ NOVO: Inicializar sequência SparkplugB
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

    // ✅ MUDADO: Cliente WiFi normal (sem TLS para rede local)
    if (!wifiClient) {
        wifiClient = new WiFiClient();
    }

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
    DEBUG_PRINTLN("Conexão: Sem TLS (rede local segura)");

    // Conectar ao broker
    return conectar();
}


bool GerenciadorMQTT::conectar() {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("WiFi não conectado - impossível conectar MQTT");
        return false;
    }

    Serial.println("\n=== 🔍 DIAGNÓSTICO COMPLETO MQTT ===");
    
    // ✅ TESTE 1: Conectividade TCP básica
    Serial.print("1. Testando conexão TCP com ");
    Serial.print(MQTT_BROKER_HOST);
    Serial.print(":");
    Serial.print(MQTT_BROKER_PORT);
    Serial.print(" ... ");
    
    WiFiClient testClient;
    testClient.setTimeout(5000);
    
    if (testClient.connect(MQTT_BROKER_HOST, MQTT_BROKER_PORT)) {
        Serial.println("✅ SUCESSO");
        Serial.println("   ⮡ Porta 1883 está respondendo");
        testClient.stop();
    } else {
        Serial.println("❌ FALHA");
        Serial.println("   ⮡ Não consegue conectar TCP");
        Serial.println("   ⮡ Verifique:");
        Serial.println("      - Se o IP está correto");
        Serial.println("      - Se o Mosquitto está rodando");
        Serial.println("      - Se o firewall permite a porta");
        return false;
    }

    // ✅ TESTE 2: Tentar conexão MQTT sem autenticação
    Serial.print("2. Testando MQTT sem autenticação... ");
    PubSubClient testMqtt1(testClient);
    testMqtt1.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    
    if (testMqtt1.connect("test_client_anon")) {
        Serial.println("✅ SUCESSO");
        Serial.println("   ⮡ Broker aceita conexões anônimas");
        testMqtt1.disconnect();
    } else {
        Serial.println("❌ FALHA");
        Serial.println("   ⮡ Broker requer autenticação");
    }

    // ✅ TESTE 3: Tentar conexão MQTT COM autenticação
    Serial.print("3. Testando MQTT com autenticação... ");
    PubSubClient testMqtt2(testClient);
    testMqtt2.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    
    if (testMqtt2.connect("test_client_auth", MQTT_USERNAME, MQTT_PASSWORD)) {
        Serial.println("✅ SUCESSO");
        Serial.println("   ⮡ Credenciais estão CORRETAS");
        testMqtt2.disconnect();
    } else {
        Serial.println("❌ FALHA");
        Serial.println("   ⮡ Erro: " + String(testMqtt2.state()));
        Serial.println("   ⮡ " + obterDescricaoErroMQTT(testMqtt2.state()));
    }

    Serial.println("=== FIM DO DIAGNÓSTICO ===\n");

    // Tentar conexão normal
    if (!mqttClient) {
        DEBUG_PRINTLN("Cliente MQTT não inicializado");
        return false;
    }

    if (mqttClient->connected()) {
        conectado = true;
        return true;
    }

    DEBUG_PRINTLN("Tentando conexão principal...");
    bool sucesso = mqttClient->connect(
        MQTT_CLIENT_ID,
        MQTT_USERNAME,
        MQTT_PASSWORD,
        MQTT_TOPIC_NDEATH,
        1,
        true,
        construirPayloadNDEATH()
    );

    if (sucesso) {
        estadoSistema.atualizarStatusMqtt(true);
        DEBUG_PRINTLN("✅ CONECTADO AO MOSQUITTO!");
        conectado = true;
        tentativas_reconexao = 0;
        ultimo_pacote = millis();

        publicarNBIRTH();
        inscreverTopicos();
        return true;
    } else {
        estadoSistema.atualizarStatusMqtt(false);
        int estado = mqttClient->state();
        DEBUG_PRINTF("❌ Falha na conexão! Estado: %d (%s)\n",
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

// NOVO: Publicar com retain flag
bool GerenciadorMQTT::publicarComRetain(const String& topico, const String& payload) {
    if (!estaConectado()) {
        Serial.println("MQTT não conectado, não é possível publicar");
        return false;
    }

    // Converter String para bytes para usar retain
    bool sucesso = mqttClient->publish(topico.c_str(), (uint8_t*)payload.c_str(), payload.length(), true);

    if (sucesso) {
        DEBUG_PRINTF("MQTT publicado (retain): %s -> %s\n", topico.c_str(), payload.c_str());
        ultimo_pacote = millis();
    } else {
        DEBUG_PRINTF("Falha ao publicar MQTT (retain): %s\n", topico.c_str());
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

// ✅ ATUALIZADO: Enviar comando usando SparkplugB DCMD
bool GerenciadorMQTT::enviarComandoRemota(int idRemota, const String& acao, int tempo) {
    // Redirecionar para publicarDCMD (SparkplugB)
    return publicarDCMD(idRemota, acao, tempo);
}

// ✅ ATUALIZADO: Enviar comando geral usando SparkplugB DCMD
bool GerenciadorMQTT::enviarComandoGeral(const String& acao, int tempo, int idRemota) {
    // Redirecionar para publicarDCMD (SparkplugB)
    return publicarDCMD(idRemota, acao, tempo);
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

// ✅ SUBSTITUÍDO: Inscrever em tópicos SparkplugB
bool GerenciadorMQTT::inscreverTopicos() {
    if (!mqttClient || !mqttClient->connected()) {
        DEBUG_PRINTLN("Cliente não conectado - não é possível inscrever");
        return false;
    }

    DEBUG_PRINTLN("=== INSCREVENDO EM TÓPICOS SPARKPLUGB ===");
    bool sucesso = true;

    // 1. DBIRTH - Nascimento de remotas
    if (mqttClient->subscribe(MQTT_TOPIC_DBIRTH_ALL)) {
        DEBUG_PRINTF("✓ Inscrito: %s (DBIRTH)\n", MQTT_TOPIC_DBIRTH_ALL);
    } else {
        DEBUG_PRINTF("✗ Falha: %s\n", MQTT_TOPIC_DBIRTH_ALL);
        sucesso = false;
    }

    // 2. DDATA - Telemetria de remotas
    if (mqttClient->subscribe(MQTT_TOPIC_DDATA_ALL)) {
        DEBUG_PRINTF("✓ Inscrito: %s (DDATA)\n", MQTT_TOPIC_DDATA_ALL);
    } else {
        DEBUG_PRINTF("✗ Falha: %s\n", MQTT_TOPIC_DDATA_ALL);
        sucesso = false;
    }

    // 3. DDEATH - Morte de remotas
    if (mqttClient->subscribe(MQTT_TOPIC_DDEATH_ALL)) {
        DEBUG_PRINTF("✓ Inscrito: %s (DDEATH)\n", MQTT_TOPIC_DDEATH_ALL);
    } else {
        DEBUG_PRINTF("✗ Falha: %s\n", MQTT_TOPIC_DDEATH_ALL);
        sucesso = false;
    }

    // 4. NCMD - Comandos para a central (do Dashboard)
    if (mqttClient->subscribe(MQTT_TOPIC_NCMD)) {
        DEBUG_PRINTF("✓ Inscrito: %s (NCMD)\n", MQTT_TOPIC_NCMD);
    } else {
        DEBUG_PRINTF("✗ Falha: %s\n", MQTT_TOPIC_NCMD);
        sucesso = false;
    }

    DEBUG_PRINTLN("===============================");
    DEBUG_PRINTF("Total de inscrições: 4 tópicos SparkplugB\n");

    return sucesso;
}


// ✅ ATUALIZADO: Configurar horário usando SparkplugB DCMD
bool GerenciadorMQTT::configurarHorarioRefeicao(int idRemota, int indiceRefeicao, int hora, int minuto, int quantidade) {
    if (!estaConectado()) {
        DEBUG_PRINTLN("MQTT não conectado - configuração de horário não enviada");
        return false;
    }

    // Construir tópico DCMD SparkplugB
    char topico[128];
    snprintf(topico, sizeof(topico), MQTT_TOPIC_DCMD_TEMPLATE, idRemota);

    // Payload SparkplugB com múltiplas métricas
    StaticJsonDocument<384> doc;
    doc["timestamp"] = millis();

    JsonArray metrics = doc.createNestedArray("metrics");

    // Comando
    JsonObject cmd = metrics.createNestedObject();
    cmd["name"] = "command";
    cmd["type"] = "string";
    cmd["value"] = "CONFIG_REFEICAO";

    // Índice da refeição
    JsonObject idx = metrics.createNestedObject();
    idx["name"] = "indice";
    idx["type"] = "int32";
    idx["value"] = indiceRefeicao;

    // Hora
    JsonObject h = metrics.createNestedObject();
    h["name"] = "hora";
    h["type"] = "int32";
    h["value"] = hora;

    // Minuto
    JsonObject m = metrics.createNestedObject();
    m["name"] = "minuto";
    m["type"] = "int32";
    m["value"] = minuto;

    // Quantidade
    JsonObject q = metrics.createNestedObject();
    q["name"] = "quantidade";
    q["type"] = "int32";
    q["value"] = quantidade;

    String payload;
    serializeJson(doc, payload);

    DEBUG_PRINTF("[CONFIG] Refeição %d → Remota %d: %02d:%02d (%dg)\n",
                 indiceRefeicao, idRemota, hora, minuto, quantidade);
    DEBUG_PRINTF("[CONFIG] Tópico: %s\n", topico);

    bool sucesso = mqttClient->publish(topico, payload.c_str());

    if (sucesso) {
        DEBUG_PRINTF("✓ Refeição %d configurada\n", indiceRefeicao);
    } else {
        DEBUG_PRINTF("✗ Falha ao configurar refeição %d\n", indiceRefeicao);
    }

    return sucesso;
}

// ✅ ATUALIZADO: Publicar estado completo usando SparkplugB NDATA
bool GerenciadorMQTT::publicarEstadoCompleto(int idRemota) {
    // Redirecionar para NDATA que contém todas as métricas do sistema
    return publicarNDATA();
}

// ✅ ATUALIZADO: Notificar mudança de configuração usando SparkplugB NDATA
bool GerenciadorMQTT::notificarMudancaConfig(int idRemota, int indiceRefeicao, int hora, int minuto, int quantidade, const char* origem) {
    // As mudanças de configuração serão refletidas no próximo NDATA
    // Podemos forçar uma publicação NDATA imediata se necessário
    return publicarNDATA();
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

// ===== SPARKPLUGB - PAYLOADS =====

const char* GerenciadorMQTT::construirPayloadNDEATH() {
    static char payload[64];
    snprintf(payload, sizeof(payload),
             "{\"timestamp\":%lu,\"seq\":%d}",
             millis(), bdSeq);
    return payload;
}

String GerenciadorMQTT::construirPayloadNBIRTH() {
    StaticJsonDocument<512> doc;
    doc["timestamp"] = millis();
    doc["seq"] = 0;
    bdSeq = 0;  // Resetar sequência no birth

    JsonArray metrics = doc.createNestedArray("metrics");

    // Métrica obrigatória: Node Control/Rebirth
    JsonObject rebirth = metrics.createNestedObject();
    rebirth["name"] = "Node Control/Rebirth";
    rebirth["type"] = "boolean";
    rebirth["value"] = false;

    // Métrica: Versão do firmware
    JsonObject version = metrics.createNestedObject();
    version["name"] = "Properties/Version";
    version["type"] = "string";
    version["value"] = SYSTEM_VERSION;

    // Métrica: Build date
    JsonObject buildDate = metrics.createNestedObject();
    buildDate["name"] = "Properties/BuildDate";
    buildDate["type"] = "string";
    buildDate["value"] = String(__DATE__) + " " + String(__TIME__);

    // Métrica: Status
    JsonObject status = metrics.createNestedObject();
    status["name"] = "status";
    status["type"] = "string";
    status["value"] = "ONLINE";

    String payload;
    serializeJson(doc, payload);

    DEBUG_PRINTF("[NBIRTH] Payload: %s\n", payload.c_str());
    return payload;
}

String GerenciadorMQTT::construirPayloadNDATA() {
    StaticJsonDocument<512> doc;
    doc["timestamp"] = millis();
    doc["seq"] = bdSeq++;  // Incrementar sequência

    JsonArray metrics = doc.createNestedArray("metrics");

    // Métrica: Uptime
    JsonObject uptime = metrics.createNestedObject();
    uptime["name"] = "uptime";
    uptime["type"] = "int64";
    uptime["value"] = millis();

    // Métrica: WiFi RSSI
    JsonObject rssi = metrics.createNestedObject();
    rssi["name"] = "wifi_rssi";
    rssi["type"] = "int32";
    rssi["value"] = WiFi.RSSI();

    // Métrica: Número de remotas online
    int remotasOnline = 0;
    for (int i = 0; i < estadoSistema.numRemotas; i++) {
        if (estadoSistema.remotas[i].online) remotasOnline++;
    }

    JsonObject remotas = metrics.createNestedObject();
    remotas["name"] = "remotas_online";
    remotas["type"] = "int32";
    remotas["value"] = remotasOnline;

    String payload;
    serializeJson(doc, payload);
    return payload;
}

String GerenciadorMQTT::construirPayloadDCMD(const String& comando, int valor) {
    StaticJsonDocument<256> doc;
    doc["timestamp"] = millis();

    JsonArray metrics = doc.createNestedArray("metrics");

    // Métrica: Comando
    JsonObject cmd = metrics.createNestedObject();
    cmd["name"] = "command";
    cmd["type"] = "string";
    cmd["value"] = comando;

    // Métrica: Valor (se houver)
    if (valor > 0) {
        JsonObject val = metrics.createNestedObject();
        val["name"] = "value";
        val["type"] = "int32";
        val["value"] = valor;
    }

    String payload;
    serializeJson(doc, payload);
    return payload;
}

// ===== SPARKPLUGB - PUBLICAÇÕES =====

bool GerenciadorMQTT::publicarNBIRTH() {
    if (!estaConectado()) {
        DEBUG_PRINTLN("[NBIRTH] MQTT não conectado");
        return false;
    }

    String payload = construirPayloadNBIRTH();

    bool sucesso = mqttClient->publish(
        MQTT_TOPIC_NBIRTH,
        payload.c_str(),
        true  // Retain
    );

    if (sucesso) {
        DEBUG_PRINTLN("[NBIRTH] ✓ Publicado com sucesso");
    } else {
        DEBUG_PRINTLN("[NBIRTH] ✗ Falha ao publicar");
    }

    return sucesso;
}

bool GerenciadorMQTT::publicarNDATA() {
    if (!estaConectado()) return false;

    String payload = construirPayloadNDATA();

    bool sucesso = mqttClient->publish(
        MQTT_TOPIC_NDATA,
        payload.c_str(),
        false  // Sem retain
    );

    if (sucesso) {
        DEBUG_PRINTLN("[NDATA] ✓ Publicado");
    }

    return sucesso;
}

bool GerenciadorMQTT::publicarDCMD(int idRemota, const String& comando, int valor) {
    if (!estaConectado()) {
        DEBUG_PRINTLN("[DCMD] MQTT não conectado");
        return false;
    }

    // Construir tópico: spBv1.0/ALIMENTADOR_PETS/DCMD/EON_CENTRAL/REMOTA_1
    char topico[128];
    snprintf(topico, sizeof(topico), MQTT_TOPIC_DCMD_TEMPLATE, idRemota);

    String payload = construirPayloadDCMD(comando, valor);

    DEBUG_PRINTF("[DCMD] Remota %d: %s (valor=%d)\n", idRemota, comando.c_str(), valor);
    DEBUG_PRINTF("[DCMD] Tópico: %s\n", topico);
    DEBUG_PRINTF("[DCMD] Payload: %s\n", payload.c_str());

    bool sucesso = mqttClient->publish(topico, payload.c_str());

    if (sucesso) {
        DEBUG_PRINTF("[DCMD] ✓ Comando enviado para Remota %d\n", idRemota);
    } else {
        DEBUG_PRINTF("[DCMD] ✗ Falha ao enviar comando para Remota %d\n", idRemota);
    }

    return sucesso;
}
