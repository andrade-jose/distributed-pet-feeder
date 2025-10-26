
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
        // Payload otimizado: reduzido de ~200 para ~60 bytes
        // Campos: s=status, i=id, r=rssi, a=ativo, t=travado
        String payload = "{\"s\":1" +  // 1=ALIVE, 0=DEAD
                         String(",\"i\":") + String(REMOTA_ID) +
                         String(",\"r\":") + String(WiFi.RSSI()) +
                         String(",\"a\":") + String(alimentacao->estaAtivo() ? 1 : 0) +
                         String(",\"t\":") + String(sistema->getServoTravado() ? 1 : 0) + String("}");

        if (mqttManager->publicar(topicHeartbeat, payload)) {
            Serial.printf("💓 Heartbeat enviado: ALIVE (remota_id: %d)\n", REMOTA_ID);
        }

        // --- Lógica de alerta de ração (Payload otimizado) ---
        extern GerenciadorHCSR04 sensorRacao;
        float distancia = sensorRacao.medirDistancia();
        // Campos: i=id, n=nivel (0=OK, 1=BAIXO), d=distancia
        String alertaPayload = "{\"i\":" + String(REMOTA_ID) +
                              ",\"n\":" + String(distancia >= sensorRacao.getDistanciaLimite() ? 1 : 0) +
                              ",\"d\":" + String(distancia, 1) + "}";  // 1 casa decimal
        mqttManager->publicar(TOPIC_ALERTA_RACAO, alertaPayload);
        // ---

        ultimoHeartbeat = agora;
    }
}

void GerenciadorComunicacao::enviarStatusMQTT(const char* topicStatus, const String& status) {
    // Payload otimizado: s=status, t=timestamp
    String payload = "{\"s\":\"" + status + "\",\"t\":" + String(millis()) + "}";
    if (mqttManager->publicar(topicStatus, payload)) {
        Serial.printf("📤 Status enviado: %s\n", status.c_str());
    }
}

void GerenciadorComunicacao::enviarConclusaoMQTT(const char* topicResposta, int tempoDecorrido, const String& comandoId) {
    // Payload otimizado: c=concluido, ts=tempo_segundos, id=comando_id, t=timestamp
    String payload = "{\"c\":1,\"ts\":" + String(tempoDecorrido) +
                     ",\"id\":\"" + comandoId + "\"" +
                     ",\"t\":" + String(millis()) + "}";
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

    // Comando alimentar formato JSON (com ou sem espaços)
    if (payload.indexOf("\"acao\"") >= 0 && payload.indexOf("\"alimentar\"") >= 0) {
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

    // Verificar se é uma mensagem informativa (não é comando)
    if (payload.indexOf("\"info\"") >= 0 || payload.indexOf("\"nota\"") >= 0) {
        Serial.println("ℹ️  Mensagem informativa recebida (ignorada)");
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