#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
private:
    const char* ssid;
    const char* password;
    bool conectado;
    unsigned long ultimaTentativa;
    const unsigned long INTERVALO_RECONEXAO = 5000;

public:
    WiFiManager(const char* ssid, const char* password);
    void iniciar();
    bool conectar();
    void verificarConexao();
    bool estaConectado();
    String obterIP();
    void desconectar();
};

#endif
