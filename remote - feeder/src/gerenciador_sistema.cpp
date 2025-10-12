#include "gerenciador_sistema.h"
#include <Arduino.h>

GerenciadorSistema::GerenciadorSistema(ServoControl* servoPtr, SensorHall* sensorPtr, 
                                      WiFiManager* wifiPtr, MQTTManager* mqttPtr,
                                      int pinoBotaoParam, int pinoLedParam)
    : servo(servoPtr), sensorHall(sensorPtr), wifiManager(wifiPtr), mqttManager(mqttPtr),
      pinoBotao(pinoBotaoParam), pinoLedStatus(pinoLedParam),
      estadoBotaoAnterior(HIGH), ultimoDebounce(0), servoTravado(false),
      ultimoPiscada(0), estadoLed(false), ultimoMonitoramento(0) {
}

void GerenciadorSistema::inicializar() {
    Serial.begin(115200);
    delay(2000);
}

void GerenciadorSistema::exibirInformacoes() {
    Serial.println("🍽️ ALIMENTADOR AUTOMÁTICO REMOTA - MQTT");
    Serial.println("==========================================");
    Serial.println("🗣️ Arquitetura:");
    Serial.println("   • ESP32 CENTRAL → Envia comandos");
    Serial.println("   • ESP32 REMOTA → Executa alimentação (ESTE)");
    Serial.println("");
    Serial.println("🔧 Hardware:");
    Serial.println("   • Servo PDI 6221MG (0°=FECHADO, 90°=ABERTO) - Pino 5");
    Serial.println("   • Sensor Hall A3144 - Pino 4");
    Serial.println("   • Botão toggle (90°=TRAVADO, 0°=DESTRAVADO) - Pino 18");
    Serial.println("   • LED Status - Pino 13");
    Serial.println("   • ESP32-D0WD-V3");
    Serial.println("");
    Serial.println("📡 MQTT Topics (REMOTA):");
    Serial.println("   • Comando: alimentador/remota/comando");
    Serial.println("   • Status: alimentador/remota/status");
    Serial.println("   • Resposta: alimentador/remota/resposta");
    Serial.println("   • Heartbeat: alimentador/remota/heartbeat");
    Serial.println("");
}

void GerenciadorSistema::configurarHardware() {
    pinMode(pinoBotao, INPUT_PULLUP);
    pinMode(pinoLedStatus, OUTPUT);
    digitalWrite(pinoLedStatus, LOW);
    Serial.println("⚙️ Hardware configurado!");
}

void GerenciadorSistema::inicializarComponentes(int pinoServo, int pinoHall) {
    servo->iniciar(pinoServo);
    servo->ativar();
    sensorHall->iniciar(pinoHall);
    Serial.println("✅ Componentes inicializados!");
}

void GerenciadorSistema::inicializarComunicacao() {
    Serial.println("📡 Inicializando comunicação...");
    wifiManager->iniciar();
    mqttManager->iniciar();
}

void GerenciadorSistema::conectarServicos(const char* topicComando) {
    if (wifiManager->conectar()) {
        if (mqttManager->conectar()) {
            mqttManager->subscrever(topicComando);
            Serial.println("✅ Serviços conectados!");
        }
    }
}

void GerenciadorSistema::testarComponentes() {
    Serial.println("🧪 TESTE INICIAL DOS COMPONENTES");
    Serial.println("==================================");

    // Teste do servo
    Serial.println("🔧 Testando servo posicional...");
    servo->moverParaAngulo(45);
    delay(1000);
    servo->moverParaAngulo(90);
    delay(1000);
    servo->moverParaAngulo(45);
    delay(1000);
    servo->moverParaAngulo(0);
    delay(1000);

    // Teste do sensor Hall
    Serial.println("🧲 Testando sensor Hall...");
    Serial.println("   💡 Aproxime um ímã do sensor para testar");

    // Aguardar 5 segundos para teste manual
    for (int i = 5; i > 0; i--) {
        Serial.printf("   Aguardando %d segundos...\n", i);
        sensorHall->verificar();
        bool detectando = sensorHall->estaDetectando();
        Serial.printf("   Estado: %s\n", detectando ? "ÍMÃ DETECTADO" : "Normal");
        delay(1000);
    }

    Serial.println("✅ Teste dos componentes concluído!");
    Serial.println("");
    servo->moverParaAngulo(0);
    delay(1000);
}

void GerenciadorSistema::finalizarInicializacao() {
    Serial.println("🚀 ESP32 REMOTA pronto para receber comandos do CENTRAL!");
}

void GerenciadorSistema::verificarConexoes() {
    wifiManager->verificarConexao();
    mqttManager->verificarConexao();
}

void GerenciadorSistema::processarBotao() {
    bool estadoBotao = digitalRead(pinoBotao);

    // Detectar mudança com debounce
    if (estadoBotao != estadoBotaoAnterior) {
        unsigned long agora = millis();
        if (agora - ultimoDebounce > DEBOUNCE_DELAY) {
            if (estadoBotao == LOW) { // Botão pressionado (pull-up)
                if (!servoTravado) {
                    // 1ª vez: TRAVAR servo em 90° (posição aberta)
                    Serial.println("🔒 Botão pressionado - TRAVANDO servo em 90° (ABERTO)");
                    servoTravado = true;
                    servo->moverParaAngulo(90);
                } else {
                    // 2ª vez: DESTRAVAR e voltar à posição de descanso (0° fechado)
                    Serial.println("🔓 Botão pressionado - DESTRAVANDO servo");
                    servoTravado = false;
                    servo->moverParaAngulo(0);
                }
            }
            ultimoDebounce = agora;
        }
        estadoBotaoAnterior = estadoBotao;
    }
}

void GerenciadorSistema::atualizarLedStatus() {
    if (wifiManager->estaConectado() && mqttManager->estaConectado()) {
        // Conexão OK: LED sempre ligado
        digitalWrite(pinoLedStatus, HIGH);
    } else {
        // Sem conexão: LED piscando
        unsigned long agora = millis();
        if (agora - ultimoPiscada >= 500) { // Piscar a cada 500ms
            estadoLed = !estadoLed;
            digitalWrite(pinoLedStatus, estadoLed);
            ultimoPiscada = agora;
        }
    }
}

void GerenciadorSistema::monitorarSistema() {
    unsigned long agora = millis();
    if (agora - ultimoMonitoramento >= INTERVALO_MONITORAMENTO) {
        // Atualizar sensor Hall
        sensorHall->verificar();
        // (Print de status removido para evitar excesso de mensagens)
        ultimoMonitoramento = agora;
    }
}

void GerenciadorSistema::delay(int ms) {
    ::delay(ms);
}

bool GerenciadorSistema::getServoTravado() const {
    return servoTravado;
}