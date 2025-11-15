#include "processador_mqtt.h"
#include "gerenciador_mqtt.h"

void ProcessadorMQTT::inicializar() {
    // Inicializar até MAX_REMOTAS (configurado em config.h)
    for (int i = 1; i <= MAX_REMOTAS; i++) {
        estadoSistema.adicionarRemota(i);
    }
    
    DEBUG_PRINTLN("Processador MQTT inicializado");
}

// ✅ ATUALIZADO: Processar mensagens SparkplugB
void ProcessadorMQTT::processarMensagem(String topico, String mensagem) {
    Serial.printf("[MQTT] Tópico: %s\n", topico.c_str());

    // Detectar tipo de mensagem SparkplugB pelo tópico
    if (topico.indexOf("/DBIRTH/") >= 0) {
        processarDBIRTH(topico, mensagem);
    }
    else if (topico.indexOf("/DDATA/") >= 0) {
        processarDDATA(topico, mensagem);
    }
    else if (topico.indexOf("/DDEATH/") >= 0) {
        processarDDEATH(topico, mensagem);
    }
    else if (topico.indexOf("/NCMD/") >= 0) {
        processarNCMD(topico, mensagem);
    }
    else {
        Serial.printf("[MQTT] Tópico não reconhecido: %s\n", topico.c_str());
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

// ===== PROCESSADORES SPARKPLUGB =====

int ProcessadorMQTT::extrairIdRemotaSparkplug(String topico) {
    // Formato: spBv1.0/ALIMENTADOR_PETS/DDATA/EON_CENTRAL/REMOTA_1
    // Extrair "REMOTA_1" e pegar o número

    int lastSlash = topico.lastIndexOf('/');
    if (lastSlash > 0) {
        String deviceId = topico.substring(lastSlash + 1);  // "REMOTA_1"

        // Remover prefixo "REMOTA_"
        if (deviceId.startsWith("REMOTA_")) {
            String idStr = deviceId.substring(7);  // "1"
            return idStr.toInt();
        }
    }

    return 1;  // Fallback
}

void ProcessadorMQTT::processarDBIRTH(String topico, String payload) {
    int remotaId = extrairIdRemotaSparkplug(topico);

    Serial.printf("[DBIRTH] Remota %d nasceu\n", remotaId);

    // Parsear payload SparkplugB
    StaticJsonDocument<512> doc;
    DeserializationError erro = deserializeJson(doc, payload);

    if (!erro && doc.containsKey("metrics")) {
        JsonArray metrics = doc["metrics"];

        // Registrar remota como online
        estadoSistema.atualizarRemota(remotaId, true, true, "ONLINE");

        // Processar métricas do DBIRTH (configurações da remota)
        for (JsonObject metric : metrics) {
            String name = metric["name"] | "";
            if (name == "Properties/Version") {
                String version = metric["value"] | "unknown";
                Serial.printf("[DBIRTH] Remota %d versão: %s\n", remotaId, version.c_str());
            }
        }
    }
}

void ProcessadorMQTT::processarDDATA(String topico, String payload) {
    int remotaId = extrairIdRemotaSparkplug(topico);

    Serial.printf("[DDATA] Remota %d - telemetria recebida\n", remotaId);

    StaticJsonDocument<512> doc;
    DeserializationError erro = deserializeJson(doc, payload);

    if (!erro && doc.containsKey("metrics")) {
        JsonArray metrics = doc["metrics"];

        // Atualizar estado como online
        estadoSistema.atualizarRemota(remotaId, true, true, "OK");

        // Processar métricas
        for (JsonObject metric : metrics) {
            String name = metric["name"] | "";

            if (name == "wifi_rssi") {
                int rssi = metric["value"] | 0;
                Serial.printf("[DDATA] Remota %d RSSI: %d\n", remotaId, rssi);
            }
            else if (name == "racao_nivel") {
                float nivel = metric["value"] | 0.0;
                Serial.printf("[DDATA] Remota %d nível ração: %.1fcm\n", remotaId, nivel);

                // Atualizar estado de alerta
                if (nivel < 5.0) {
                    estadoSistema.atualizarRemota(remotaId, true, true, "BAIXO");
                } else {
                    estadoSistema.atualizarRemota(remotaId, true, true, "OK");
                }
            }
            else if (name == "alimentacao_ativa") {
                bool ativa = metric["value"] | false;
                Serial.printf("[DDATA] Remota %d alimentação ativa: %d\n", remotaId, ativa);
            }
        }
    }
}

void ProcessadorMQTT::processarDDEATH(String topico, String payload) {
    int remotaId = extrairIdRemotaSparkplug(topico);

    Serial.printf("[DDEATH] ✗ Remota %d desconectou (Last Will)\n", remotaId);

    // Marcar como offline
    estadoSistema.atualizarRemota(remotaId, false, false, "OFFLINE");
}

void ProcessadorMQTT::processarNCMD(String topico, String payload) {
    Serial.printf("[NCMD] Comando recebido do Dashboard\n");

    StaticJsonDocument<256> doc;
    DeserializationError erro = deserializeJson(doc, payload);

    if (!erro && doc.containsKey("metrics")) {
        JsonArray metrics = doc["metrics"];

        int remotaId = 0;
        String comando = "";
        int valor = 0;

        // Extrair métricas do comando
        for (JsonObject metric : metrics) {
            String name = metric["name"] | "";

            if (name == "remota_id") {
                remotaId = metric["value"] | 0;
            }
            else if (name == "command") {
                comando = metric["value"] | "";
            }
            else if (name == "value") {
                valor = metric["value"] | 0;
            }
        }

        Serial.printf("[NCMD] Remota: %d, Comando: %s, Valor: %d\n",
                     remotaId, comando.c_str(), valor);

        // Repassar comando para a remota via DCMD
        if (remotaId > 0 && comando.length() > 0) {
            GerenciadorMQTT* mqtt = GerenciadorMQTT::obterInstancia();
            if (mqtt) {
                mqtt->publicarDCMD(remotaId, comando, valor);
            }
        }
    }
}