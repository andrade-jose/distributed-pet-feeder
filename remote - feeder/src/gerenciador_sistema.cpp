#include "gerenciador_sistema.h"
#include <Arduino.h>

GerenciadorSistema::GerenciadorSistema(ServoControl* servoPtr,
                                      WiFiManager* wifiPtr, MQTTManager* mqttPtr,
                                      int pinoBotaoParam, int pinoLedParam)
    : servo(servoPtr), wifiManager(wifiPtr), mqttManager(mqttPtr),
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
    Serial.println("   • Botão toggle (90°=TRAVADO, 0°=DESTRAVADO) - Pino 33");
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
    Serial.printf("🔍 DEBUG - Botão configurado no GPIO %d como INPUT_PULLUP\n", pinoBotao);
    Serial.printf("🔍 DEBUG - LED configurado no GPIO %d como OUTPUT\n", pinoLedStatus);
    Serial.printf("🔍 DEBUG - Estado inicial do botão: %s\n",
                  digitalRead(pinoBotao) == LOW ? "LOW (pressionado)" : "HIGH (solto)");
}

void GerenciadorSistema::inicializarComponentes(int pinoServo) {
    servo->iniciar(pinoServo);
    servo->ativar();
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

    // Teste do servo DESABILITADO para evitar brownout durante boot
    Serial.println("⚠️ Teste do servo DESABILITADO (evita brownout no boot)");
    Serial.println("💡 O servo será movido apenas quando você pressionar o botão ou receber comando MQTT");

    // Posicionar servo em 0° (fechado) sem movimentos bruscos
    Serial.println("🔧 Posicionando servo em 0° (FECHADO - posição inicial)...");
    servo->moverParaAngulo(0);
    delay(500);

    Serial.println("✅ Componentes prontos!");
    Serial.println("");
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

    // Debug contínuo do estado do botão (a cada mudança)
    static bool ultimoEstadoDebug = HIGH;
    if (estadoBotao != ultimoEstadoDebug) {
        Serial.printf("🔍 DEBUG BOTÃO - Leitura GPIO %d: %s (anterior: %s)\n",
                      pinoBotao,
                      estadoBotao == LOW ? "LOW (pressionado)" : "HIGH (solto)",
                      estadoBotaoAnterior == LOW ? "LOW" : "HIGH");
        ultimoEstadoDebug = estadoBotao;
    }

    // Detectar mudança com debounce
    if (estadoBotao != estadoBotaoAnterior) {
        unsigned long agora = millis();
        unsigned long tempoDesdeUltimoDebounce = agora - ultimoDebounce;

        Serial.printf("🔍 DEBUG - Mudança detectada! Tempo desde último debounce: %lu ms (mínimo: %lu ms)\n",
                      tempoDesdeUltimoDebounce, DEBOUNCE_DELAY);

        if (agora - ultimoDebounce > DEBOUNCE_DELAY) {
            Serial.println("✅ DEBUG - Debounce OK! Processando mudança...");

            if (estadoBotao == LOW) { // Botão pressionado (pull-up)
                Serial.printf("🔍 DEBUG - Botão PRESSIONADO | Servo travado: %s\n",
                              servoTravado ? "SIM" : "NÃO");

                if (!servoTravado) {
                    // 1ª vez: TRAVAR servo em 90° (posição aberta)
                    Serial.println("🔒 Botão pressionado - TRAVANDO servo em 90° (ABERTO)");
                    servoTravado = true;
                    servo->moverParaAngulo(90);
                    Serial.println("✅ DEBUG - Servo TRAVADO em 90°");
                } else {
                    // 2ª vez: DESTRAVAR e voltar à posição de descanso (0° fechado)
                    Serial.println("🔓 Botão pressionado - DESTRAVANDO servo");
                    servoTravado = false;
                    servo->moverParaAngulo(0);
                    Serial.println("✅ DEBUG - Servo DESTRAVADO em 0°");
                }
            } else {
                Serial.println("🔍 DEBUG - Botão SOLTO (transição HIGH)");
            }
            ultimoDebounce = agora;
            estadoBotaoAnterior = estadoBotao; // Atualiza apenas após debounce
            Serial.printf("✅ DEBUG - Estado anterior atualizado para: %s\n",
                          estadoBotaoAnterior == LOW ? "LOW" : "HIGH");
        } else {
            Serial.println("⏰ DEBUG - Debounce ainda não passou, ignorando mudança");
        }
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
        // Monitoramento do sistema
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