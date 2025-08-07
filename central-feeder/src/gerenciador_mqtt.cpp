#include "gerenciador_mqtt.h"

// Inicialização de variáveis estáticas
WiFiClientSecure GerenciadorMQTT::wifiClient;
PubSubClient GerenciadorMQTT::mqttClient(wifiClient);
bool GerenciadorMQTT::conectado = false;
unsigned long GerenciadorMQTT::ultimoReconectar = 0;
unsigned long GerenciadorMQTT::ultimoHeartbeat = 0;
int GerenciadorMQTT::tentativasReconexao = 0;

// Callbacks
void (*GerenciadorMQTT::callbackStatusRemota)(int, String) = nullptr;
void (*GerenciadorMQTT::callbackVidaRemota)(int, bool) = nullptr;
void (*GerenciadorMQTT::callbackRespostaRemota)(int, String) = nullptr;

bool GerenciadorMQTT::inicializar() {
    DEBUG_MQTT_PRINTLN("=== Inicializando Gerenciador MQTT ===");
    
    // Configurar cliente WiFi SSL
    wifiClient.setInsecure(); // Para HiveMQ Cloud (aceita certificado SSL)
    DEBUG_MQTT_PRINTLN("✅ Cliente SSL configurado (insecure mode)");
    
    // Configurar cliente MQTT
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(onMensagemRecebida);
    mqttClient.setKeepAlive(MQTT_KEEP_ALIVE);
    
    DEBUG_MQTT_PRINTF("📡 Server: %s:%d\n", MQTT_SERVER, MQTT_PORT);
    DEBUG_MQTT_PRINTF("🆔 Client ID: %s\n", MQTT_CLIENT_ID);
    DEBUG_MQTT_PRINTF("👤 Username: %s\n", MQTT_USERNAME);
    DEBUG_MQTT_PRINTF("🔐 Password: %s\n", MQTT_PASSWORD);
    DEBUG_MQTT_PRINTLN("🔒 SSL: Habilitado (WebSocket Secure)");
    
    return true;
}

void GerenciadorMQTT::atualizar() {
    unsigned long agora = millis();
    
    // Manter conexão MQTT ativa
    if (conectado) {
        mqttClient.loop();
        
        // Verificar se perdeu conexão
        if (!mqttClient.connected()) {
            conectado = false;
            DEBUG_MQTT_PRINTLN("⚠️ Conexão MQTT perdida!");
        }
    }
    
    // Tentar reconectar se necessário
    if (!conectado && (agora - ultimoReconectar > MQTT_RECONNECT_INTERVAL)) {
        ultimoReconectar = agora;
        DEBUG_MQTT_PRINTF("🔄 Tentativa de reconexão %d/%d\n", tentativasReconexao + 1, MQTT_MAX_RETRIES);
        
        if (reconectar()) {
            conectado = true;
            tentativasReconexao = 0;
            DEBUG_MQTT_PRINTLN("✅ Reconexão bem-sucedida!");
        } else {
            tentativasReconexao++;
            DEBUG_MQTT_PRINTF("❌ Tentativa %d falhou\n", tentativasReconexao);
            
            if (tentativasReconexao >= MQTT_MAX_RETRIES) {
                DEBUG_MQTT_PRINTLN("🚫 Máximo de tentativas MQTT atingido - aguardando próximo ciclo");
                tentativasReconexao = 0; // Reset para tentar novamente depois
            }
        }
    }
    
    // Atualizar heartbeat
    ultimoHeartbeat = agora;
}

bool GerenciadorMQTT::conectar() {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_MQTT_PRINTLN("❌ WiFi não conectado - impossível conectar MQTT");
        return false;
    }
    
    DEBUG_MQTT_PRINTLN("📶 WiFi conectado - iniciando conexão MQTT");
    return reconectar();
}

