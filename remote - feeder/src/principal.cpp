#include <Arduino.h>
#include <WiFi.h>
#include "ServoControl.h"
#include "SensorHall.h"
#include "WiFiManager.h"
#include "MQTTManager.h"

// ===== CONFIGURAÇÃO WIFI =====
const char* WIFI_SSID = "Coelhoandrade";        
const char* WIFI_PASSWORD = "190520jg";         

// ===== CONFIGURAÇÃO MQTT =====
const char* MQTT_SERVER = "9aa85a8cfb4a4ba896f2289aa408ba5a.s1.eu.hivemq.cloud";  
const int MQTT_PORT = 8883;                     
const char* MQTT_CLIENT_ID = "ESP32_Remota_001"; 
const char* MQTT_USERNAME = "Romota1";          
const char* MQTT_PASSWORD = "Senha1234";        

// ===== TÓPICOS MQTT =====
const char* TOPIC_COMANDO = "alimentador/remota/comando";           
const char* TOPIC_STATUS = "alimentador/remota/status";             
const char* TOPIC_RESPOSTA = "alimentador/remota/resposta";         
const char* TOPIC_HEARTBEAT = "alimentador/remota/heartbeat";       

// ===== CONFIGURAÇÃO DO HARDWARE =====
const int PINO_SERVO = 5;      
const int PINO_HALL = 4;       
const int PINO_BOTAO = 18;      
const int PINO_LED_STATUS = 13;

// ===== INSTÂNCIAS DOS COMPONENTES =====
ServoControl servo;
SensorHall sensorHall;
WiFiManager wifiManager(WIFI_SSID, WIFI_PASSWORD);
MQTTManager mqttManager(&wifiManager, MQTT_SERVER, MQTT_PORT, MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);

// ===== VARIÁVEIS DE CONTROLE =====
int tempoAlimentacaoSegundos = 5;  // Tempo desejado de alimentação (em segundos)
unsigned long inicioAlimentacao = 0; // Timestamp do início da alimentação
bool alimentacaoAtiva = false;     // Se está executando alimentação
bool servoAberto = false;          // Controle de posição (false = 0° fechado, true = 90° aberto)
unsigned long ultimoMovimento = 0;
const unsigned long TEMPO_MOVIMENTO = 2000; // 2 segundos para completar movimento

// ===== VARIÁVEIS DO BOTÃO =====
bool estadoBotaoAnterior = HIGH;
unsigned long ultimoDebounce = 0;
const unsigned long DEBOUNCE_DELAY = 50;
bool servoTravado = false; // Nova variável para controlar se servo está travado em 180°

// ===== VARIÁVEIS DE COMUNICAÇÃO =====
unsigned long ultimoHeartbeat = 0;
const unsigned long INTERVALO_HEARTBEAT = 30000; // 30 segundos
String idComandoAtual = "";                      // ID do comando sendo executado

// ===== DECLARAÇÕES DE FUNÇÕES =====
void testarComponentes();
void iniciarAlimentacao(int tempoSegundos);
void pararAlimentacao();
void sistemaAlimentacao();
void monitorarSistema();
void processarBotao();
void atualizarLedStatus();
void callbackMQTT(String topic, String payload);
void enviarStatusMQTT(String status);
void enviarConclusaoMQTT(int tempoDecorrido);
void enviarHeartbeat();
void processarComandoCentral(String payload);

