#ifndef GERENCIADOR_SENSORHALL_H
#define GERENCIADOR_SENSORHALL_H

#include <Arduino.h>

class SensorHall {
private:
    int pino;
    bool estadoAtual;
    bool estadoAnterior;
    unsigned long ultimaLeitura;
    unsigned long contadorDeteccoes;
    bool detectandoAtualmente;
    
    static const unsigned long INTERVALO_LEITURA = 50; // 50ms entre leituras

public:
    SensorHall();
    
    void iniciar(int pino);
    void verificar();
    bool estaDetectando();
    bool mudouEstado();
    unsigned long obterContador();
    void resetarContador();
    void testar();
};

#endif