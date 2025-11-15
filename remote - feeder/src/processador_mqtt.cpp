#include "processador_mqtt.h"
#include <WiFi.h>

ProcessadorMQTT::ProcessadorMQTT(MQTTManager* mqtt,
                                 GerenciadorAlimentacao* alim,
                                 GerenciadorSistema* sist)
    : mqttManager(mqtt), alimentacao(alim), sistema(sist),
      seq(0), birthEnviado(false), ultimoDDATA(0) {
}

// ===== DBIRTH - BIRTH CERTIFICATE =====
bool ProcessadorMQTT::enviarDBIRTH() {
    if (!mqttManager->estaConectado()) {
        Serial.println("❌ MQTT não conectado - impossível enviar DBIRTH");
        return false;
    }

    Serial.println("📋 Enviando DBIRTH (Birth Certificate)...");

    // Criar payload SparkplugB
    SparkplugBPayload payload;
    payload.setTimestamp(millis());
    payload.setBdSeq(mqttManager->getBdSeq());

    // Métricas de Configuração (Device Birth)
    payload.addMetric("remota_id", REMOTA_ID);
    payload.addMetric("device_id", String(DEVICE_ID));
    payload.addMetric("firmware_version", String("1.0.0"));
    payload.addMetric("servo_pin", PINO_SERVO);
    payload.addMetric("hcsr04_trigger_pin", PINO_HCSR04_TRIGGER);
    payload.addMetric("hcsr04_echo_pin", PINO_HCSR04_ECHO);
    payload.addMetric("distancia_limite_racao", DISTANCIA_LIMITE_RACAO);

    // Status inicial
    payload.addMetric("online", true);

    // Publicar DBIRTH
    bool sucesso = mqttManager->publicar(TOPIC_DBIRTH, payload.toJSON(), 1);

    if (sucesso) {
        birthEnviado = true;
        seq = 0; // Reset da sequência ao enviar DBIRTH
        mqttManager->incrementBdSeq();
        Serial.println("✅ DBIRTH enviado com sucesso!");
    } else {
        Serial.println("❌ Falha ao enviar DBIRTH");
    }

    return sucesso;
}

// ===== DDATA - TELEMETRIA =====
bool ProcessadorMQTT::enviarDDATA(float distanciaRacao, bool racaoBaixa) {
    if (!mqttManager->estaConectado()) {
        Serial.println("❌ MQTT não conectado - impossível enviar DDATA");
        return false;
    }

    if (!birthEnviado) {
        Serial.println("⚠️  DBIRTH não foi enviado ainda - enviando agora...");
        enviarDBIRTH();
    }

    // Criar payload SparkplugB
    SparkplugBPayload payload;
    payload.setTimestamp(millis());
    payload.setSeq(seq);

    // Métricas de Telemetria
    payload.addMetric("rssi", WiFi.RSSI());
    payload.addMetric("servo_ativo", alimentacao->estaAtivo());
    payload.addMetric("servo_travado", sistema->getServoTravado());
    payload.addMetric("distancia_racao", distanciaRacao);
    payload.addMetric("racao_baixa", racaoBaixa);
    payload.addMetric("uptime", (int32_t)(millis() / 1000)); // Segundos

    // Publicar DDATA
    bool sucesso = mqttManager->publicar(TOPIC_DDATA, payload.toJSON(), 1);

    if (sucesso) {
        seq++;
        ultimoDDATA = millis();
        Serial.printf("📊 DDATA enviado (seq: %llu)\n", seq - 1);
    } else {
        Serial.println("❌ Falha ao enviar DDATA");
    }

    return sucesso;
}

// ===== DDEATH - DEATH CERTIFICATE =====
bool ProcessadorMQTT::enviarDDEATH() {
    if (!mqttManager->estaConectado()) {
        Serial.println("❌ MQTT não conectado - impossível enviar DDEATH");
        return false;
    }

    Serial.println("💀 Enviando DDEATH (Death Certificate)...");

    // Criar payload SparkplugB
    SparkplugBPayload payload;
    payload.setTimestamp(millis());
    payload.setBdSeq(mqttManager->getBdSeq());

    // Publicar DDEATH
    bool sucesso = mqttManager->publicar(TOPIC_DDEATH, payload.toJSON(), 1);

    if (sucesso) {
        birthEnviado = false;
        Serial.println("✅ DDEATH enviado com sucesso!");
    } else {
        Serial.println("❌ Falha ao enviar DDEATH");
    }

    return sucesso;
}

// ===== DCMD - PROCESSAR COMANDOS =====
void ProcessadorMQTT::processarDCMD(const String& payload) {
    Serial.println("📥 Processando DCMD (comando SparkplugB)...");

    // Parsear payload SparkplugB
    SparkplugBPayload cmd = SparkplugBParser::parseJSON(payload);

    // Extrair métricas do comando
    String command;
    int32_t duration = 5; // Padrão: 5 segundos

    if (cmd.getMetric("command", command)) {
        Serial.printf("🎯 Comando recebido: %s\n", command.c_str());

        // Extrair duração (se fornecida)
        cmd.getMetric("duration", duration);

        // Processar comando
        if (command == "feed" || command == "alimentar") {
            // Validar duração
            if (duration < 1) duration = 1;
            if (duration > 60) duration = 60;

            Serial.printf("🍽️  Alimentando por %d segundos...\n", duration);

            String idComando = "DCMD_" + String(millis());
            alimentacao->setIdComando(idComando);
            alimentacao->iniciar(duration);

        } else if (command == "stop" || command == "parar") {
            Serial.println("🛑 Parando alimentação...");
            alimentacao->parar();

        } else if (command == "status") {
            Serial.println("ℹ️  Solicitação de status - enviando DDATA...");
            // Força envio imediato de DDATA
            extern GerenciadorHCSR04 sensorRacao;
            float distancia = sensorRacao.medirDistancia();
            bool racaoBaixa = (distancia >= sensorRacao.getDistanciaLimite());
            enviarDDATA(distancia, racaoBaixa);

        } else {
            Serial.printf("❌ Comando não reconhecido: %s\n", command.c_str());
        }
    } else {
        Serial.println("❌ Comando SparkplugB inválido - métrica 'command' não encontrada");
    }
}

// ===== UTILITÁRIOS =====

uint64_t ProcessadorMQTT::getSeq() {
    return seq;
}

void ProcessadorMQTT::incrementSeq() {
    seq++;
}

bool ProcessadorMQTT::getBirthEnviado() {
    return birthEnviado;
}

void ProcessadorMQTT::loop() {
    // Enviar DDATA a cada 5 segundos
    unsigned long agora = millis();
    if (birthEnviado && (agora - ultimoDDATA >= INTERVALO_HEARTBEAT)) {
        extern GerenciadorHCSR04 sensorRacao;
        float distancia = sensorRacao.medirDistancia();
        bool racaoBaixa = (distancia >= sensorRacao.getDistanciaLimite());
        enviarDDATA(distancia, racaoBaixa);
    }
}
