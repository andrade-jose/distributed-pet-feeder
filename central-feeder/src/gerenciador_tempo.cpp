#include "gerenciador_tempo.h"

// Inicializar variáveis estáticas
RTC_DS1307 GerenciadorTempo::rtc;
DadosTempo GerenciadorTempo::tempoAtual;
unsigned long GerenciadorTempo::ultimaSincronizacaoNTP = 0;
unsigned long GerenciadorTempo::ultimaAtualizacaoTempo = 0;
bool GerenciadorTempo::sincronizacaoNtpHabilitada = true;
bool GerenciadorTempo::rtcEstaConectado = false;
void (*GerenciadorTempo::callbackAtualizacaoTempo)(DadosTempo) = nullptr;

void GerenciadorTempo::inicializar()
{
    DEBUG_PRINTLN("=== Inicializando Gerenciador de Tempo ===");

    // Inicializar RTC
    if (rtc.begin())
    {
        rtcEstaConectado = true;
    DEBUG_PRINTLN("RTC DS1307 conectado com sucesso");

        // Verificar se RTC está rodando
        if (!rtc.isrunning())
        {
            DEBUG_PRINTLN("RTC não estava rodando, configurando hora atual");
            // Se não está rodando, definir uma hora padrão
            rtc.adjust(DateTime(2025, 8, 1, 12, 0, 0));
        }

    // Ler hora atual do RTC
    atualizarDoRTC();
    }
    else
    {
    rtcEstaConectado = false;
    DEBUG_PRINTLN("ERRO: RTC DS1307 não encontrado!");

    // Usar hora padrão se RTC não funcionar
    tempoAtual.hora = 12;
    tempoAtual.minuto = 0;
        tempoAtual.segundo = 0;
        tempoAtual.dia = 1;
        tempoAtual.mes = 8;
        tempoAtual.ano = 2025;
        atualizarDadosTempo(DateTime(2025, 8, 1, 12, 0, 0));
    }

    // Configurar NTP se WiFi disponÃ­vel
    if (WiFi.status() == WL_CONNECTED && sincronizacaoNtpHabilitada)
    {
        DEBUG_PRINTLN("WiFi conectado, iniciando sincronizaÃ§Ã£o NTP");
        sincronizarComNTP();
    }

    DEBUG_PRINTF("Hora atual: %s %s\n", tempoAtual.dataFormatada.c_str(), tempoAtual.tempoFormatado.c_str());
}

void GerenciadorTempo::atualizar()
{
    unsigned long agora = millis();

    // Atualizar hora a cada segundo
    if (agora - ultimaAtualizacaoTempo >= 1000)
    {
        ultimaAtualizacaoTempo = agora;
        
        // Salvar a hora formatada anterior
        String tempoFormatadoAnterior = tempoAtual.tempoFormatado;
        
        atualizarDoRTC();
        
        // SÃ³ notificar se a hora formatada realmente mudou (minutos)
        if (tempoFormatadoAnterior != tempoAtual.tempoFormatado) {
            notificarAtualizacaoTempo();
        }
    }

    // Verificar se precisa sincronizar com NTP (a cada 2 horas)
    if (sincronizacaoNtpHabilitada && WiFi.status() == WL_CONNECTED && precisaSincronizarNTP())
    {
        DEBUG_PRINTLN("Sincronizando com NTP...");
        sincronizarComNTP();
    }
}

DadosTempo GerenciadorTempo::obterTempoAtual()
{
    return tempoAtual;
}

String GerenciadorTempo::obterTempoFormatado()
{
    return tempoAtual.tempoFormatado;
}

String GerenciadorTempo::obterDataFormatada()
{
    return tempoAtual.dataFormatada;
}

String GerenciadorTempo::obterDataTempoFormatado()
{
    return tempoAtual.dataFormatada + " " + tempoAtual.tempoFormatado;
}

