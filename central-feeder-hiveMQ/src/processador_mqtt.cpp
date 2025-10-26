#include "processador_mqtt.h"
#include "gerenciador_mqtt.h"

void ProcessadorMQTT::inicializar() {
    // Inicializar até MAX_REMOTAS (configurado em config.h)
    for (int i = 1; i <= MAX_REMOTAS; i++) {
        estadoSistema.adicionarRemota(i);
    }
    
    DEBUG_PRINTLN("Processador MQTT inicializado");
}

void ProcessadorMQTT::processarMensagem(String topico, String mensagem) {
    Serial.printf("[MQTT] Topico: %s, Mensagem: %s\n",
                 topico.c_str(), mensagem.c_str());

    // NOVO: Processar configurações do Dashboard
    if (topico == MQTT_TOPIC_CONFIG_SET) {
        processarConfiguracaoDashboard(mensagem);
        return;
    }

    if (topico == MQTT_TOPIC_CONFIG_QUERY) {
        processarSolicitacaoConfig(mensagem);
        return;
    }

    // IMPORTANTE: Processar heartbeat geral ANTES de extrair ID do tópico
    // porque "a/r/hb" não tem ID no caminho, apenas no payload JSON
    if (topico == MQTT_TOPIC_HEARTBEAT_GERAL) {
        // Heartbeat geral - payload otimizado
        // Formato novo: {"s":1,"i":1,"r":-45,"a":0,"t":0}
        // Formato antigo: {"remota_id":1,"wifi_rssi":-45,...}
        StaticJsonDocument<200> doc;
        DeserializationError erro = deserializeJson(doc, mensagem);
        if (!erro) {
            int remotaId = 0;

            // Tentar formato novo primeiro (campos curtos)
            if (doc.containsKey("i")) {
                remotaId = doc["i"] | 0;           // i = remota_id (default 0 para detectar erro)

                // VALIDAR ID ANTES DE PROCESSAR
                if (remotaId < 1 || remotaId > MAX_REMOTAS) {
                    Serial.printf("[ERRO] Heartbeat com ID inválido: %d (deve ser 1-%d)\n",
                                 remotaId, MAX_REMOTAS);
                    Serial.printf("[DEBUG] Payload: %s\n", mensagem.c_str());
                    return;
                }

                int status = doc["s"] | 0;         // s = status (1=ALIVE)
                int rssi = doc["r"] | 0;           // r = rssi
                int ativo = doc["a"] | 0;          // a = alimentacao_ativa
                int travado = doc["t"] | 0;        // t = servo_travado

                processarHeartbeat(status == 1 ? "ALIVE" : "DEAD", remotaId);
                Serial.printf("[HB] Remota %d: RSSI=%d, Ativo=%d, Travado=%d\n",
                             remotaId, rssi, ativo, travado);
            }
            // Fallback para formato antigo (compatibilidade)
            else if (doc.containsKey("remota_id")) {
                remotaId = doc["remota_id"] | 0;

                // VALIDAR ID
                if (remotaId < 1 || remotaId > MAX_REMOTAS) {
                    Serial.printf("[ERRO] Heartbeat (antigo) com ID inválido: %d\n", remotaId);
                    return;
                }

                int rssi = doc["wifi_rssi"] | 0;
                unsigned long uptime = doc["uptime"] | 0;
                processarHeartbeat("ALIVE", remotaId);
            }
        }
        return; // Importante: retornar após processar heartbeat geral
    }

    // Extrair ID da remota do tópico (para outros tipos de mensagem)
    int remotaId = extrairIdRemota(topico);

    // Processar por tipo (suporta tópicos novos e antigos)
    if (topico.indexOf("/st") > 0 || topico.indexOf("/status") > 0) {
        processarStatus(mensagem, remotaId);
    } else if (topico.indexOf("/hb") > 0 || topico.indexOf("/vida") > 0) {
        processarHeartbeat(mensagem, remotaId);
    } else if (topico.indexOf("/rsp") > 0 || topico.indexOf("/resposta") > 0 ||
               topico.indexOf("/done") > 0 || topico.indexOf("/concluido") > 0) {
        // Resposta de comando (payload otimizado)
        StaticJsonDocument<200> doc;
        DeserializationError erro = deserializeJson(doc, mensagem);
        if (!erro) {
            // Formato novo: {"c":1,"ts":5,"id":"ALIMENTAR_123456","t":123456}
            if (doc.containsKey("c")) {
                int concluido = doc["c"] | 0;
                int tempoSegundos = doc["ts"] | 0;
                String cmdId = doc["id"] | "";
                Serial.printf("[RESPOSTA] Remota %d: Concluido=%d, Tempo=%ds, ID=%s\n",
                             remotaId, concluido, tempoSegundos, cmdId.c_str());
            }
            // Formato antigo (compatibilidade)
            else if (doc.containsKey("concluido")) {
                Serial.printf("[RESPOSTA] Remota %d: %s\n", remotaId, mensagem.c_str());
            }
        } else {
            Serial.printf("[RESPOSTA] Remota %d: %s\n", remotaId, mensagem.c_str());
        }
    } else if (topico.indexOf("/alr") > 0 || topico.indexOf("/alerta") > 0 || topico == MQTT_TOPIC_ALERTA_RACAO) {
        processarAlerta(mensagem, remotaId);
    }
}

