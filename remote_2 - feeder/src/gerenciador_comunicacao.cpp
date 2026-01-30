
#include "gerenciador_comunicacao.h"
#include <WiFi.h>
#include <Arduino.h>
#include "gerenciador_hcsr04.h"
#include "config.h"

// Permite acesso ao sensorRacao global definido em principal.cpp
extern GerenciadorHCSR04 sensorRacao;

// Inicializar ponteiro estático
GerenciadorComunicacao* GerenciadorComunicacao::instancia = nullptr;

GerenciadorComunicacao::GerenciadorComunicacao(MQTTManager* mqttPtr, GerenciadorAlimentacao* alimentacaoPtr, 
                                              GerenciadorSistema* sistemaPtr)
    : mqttManager(mqttPtr), alimentacao(alimentacaoPtr), sistema(sistemaPtr), ultimoHeartbeat(0) {
    // Definir instância estática para callback
    instancia = this;
}

void GerenciadorComunicacao::definirCallback() {
    mqttManager->definirCallback(callbackMQTT);
}

void GerenciadorComunicacao::processarMensagens() {
    mqttManager->loop();
}

void GerenciadorComunicacao::enviarHeartbeat(const char* topicHeartbeat) {
    unsigned long agora = millis();
    if (agora - ultimoHeartbeat >= INTERVALO_HEARTBEAT) {
        String payload = "{\"status\":\"ALIVE\"" +
                         String(",\"remota_id\":2") +
                         String(",\"uptime\":") + String(millis()) +
                         String(",\"wifi_rssi\":") + String(WiFi.RSSI()) +
                         String(",\"free_heap\":") + String(ESP.getFreeHeap()) +
                         String(",\"alimentacao_ativa\":") + String(alimentacao->estaAtivo() ? "true" : "false") +
                         String(",\"servo_travado\":") + String(sistema->getServoTravado() ? "true" : "false") + String("}");

        if (mqttManager->publicar(topicHeartbeat, payload)) {
            Serial.printf("💓 Heartbeat enviado: ALIVE (remota_id: 2)\n");
        }

        // --- Lógica de alerta de ração ---
        extern GerenciadorHCSR04 sensorRacao;
        float distancia = sensorRacao.medirDistancia();
        if (distancia >= sensorRacao.getDistanciaLimite()) {
            mqttManager->publicar(TOPIC_ALERTA_RACAO, "BAIXO");
        } else {
            mqttManager->publicar(TOPIC_ALERTA_RACAO, "OK");
        }
        // ---

        ultimoHeartbeat = agora;
    }
}

void GerenciadorComunicacao::enviarStatusMQTT(const char* topicStatus, const String& status) {
    String payload = "{\"status\":\"" + status + "\",\"timestamp\":" + String(millis()) + "}";
    if (mqttManager->publicar(topicStatus, payload)) {
        Serial.printf("📤 Status enviado: %s\n", status.c_str());
    }
}

void GerenciadorComunicacao::enviarConclusaoMQTT(const char* topicResposta, int tempoDecorrido, const String& comandoId) {
    String payload = "{\"concluido\":true,\"tempo_segundos\":" + String(tempoDecorrido) +
                     ",\"comando_id\":\"" + comandoId + "\"" +
                     ",\"timestamp\":" + String(millis()) + "}";
    if (mqttManager->publicar(topicResposta, payload)) {
        Serial.printf("📤 Conclusão enviada: %d segundos de alimentação [ID: %s]\n", tempoDecorrido, comandoId.c_str());
    }
}

void GerenciadorComunicacao::processarComandoCentral(const String& payload) {
    // Formato esperado: {"acao":"alimentar","tempo":5,"remota_id":1}
    // ou comandos simples: STOP, STATUS, PING

    if (payload == "PING") {
        // Implementar envio de status PONG
        Serial.println("📥 Comando PING recebido");
        return;
    }

    if (payload == "STATUS") {
        String statusAtual;
        if (alimentacao->estaAtivo()) {
            statusAtual = "ATIVO";
        } else if (sistema->getServoTravado()) {
            statusAtual = "INATIVO";
        } else {
            statusAtual = "DISPONIVEL";
        }

        String statusTravamento = sistema->getServoTravado() ? "_TRAVADO" : "";
        Serial.printf("📥 Status solicitado: %s%s\n", statusAtual.c_str(), statusTravamento.c_str());
        return;
    }

    if (payload == "STOP") {
        if (alimentacao->estaAtivo()) {
            alimentacao->parar();
            Serial.println("📥 Comando STOP - Alimentação parada");
        } else {
            Serial.println("📥 Comando STOP - Sistema já parado");
        }
        return;
    }

    // Comando alimentar formato JSON
    if (payload.indexOf("\"acao\":\"alimentar\"") > 0) {
        Serial.println("📥 Comando 'alimentar' recebido da central");
        
        // Extrair tempo (se fornecido)
        int startTempo = payload.indexOf("\"tempo\":") + 8;
        int endTempo = payload.indexOf(",", startTempo);
        if (endTempo == -1) endTempo = payload.indexOf("}", startTempo);
        
        int tempoSegundos = 5; // padrão
        if (startTempo > 8) {
            String tempoStr = payload.substring(startTempo, endTempo);
            tempoSegundos = tempoStr.toInt();
        }
        
        // Extrair remota_id se fornecido
        int startId = payload.indexOf("\"remota_id\":") + 12;
        int endId = payload.indexOf(",", startId);
        if (endId == -1) endId = payload.indexOf("}", startId);
        
        int remotaId = 1; // padrão
        if (startId > 12) {
            String idStr = payload.substring(startId, endId);
            remotaId = idStr.toInt();
        }
        
        Serial.printf("🎯 Comando alimentar: %d segundos para remota %d\n", tempoSegundos, remotaId);
        
        // Validar tempo (1 a 60 segundos)
        if (tempoSegundos < 1) tempoSegundos = 1;
        if (tempoSegundos > 60) tempoSegundos = 60;
        
        String idComando = "ALIMENTAR_" + String(millis());
        alimentacao->setIdComando(idComando);
        alimentacao->iniciar(tempoSegundos);
        return;
    }

    // Comando formato legado: a3, a5, etc.
    if (payload.startsWith("a") && payload.length() > 1) {
        String numeroStr = payload.substring(1);
        int tempoSegundos = numeroStr.toInt();

        if (tempoSegundos > 0 && tempoSegundos <= 60) {
            String idComando = "LEGACY_" + String(millis());
            alimentacao->setIdComando(idComando);
            Serial.printf("🎯 Comando legado: %d segundos [ID: %s]\n", tempoSegundos, idComando.c_str());
            alimentacao->iniciar(tempoSegundos);
        } else {
            Serial.println("❌ Erro: Parâmetro inválido para comando legado");
        }
        return;
    }

    // Comando não reconhecido
    Serial.println("❌ Comando não reconhecido do CENTRAL!");
}

// Callback estático para MQTT
void GerenciadorComunicacao::callbackMQTT(String topic, String payload) {
    if (instancia) {
        Serial.printf("📥 MQTT recebido [%s]: %s\n", topic.c_str(), payload.c_str());
        instancia->processarComandoCentral(payload);
    }
}