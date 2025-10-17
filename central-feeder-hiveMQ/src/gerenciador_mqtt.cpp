#include "gerenciador_mqtt.h"

// Variáveis estáticas
GerenciadorMQTT* GerenciadorMQTT::instance = nullptr;
WiFiClientSecure* GerenciadorMQTT::wifiClient = nullptr;
PubSubClient* GerenciadorMQTT::mqttClient = nullptr;

// Construtor
GerenciadorMQTT::GerenciadorMQTT() :
    conectado(false),
    ultimo_pacote(0),
    ultimo_reconectar(0),
    pacotes_perdidos(0),
    tentativas_reconexao(0),
    callbackStatusRemota(nullptr),
    callbackVidaRemota(nullptr),
    callbackRespostaRemota(nullptr),
    callbackAlertaRacao(nullptr) {
    instance = this;
}

// Destrutor
GerenciadorMQTT::~GerenciadorMQTT() {
    desconectar();
    if (mqttClient) {
        delete mqttClient;
        mqttClient = nullptr;
    }
    if (wifiClient) {
        delete wifiClient;
        wifiClient = nullptr;
    }
    instance = nullptr;
}

// Métodos estáticos para inicializar e atualizar
void GerenciadorMQTT::inicializar() {
    if (!instance) {
        instance = new GerenciadorMQTT();
    }

    // Aguardar WiFi conectar antes de inicializar MQTT
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_MQTT_PRINTLN("Aguardando WiFi conectar...");

        int tentativas = 0;
        while (WiFi.status() != WL_CONNECTED && tentativas < 40) {
            delay(500);
            tentativas++;
        }

        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_MQTT_PRINTLN("AVISO: WiFi não conectou - MQTT será iniciado depois");
            return;
        }

        DEBUG_MQTT_PRINTLN("WiFi conectado! Iniciando MQTT...");
    }

    instance->init();
}

void GerenciadorMQTT::atualizar() {
    if (!instance) {
        return;
    }

    // Se MQTT ainda não foi inicializado mas WiFi já conectou, inicializar agora
    static bool tentouInicializar = false;
    if (!instance->conectado && WiFi.status() == WL_CONNECTED && !tentouInicializar) {
        DEBUG_MQTT_PRINTLN("WiFi conectou - tentando inicializar MQTT agora...");
        tentouInicializar = true;
        instance->init();
    }

    instance->loop();
}

// Callback estático para PubSubClient
void GerenciadorMQTT::callbackMQTT(char* topic, byte* payload, unsigned int length) {
    if (!instance) return;

    // Converter payload para String
    String payloadStr;
    payloadStr.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }

    // Processar mensagem
    instance->processarMensagem(String(topic), payloadStr);
}

// Inicialização
bool GerenciadorMQTT::init() {
    DEBUG_MQTT_PRINTLN("=== Inicializando Cliente MQTT HiveMQ ===");

    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_MQTT_PRINTLN("WiFi não conectado!");
        return false;
    }

    // Criar cliente WiFi seguro
    if (!wifiClient) {
        wifiClient = new WiFiClientSecure();
    }

    // Configurar certificado raiz para TLS
    wifiClient->setCACert(HIVEMQ_ROOT_CA);

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

    DEBUG_MQTT_PRINTF("Servidor: %s:%d\n", MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    DEBUG_MQTT_PRINTLN("TLS/SSL: Habilitado");

    // Conectar ao broker
    return conectar();
}