void GerenciadorTempo::definirTempo(int hora, int minuto, int segundo)
{
    if (!tempoValido(hora, minuto, segundo))
    {
        DEBUG_PRINTLN("Hora invÃ¡lida!");
        return;
    }

    DateTime agora = rtc.now();
    DateTime novoTempo(agora.year(), agora.month(), agora.day(), hora, minuto, segundo);

    if (rtcEstaConectado)
    {
        rtc.adjust(novoTempo);
        DEBUG_PRINTF("Hora definida: %02d:%02d:%02d\n", hora, minuto, segundo);
    }

    atualizarDadosTempo(novoTempo);
    notificarAtualizacaoTempo();
}

void GerenciadorTempo::definirData(int dia, int mes, int ano)
{
    if (!dataValida(dia, mes, ano))
    {
        DEBUG_PRINTLN("Data invÃ¡lida!");
        return;
    }

    DateTime agora = rtc.now();
    DateTime novaDataTempo(ano, mes, dia, agora.hour(), agora.minute(), agora.second());

    if (rtcEstaConectado)
    {
        rtc.adjust(novaDataTempo);
        DEBUG_PRINTF("Data definida: %02d/%02d/%d\n", dia, mes, ano);
    }

    atualizarDadosTempo(novaDataTempo);
    notificarAtualizacaoTempo();
}

void GerenciadorTempo::definirDataTempo(int dia, int mes, int ano, int hora, int minuto, int segundo)
{
    if (!dataValida(dia, mes, ano) || !tempoValido(hora, minuto, segundo))
    {
        DEBUG_PRINTLN("Data/hora invÃ¡lida!");
        return;
    }

    DateTime novaDataTempo(ano, mes, dia, hora, minuto, segundo);

    if (rtcEstaConectado)
    {
        rtc.adjust(novaDataTempo);
        DEBUG_PRINTF("Data/hora definida: %02d/%02d/%d %02d:%02d:%02d\n",
                     dia, mes, ano, hora, minuto, segundo);
    }

    atualizarDadosTempo(novaDataTempo);
    notificarAtualizacaoTempo();
}

void GerenciadorTempo::sincronizarComNTP()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        DEBUG_PRINTLN("WiFi nÃ£o conectado, pulando sincronizaÃ§Ã£o NTP");
        return;
    }

    DEBUG_PRINTLN("Iniciando sincronizaÃ§Ã£o NTP...");

    // Configurar NTP
    configTime(NTP_TIMEZONE_OFFSET * 3600, NTP_DAYLIGHT_OFFSET * 3600, NTP_SERVER);

    // Aguardar sincronizaÃ§Ã£o (mÃ¡ximo 10 segundos)
    struct tm infoTempo;
    int tentativas = 0;
    while (!getLocalTime(&infoTempo) && tentativas < 10)
    {
        delay(1000);
        tentativas++;
        DEBUG_PRINT(".");
    }

    if (getLocalTime(&infoTempo))
    {
        // Converter para DateTime e atualizar RTC
        DateTime tempoNtp(infoTempo.tm_year + 1900,
                         infoTempo.tm_mon + 1,
                         infoTempo.tm_mday,
                         infoTempo.tm_hour,
                         infoTempo.tm_min,
                         infoTempo.tm_sec);

        if (rtcEstaConectado)
        {
            rtc.adjust(tempoNtp);
        }

        atualizarDadosTempo(tempoNtp);
        ultimaSincronizacaoNTP = millis();

        DEBUG_PRINTLN("\nSincronização NTP bem-sucedida!");
        DEBUG_PRINTF("Nova hora: %s %s\n", tempoAtual.dataFormatada.c_str(), tempoAtual.tempoFormatado.c_str());

        notificarAtualizacaoTempo();
    }
    else
    {
        DEBUG_PRINTLN("\nFalha na sincronização NTP");
    }
}

bool GerenciadorTempo::sincronizacaoNTPHabilitada()
{
    return sincronizacaoNtpHabilitada;
}

void GerenciadorTempo::habilitarSincronizacaoNTP(bool habilitar)
{
    sincronizacaoNtpHabilitada = habilitar;
    DEBUG_PRINTF("Sincronização NTP %s\n", habilitar ? "habilitada" : "desabilitada");
}

