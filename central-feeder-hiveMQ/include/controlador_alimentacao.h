// controlador_alimentacao.h
#ifndef CONTROLADOR_ALIMENTACAO_H
#define CONTROLADOR_ALIMENTACAO_H

#include <Arduino.h>
#include "gerenciador_tempo.h"
#include "gerenciador_mqtt.h"
#include "sistema_estado.h"

class ControladorAlimentacao {
private:
    static bool _inicializado;
    static unsigned long _ultimaExecucao;
    static int _ultimaHoraExecutada;
    static int _ultimoMinutoExecutado;
    
    static void verificarHorariosAlimentacao();
    static void executarAlimentacao(int idRemota, int indiceRefeicao);

public:
    static void inicializar();
    static void atualizar();
};

#endif