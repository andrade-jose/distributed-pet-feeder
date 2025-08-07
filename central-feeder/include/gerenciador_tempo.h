#ifndef GERENCIADOR_TEMPO_H
#define GERENCIADOR_TEMPO_H

#include <Arduino.h>
#include <RTClib.h>
#include <WiFi.h>
#include <time.h>
#include "config.h"

struct DadosTempo {
    int hora;
    int minuto; 
    int segundo;
    int dia;
    int mes;
    int ano;
    String tempoFormatado;    // "14:35"
    String dataFormatada;     // "01/08/2025"
    String diaSemana;         // "Quinta"
};

class GerenciadorTempo {
public:
    static void inicializar();
    static void atualizar();
    static DadosTempo obterTempoAtual();
    static String obterTempoFormatado();
    static String obterDataFormatada();
    static String obterDataTempoFormatado();
    
    // Configuração manual de hora
    static void definirTempo(int hora, int minuto, int segundo);
    static void definirData(int dia, int mes, int ano);
    static void definirDataTempo(int dia, int mes, int ano, int hora, int minuto, int segundo);
    
    // Sincronização NTP
    static void sincronizarComNTP();
    static bool sincronizacaoNTPHabilitada();
    static void habilitarSincronizacaoNTP(bool habilitar);
    static unsigned long obterUltimaSincronizacaoNTP();
    static bool precisaSincronizarNTP();
    
    // Status
    static bool rtcConectado();
    static bool tempoValido();
    static String obterStringStatus();
    
    // Callback para notificação de mudança
    static void definirCallbackAtualizacaoTempo(void (*callback)(DadosTempo));
    
private:
    // Hardware
    static RTC_DS1307 rtc;
    
    // Dados atuais
    static DadosTempo tempoAtual;
    
    // Controle de sincronização
    static unsigned long ultimaSincronizacaoNTP;
    static unsigned long ultimaAtualizacaoTempo;
    static bool sincronizacaoNtpHabilitada;
    static bool rtcEstaConectado;
    
    // Callback
    static void (*callbackAtualizacaoTempo)(DadosTempo);
    
    // Métodos privados
    static void atualizarDoRTC();
    static void atualizarDadosTempo(DateTime dt);
    static String obterNomeDiaSemana(int diaSemana);
    static String formatarDoisDigitos(int valor);
    static void notificarAtualizacaoTempo();
    static bool tempoValido(int hora, int minuto, int segundo);
    static bool dataValida(int dia, int mes, int ano);
};

#endif
