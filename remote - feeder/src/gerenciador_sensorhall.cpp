#include "SensorHall.h"

SensorHall::SensorHall()
{
    pinoSensor = -1;
    contador = 0;
    estadoAnterior = false;
    sensorAtivo = false;
}

void SensorHall::iniciar(int pino)
{
    pinoSensor = pino;
    pinMode(pinoSensor, INPUT_PULLUP);
    sensorAtivo = true;
    Serial.printf("🧲 Sensor Hall iniciado no pino GPIO %d\n", pinoSensor);
}

void SensorHall::resetar()
{
    contador = 0;
}

int SensorHall::obterContagem()
{
    return contador;
}

bool SensorHall::estaDetectando()
{
    if (pinoSensor == -1)
        return false;
    return digitalRead(pinoSensor) == LOW;
}

bool SensorHall::estaConectado()
{
    return (pinoSensor != -1 && sensorAtivo);
}

void SensorHall::verificar()
{
    if (!sensorAtivo)
        return;

    bool estadoAtual = digitalRead(pinoSensor) == LOW;

    if (estadoAtual && !estadoAnterior)
    {
        contador++;
        Serial.printf("🔄 Ímã detectado! Contador: %d\n", contador);
    }

    estadoAnterior = estadoAtual;
}
