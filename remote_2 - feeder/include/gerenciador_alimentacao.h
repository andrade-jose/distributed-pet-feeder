#ifndef GERENCIADOR_ALIMENTACAO_H
#define GERENCIADOR_ALIMENTACAO_H

#include "gerenciador_servo.h"
#include "gerenciador_sensorhall.h"

class GerenciadorAlimentacao {
private:
    ServoControl* servo;
    SensorHall* sensorHall;
    
    // Variáveis de controle
    int tempoAlimentacaoSegundos;
    unsigned long inicioAlimentacao;
    bool alimentacaoAtiva;
    bool servoAberto;
    unsigned long ultimoMovimento;
    bool servoTravado;
    String idComandoAtual;
    
    static const unsigned long TEMPO_MOVIMENTO = 2000;

public:
    GerenciadorAlimentacao(ServoControl* servoPtr, SensorHall* sensorPtr);
    
    void iniciar(int tempoSegundos);
    void parar();
    void executarCiclo();
    void pausar();
    void retomar();
    
    bool estaAtivo() const;
    bool estaServoPausado() const;
    void setServoTravado(bool travado);
    void setIdComando(const String& id);
    
    int getTempoDecorrido() const;
    int getTempoRestante() const;
    String getIdComando() const;
};

#endif