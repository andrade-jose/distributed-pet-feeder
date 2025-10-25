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
    
    // Extrair ID da remota
    int remotaId = extrairIdRemota(topico);
    
    // Processar por tipo
    if (topico.indexOf("/status") > 0) {
        processarStatus(mensagem, remotaId);
    } else if (topico.indexOf("/vida") > 0) {
        processarHeartbeat(mensagem, remotaId);
    } else if (topico.indexOf("/resposta") > 0 || topico.indexOf("/concluido") > 0) {
        // Resposta de comando
        Serial.printf("[RESPOSTA] Remota %d: %s\n", remotaId, mensagem.c_str());
    } else if (topico.indexOf("/alerta") > 0 || topico == MQTT_TOPIC_ALERTA_RACAO) {
        processarAlerta(mensagem, remotaId);
    } else if (topico == MQTT_TOPIC_HEARTBEAT_GERAL) {
        // Heartbeat geral - extrair ID do payload
        StaticJsonDocument<200> doc;
        DeserializationError erro = deserializeJson(doc, mensagem);
        if (!erro) {
            remotaId = doc["remota_id"] | 1;
            int rssi = doc["wifi_rssi"] | 0;
            unsigned long uptime = doc["uptime"] | 0;
            processarHeartbeat("ALIVE", remotaId);
        }
    }
}

int ProcessadorMQTT::extrairIdRemota(String topico) {
    // "alimentador/remota1/status" -> retorna 1
    int start = topico.indexOf("/remota") + 7;
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
    if (mensagem == "RACAO_BAIXA" || mensagem.indexOf("baixo") >= 0) {
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