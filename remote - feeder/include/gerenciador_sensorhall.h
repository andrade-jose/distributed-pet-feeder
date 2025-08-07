#ifndef SENSOR_HALL_H
#define SENSOR_HALL_H

#include <Arduino.h>

class SensorHall
{
public:
    SensorHall();
    void iniciar(int pino);
    void resetar();
    int obterContagem();
    bool estaDetectando(); // se ímã está presente agora
    bool estaConectado();  // ver se pino foi configurado corretamente
    void verificar();      // chamada no loop() para detectar e contar

private:
    int pinoSensor;
    int contador;
    bool estadoAnterior;
    bool sensorAtivo;
};

#endif