int ProcessadorMQTT::extrairIdRemota(String topico) {
    // Suporta formatos novo e antigo:
    // Novo: "a/r/1/cmd" -> retorna 1
    // Antigo: "alimentador/remota/1/status" -> retorna 1

    // Tentar formato novo primeiro (a/r/ID/...)
    int start = topico.indexOf("a/r/");
    if (start >= 0) {
        start += 4; // Pular "a/r/"
        int end = topico.indexOf("/", start);
        if (end == -1) end = topico.length();
        String idStr = topico.substring(start, end);
        int id = idStr.toInt();
        if (id > 0) return id;
    }

    // Fallback para formato antigo
    start = topico.indexOf("/remota") + 7;
    int end = topico.indexOf("/", start);
    if (end == -1) end = topico.length();

    String idStr = topico.substring(start, end);
    return idStr.toInt();
}

String ProcessadorMQTT::extrairTipoTopico(String topico) {
    int lastSlash = topico.lastIndexOf("/");
    if (lastSlash != -1) {
        return topico.substring(lastSlash + 1);
    }
    return "";
}

void ProcessadorMQTT::processarStatus(String mensagem, int remotaId) {
    bool online = (mensagem == "ONLINE" || mensagem == "CONECTADO" || mensagem == "ALIVE");
    atualizarEstadoRemota(remotaId, online, mensagem);
    Serial.printf("[STATUS] Remota %d: %s\n", remotaId, online ? "ONLINE" : "OFFLINE");
}

void ProcessadorMQTT::processarHeartbeat(String mensagem, int remotaId) {
    bool viva = (mensagem == "ALIVE" || mensagem == "1" || mensagem == "true");
    atualizarEstadoRemota(remotaId, viva, "HEARTBEAT");
    Serial.printf("[HEARTBEAT] Remota %d: %s\n", remotaId, viva ? "VIVA" : "MORTA");
}

void ProcessadorMQTT::processarAlerta(String mensagem, int remotaId) {
    // Payload otimizado: {"i":1,"n":1,"d":12.5}
    // Payload antigo: {"remota_id":1,"nivel":"BAIXO","distancia":12.5}

    StaticJsonDocument<100> doc;
    DeserializationError erro = deserializeJson(doc, mensagem);

    if (!erro) {
        // Formato novo (campos curtos)
        if (doc.containsKey("i")) {
            remotaId = doc["i"] | remotaId;    // i = remota_id
            int nivel = doc["n"] | 0;          // n = nivel (0=OK, 1=BAIXO)
            float distancia = doc["d"] | 0.0;  // d = distancia

            if (nivel == 1) {
                estadoSistema.atualizarRemota(remotaId, true, true, "BAIXO");
                Serial.printf("[ALERTA] Remota %d: Racao BAIXA! (%.1fcm)\n", remotaId, distancia);
            } else {
                estadoSistema.atualizarRemota(remotaId, true, true, "OK");
                Serial.printf("[ALERTA] Remota %d: Racao OK (%.1fcm)\n", remotaId, distancia);
            }
        }
        // Formato antigo (compatibilidade)
        else if (doc.containsKey("nivel")) {
            String nivel = doc["nivel"] | "OK";
            if (nivel == "BAIXO") {
                estadoSistema.atualizarRemota(remotaId, true, true, "BAIXO");
                Serial.printf("[ALERTA] Remota %d: Racao baixa!\n", remotaId);
            } else {
                estadoSistema.atualizarRemota(remotaId, true, true, "OK");
                Serial.printf("[ALERTA] Remota %d: Racao normalizada\n", remotaId);
            }
        }
    }
    // Mensagens simples (compatibilidade)
    else if (mensagem == "RACAO_BAIXA" || mensagem.indexOf("baixo") >= 0) {
        estadoSistema.atualizarRemota(remotaId, true, true, "BAIXO");
        Serial.printf("[ALERTA] Remota %d: Racao baixa!\n", remotaId);
    } else if (mensagem == "RACAO_OK") {
        estadoSistema.atualizarRemota(remotaId, true, true, "OK");
        Serial.printf("[ALERTA] Remota %d: Racao normalizada\n", remotaId);
    }
}

