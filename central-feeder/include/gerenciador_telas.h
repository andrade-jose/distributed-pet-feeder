#pragma once

#include <Arduino.h>
#include "display.h"
#include "botoes.h"
#include "gerenciador_tempo.h"
#include "gerenciador_mqtt.h"
#include "config.h"

// Enumeração dos tipos de tela
enum class TipoTela
{
    INICIO,
    INFO_WIFI,
    CONFIG_CENTRAL,
    CONFIG_ULTIMO_BOOT,
    CONFIG_RESETAR,
    LISTA_REMOTAS_P1,
    LISTA_REMOTAS_P2,
    REMOTA_ESPECIFICA,
    CONFIG_REFEICAO,
    EDITAR_HORA,
    EDITAR_QUANTIDADE,
    ALERTA_RACAO_BAIXA  // Já existe - não precisa adicionar
};

// Estrutura para dados de uma refeição
struct Refeicao
{
    int hora;
    int minuto;
    int quantidade;        // em gramas
    String ultimaExecucao; // "07:45" ou "Nunca"
};

// Estrutura para dados de uma remota
struct Remota
{
    int id;
    String nome;
    bool conectada;
    unsigned long ultimoHeartbeat; // Timestamp do último heartbeat recebido
    String nivelRacao;            // Nível atual da ração ("OK", "BAIXO", etc)
    Refeicao refeicoes[REFEICOES_PER_REMOTA];
};

// Classe principal do gerenciador de telas
class GerenciadorTelas
{
private:
    // Flag para indicar necessidade de atualização de tela
    static bool precisaRedraw;
    // Estado da aplicação
    static TipoTela telaAtual;
    static TipoTela telaAnterior;
    static int opcaoSelecionada;
    static int paginaAtual;  // Índice da página atual (0, 1, 2...)
    static int campoEdicao;
    static bool estaEditando;

    // Sistema de paginação dinâmica para remotas
    static int remotasConectadasCache[MAX_REMOTAS];  // Índices das remotas conectadas
    static int numRemotasConectadasCache;            // Total de remotas conectadas

    // Dados do sistema
    static String wifiSSID;
    static bool wifiConectado;
    static String qualidadeWifi;
    static bool mqttConectado;
    static bool luzLcd;
    static String dataUltimoBooter;
    static String horaUltimoBooter;

    // Dados das remotas
    static Remota remotas[MAX_REMOTAS];
    static int remotaAtual;
    static int refeicaoAtual;
    static int numeroRemotas;

    // Controle de tempo
    static unsigned long ultimaAtualizacao;
    static unsigned long inicioTela;
    static bool timeoutHabilitado;
    
    // Controle de verificação automática de horários
    static unsigned long ultimaVerificacaoHorarios;

    // Controle de sistema de alerta de ração
    static bool alertaRacaoAtivo;
    static int remotaComAlerta;
    static unsigned long ultimaPiscadaAlerta;
    static bool estadoPiscaAlerta;

    // Métodos privados para renderização
    static void renderizarInicio();
    static void renderizarInfoWifi();
    static void renderizarConfigCentral();
    static void renderizarUltimoBooter();
    static void renderizarResetar();
    static void renderizarListaRemotasP1();
    static void renderizarListaRemotasP2();
    static void renderizarListaRemotasPaginada();  // Nova função genérica
    static void renderizarRemotaEspecifica();
    static void renderizarConfigRefeicao();
    static void renderizarEditarHora();
    static void renderizarEditarQuantidade();
    static void renderizarAlertaRacao();
    static void navegarAlertaRacao();

    // Métodos auxiliares para paginação
    static void atualizarCacheRemotasConectadas();
    static int calcularTotalPaginas();
    static int obterRemotasVisiveis(int* indicesRemotas, int maxRemotas);

    // Métodos privados para navegação
    static void navegarInicio();
    static void navegarInfoWifi();
    static void navegarConfigCentral();
    static void navegarUltimoBooter();
    static void navegarResetar();
    static void navegarListaRemotasP1();
    static void navegarListaRemotasP2();
    static void navegarListaRemotasPaginada();  // Nova função genérica
    static void navegarRemotaEspecifica();
    static void navegarConfigRefeicao();
    static void navegarEditarHora();
    static void navegarEditarQuantidade();

    // Métodos utilitários
    static void resetarSelecao();
    static void irParaTela(TipoTela tela);
    static void voltar();
    static String formatarHora(int hora, int minuto);
    static String obterStatusRemota(int indice);
    static void atualizarBarraProgresso(int progresso);
    static void centralizarTexto(int linha, String texto);
    static void desenharSeletor(int linha);
    static bool verificarTimeout();

    // Métodos para persistência
    static void salvarDadosRemota();
    static void carregarDadosRemota();

    // Callbacks
    static void (*callbackResetSistema)();
    static void (*callbackAtualizacaoRefeicao)(int, int, int, int, int);
    
    // Método auxiliar para verificar timeout de heartbeat
    static void verificarTimeoutRemotas(unsigned long agora);

public:
    // Inicialização
    static void inicializar();

    // Loop principal
    static void atualizar();

    // Renderização
    static void renderizar();

    // Navegação
    static void gerenciarNavegacao();

    // Atualização de dados externos
    static void atualizarStatusWifi(String ssid, bool conectado, String qualidade);
    static void atualizarStatusMqtt(bool conectado);
    static void atualizarTempo();
    static void atualizarTempoBooter(String data, String hora);
    
    // Verificação automática de horários
    static void verificarHorariosAutomaticos();

    // Controle de remotas
    static void adicionarRemota(int id, String nome, bool conectada);
    static void atualizarStatusRemota(int id, bool conectada);
    static void definirRefeicao(int idRemota, int indiceRefeicao, int hora, int minuto, int quantidade);
    static void atualizarUltimaExecucao(int idRemota, int indiceRefeicao, String tempo);

    // Configurações
    static void alternarLuzLcd();
    static void resetarSistema();

    // Estado atual
    static TipoTela obterTelaAtual();
    static bool estaEmModoEdicao();

    // Callback para ações do sistema
    static void definirCallbackResetSistema(void (*callback)());
    static void definirCallbackAtualizacaoRefeicao(void (*callback)(int, int, int, int, int));
    
    // Callbacks MQTT - DECLARAÇÕES ADICIONADAS
    static void onStatusRemotaRecebido(int idRemota, const String& status);
    static void onVidaRemotaRecebida(int idRemota, const String& status);
    static void onRespostaRemotaRecebida(int idRemota, const String& resposta);
    static void onAlertaRacaoRecebido(int idRemota, const String& alerta);
    
    // Outros métodos já existentes
    static void verificarAlertas();
    static void sincronizarComPreferences();
};