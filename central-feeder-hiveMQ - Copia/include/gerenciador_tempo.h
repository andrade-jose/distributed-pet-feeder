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
    String tempoFormatado;
    String dataFormatada;
    String diaSemana;
};

class GerenciadorTempo {
public:
    static void inicializar();
    static void atualizar();
    static DadosTempo obterTempoAtual();
    static String obterTempoFormatado();
    static String obterDataFormatada();
    static String obterDataTempoFormatado();
    
    static void definirTempo(int hora, int minuto, int segundo);
    static void definirData(int dia, int mes, int ano);
    static void definirDataTempo(int dia, int mes, int ano, int hora, int minuto, int segundo);
    
    // ✅ APENAS o método - não a variável
    static bool rtcEstaConectado();

    static void sincronizarComNTP();
    static bool sincronizacaoNTPHabilitada();
    static void habilitarSincronizacaoNTP(bool habilitar);
    static unsigned long obterUltimaSincronizacaoNTP();
    static bool precisaSincronizarNTP();
    
    static bool tempoValido();
    static void definirCallbackAtualizacaoTempo(void (*callback)(DadosTempo));
    
    // Métodos públicos de formatação
    static String formatarDoisDigitos(int valor);
    static String formatarNumeroComZeros(int valor, int digitos);
    
private:
    static RTC_DS1307 rtc;
    static DadosTempo tempoAtual;
    static unsigned long ultimaSincronizacaoNTP;
    static unsigned long ultimaAtualizacaoTempo;
    static bool sincronizacaoNtpHabilitada;
    static bool _rtcEstaConectado;  // ✅ Variável privada com nome diferente
    static void (*callbackAtualizacaoTempo)(DadosTempo);
    
    static void atualizarDoRTC();
    static void atualizarDadosTempo(DateTime dt);
    static String obterNomeDiaSemana(int diaSemana);
    static void notificarAtualizacaoTempo();
    static bool tempoValido(int hora, int minuto, int segundo);
    static bool dataValida(int dia, int mes, int ano);
};

#endif