#ifndef GERENCIADOR_VPN_H
#define GERENCIADOR_VPN_H

#include <Arduino.h>
#include <WireGuard.h>

class GerenciadorVPN {
private:
    WireGuard wg;
    bool conectado;
    unsigned long ultimaTentativa;
    
public:
    GerenciadorVPN();
    bool inicializar();
    bool estaConectado();
    void atualizar();
    
private:
    bool conectarWireGuard();
};

extern GerenciadorVPN gerenciadorVPN;

#endif