// ===== CONFIGURAÇÃO INICIAL =====
void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("🍽️ ALIMENTADOR AUTOMÁTICO REMOTA - MQTT");
    Serial.println("==========================================");
    Serial.println("🏗️ Arquitetura:");
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
    Serial.println("🔧 Lógica do Sensor Hall:");
    Serial.println("   • Sensor DETECTA = Servo confirmado na posição 0° (fechado)");
    Serial.println("   • Alimentação = Manter servo em 90° pelo tempo especificado");
    Serial.println("   • Confirmação redundante de posicionamento preciso");
    Serial.println("");
    Serial.println("⏰ Lógica de Alimentação:");
    Serial.println("   • Comando especifica duração em segundos (1-60s)");
    Serial.println("   • Servo: 0°→90° (manter tempo) →0°");
    Serial.println("   • Confirmação de posição inicial e final");
    Serial.println("");
    Serial.println("📚 Documentação:");
    Serial.println("   • README.md - Visão geral e uso");
    Serial.println("   • docs/TECHNICAL_DOCUMENTATION.md - Detalhes técnicos");
    Serial.println("   • docs/MQTT_API_REFERENCE.md - API MQTT");
    Serial.println("   • docs/INSTALLATION_GUIDE.md - Guia de instalação");
    Serial.println("");

    // Configurar pinos
    pinMode(PINO_BOTAO, INPUT_PULLUP);
    pinMode(PINO_LED_STATUS, OUTPUT);
    digitalWrite(PINO_LED_STATUS, LOW); // LED apagado inicialmente

    // Inicializar componentes
    Serial.println("⚙️ Inicializando componentes...");

    servo.iniciar(PINO_SERVO);
    servo.ativar();

    sensorHall.iniciar(PINO_HALL);

    Serial.println("✅ Hardware inicializado!");

    // Inicializar comunicação
    Serial.println("📡 Inicializando comunicação...");
    wifiManager.iniciar();
    mqttManager.iniciar();
    mqttManager.definirCallback(callbackMQTT);

    // Conectar WiFi e MQTT
    if (wifiManager.conectar())
    {
        if (mqttManager.conectar())
        {
            mqttManager.subscrever(TOPIC_COMANDO);
            enviarStatusMQTT("online");
        }
    }

    // Teste inicial dos componentes
    testarComponentes();

    Serial.println("🚀 ESP32 REMOTA pronto para receber comandos do CENTRAL!");
}

// ===== TESTE INICIAL DOS COMPONENTES =====
void testarComponentes()
{
    Serial.println("🧪 TESTE INICIAL DOS COMPONENTES");
    Serial.println("==================================");

    // Teste do servo
    Serial.println("🔧 Testando servo posicional...");
    servo.moverParaAngulo(45); // Teste 45°
    delay(1000);
    servo.moverParaAngulo(90); // Aberto
    delay(1000);
    servo.moverParaAngulo(45); // Teste 45°
    delay(1000);
    servo.moverParaAngulo(0); // Fechado
    delay(1000);

    // Teste do sensor Hall
    Serial.println("🧲 Testando sensor Hall...");
    Serial.println("   💡 Aproxime um ímã do sensor para testar");

    // Aguardar 5 segundos para teste manual
    for (int i = 5; i > 0; i--)
    {
        Serial.printf("   Aguardando %d segundos...\n", i);
        sensorHall.verificar(); // Verificar estado e contar
        bool detectando = sensorHall.estaDetectando();
        Serial.printf("   Estado: %s\n", detectando ? "ÍMÃÇ DETECTADO" : "Normal");
        delay(1000);
    }

    Serial.println("✅ Teste dos componentes concluído!");
    Serial.println("");
    servo.moverParaAngulo(0); // Voltar à posição de descanso (fechado)
    delay(1000);
}

// ===== LOOP PRINCIPAL =====
void loop()
{
    // Verificar e manter conexões WiFi/MQTT
    wifiManager.verificarConexao();
    mqttManager.verificarConexao();

    // Processar mensagens MQTT
    mqttManager.loop();

    // Processar botão físico
    processarBotao();

    // Atualizar LED de status
    atualizarLedStatus();

    // Executar sistema de alimentação se estiver ativo
    sistemaAlimentacao();

    // Monitorar sistema continuamente
    monitorarSistema();

    // Enviar heartbeat periodicamente
    enviarHeartbeat();

    // Pequeno delay para não sobrecarregar o processador
    delay(50);
}

// ===== CALLBACK MQTT =====
void callbackMQTT(String topic, String payload)
{
    Serial.printf("📥 MQTT recebido [%s]: %s\n", topic.c_str(), payload.c_str());

    // Processar comandos do CENTRAL
    processarComandoCentral(payload);
}

