#include "gerenciador_alimentacao.h"
#include <Arduino.h>

GerenciadorAlimentacao::GerenciadorAlimentacao(ServoControl* servoPtr, SensorHall* sensorPtr)
    : servo(servoPtr), sensorHall(sensorPtr), tempoAlimentacaoSegundos(5), 
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
    Serial.println("Alimentacao parada");
}

void GerenciadorAlimentacao::executarCiclo() {
    // Implementação básica do ciclo
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