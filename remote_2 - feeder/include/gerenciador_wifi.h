#ifndef GERENCIADOR_WIFI_H
#define GERENCIADOR_WIFI_H

#include <WiFi.h>
#include <Arduino.h>

class WiFiManager {
private:
    const char* ssid;
    const char* password;
    bool conectado;
    unsigned long ultimaTentativa;
    static const unsigned long INTERVALO_RECONEXAO = 30000; // 30 segundos

public:
    WiFiManager(const char* ssid, const char* password);
    
    void iniciar();
    bool conectar();
    void verificarConexao();
    bool estaConectado();
    String obterIP();
    int obterRSSI();
    void desconectar();
};

#endif