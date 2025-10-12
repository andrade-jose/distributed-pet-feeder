#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <Arduino.h>
#include <ESP32Servo.h>

class ServoControl
{
public:
    ServoControl();
    void iniciar(int pino);
    void ativar();
    void desativar();
    void girarHorarioComFlag(int velocidade = 100); // 0–100%
    void girarHorario(int velocidade = 100);        // versão silenciosa
    void moverPara90AntiHorario();                  // volta para neutro 90
    void parar();
    void pararImediato();                           // para sem delay
    void testar();
    bool estaAtivo();
    void moverParaAngulo(int angulo);               // Move para ângulo específico (0-180°)
    void alimentar(int porcao);                     

private:
    Servo servo;
    int pinoServo;
    bool ativo;
    int pwmParaVelocidade(int percentual, bool horario);
};

#endif