unsigned long GerenciadorTempo::obterUltimaSincronizacaoNTP()
{
    return ultimaSincronizacaoNTP;
}

bool GerenciadorTempo::precisaSincronizarNTP()
{
    if (!sincronizacaoNtpHabilitada || ultimaSincronizacaoNTP == 0)
    {
        return true; // Primeira sincronizaÃ§Ã£o
    }

    return (millis() - ultimaSincronizacaoNTP) >= NTP_UPDATE_INTERVAL;
}

bool GerenciadorTempo::rtcConectado()
{
    return rtcEstaConectado;
}

bool GerenciadorTempo::tempoValido()
{
    return rtcEstaConectado && (tempoAtual.ano >= 2024);
}

String GerenciadorTempo::obterStringStatus()
{
    String status = "";

    if (!rtcEstaConectado)
    {
        status += "RTC: Desconectado ";
    }
    else
    {
        status += "RTC: OK ";
    }

    if (!tempoValido())
    {
        status += "Hora: Inválida ";
    }
    else
    {
        status += "Hora: OK ";
    }

    if (sincronizacaoNtpHabilitada)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            unsigned long tempoDesdeSync = (millis() - ultimaSincronizacaoNTP) / 1000 / 60; // minutos
            status += "NTP: " + String(tempoDesdeSync) + "min atras";
        }
        else
        {
            status += "NTP: WiFi desconectado";
        }
    }
    else
    {
        status += "NTP: Desabilitado";
    }

    return status;
}

void GerenciadorTempo::definirCallbackAtualizacaoTempo(void (*callback)(DadosTempo))
{
    callbackAtualizacaoTempo = callback;
}

void GerenciadorTempo::atualizarDoRTC()
{
    if (rtcEstaConectado)
    {
        DateTime agora = rtc.now();
        atualizarDadosTempo(agora);
    }
}

void GerenciadorTempo::atualizarDadosTempo(DateTime dt)
{
    tempoAtual.hora = dt.hour();
    tempoAtual.minuto = dt.minute();
    tempoAtual.segundo = dt.second();
    tempoAtual.dia = dt.day();
    tempoAtual.mes = dt.month();
    tempoAtual.ano = dt.year();

    // Formatar strings - hora apenas com HH:MM (sem segundos)
    tempoAtual.tempoFormatado = formatarDoisDigitos(tempoAtual.hora) + ":" +
                                formatarDoisDigitos(tempoAtual.minuto);

    tempoAtual.dataFormatada = formatarDoisDigitos(tempoAtual.dia) + "/" +
                               formatarDoisDigitos(tempoAtual.mes) + "/" +
                               String(tempoAtual.ano);

    tempoAtual.diaSemana = obterNomeDiaSemana(dt.dayOfTheWeek());
}

String GerenciadorTempo::obterNomeDiaSemana(int diaSemana)
{
    String dias[] = {"Domingo", "Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sábado"};
    if (diaSemana >= 0 && diaSemana <= 6)
    {
        return dias[diaSemana];
    }
    return "Inválido";
}

String GerenciadorTempo::formatarDoisDigitos(int valor)
{
    return (valor < 10) ? "0" + String(valor) : String(valor);
}

void GerenciadorTempo::notificarAtualizacaoTempo()
{
    // Chamar callback personalizado se definido
    if (callbackAtualizacaoTempo)
    {
        callbackAtualizacaoTempo(tempoAtual);
    }
}

bool GerenciadorTempo::tempoValido(int hora, int minuto, int segundo)
{
    return (hora >= 0 && hora <= 23) &&
           (minuto >= 0 && minuto <= 59) &&
           (segundo >= 0 && segundo <= 59);
}

bool GerenciadorTempo::dataValida(int dia, int mes, int ano)
{
    return (dia >= 1 && dia <= 31) &&
           (mes >= 1 && mes <= 12) &&
           (ano >= 2024 && ano <= 2099);
}