bool GerenciadorMQTT::reconectar() {
    DEBUG_MQTT_PRINT("🔄 Conectando ao MQTT...");
    
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
        DEBUG_MQTT_PRINTLN(" ✅ Conectado!");
        
        // Debug detalhado dos tópicos configurados
        DEBUG_MQTT_PRINTLN("🔍 === VERIFICAÇÃO DE TÓPICOS ===");
        DEBUG_MQTT_PRINTLN("📋 Tópicos que a CENTRAL vai usar:");
        DEBUG_MQTT_PRINTF("   📤 Comando para remotas: %s (com %%d para ID)\n", MQTT_TOPIC_COMANDO_REMOTA);
        DEBUG_MQTT_PRINTF("   📤 Horário para remotas: %s (com %%d para ID)\n", MQTT_TOPIC_HORARIO_REMOTA);
        DEBUG_MQTT_PRINTF("   📤 Tempo movimento: %s (com %%d para ID)\n", MQTT_TOPIC_TEMPO_MOVIMENTO);
        DEBUG_MQTT_PRINTF("   📤 Status central: %s\n", MQTT_TOPIC_CENTRAL_STATUS);
        DEBUG_MQTT_PRINTLN();
        DEBUG_MQTT_PRINTLN("📋 Tópicos que a CENTRAL vai ESCUTAR:");
        DEBUG_MQTT_PRINTF("   📥 Status das remotas: %s (com %%d para ID)\n", MQTT_TOPIC_STATUS_REMOTA);
        DEBUG_MQTT_PRINTF("   📥 Vida específica: %s (com %%d para ID)\n", MQTT_TOPIC_VIDA_REMOTA);
        DEBUG_MQTT_PRINTF("   📥 Vida geral: %s\n", MQTT_TOPIC_HEARTBEAT_GERAL);
        DEBUG_MQTT_PRINTF("   📥 Respostas: %s (com %%d para ID)\n", MQTT_TOPIC_RESPOSTA_REMOTA);
        DEBUG_MQTT_PRINTF("   📥 Comandos para central: %s\n", MQTT_TOPIC_CENTRAL_COMANDO);
        DEBUG_MQTT_PRINTLN();
        DEBUG_MQTT_PRINTLN("⚠️  COMPATIBILIDADE COM REMOTA:");
        DEBUG_MQTT_PRINTLN("   Remota envia: alimentador/remota/comando");
        DEBUG_MQTT_PRINTLN("   Central espera: alimentador/remota/%d/comando");
        DEBUG_MQTT_PRINTLN("   Remota envia: alimentador/remota/status");
        DEBUG_MQTT_PRINTLN("   Central espera: alimentador/remota/%d/status");
        DEBUG_MQTT_PRINTLN("   Remota envia: alimentador/remota/heartbeat ✅");
        DEBUG_MQTT_PRINTLN("   Central espera: alimentador/remota/heartbeat ✅");
        DEBUG_MQTT_PRINTLN("   Remota envia: alimentador/remota/concluido");
        DEBUG_MQTT_PRINTLN("   Central espera: alimentador/remota/%d/resposta");
        DEBUG_MQTT_PRINTLN("🔍 ===========================");
        
        // Inscrever em todos os tópicos necessários
        DEBUG_MQTT_PRINTLN("📥 Inscrevendo em tópicos...");
        inscreverStatusRemotas();
        inscreverVidaRemotas();
        inscreverRespostasRemotas();
        
        // Publicar status da central
        DEBUG_MQTT_PRINTLN("📤 Publicando status da central...");
        publicarStatusCentral("ONLINE");
        
        conectado = true;
        return true;
    } else {
        int estado = mqttClient.state();
        DEBUG_MQTT_PRINTF(" ❌ Falhou (código %d - %s)\n", estado, obterDescricaoErroMQTT(estado).c_str());
        conectado = false;
        return false;
    }
}

