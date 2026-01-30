#include "gerenciador_sensorhall.h"

SensorHall::SensorHall() 
    : pino(-1), estadoAtual(false), estadoAnterior(false), 
      ultimaLeitura(0), contadorDeteccoes(0), detectandoAtualmente(false) {
}

void SensorHall::iniciar(int pinoParam) {
    pino = pinoParam;
    pinMode(pino, INPUT);
    
    estadoAtual = false;
    estadoAnterior = false;
    contadorDeteccoes = 0;
    detectandoAtualmente = false;
    ultimaLeitura = 0;
    
    Serial.printf("🧲 Sensor Hall A3144 iniciado no pino %d\n", pino);
    Serial.println("   💡 LOW = Ímã detectado, HIGH = Normal");
}

void SensorHall::verificar() {
    unsigned long agora = millis();
    
    // Limitar frequência de leitura
    if (agora - ultimaLeitura < INTERVALO_LEITURA) {
        return;
    }
    
    ultimaLeitura = agora;
    estadoAnterior = estadoAtual;
    
    // A3144: LOW quando detecta ímã, HIGH quando normal
    bool leituraDigital = digitalRead(pino);
    estadoAtual = !leituraDigital; // Inverter para que true = detectando
    
    // Detectar mudança de estado
    if (estadoAtual && !estadoAnterior) {
        // Começou a detectar
        contadorDeteccoes++;
        detectandoAtualmente = true;
        Serial.printf("🧲 Ímã DETECTADO! (Contagem: %lu)\n", contadorDeteccoes);
    } else if (!estadoAtual && estadoAnterior) {
        // Parou de detectar
        detectandoAtualmente = false;
        Serial.println("🧲 Ímã removido");
    }
}

bool SensorHall::estaDetectando() {
    return estadoAtual;
}

bool SensorHall::mudouEstado() {
    return (estadoAtual != estadoAnterior);
}

unsigned long SensorHall::obterContador() {
    return contadorDeteccoes;
}

void SensorHall::resetarContador() {
    contadorDeteccoes = 0;
    Serial.println("🔄 Contador do sensor Hall resetado");
}

void SensorHall::testar() {
    Serial.println("🧪 Teste do Sensor Hall A3144");
    Serial.println("   Aproxime um ímã do sensor...");
    
    for (int i = 0; i < 20; i++) {
        verificar();
        
        bool detectando = estaDetectando();
        Serial.printf("   Leitura %d: %s (pino=%s)\n", 
                     i + 1, 
                     detectando ? "ÍMÃ DETECTADO" : "Normal",
                     digitalRead(pino) ? "HIGH" : "LOW");
        
        delay(500);
    }
    
    Serial.printf("✅ Teste concluído. Total de detecções: %lu\n", contadorDeteccoes);
}