#ifndef GERENCIADOR_SISTEMA_H
#define GERENCIADOR_SISTEMA_H

#include "gerenciador_servo.h"
#include "gerenciador_sensorhall.h"
#include "gerenciador_wifi.h"
#include "gerenciador_mqtt.h"

class GerenciadorSistema {
private:
    ServoControl* servo;
    SensorHall* sensorHall;
    WiFiManager* wifiManager;
    MQTTManager* mqttManager;
    
    int pinoBotao;
    int pinoLedStatus;
    
    // Variáveis do botão
    bool estadoBotaoAnterior;
    unsigned long ultimoDebounce;
    bool servoTravado;
    
    // Variáveis do LED
    unsigned long ultimoPiscada;
    bool estadoLed;
    
    // Monitoramento
    unsigned long ultimoMonitoramento;
    
    static const unsigned long DEBOUNCE_DELAY = 50;
    static const unsigned long INTERVALO_MONITORAMENTO = 1000;

public:
    GerenciadorSistema(ServoControl* servoPtr, SensorHall* sensorPtr, 
                      WiFiManager* wifiPtr, MQTTManager* mqttPtr,
                      int pinoBotao, int pinoLed);
    
    void inicializar();
    void exibirInformacoes();
    void configurarHardware();
    void inicializarComponentes(int pinoServo, int pinoHall);
    void inicializarComunicacao();
    void conectarServicos(const char* topicComando);
    void testarComponentes();
    void finalizarInicializacao();
    
    void verificarConexoes();
    void processarBotao();
    void atualizarLedStatus();
    void monitorarSistema();
    void delay(int ms);
    
    bool getServoTravado() const;
};

#endif