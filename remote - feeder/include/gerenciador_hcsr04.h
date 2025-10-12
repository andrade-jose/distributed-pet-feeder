#ifndef GERENCIADOR_HCSR04_H
#define GERENCIADOR_HCSR04_H

#include <Arduino.h>


#include "gerenciador_mqtt.h"

class GerenciadorHCSR04 {
public:
    GerenciadorHCSR04(uint8_t triggerPin, uint8_t echoPin, float distanciaLimite, MQTTManager* mqttManager, const char* topicoAlerta);
    void iniciar();
    float medirDistancia();
    void monitorar();
    float getDistanciaLimite() const { return _distanciaLimite; }
private:
    uint8_t _triggerPin;
    uint8_t _echoPin;
    float _distanciaLimite;
    MQTTManager* _mqttManager;
    const char* _topicoAlerta;
    bool _alertaEnviado;
};

#endif // GERENCIADOR_HCSR04_H
