#include "gerenciador_alimentacao.h"
#include <Arduino.h>

GerenciadorAlimentacao::GerenciadorAlimentacao(ServoControl* servoPtr)
    : servo(servoPtr), tempoAlimentacaoSegundos(5),
      inicioAlimentacao(0), alimentacaoAtiva(false), servoAberto(false),
      ultimoMovimento(0), servoTravado(false), idComandoAtual("") {
}

void GerenciadorAlimentacao::iniciar(int tempoSegundos) {
    // Implementação básica
    tempoAlimentacaoSegundos = tempoSegundos;
    alimentacaoAtiva = true;
    inicioAlimentacao = millis();
    Serial.printf("Alimentacao iniciada: %d segundos\n", tempoSegundos);
}

void GerenciadorAlimentacao::parar() {
    alimentacaoAtiva = false;
    servo->parar();
    delay(200);
    servo->moverParaAngulo(0);
    servoAberto = false;
    Serial.println("⏹️  Alimentação parada manualmente");
    Serial.println("🔙 Servo retornou para posição inicial (0°)");
}

void GerenciadorAlimentacao::executarCiclo() {
    if (!alimentacaoAtiva) return;

    unsigned long agora = millis();
    unsigned long tempoDecorrido = (agora - inicioAlimentacao) / 1000;

    // Verificar se o tempo de alimentação já passou
    if (tempoDecorrido >= tempoAlimentacaoSegundos) {
        alimentacaoAtiva = false;
        servo->parar();

        // Voltar para posição inicial (0 graus)
        delay(200);
        servo->moverParaAngulo(0);
        servoAberto = false;

        Serial.printf("✅ Alimentação concluída: %d segundos [ID: %s]\n",
                     tempoAlimentacaoSegundos, idComandoAtual.c_str());
        Serial.println("🔙 Servo retornou para posição inicial (0°)");
        return;
    }

    // Se está ativo e não está travado, girar o servo
    if (!servoTravado) {
        if (!servoAberto) {
            servo->ativar();
            servo->girarHorario(100); // velocidade máxima
            servoAberto = true;
            Serial.println("🔄 Servo iniciado (girando)");
        }
    } else {
        // Se está travado, parar o servo
        if (servoAberto) {
            servo->parar();
            servoAberto = false;
            Serial.println("⏸️  Servo pausado (travado)");
        }
    }
}

void GerenciadorAlimentacao::pausar() {
    servoTravado = true;
}

void GerenciadorAlimentacao::retomar() {
    servoTravado = false;
}

bool GerenciadorAlimentacao::estaAtivo() const {
    return alimentacaoAtiva;
}

bool GerenciadorAlimentacao::estaServoPausado() const {
    return servoTravado;
}

void GerenciadorAlimentacao::setServoTravado(bool travado) {
    servoTravado = travado;
}

void GerenciadorAlimentacao::setIdComando(const String& id) {
    idComandoAtual = id;
}

int GerenciadorAlimentacao::getTempoDecorrido() const {
    if (!alimentacaoAtiva) return 0;
    return (millis() - inicioAlimentacao) / 1000;
}

int GerenciadorAlimentacao::getTempoRestante() const {
    if (!alimentacaoAtiva) return 0;
    int decorrido = getTempoDecorrido();
    return (decorrido < tempoAlimentacaoSegundos) ? (tempoAlimentacaoSegundos - decorrido) : 0;
}

String GerenciadorAlimentacao::getIdComando() const {
    return idComandoAtual;
}