// Conectar ao broker MQTT
bool GerenciadorMQTT::conectar() {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_MQTT_PRINTLN("WiFi não conectado - impossível conectar MQTT");
        return false;
    }

    if (!mqttClient) {
        DEBUG_MQTT_PRINTLN("Cliente MQTT não inicializado");
        return false;
    }

    // Já conectado
    if (mqttClient->connected()) {
        conectado = true;
        return true;
    }

    DEBUG_MQTT_PRINTLN("Conectando ao HiveMQ Cloud...");
    DEBUG_MQTT_PRINTF("Cliente ID: %s\n", MQTT_CLIENT_ID);

    // Tentar conectar com usuário e senha
    bool sucesso = mqttClient->connect(
        MQTT_CLIENT_ID,
        MQTT_USERNAME,
        MQTT_PASSWORD
    );

    if (sucesso) {
        DEBUG_MQTT_PRINTLN("✓ CONECTADO AO HIVEMQ CLOUD!");
        conectado = true;
        tentativas_reconexao = 0;
        ultimo_pacote = millis();

        // Inscrever em tópicos
        inscreverTopicos();

        // Publicar status da central
        publicarStatusCentral("ONLINE");

        return true;
    } else {
        int estado = mqttClient->state();
        DEBUG_MQTT_PRINTF("✗ Falha na conexão! Estado: %d (%s)\n",
                         estado, obterDescricaoErroMQTT(estado).c_str());
        conectado = false;
        tentativas_reconexao++;
        return false;
    }
}

// Desconectar
void GerenciadorMQTT::desconectar() {
    if (mqttClient && mqttClient->connected()) {
        DEBUG_MQTT_PRINTLN("Desconectando do HiveMQ Cloud...");
        publicarStatusCentral("OFFLINE");
        mqttClient->disconnect();
        conectado = false;
        DEBUG_MQTT_PRINTLN("Desconectado");
    }
}

// Verificar se está conectado
bool GerenciadorMQTT::estaConectado() const {
    return conectado && mqttClient && mqttClient->connected() && (WiFi.status() == WL_CONNECTED);
}