// ===== PROCESSAR BOTÃO =====
void processarBotao()
{
    bool estadoBotao = digitalRead(PINO_BOTAO);

    // Detectar mudança com debounce
    if (estadoBotao != estadoBotaoAnterior)
    {
        unsigned long agora = millis();
        if (agora - ultimoDebounce > DEBOUNCE_DELAY)
        {
            if (estadoBotao == LOW)
            { // Botão pressionado (pull-up)

                if (!servoTravado)
                {
                    // 1ª vez: TRAVAR servo em 90° (posição aberta)
                    Serial.println("🔒 Botão pressionado - TRAVANDO servo em 90° (ABERTO)");
                    servoTravado = true;

                    // Se estiver alimentando, pausar (mas não parar completamente)
                    if (alimentacaoAtiva)
                    {
                        Serial.println("⏸️ Alimentação PAUSADA - servo travado");
                        enviarStatusMQTT("pausado_botao");
                    }

                    servo.moverParaAngulo(90);
                    enviarStatusMQTT("servo_travado_90");
                }
                else
                {
                    // 2ª vez: DESTRAVAR e voltar à posição de descanso (0° fechado)
                    Serial.println("🔓 Botão pressionado - DESTRAVANDO servo");
                    servoTravado = false;

                    // Sempre ir para posição de descanso 0° (fechado) ao destravar
                    Serial.println("🏠 Movendo para posição de descanso (0° FECHADO)");
                    servo.moverParaAngulo(0);

                    if (alimentacaoAtiva)
                    {
                        Serial.println("▶️ Alimentação RETOMADA - servo destravado");
                        enviarStatusMQTT("retomado_botao");

                        // Retomar da posição atual para continuar o ciclo
                        Serial.println("🔄 Retomando ciclo de alimentação...");
                        ultimoMovimento = millis(); // Resetar timer para movimento imediato
                    }
                    else
                    {
                        // Sistema não estava alimentando - voltar ao estado ativo
                        Serial.println("✅ Sistema ATIVO novamente - posição 0° (fechado) - pronto para comandos");
                        enviarStatusMQTT("sistema_ativo_0");
                    }
                }
            }
            ultimoDebounce = agora;
        }
        estadoBotaoAnterior = estadoBotao;
    }
}

// ===== ATUALIZAR LED STATUS =====
void atualizarLedStatus()
{
    static unsigned long ultimoPiscada = 0;
    static bool estadoLed = false;

    if (wifiManager.estaConectado() && mqttManager.estaConectado())
    {
        // Conexão OK: LED sempre ligado
        digitalWrite(PINO_LED_STATUS, HIGH);
    }
    else
    {
        // Sem conexão: LED piscando
        unsigned long agora = millis();
        if (agora - ultimoPiscada >= 500)
        { // Piscar a cada 500ms
            estadoLed = !estadoLed;
            digitalWrite(PINO_LED_STATUS, estadoLed);
            ultimoPiscada = agora;
        }
    }
}

// ===== ENVIAR STATUS MQTT =====
void enviarStatusMQTT(String status)
{
    String payload = "{\"status\":\"" + status + "\",\"timestamp\":" + String(millis()) + "}";
    if (mqttManager.publicar(TOPIC_STATUS, payload))
    {
        Serial.printf("📤 Status enviado: %s\n", status.c_str());
    }
}

// ===== ENVIAR CONCLUSÃO MQTT =====
void enviarConclusaoMQTT(int tempoDecorrido)
{
    String payload = "{\"concluido\":true,\"tempo_segundos\":" + String(tempoDecorrido) +
                     ",\"comando_id\":\"" + idComandoAtual + "\"" +
                     ",\"timestamp\":" + String(millis()) + "}";
    if (mqttManager.publicar(TOPIC_RESPOSTA, payload))
    {
        Serial.printf("📤 Conclusão enviada: %d segundos de alimentação [ID: %s]\n", tempoDecorrido, idComandoAtual.c_str());
    }
    idComandoAtual = ""; // Limpar ID após envio
}

