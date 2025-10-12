#include "gerenciador_mqtt.h"

// Variáveis estáticas
GerenciadorMQTT* GerenciadorMQTT::instance = nullptr;
PicoMQTT::ServerLocalSubscribe* GerenciadorMQTT::broker = nullptr;

// Construtor
GerenciadorMQTT::GerenciadorMQTT() :
    broker_iniciado(false),
    conectado(false),
    ultimo_pacote(0),
    ultimo_reconectar(0),
    pacotes_perdidos(0),
    tentativas_reconexao(0),
    clientes_conectados(0),
    tempo_inicio_broker(0),
    callbackStatusRemota(nullptr),
    callbackVidaRemota(nullptr),
    callbackRespostaRemota(nullptr),
    callbackAlertaRacao(nullptr) {
    instance = this;
}

// Destrutor
GerenciadorMQTT::~GerenciadorMQTT() {
    pararBroker();
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
            return;  // Não inicializar ainda, será feito no loop
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
    if (!instance->broker_iniciado && WiFi.status() == WL_CONNECTED && !tentouInicializar) {
        DEBUG_MQTT_PRINTLN("WiFi conectou - tentando inicializar MQTT agora...");
        tentouInicializar = true;
        instance->init();
    }

    instance->loop();
}

// Inicialização
bool GerenciadorMQTT::init() {
    DEBUG_MQTT_PRINTLN("=== Inicializando Gerenciador MQTT ===");

    // 1. Iniciar broker MQTT local
    if (!iniciarBroker()) {
        DEBUG_MQTT_PRINTLN("ERRO: Falha ao iniciar broker MQTT!");
        return false;
    }

    // 2. Aguardar broker estabilizar
    DEBUG_MQTT_PRINTLN("Aguardando broker estabilizar...");
    delay(1000);

    // 3. Inscrever nos tópicos usando PicoMQTT
    DEBUG_MQTT_PRINTLN("Inscrevendo em tópicos...");
    inscreverTopicos();

    // 4. Marcar como conectado
    conectado = true;
    DEBUG_MQTT_PRINTLN("Broker MQTT pronto!");

    return true;
}


// Iniciar Broker MQTT Local
bool GerenciadorMQTT::iniciarBroker() {
    DEBUG_MQTT_PRINTLN("=== Iniciando Broker MQTT Local ===");

    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_MQTT_PRINTLN("WiFi não conectado - impossível iniciar broker!");
        return false;
    }

    if (!broker) {
        // PicoMQTT: porta é definida no construtor
        // Usar ServerLocalSubscribe para permitir subscriptions locais
        broker = new PicoMQTT::ServerLocalSubscribe(MQTT_BROKER_PORT);
    }

    // Iniciar broker
    broker->begin();

    broker_iniciado = true;
    tempo_inicio_broker = millis();

    IPAddress ip = WiFi.localIP();
    DEBUG_MQTT_PRINTLN("BROKER MQTT INICIADO COM SUCESSO!");
    DEBUG_MQTT_PRINTF("   IP: %s\n", ip.toString().c_str());
    DEBUG_MQTT_PRINTF("   Porta: %d\n", MQTT_BROKER_PORT);
    DEBUG_MQTT_PRINTF("   Máximo de clientes: %d\n", MQTT_MAX_CLIENTS);
    DEBUG_MQTT_PRINTLN("   Suporte a subscribe local: SIM");
    DEBUG_MQTT_PRINTLN("===============================");

    return true;
}

// Parar Broker MQTT
void GerenciadorMQTT::pararBroker() {
    if (broker && broker_iniciado) {
        DEBUG_MQTT_PRINTLN("Parando broker MQTT...");
        // uMQTTBroker não tem método stop, ele é gerenciado automaticamente
        broker_iniciado = false;
        DEBUG_MQTT_PRINTLN("Broker MQTT parado");
    }
}

// Loop para atualizar broker
void GerenciadorMQTT::loop() {
    // Atualizar broker PicoMQTT
    if (broker && broker_iniciado) {
        broker->loop();
    }

    // Verificar timeout de remotas a cada ciclo
    verificarTimeoutRemotas();
}

// Conectar ao MQTT (broker local)
bool GerenciadorMQTT::conectar() {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_MQTT_PRINTLN("WiFi não conectado - impossível conectar MQTT");
        return false;
    }

    if (!broker_iniciado) {
        DEBUG_MQTT_PRINTLN("Broker não iniciado - tentando iniciar...");
        return iniciarBroker();
    }

    conectado = broker_iniciado;
    return conectado;
}

// Desconectar
void GerenciadorMQTT::desconectar() {
    if (conectado) {
        DEBUG_MQTT_PRINTLN("Parando broker MQTT...");
        pararBroker();
        conectado = false;
        DEBUG_MQTT_PRINTLN("Broker MQTT parado");
    }
}