// Loop para manter conexão e processar mensagens
void GerenciadorMQTT::loop() {
    if (!mqttClient) {
        return;
    }

    // Se não estiver conectado, tentar reconectar
    if (!mqttClient->connected()) {
        conectado = false;

        unsigned long agora = millis();
        if (agora - ultimo_reconectar > MQTT_RECONNECT_INTERVAL) {
            ultimo_reconectar = agora;

            if (tentativas_reconexao < MQTT_MAX_RETRIES) {
                DEBUG_MQTT_PRINTLN("Tentando reconectar...");
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

    // Verificar timeout de remotas
    verificarTimeoutRemotas();
}

// Enviar comando para remota específica
bool GerenciadorMQTT::enviarComandoRemota(int idRemota, const String& acao, int tempo) {
    if (!estaConectado()) {
        DEBUG_MQTT_PRINTLN("MQTT não conectado - comando não enviado");
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

    DEBUG_MQTT_PRINTF("Enviando comando para Remota %d...\n", idRemota);
    DEBUG_MQTT_PRINTF("   Tópico: %s\n", topico.c_str());
    DEBUG_MQTT_PRINTF("   Payload: %s\n", payload.c_str());

    // Publicar
    bool sucesso = mqttClient->publish(topico.c_str(), payload.c_str());

    if (sucesso) {
        DEBUG_MQTT_PRINTF("✓ Comando enviado para Remota %d\n", idRemota);
        ultimo_pacote = millis();
    } else {
        DEBUG_MQTT_PRINTF("✗ Falha ao enviar comando para Remota %d\n", idRemota);
    }

    return sucesso;
}

// Enviar comando geral
bool GerenciadorMQTT::enviarComandoGeral(const String& acao, int tempo, int idRemota) {
    if (!estaConectado()) {
        DEBUG_MQTT_PRINTLN("MQTT não conectado - comando geral não enviado");
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

    DEBUG_MQTT_PRINTF("Enviando comando GERAL para Remota %d...\n", idRemota);
    DEBUG_MQTT_PRINTF("  Tópico: %s\n", MQTT_TOPIC_COMANDO_GERAL);
    DEBUG_MQTT_PRINTF("  Payload: %s\n", payload.c_str());

    bool sucesso = mqttClient->publish(MQTT_TOPIC_COMANDO_GERAL, payload.c_str());

    if (sucesso) {
        DEBUG_MQTT_PRINTF("✓ Comando geral enviado para Remota %d\n", idRemota);
        ultimo_pacote = millis();
    } else {
        DEBUG_MQTT_PRINTF("✗ Falha ao enviar comando geral\n");
    }

    return sucesso;
}

// Configurar horário da remota
bool GerenciadorMQTT::configurarHorarioRemota(int idRemota, int hora, int minuto, int quantidade) {
    if (!estaConectado()) {
        DEBUG_MQTT_PRINTLN("MQTT não conectado - configuração de horário não enviada");
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

    DEBUG_MQTT_PRINTF("Configurando horário para Remota %d...\n", idRemota);
    DEBUG_MQTT_PRINTF("  Tópico: %s\n", topico.c_str());
    DEBUG_MQTT_PRINTF("  Payload: %s\n", payload.c_str());

    bool sucesso = mqttClient->publish(topico.c_str(), payload.c_str());

    if (sucesso) {
        DEBUG_MQTT_PRINTF("✓ Horário configurado: Remota %d às %02d:%02d (%dg)\n",
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
        DEBUG_MQTT_PRINTF("✓ Tempo configurado para Remota %d: %d segundos\n", idRemota, tempo);
        ultimo_pacote = millis();
    }

    return sucesso;
}

// Solicitar status da remota
bool GerenciadorMQTT::solicitarStatusRemota(int idRemota) {
    return enviarComandoRemota(idRemota, MQTT_CMD_STATUS);
}

// Inscrever em tópicos
bool GerenciadorMQTT::inscreverTopicos() {
    if (!mqttClient || !mqttClient->connected()) {
        DEBUG_MQTT_PRINTLN("Cliente não conectado - não é possível inscrever");
        return false;
    }

    DEBUG_MQTT_PRINTLN("=== INSCREVENDO EM TÓPICOS ===");

    bool sucesso = true;

    // Status das remotas (com wildcard)
    String topico1 = "alimentador/remota/+/status";
    if (mqttClient->subscribe(topico1.c_str())) {
        DEBUG_MQTT_PRINTF("✓ Inscrito: %s\n", topico1.c_str());
    } else {
        DEBUG_MQTT_PRINTF("✗ Falha: %s\n", topico1.c_str());
        sucesso = false;
    }

    // Status geral (compatibilidade)
    String topico2 = "alimentador/remota/status";
    if (mqttClient->subscribe(topico2.c_str())) {
        DEBUG_MQTT_PRINTF("✓ Inscrito: %s\n", topico2.c_str());
    } else {
        DEBUG_MQTT_PRINTF("✗ Falha: %s\n", topico2.c_str());
        sucesso = false;
    }

    // Vida das remotas
    String topico3 = "alimentador/remota/+/vida";
    if (mqttClient->subscribe(topico3.c_str())) {
        DEBUG_MQTT_PRINTF("✓ Inscrito: %s\n", topico3.c_str());
    } else {
        DEBUG_MQTT_PRINTF("✗ Falha: %s\n", topico3.c_str());
        sucesso = false;
    }

    // Heartbeat geral
    if (mqttClient->subscribe(MQTT_TOPIC_HEARTBEAT_GERAL)) {
        DEBUG_MQTT_PRINTF("✓ Inscrito: %s\n", MQTT_TOPIC_HEARTBEAT_GERAL);
    } else {
        DEBUG_MQTT_PRINTF("✗ Falha: %s\n", MQTT_TOPIC_HEARTBEAT_GERAL);
        sucesso = false;
    }

    // Respostas das remotas
    String topico5 = "alimentador/remota/+/resposta";
    if (mqttClient->subscribe(topico5.c_str())) {
        DEBUG_MQTT_PRINTF("✓ Inscrito: %s\n", topico5.c_str());
    } else {
        DEBUG_MQTT_PRINTF("✗ Falha: %s\n", topico5.c_str());
        sucesso = false;
    }

    // Respostas gerais (compatibilidade)
    if (mqttClient->subscribe(MQTT_TOPIC_CONCLUIDO_GERAL)) {
        DEBUG_MQTT_PRINTF("✓ Inscrito: %s\n", MQTT_TOPIC_CONCLUIDO_GERAL);
    } else {
        DEBUG_MQTT_PRINTF("✗ Falha: %s\n", MQTT_TOPIC_CONCLUIDO_GERAL);
        sucesso = false;
    }

    // Alertas de ração
    if (mqttClient->subscribe(MQTT_TOPIC_ALERTA_RACAO)) {
        DEBUG_MQTT_PRINTF("✓ Inscrito: %s\n", MQTT_TOPIC_ALERTA_RACAO);
    } else {
        DEBUG_MQTT_PRINTF("✗ Falha: %s\n", MQTT_TOPIC_ALERTA_RACAO);
        sucesso = false;
    }

    DEBUG_MQTT_PRINTLN("===============================");

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

// Processar mensagem recebida
void GerenciadorMQTT::processarMensagem(const String& topico, const String& payload) {
    int idRemota = extrairIdRemotaDoTopico(topico);

    // Debug
    if (topico.indexOf("/remota") >= 0) {
        DEBUG_MQTT_PRINTLN();
        DEBUG_MQTT_PRINTLN("=== MENSAGEM RECEBIDA ===");
        DEBUG_MQTT_PRINTF("   Tópico: %s\n", topico.c_str());
        DEBUG_MQTT_PRINTF("   Payload: %s\n", payload.c_str());
        DEBUG_MQTT_PRINTF("   ID Remota: %d\n", idRemota);
        DEBUG_MQTT_PRINTLN("=========================");
    }

    // Processar baseado no tipo de tópico
    if (topico.indexOf("/status") > 0) {
        processarMensagemStatus(idRemota, payload);
    } else if (topico.indexOf("/vida") > 0) {
        processarMensagemVida(idRemota, payload);
    } else if (topico.indexOf("/resposta") > 0 || topico.indexOf("/concluido") > 0) {
        processarMensagemResposta(idRemota, payload);
    } else if (topico == MQTT_TOPIC_HEARTBEAT_GERAL) {
        processarHeartbeatGeral(payload);
    } else if (topico == MQTT_TOPIC_ALERTA_RACAO) {
        processarAlertaRacao(payload);
    }

    ultimo_pacote = millis();
}

// Processar mensagem de status
void GerenciadorMQTT::processarMensagemStatus(int idRemota, String payload) {
    StaticJsonDocument<300> doc;
    DeserializationError erro = deserializeJson(doc, payload);

    String status = "UNKNOWN";
    if (!erro) {
        status = doc["status"] | "UNKNOWN";
    } else {
        status = payload;
    }

    DEBUG_MQTT_PRINTF("Status Remota %d: %s\n", idRemota, status.c_str());

    if (callbackStatusRemota) {
        callbackStatusRemota(idRemota, status);
    }
}

// Processar mensagem de vida/heartbeat
void GerenciadorMQTT::processarMensagemVida(int idRemota, String payload) {
    StaticJsonDocument<200> doc;
    DeserializationError erro = deserializeJson(doc, payload);

    String status = "UNKNOWN";
    if (!erro) {
        status = doc["status"] | "UNKNOWN";
    } else {
        status = payload;
    }

    DEBUG_MQTT_PRINTF("Heartbeat Remota %d: %s\n", idRemota, status.c_str());

    if (callbackVidaRemota) {
        callbackVidaRemota(idRemota, status);
    }
}

// Processar mensagem de resposta
void GerenciadorMQTT::processarMensagemResposta(int idRemota, String payload) {
    StaticJsonDocument<300> doc;
    DeserializationError erro = deserializeJson(doc, payload);

    String resultado = payload;
    if (!erro) {
        resultado = doc["resultado"] | payload;
    }

    DEBUG_MQTT_PRINTF("Resposta Remota %d: %s\n", idRemota, resultado.c_str());

    if (callbackRespostaRemota) {
        callbackRespostaRemota(idRemota, resultado);
    }
}

// Processar heartbeat geral
void GerenciadorMQTT::processarHeartbeatGeral(String payload) {
    StaticJsonDocument<300> doc;
    DeserializationError erro = deserializeJson(doc, payload);

    int idRemota = 1;
    String status = "UNKNOWN";
    int rssi = 0;
    unsigned long uptime = 0;

    if (!erro) {
        idRemota = doc["remota_id"] | 1;
        status = doc["status"] | "UNKNOWN";
        rssi = doc["wifi_rssi"] | 0;
        uptime = doc["uptime"] | 0;
    } else {
        status = payload;
    }

    DEBUG_MQTT_PRINTF("Heartbeat Geral Remota %d: %s\n", idRemota, status.c_str());

    if (status == "ALIVE") {
        atualizarStatusRemota(idRemota, rssi, uptime);
    }

    if (callbackVidaRemota) {
        callbackVidaRemota(idRemota, status);
    }
}

// Processar alerta de ração
void GerenciadorMQTT::processarAlertaRacao(String payload) {
    StaticJsonDocument<300> doc;
    DeserializationError erro = deserializeJson(doc, payload);

    int idRemota = 1;
    String nivelRacao = "DESCONHECIDO";

    if (!erro) {
        idRemota = doc["remota_id"] | 1;
        nivelRacao = doc["nivel"] | doc["status"] | payload;
    } else {
        nivelRacao = payload;
    }

    DEBUG_MQTT_PRINTF("Alerta Ração Remota %d: %s\n", idRemota, nivelRacao.c_str());

    if (callbackAlertaRacao) {
        callbackAlertaRacao(idRemota, nivelRacao);
    }
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

// Definir callbacks
void GerenciadorMQTT::definirCallbackStatusRemota(void (*callback)(int, const String&)) {
    callbackStatusRemota = callback;
}

void GerenciadorMQTT::definirCallbackVidaRemota(void (*callback)(int, const String&)) {
    callbackVidaRemota = callback;
}

void GerenciadorMQTT::definirCallbackRespostaRemota(void (*callback)(int, const String&)) {
    callbackRespostaRemota = callback;
}

void GerenciadorMQTT::definirCallbackAlertaRacao(void (*callback)(int, const String&)) {
    callbackAlertaRacao = callback;
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

// Verificar se remota está conectada
bool GerenciadorMQTT::remotaEstaConectada(int idRemota) const {
    auto it = remotas_ativas.find(idRemota);
    if (it != remotas_ativas.end()) {
        return it->second.conectada;
    }
    return false;
}

// Atualizar status da remota
void GerenciadorMQTT::atualizarStatusRemota(int idRemota, int rssi, unsigned long uptime) {
    InfoRemota& info = remotas_ativas[idRemota];
    info.conectada = true;
    info.ultimo_heartbeat = millis();
    info.rssi = rssi;
    info.uptime = uptime;

    DEBUG_MQTT_PRINTF("✓ Remota %d atualizada: RSSI=%d dBm, Uptime=%lu s\n",
                      idRemota, rssi, uptime / 1000);
}

// Verificar timeout de remotas
void GerenciadorMQTT::verificarTimeoutRemotas() {
    unsigned long agora = millis();

    for (auto& par : remotas_ativas) {
        int idRemota = par.first;
        InfoRemota& info = par.second;

        if (info.conectada) {
            if (agora - info.ultimo_heartbeat > TIMEOUT_REMOTA) {
                info.conectada = false;
                DEBUG_MQTT_PRINTF("✗ Remota %d desconectada (timeout)\n", idRemota);
            }
        }
    }
}
