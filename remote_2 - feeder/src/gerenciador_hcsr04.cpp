
#include "gerenciador_hcsr04.h"

GerenciadorHCSR04::GerenciadorHCSR04(uint8_t triggerPin, uint8_t echoPin, float distanciaLimite, MQTTManager* mqttManager, const char* topicoAlerta)
    : _triggerPin(triggerPin), _echoPin(echoPin), _distanciaLimite(distanciaLimite), _mqttManager(mqttManager), _topicoAlerta(topicoAlerta), _alertaEnviado(false) {}

void GerenciadorHCSR04::iniciar() {
    pinMode(_triggerPin, OUTPUT);
    pinMode(_echoPin, INPUT);
}

float GerenciadorHCSR04::medirDistancia() {
    digitalWrite(_triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(_triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_triggerPin, LOW);
    long duracao = pulseIn(_echoPin, HIGH, 30000); // timeout 30ms
    float distancia = duracao * 0.034 / 2;
    return distancia;
}

void GerenciadorHCSR04::monitorar() {
    float distancia = medirDistancia();
    Serial.printf("[HC-SR04] Distância medida: %.2f cm\n", distancia);
    if (distancia >= _distanciaLimite) {
        if (!_alertaEnviado && _mqttManager) {
            _mqttManager->publicar(_topicoAlerta, "Nivel de ração baixo detectado pelo sensor ultrassônico.");
            _alertaEnviado = true;
        }
    } else {
        _alertaEnviado = false;
    }
}