void ProcessadorMQTT::atualizarEstadoRemota(int id, bool online, String status) {
    estadoSistema.atualizarRemota(id, true, online, status);
}

// NOVO: Processar configuração vinda do Dashboard
void ProcessadorMQTT::processarConfiguracaoDashboard(String mensagem) {
    // Payload: {"r":1,"i":0,"h":9,"m":15,"q":300}
    StaticJsonDocument<128> doc;
    DeserializationError erro = deserializeJson(doc, mensagem);

    if (erro) {
        Serial.printf("[CONFIG] Erro ao parsear JSON: %s\n", erro.c_str());
        return;
    }

    int remotaId = doc["r"] | 0;
    int indiceRefeicao = doc["i"] | 0;
    int hora = doc["h"] | 0;
    int minuto = doc["m"] | 0;
    int quantidade = doc["q"] | 0;

    Serial.printf("[CONFIG] Dashboard → Remota %d, Refeicao %d: %02d:%02d (%dg)\n",
                 remotaId, indiceRefeicao, hora, minuto, quantidade);

    // Validar
    if (remotaId < 1 || remotaId > MAX_REMOTAS) {
        Serial.println("[CONFIG] ID de remota inválido!");
        return;
    }

    if (indiceRefeicao < 0 || indiceRefeicao > 2) {
        Serial.println("[CONFIG] Índice de refeição inválido!");
        return;
    }

    if (hora < 0 || hora > 23 || minuto < 0 || minuto > 59) {
        Serial.println("[CONFIG] Horário inválido!");
        return;
    }

    if (quantidade < 0 || quantidade > 990) {
        Serial.println("[CONFIG] Quantidade inválida!");
        return;
    }

    // Atualizar estadoSistema
    estadoSistema.definirRefeicao(remotaId, indiceRefeicao, hora, minuto, quantidade);

    // Enviar para a remota via MQTT
    GerenciadorMQTT* mqtt = GerenciadorMQTT::obterInstancia();
    if (mqtt) {
        mqtt->configurarHorarioRefeicao(remotaId, indiceRefeicao, hora, minuto, quantidade);

        // Notificar outros dashboards da mudança (origem = "d")
        mqtt->notificarMudancaConfig(remotaId, indiceRefeicao, hora, minuto, quantidade, "d");

        // Publicar estado completo com retain
        mqtt->publicarEstadoCompleto(remotaId);
    }

    Serial.println("[CONFIG] Configuração aplicada com sucesso!");
}

// NOVO: Processar solicitação de configuração do Dashboard
void ProcessadorMQTT::processarSolicitacaoConfig(String mensagem) {
    // Payload: {"r":1}
    StaticJsonDocument<64> doc;
    DeserializationError erro = deserializeJson(doc, mensagem);

    if (erro) {
        Serial.printf("[CONFIG] Erro ao parsear solicitação: %s\n", erro.c_str());
        return;
    }

    int remotaId = doc["r"] | 0;

    if (remotaId < 1 || remotaId > MAX_REMOTAS) {
        Serial.println("[CONFIG] ID de remota inválido na solicitação!");
        return;
    }

    Serial.printf("[CONFIG] Solicitação de config da Remota %d recebida\n", remotaId);

    // Publicar estado completo (que já tem retain, mas força envio)
    GerenciadorMQTT* mqtt = GerenciadorMQTT::obterInstancia();
    if (mqtt) {
        mqtt->publicarEstadoCompleto(remotaId);
    }
}