// Verificar se está conectado
bool GerenciadorMQTT::estaConectado() const {
    return conectado && broker_iniciado && (WiFi.status() == WL_CONNECTED);
}

// Enviar comando para remota específica
bool GerenciadorMQTT::enviarComandoRemota(int idRemota, const String& acao, int tempo) {
    if (!estaConectado()) {
        DEBUG_MQTT_PRINTLN("Broker não conectado - comando não enviado");
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

    // Publicar usando PicoMQTT
    broker->publish(topico, payload);

    DEBUG_MQTT_PRINTF("Comando enviado com sucesso para Remota %d\n", idRemota);
    ultimo_pacote = millis();

    return true;
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

    broker->publish(MQTT_TOPIC_COMANDO_GERAL, payload);

    DEBUG_MQTT_PRINTF("Comando geral enviado com sucesso para Remota %d\n", idRemota);
    ultimo_pacote = millis();

    return true;
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

    broker->publish(topico, payload);

    DEBUG_MQTT_PRINTF("Horário configurado: Remota %d às %02d:%02d (%dg)\n",
                      idRemota, hora, minuto, quantidade);
    ultimo_pacote = millis();

    return true;
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

    broker->publish(topico, payload);

    DEBUG_MQTT_PRINTF("Tempo configurado para Remota %d: %d segundos\n", idRemota, tempo);
    ultimo_pacote = millis();

    return true;
}

// Solicitar status da remota
bool GerenciadorMQTT::solicitarStatusRemota(int idRemota) {
    return enviarComandoRemota(idRemota, MQTT_CMD_STATUS);
}

// Inscrever em tópicos
bool GerenciadorMQTT::inscreverTopicos() {
    DEBUG_MQTT_PRINTLN("=== INSCREVENDO EM TÓPICOS ===");

    // Status das remotas
    String topico1 = "alimentador/remota/+/status";
    broker->subscribe(topico1, [this](char* topic, char* payload) {
        processarMensagem(String(topic), String(payload));
    });
    DEBUG_MQTT_PRINTF("Inscrito em: %s - OK\n", topico1.c_str());

    // Status geral (compatibilidade)
    String topico2 = "alimentador/remota/status";
    broker->subscribe(topico2, [this](char* topic, char* payload) {
        processarMensagem(String(topic), String(payload));
    });
    DEBUG_MQTT_PRINTF("Inscrito em: %s - OK\n", topico2.c_str());

    // Vida das remotas
    String topico3 = "alimentador/remota/+/vida";
    broker->subscribe(topico3, [this](char* topic, char* payload) {
        processarMensagem(String(topic), String(payload));
    });
    DEBUG_MQTT_PRINTF("Inscrito em: %s - OK\n", topico3.c_str());

    // Heartbeat geral
    String topico4 = MQTT_TOPIC_HEARTBEAT_GERAL;
    broker->subscribe(topico4, [this](char* topic, char* payload) {
        processarMensagem(String(topic), String(payload));
    });
    DEBUG_MQTT_PRINTF("Inscrito em: %s - OK\n", topico4.c_str());

    // Respostas das remotas
    String topico5 = "alimentador/remota/+/resposta";
    broker->subscribe(topico5, [this](char* topic, char* payload) {
        processarMensagem(String(topic), String(payload));
    });
    DEBUG_MQTT_PRINTF("Inscrito em: %s - OK\n", topico5.c_str());

    // Respostas gerais (compatibilidade)
    String topico6 = "alimentador/remota/concluido";
    broker->subscribe(topico6, [this](char* topic, char* payload) {
        processarMensagem(String(topic), String(payload));
    });
    DEBUG_MQTT_PRINTF("Inscrito em: %s - OK\n", topico6.c_str());

    // Alertas de ração
    String topico7 = MQTT_TOPIC_ALERTA_RACAO;
    broker->subscribe(topico7, [this](char* topic, char* payload) {
        processarMensagem(String(topic), String(payload));
    });
    DEBUG_MQTT_PRINTF("Inscrito em: %s - OK\n", topico7.c_str());

    DEBUG_MQTT_PRINTLN("===============================");

    return true;
}

// Publicar status da central
bool GerenciadorMQTT::publicarStatusCentral(String status) {
    if (!estaConectado()) {
        return false;
    }

    StaticJsonDocument<200> doc;
    doc["status"] = status;
    doc["timestamp"] = millis();
    doc["remotas_configuradas"] = MAX_REMOTAS;

    String payload;
    serializeJson(doc, payload);

    broker->publish(MQTT_TOPIC_CENTRAL_STATUS, payload);

    return true;
}


// Processar mensagem recebida
void GerenciadorMQTT::processarMensagem(const String& topico, const String& payload) {
    int idRemota = extrairIdRemotaDoTopico(topico);
    
    // Debug apenas para tópicos relevantes
    if (topico.indexOf("/remota") >= 0) {
        DEBUG_MQTT_PRINTLN();
        DEBUG_MQTT_PRINTLN("=== MENSAGEM MQTT RECEBIDA ===");
        DEBUG_MQTT_PRINTF("   Tópico: %s\n", topico.c_str());
        DEBUG_MQTT_PRINTF("   Payload: %s\n", payload.c_str());
        DEBUG_MQTT_PRINTF("   ID Remota: %d\n", idRemota);
        DEBUG_MQTT_PRINTLN("===============================");
    }
    
    // Processar baseado no tipo de tópico
    if (topico.indexOf("/status") > 0) {
        processarMensagemStatus(idRemota, payload);
    } else if (topico.indexOf("/vida") > 0) {
        processarMensagemVida(idRemota, payload);
    } else if (topico.indexOf("/resposta") > 0) {
        processarMensagemResposta(idRemota, payload);
    } else if (topico == MQTT_TOPIC_HEARTBEAT_GERAL) {
        processarHeartbeatGeral(payload);
    } else if (topico == MQTT_TOPIC_ALERTA_RACAO) {
        processarAlertaRacao(payload);
    }
}

// Processar mensagem de status
void GerenciadorMQTT::processarMensagemStatus(int idRemota, String payload) {
    StaticJsonDocument<300> doc;
    DeserializationError erro = deserializeJson(doc, payload);
    
    String status = "UNKNOWN";
    if (!erro) {
        status = doc["status"] | "UNKNOWN";
    } else {
        // Se não é JSON, usar payload como status
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

    int idRemota = 1; // Default
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

    // Atualizar status da remota
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
    
    int idRemota = 1; // Default
    String nivelRacao = "DESCONHECIDO";
    
    if (!erro) {
        idRemota = doc["remota_id"] | 1;
        nivelRacao = doc["nivel"] | doc["status"] | payload;
    } else {
        nivelRacao = payload;
        // Tentar extrair ID da remota do texto
        if (payload.indexOf("Remota") >= 0) {
            int inicio = payload.indexOf("Remota") + 6;
            String numeroStr = payload.substring(inicio, inicio + 2);
            idRemota = numeroStr.toInt();
            if (idRemota == 0) idRemota = 1;
        }
    }
    
    DEBUG_MQTT_PRINTF("Alerta Ração Remota %d: %s\n", idRemota, nivelRacao.c_str());
    
    if (callbackAlertaRacao) {
        callbackAlertaRacao(idRemota, nivelRacao);
    }
}

// Extrair ID da remota do tópico
int GerenciadorMQTT::extrairIdRemotaDoTopico(String topico) {
    // Formato: alimentador/remota/X/status (onde X é o ID)
    int inicio = topico.indexOf("/remota/") + 8;
    int fim = topico.indexOf("/", inicio);
    
    if (inicio > 7 && fim > inicio) {
        String idStr = topico.substring(inicio, fim);
        return idStr.toInt();
    }
    
    return 1; // Default para Remota 1 se não conseguir extrair
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
    if (conectado && broker_iniciado) {
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
        case -4: return "MQTT_CONNECTION_TIMEOUT - Timeout na conexão";
        case -3: return "MQTT_CONNECTION_LOST - Conexão perdida";
        case -2: return "MQTT_CONNECT_FAILED - Falha na conexão";
        case -1: return "MQTT_DISCONNECTED - Desconectado";
        case 0:  return "MQTT_CONNECTED - Conectado";
        case 1:  return "MQTT_CONNECT_BAD_PROTOCOL - Protocolo inválido";
        case 2:  return "MQTT_CONNECT_BAD_CLIENT_ID - Client ID inválido";
        case 3:  return "MQTT_CONNECT_UNAVAILABLE - Servidor indisponível";
        case 4:  return "MQTT_CONNECT_BAD_CREDENTIALS - Credenciais inválidas";
        case 5:  return "MQTT_CONNECT_UNAUTHORIZED - Não autorizado";
        default: return "Erro desconhecido: " + String(codigo);
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

// Novos getters para broker
int GerenciadorMQTT::getClientesConectados() const {
    // Contar remotas que estão conectadas
    int count = 0;
    for (const auto& par : remotas_ativas) {
        if (par.second.conectada) {
            count++;
        }
    }
    return count;
}

bool GerenciadorMQTT::brokerEstaRodando() const {
    return broker_iniciado;
}

String GerenciadorMQTT::getIPBroker() const {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
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
            // Verificar se passou do timeout
            if (agora - info.ultimo_heartbeat > TIMEOUT_REMOTA) {
                info.conectada = false;
                DEBUG_MQTT_PRINTF("✗ Remota %d desconectada (timeout)\n", idRemota);
            }
        }
    }
}