// ===== SISTEMA DE ALIMENTAÇÃO =====
void sistemaAlimentacao()
{
    if (!alimentacaoAtiva)
    {
        return; // Não está alimentando, sair silenciosamente
    }

    // ⭐ LÓGICA DE PAUSA: Se servo está travado, não mover
    if (servoTravado)
    {
        // Servo travado em 90° (posição aberta) - alimentação pausada
        return; // Sair sem fazer nada, mas manter alimentacaoAtiva = true
    }

    // Atualizar estado do sensor Hall
    sensorHall.verificar();
    bool sensorDetectando = sensorHall.estaDetectando();

    // ⭐ LÓGICA DE TEMPO: Alimentação controlada por duração
    // 1. Posicionar em 0° (fechado) com confirmação do sensor
    // 2. Mover para 90° (aberto) e MANTER pelo tempo especificado
    // 3. Voltar para 0° (fechado) com confirmação do sensor
    // NÃO HÁ CICLOS REPETITIVOS - APENAS UM MOVIMENTO POR COMANDO

    unsigned long agora = millis();
    unsigned long tempoDecorrido = (agora - inicioAlimentacao) / 1000; // em segundos
    
    if (!servoAberto)
    {
        // Servo deveria estar em 0° (fechado)
        if (sensorDetectando)
        {
            // ✅ Confirmação: Servo está realmente na posição 0° (fechado)
            // Aguardar tempo antes de abrir
            if (agora - ultimoMovimento >= TEMPO_MOVIMENTO)
            {
                Serial.printf("🍽️ Abrindo para 90° - ALIMENTANDO por %d segundos\n", tempoAlimentacaoSegundos);
                servo.moverParaAngulo(90);
                servoAberto = true;
                ultimoMovimento = agora;
            }
        }
        else
        {
            // ❌ Servo não está na posição 0° - forçar reposicionamento
            Serial.println("⚠️ Servo fora de posição! Reposicionando para 0°...");
            servo.moverParaAngulo(0);
            ultimoMovimento = agora; // Resetar timer
        }
    }
    else
    {
        // Servo está em 90° (aberto - alimentando)
        // Verificar se o tempo de alimentação foi atingido
        if (tempoDecorrido >= tempoAlimentacaoSegundos)
        {
            Serial.printf("⏰ Tempo de alimentação esgotado! (%lu segundos)\n", tempoDecorrido);
            Serial.println("🔒 Fechando para 0° (finalizando alimentação)");
            servo.moverParaAngulo(0);
            servoAberto = false;
            
            // Aguardar um pouco para o servo se mover antes de verificar sensor
            delay(500);
            
            // Verificar se chegou na posição 0°
            sensorHall.verificar();
            if (sensorHall.estaDetectando())
            {
                // ✅ Alimentação finalizada com confirmação do sensor!
                Serial.printf("✅ ALIMENTAÇÃO CONCLUÍDA! Duração: %lu segundos\n", tempoDecorrido);
                enviarConclusaoMQTT(tempoDecorrido);
                pararAlimentacao();
                return;
            }
            else
            {
                // ❌ Servo não chegou na posição 0° - tentar novamente
                Serial.println("❌ Servo não confirmou posição 0°! Tentando novamente...");
                ultimoMovimento = agora; // Dar mais tempo para tentar novamente
            }
        }
        else
        {
            // Ainda alimentando - mostrar progresso ocasionalmente
            static unsigned long ultimoProgresso = 0;
            if (agora - ultimoProgresso >= 1000) // A cada segundo
            {
                unsigned long tempoRestante = tempoAlimentacaoSegundos - tempoDecorrido;
                Serial.printf("🍽️ Alimentando... Tempo restante: %lu segundos\n", tempoRestante);
                ultimoProgresso = agora;
            }
        }
    }
}