void GerenciadorMQTT::desconectar() {
    if (conectado) {
        DEBUG_MQTT_PRINTLN("📴 Desconectando MQTT...");
        publicarStatusCentral("OFFLINE");
        mqttClient.disconnect();
        conectado = false;
        DEBUG_MQTT_PRINTLN("✅ MQTT desconectado");
    }
}

bool GerenciadorMQTT::estaConectado() {
    return conectado && mqttClient.connected();
}

bool GerenciadorMQTT::enviarComandoRemota(int idRemota, String acao, int tempo) {
    if (!estaConectado()) {
        DEBUG_MQTT_PRINTLN("❌ MQTT não conectado - comando não enviado");
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
    
    DEBUG_MQTT_PRINTF("📤 Enviando comando para Remota %d...\n", idRemota);
    DEBUG_MQTT_PRINTF("   Tópico: %s\n", topico.c_str());
    DEBUG_MQTT_PRINTF("   Payload: %s\n", payload.c_str());
    
    bool resultado = mqttClient.publish(topico.c_str(), payload.c_str());
    
    if (resultado) {
        DEBUG_MQTT_PRINTF("✅ Comando enviado com sucesso para Remota %d\n", idRemota);
    } else {
        DEBUG_MQTT_PRINTF("❌ Falha ao enviar comando para Remota %d\n", idRemota);
    }
    
    return resultado;
}

bool GerenciadorMQTT::enviarComandoGeral(String acao, int tempo, int idRemota) {
    if (!estaConectado()) {
        DEBUG_MQTT_PRINTLN("❌ MQTT não conectado - comando geral não enviado");
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
    
    DEBUG_MQTT_PRINTF("📤 Enviando comando GERAL para Remota %d...\n", idRemota);
    DEBUG_MQTT_PRINTF("   Tópico: %s\n", MQTT_TOPIC_COMANDO_GERAL);
    DEBUG_MQTT_PRINTF("   Payload: %s\n", payload.c_str());
    
    bool resultado = mqttClient.publish(MQTT_TOPIC_COMANDO_GERAL, payload.c_str());
    
    if (resultado) {
        DEBUG_MQTT_PRINTF("✅ Comando geral enviado com sucesso para Remota %d\n", idRemota);
    } else {
        DEBUG_MQTT_PRINTF("❌ Falha ao enviar comando geral para Remota %d\n", idRemota);
    }
    
    return resultado;
}

bool GerenciadorMQTT::configurarHorarioRemota(int idRemota, int hora, int minuto, int quantidade) {
    if (!estaConectado()) {
        DEBUG_MQTT_PRINTLN("❌ MQTT não conectado - configuração de horário não enviada");
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
    
    DEBUG_MQTT_PRINTF("⏰ Configurando horário para Remota %d...\n", idRemota);
    DEBUG_MQTT_PRINTF("   Tópico: %s\n", topico.c_str());
    DEBUG_MQTT_PRINTF("   Payload: %s\n", payload.c_str());
    
    bool resultado = mqttClient.publish(topico.c_str(), payload.c_str());
    
    if (resultado) {
        DEBUG_MQTT_PRINTF("✅ Horário configurado: Remota %d às %02d:%02d (%dg)\n", 
                          idRemota, hora, minuto, quantidade);
    } else {
        DEBUG_MQTT_PRINTF("❌ Falha ao configurar horário para Remota %d\n", idRemota);
    }
    
    return resultado;
}

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
    
    bool resultado = mqttClient.publish(topico.c_str(), payload.c_str());
    
    if (resultado) {
        Serial.printf("📤 Tempo configurado para Remota %d: %d segundos\n", idRemota, tempo);
    }
    
    return resultado;
}

bool GerenciadorMQTT::solicitarStatusRemota(int idRemota) {
    return enviarComandoRemota(idRemota, MQTT_CMD_STATUS);
}

bool GerenciadorMQTT::enviarPingRemota(int idRemota) {
    return enviarComandoRemota(idRemota, MQTT_CMD_HEARTBEAT);
}

bool GerenciadorMQTT::inscreverStatusRemotas() {
    // Inscrever no tópico específico por remota (padrão atual)
    String topico1 = "alimentador/remota/+/status";
    bool resultado1 = mqttClient.subscribe(topico1.c_str());
    
    if (resultado1) {
        DEBUG_MQTT_PRINTF("📥 Inscrito em: %s\n", topico1.c_str());
    } else {
        DEBUG_MQTT_PRINTF("❌ Falha ao inscrever em: %s\n", topico1.c_str());
    }
    
    // Inscrever no tópico geral de status (para compatibilidade com remota)
    String topico2 = "alimentador/remota/status";
    bool resultado2 = mqttClient.subscribe(topico2.c_str());
    
    if (resultado2) {
        DEBUG_MQTT_PRINTF("📥 Inscrito em: %s (compatibilidade)\n", topico2.c_str());
    } else {
        DEBUG_MQTT_PRINTF("❌ Falha ao inscrever em: %s\n", topico2.c_str());
    }
    
    return resultado1 && resultado2;
}

bool GerenciadorMQTT::inscreverVidaRemotas() {
    // Inscrever no tópico específico por remota
    String topico1 = "alimentador/remota/+/vida";
    bool resultado1 = mqttClient.subscribe(topico1.c_str());
    
    if (resultado1) {
        DEBUG_MQTT_PRINTF("📥 Inscrito em: %s\n", topico1.c_str());
    } else {
        DEBUG_MQTT_PRINTF("❌ Falha ao inscrever em: %s\n", topico1.c_str());
    }
    
    // Inscrever no tópico geral de heartbeat
    String topico2 = MQTT_TOPIC_HEARTBEAT_GERAL;
    bool resultado2 = mqttClient.subscribe(topico2.c_str());
    
    if (resultado2) {
        DEBUG_MQTT_PRINTF("📥 Inscrito em: %s\n", topico2.c_str());
    } else {
        DEBUG_MQTT_PRINTF("❌ Falha ao inscrever em: %s\n", topico2.c_str());
    }
    
    return resultado1 && resultado2;
}

bool GerenciadorMQTT::inscreverRespostasRemotas() {
    // Inscrever no tópico específico por remota (padrão atual)
    String topico1 = "alimentador/remota/+/resposta";
    bool resultado1 = mqttClient.subscribe(topico1.c_str());
    
    if (resultado1) {
        DEBUG_MQTT_PRINTF("📥 Inscrito em: %s\n", topico1.c_str());
    } else {
        DEBUG_MQTT_PRINTF("❌ Falha ao inscrever em: %s\n", topico1.c_str());
    }
    
    // Inscrever no tópico geral de concluído (para compatibilidade com remota)
    String topico2 = "alimentador/remota/concluido";
    bool resultado2 = mqttClient.subscribe(topico2.c_str());
    
    if (resultado2) {
        DEBUG_MQTT_PRINTF("📥 Inscrito em: %s (compatibilidade)\n", topico2.c_str());
    } else {
        DEBUG_MQTT_PRINTF("❌ Falha ao inscrever em: %s\n", topico2.c_str());
    }
    
    return resultado1 && resultado2;
}

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
    
    return mqttClient.publish(MQTT_TOPIC_CENTRAL_STATUS, payload.c_str());
}

void GerenciadorMQTT::onMensagemRecebida(char* topico, byte* payload, unsigned int comprimento) {
    // Converter payload para string
    String mensagem;
    for (unsigned int i = 0; i < comprimento; i++) {
        mensagem += (char)payload[i];
    }
    
    String topicoStr = String(topico);
    int idRemota = extrairIdRemotaDoTopico(topicoStr);
    
    DEBUG_MQTT_PRINTLN();
    DEBUG_MQTT_PRINTLN("📥 === MENSAGEM MQTT RECEBIDA ===");
    DEBUG_MQTT_PRINTF("   Tópico: %s\n", topico);
    DEBUG_MQTT_PRINTF("   Payload: %s\n", mensagem.c_str());
    DEBUG_MQTT_PRINTF("   Tamanho: %d bytes\n", comprimento);
    DEBUG_MQTT_PRINTF("   ID da Remota extraído: %d\n", idRemota);
    
    // Processar baseado no tipo de tópico
    if (topicoStr.indexOf("/status") > 0) {
        DEBUG_MQTT_PRINTF("   Processando como STATUS da Remota %d\n", idRemota);
        processarMensagemStatus(idRemota, mensagem);
    } else if (topicoStr.indexOf("/vida") > 0) {
        DEBUG_MQTT_PRINTF("   Processando como HEARTBEAT da Remota %d\n", idRemota);
        processarMensagemVida(idRemota, mensagem);
    } else if (topicoStr.indexOf("/resposta") > 0) {
        DEBUG_MQTT_PRINTF("   Processando como RESPOSTA da Remota %d\n", idRemota);
        processarMensagemResposta(idRemota, mensagem);
    } else if (topicoStr == MQTT_TOPIC_HEARTBEAT_GERAL) {
        DEBUG_MQTT_PRINTLN("   Processando HEARTBEAT GERAL");
        processarHeartbeatGeral(mensagem);
    } else {
        DEBUG_MQTT_PRINTF("   ⚠️ Tipo de tópico não reconhecido: %s\n", topicoStr.c_str());
    }
    
    DEBUG_MQTT_PRINTLN("📥 ==============================");
    DEBUG_MQTT_PRINTLN();
}

void GerenciadorMQTT::processarMensagemStatus(int idRemota, String payload) {
    DEBUG_MQTT_PRINTF("🔍 Processando status da Remota %d: %s\n", idRemota, payload.c_str());
    
    StaticJsonDocument<300> doc;
    DeserializationError erro = deserializeJson(doc, payload);
    
    if (erro) {
        DEBUG_MQTT_PRINTF("❌ Erro ao processar JSON de status: %s\n", erro.c_str());
        return;
    }
    
    String status = doc["status"] | "UNKNOWN";
    DEBUG_MQTT_PRINTF("📊 Status extraído: %s\n", status.c_str());
    
    if (callbackStatusRemota) {
        DEBUG_MQTT_PRINTF("📞 Chamando callback de status para Remota %d\n", idRemota);
        callbackStatusRemota(idRemota, status);
    } else {
        DEBUG_MQTT_PRINTLN("⚠️ Callback de status não definido");
    }
}

void GerenciadorMQTT::processarMensagemVida(int idRemota, String payload) {
    DEBUG_MQTT_PRINTF("💓 Processando heartbeat da Remota %d: %s\n", idRemota, payload.c_str());
    
    StaticJsonDocument<200> doc;
    DeserializationError erro = deserializeJson(doc, payload);
    
    if (erro) {
        DEBUG_MQTT_PRINTF("❌ Erro ao processar JSON de vida: %s\n", erro.c_str());
        return;
    }
    
    String status = doc["status"] | "UNKNOWN";
    bool viva = (status == "ALIVE");
    DEBUG_MQTT_PRINTF("💗 Remota %d está %s\n", idRemota, viva ? "VIVA" : "PERDIDA");
    
    if (callbackVidaRemota) {
        DEBUG_MQTT_PRINTF("📞 Chamando callback de vida para Remota %d\n", idRemota);
        callbackVidaRemota(idRemota, viva);
    } else {
        DEBUG_MQTT_PRINTLN("⚠️ Callback de vida não definido");
    }
}

void GerenciadorMQTT::processarMensagemResposta(int idRemota, String payload) {
    DEBUG_MQTT_PRINTF("💬 Processando resposta da Remota %d: %s\n", idRemota, payload.c_str());
    
    StaticJsonDocument<300> doc;
    DeserializationError erro = deserializeJson(doc, payload);
    
    if (erro) {
        DEBUG_MQTT_PRINTF("⚠️ Payload não é JSON válido, usando como string: %s\n", payload.c_str());
        if (callbackRespostaRemota) {
            callbackRespostaRemota(idRemota, payload);
        }
        return;
    }
    
    String resultado = doc["resultado"] | payload;
    DEBUG_MQTT_PRINTF("📋 Resultado extraído: %s\n", resultado.c_str());
    
    if (callbackRespostaRemota) {
        DEBUG_MQTT_PRINTF("📞 Chamando callback de resposta para Remota %d\n", idRemota);
        callbackRespostaRemota(idRemota, resultado);
    } else {
        DEBUG_MQTT_PRINTLN("⚠️ Callback de resposta não definido");
    }
}

void GerenciadorMQTT::processarHeartbeatGeral(String payload) {
    DEBUG_MQTT_PRINTF("💓 Processando heartbeat geral: %s\n", payload.c_str());
    
    StaticJsonDocument<300> doc;
    DeserializationError erro = deserializeJson(doc, payload);
    
    if (erro) {
        DEBUG_MQTT_PRINTF("⚠️ Heartbeat não é JSON válido: %s\n", payload.c_str());
        return;
    }
    
    // Extrair informações do heartbeat
    String status = doc["status"] | "UNKNOWN";
    
    // Assumir que é da Remota 1 se não especificado
    // Você pode melhorar isso adicionando um campo "remota_id" no JSON
    int idRemota = doc["remota_id"] | 1; // Default para Remota 1
    
    bool viva = (status == "DISPONIVEL" || status == "ONLINE" || status == "ALIVE");
    
    DEBUG_MQTT_PRINTF("💓 Heartbeat: Remota %d está %s\n", idRemota, viva ? "VIVA" : "MORTA");
    
    if (callbackVidaRemota) {
        DEBUG_MQTT_PRINTF("📞 Chamando callback de vida para Remota %d\n", idRemota);
        callbackVidaRemota(idRemota, viva);
    } else {
        DEBUG_MQTT_PRINTLN("⚠️ Callback de vida não definido");
    }
}

int GerenciadorMQTT::extrairIdRemotaDoTopico(String topico) {
    // Formato: alimentador/remota/X/status (onde X é o ID)
    int inicio = topico.indexOf("/remota/") + 8;
    int fim = topico.indexOf("/", inicio);
    
    if (inicio > 7 && fim > inicio) {
        String idStr = topico.substring(inicio, fim);
        return idStr.toInt();
    }
    
    return 0; // ID inválido
}

String GerenciadorMQTT::construirTopico(const char* template_topico, int idRemota) {
    char buffer[100];
    snprintf(buffer, sizeof(buffer), template_topico, idRemota);
    return String(buffer);
}

void GerenciadorMQTT::definirCallbackStatusRemota(void (*callback)(int, String)) {
    callbackStatusRemota = callback;
}

void GerenciadorMQTT::definirCallbackVidaRemota(void (*callback)(int, bool)) {
    callbackVidaRemota = callback;
}

void GerenciadorMQTT::definirCallbackRespostaRemota(void (*callback)(int, String)) {
    callbackRespostaRemota = callback;
}

String GerenciadorMQTT::obterStatusConexao() {
    if (conectado && mqttClient.connected()) {
        return "Conectado";
    } else if (tentativasReconexao > 0) {
        return "Reconectando...";
    } else {
        return "Desconectado";
    }
}

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

unsigned long GerenciadorMQTT::obterUltimoHeartbeat() {
    return ultimoHeartbeat;
}

void GerenciadorMQTT::resetarTentativasReconexao() {
    tentativasReconexao = 0;
}