// ===== INICIAR ALIMENTAÇÃO =====
void iniciarAlimentacao(int tempoSegundos)
{
    if (alimentacaoAtiva)
    {
        Serial.println("⚠️ Alimentação já está ativa!");
        return;
    }

    Serial.println("🍽️ INICIANDO ALIMENTAÇÃO");
    Serial.println("=========================");
    Serial.printf("⏰ Duração: %d segundos\n", tempoSegundos);
    Serial.println("📍 Sensor Hall confirma posição 0° (fechado)");
    Serial.println("🔄 Sequência: 0°→90° (manter tempo) →0°");
    Serial.println("✅ Controle por tempo ao invés de movimentos");
    Serial.println("");

    // Configurar alimentação
    tempoAlimentacaoSegundos = tempoSegundos;
    alimentacaoAtiva = true;
    inicioAlimentacao = millis(); // Marcar início da alimentação

    // Posicionar servo na posição inicial fechada (0°)
    Serial.println("📐 Posicionando servo em 0° (posição fechada)");
    servo.moverParaAngulo(0);
    servoAberto = false;

    // Aguardar um momento para estabilizar e verificar sensor
    delay(1000);
    
    sensorHall.verificar();
    if (sensorHall.estaDetectando())
    {
        Serial.println("✅ Sensor confirma: Servo na posição 0° (fechado)");
    }
    else
    {
        Serial.println("⚠️ Sensor não detecta posição 0° - ajustando...");
        delay(500);
        servo.moverParaAngulo(0); // Tentar novamente
        delay(1000);
    }

    ultimoMovimento = millis();
    Serial.printf("🚀 Alimentação iniciada! Duração programada: %d segundos\n", tempoSegundos);
}

// ===== PARAR ALIMENTAÇÃO =====
void pararAlimentacao()
{
    alimentacaoAtiva = false;
    servoTravado = false; // ⭐ Resetar travamento quando parar alimentação

    // Calcular tempo total decorrido
    unsigned long tempoTotal = (millis() - inicioAlimentacao) / 1000;

    // Posicionar servo na posição de descanso (0° fechado)
    Serial.println("🏁 Finalizando alimentação...");
    servo.moverParaAngulo(0);
    
    // Aguardar e verificar se chegou na posição
    delay(1000);
    sensorHall.verificar();
    if (sensorHall.estaDetectando())
    {
        Serial.println("✅ Servo confirmado na posição 0° (fechado)");
    }

    Serial.printf("📊 Resumo: %lu segundos de alimentação (programado: %d segundos)\n",
                  tempoTotal, tempoAlimentacaoSegundos);
    Serial.println("✅ Sistema em repouso (posição 0° fechado)");
    Serial.println("");

    // Enviar status final via MQTT
    enviarStatusMQTT("finalizado");
}

// ===== MONITORAMENTO CONTÍNUO =====
void monitorarSistema()
{
    static unsigned long ultimoMonitoramento = 0;
    const unsigned long INTERVALO_MONITORAMENTO = 1000; // 1 segundo

    unsigned long agora = millis();
    if (agora - ultimoMonitoramento >= INTERVALO_MONITORAMENTO)
    {

        // Atualizar sensor Hall
        sensorHall.verificar();

        // Mostrar status apenas se estiver alimentando
        if (alimentacaoAtiva)
        {
            bool sensorConfirma = sensorHall.estaDetectando();
            String statusTravamento = servoTravado ? "TRAVADO-90°" : "LIVRE";
            String posicaoAtual;
            String confirmacao;
            
            // Calcular tempo decorrido e restante
            unsigned long tempoDecorrido = (agora - inicioAlimentacao) / 1000;
            unsigned long tempoRestante = (tempoDecorrido < tempoAlimentacaoSegundos) ? 
                                        (tempoAlimentacaoSegundos - tempoDecorrido) : 0;

            if (servoTravado)
            {
                posicaoAtual = "90°(TRAVADO)";
                confirmacao = "N/A";
            }
            else
            {
                posicaoAtual = servoAberto ? "90°(ABERTO)" : "0°(FECHADO)";
                confirmacao = sensorConfirma ? "✅CONFIRMADO" : "❌NÃO_CONFIRMADO";
            }

            Serial.printf("📊 Status: %lus/%ds | Restante: %lus | Posição: %s | Confirmação: %s | Servo: %s | WiFi: %s | MQTT: %s\n",
                          tempoDecorrido, tempoAlimentacaoSegundos, tempoRestante,
                          posicaoAtual.c_str(),
                          confirmacao.c_str(),
                          statusTravamento.c_str(),
                          wifiManager.estaConectado() ? "OK" : "❌",
                          mqttManager.estaConectado() ? "OK" : "❌");
        }

        ultimoMonitoramento = agora;
    }
}

// ===== PROCESSAR COMANDO DO CENTRAL =====
void processarComandoCentral(String payload)
{
    // Formato esperado NOVO: {"acao":"alimentar","tempo":5,"remota_id":1}
    // ou comandos simples: STOP, STATUS, PING

    if (payload == "PING")
    {
        enviarStatusMQTT("PONG");
        return;
    }

    if (payload == "STATUS")
    {
        String statusAtual;
        if (alimentacaoAtiva)
        {
            statusAtual = "ATIVO";
        }
        else if (servoTravado)
        {
            statusAtual = "INATIVO";
        }
        else
        {
            statusAtual = "DISPONIVEL"; // Sistema pronto para receber comandos
        }

        String statusTravamento = servoTravado ? "_TRAVADO" : "";
        enviarStatusMQTT(statusAtual + statusTravamento);
        return;
    }

    if (payload == "STOP")
    {
        if (alimentacaoAtiva)
        {
            pararAlimentacao();
            enviarStatusMQTT("PARADO_CENTRAL");
        }
        else
        {
            enviarStatusMQTT("JA_PARADO");
        }
        return;
    }

    // ===== PROCESSAR COMANDO "alimentar" DA CENTRAL =====
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
        
        idComandoAtual = "ALIMENTAR_" + String(millis());
        enviarStatusMQTT("INICIANDO_" + idComandoAtual);
        iniciarAlimentacao(tempoSegundos);
        return;
    }

    // Comando formato legado: a3, a5, etc. (CONVERTIDO PARA TEMPO)
    if (payload.startsWith("a") && payload.length() > 1)
    {
        String numeroStr = payload.substring(1);
        int tempoSegundos = numeroStr.toInt();

        if (tempoSegundos > 0 && tempoSegundos <= 60)
        {
            idComandoAtual = "LEGACY_" + String(millis());
            Serial.printf("🎯 Comando legado: %d segundos [ID: %s]\n", tempoSegundos, idComandoAtual.c_str());
            enviarStatusMQTT("INICIANDO_" + idComandoAtual);
            iniciarAlimentacao(tempoSegundos);
        }
        else
        {
            enviarStatusMQTT("ERRO_PARAMETRO_LEGACY");
        }
        return;
    }

    // Comando não reconhecido
    Serial.println("❌ Comando não reconhecido do CENTRAL!");
    enviarStatusMQTT("COMANDO_INVALIDO");
}

// ===== ENVIAR HEARTBEAT =====
void enviarHeartbeat()
{
    unsigned long agora = millis();
    if (agora - ultimoHeartbeat >= INTERVALO_HEARTBEAT)
    {
        String payload = "{\"status\":\"ALIVE\"" +
                         String(",\"remota_id\":1") +
                         String(",\"uptime\":") + String(millis()) +
                         String(",\"wifi_rssi\":") + String(WiFi.RSSI()) +
                         String(",\"free_heap\":") + String(ESP.getFreeHeap()) +
                         String(",\"alimentacao_ativa\":") + String(alimentacaoAtiva ? "true" : "false") +
                         String(",\"servo_travado\":") + String(servoTravado ? "true" : "false") + String("}");

        if (mqttManager.publicar(TOPIC_HEARTBEAT, payload))
        {
            Serial.printf("💓 Heartbeat enviado: ALIVE (remota_id: 1)\n");
        }
        ultimoHeartbeat = agora;
    }
}
