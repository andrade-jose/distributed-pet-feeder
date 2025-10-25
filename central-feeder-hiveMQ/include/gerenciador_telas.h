#pragma once
#include <Arduino.h>
#include "display.h"
#include "botoes.h"
#include "gerenciador_tempo.h"
#include "sistema_estado.h"
#include "config.h"

// Enumeração dos tipos de tela
enum class TipoTela {
    INICIO,
    INFO_WIFI,
    CONFIG_CENTRAL,
    CONFIG_ULTIMO_BOOT,
    CONFIG_RESETAR,
    LISTA_REMOTAS,
    BUSCAR_REMOTA,
    REMOTA_ESPECIFICA,
    CONFIG_REFEICAO,
    EDITAR_HORA,
    EDITAR_QUANTIDADE,
    ALERTA_RACAO_BAIXA
};


class GerenciadorTelas {
private:
    // Estado da tela
    static TipoTela telaAtual;
    static TipoTela telaAnterior;
    static int opcaoSelecionada;
    static int campoEdicao;
    static bool estaEditando;
    static bool precisaRedraw;

    // Dados do sistema
    static String wifiSSID;
    static bool wifiConectado;
    static String qualidadeWifi;
    static bool mqttConectado;
    static bool luzLcd;
    static String dataUltimoBooter;
    static String horaUltimoBooter;
    

    // Dados das remotas
    static int indiceRemotaAtual; // índice no estadoSistema.remotas[]
    static int indiceRefeicaoAtual;
    static int idRemotaBusca; // ID da remota sendo buscada (1-6)


    // Controle de tempo
    static unsigned long ultimaAtualizacao;
    static unsigned long inicioTela;
    static bool timeoutHabilitado;
    static unsigned long ultimaVerificacaoHorarios;

    // Sistema de alerta
    static bool alertaRacaoAtivo;
    static int remotaComAlerta;
    static unsigned long ultimaPiscadaAlerta;
    static bool estadoPiscaAlerta;

    // Callbacks
    static void (*callbackResetSistema)();
    static void (*callbackAtualizacaoRefeicao)(int, int, int, int, int);

    // ===== MÉTODOS PRIVADOS =====
    
    // Renderização
    static void renderizarInicio();
    static void renderizarInfoWifi();
    static void renderizarConfigCentral();
    static void renderizarUltimoBooter();
    static void renderizarResetar();
    static void renderizarListaRemotas();
    static void renderizarBuscarRemota();
    static void renderizarRemotaEspecifica();
    static void renderizarConfigRefeicao();
    static void renderizarEditarHora();
    static void renderizarEditarQuantidade();
    static void renderizarAlertaRacao();

    // Navegação
    static void navegarInicio();
    static void navegarInfoWifi();
    static void navegarConfigCentral();
    static void navegarUltimoBooter();
    static void navegarResetar();
    static void navegarListaRemotas();
    static void navegarBuscarRemota();
    static void navegarRemotaEspecifica();
    static void navegarConfigRefeicao();
    static void navegarEditarHora();
    static void navegarEditarQuantidade();
    static void navegarAlertaRacao();

    // Utilitários
    static void resetarSelecao();
    static void irParaTela(TipoTela tela);
    static void voltar();
    static String formatarHora(int hora, int minuto);
    static String formatarQuantidade(int quantidade);
    static void centralizarTexto(int linha, String texto);
    static bool verificarTimeout();
    
    // Gerenciamento de remotas
    static void atualizarCacheRemotasConectadas(int* indices, int* count);
    static int calcularOpcoesRemotas(int numConectadas);
    
    // Persistência
    static void salvarDadosRemota();
    static void carregarDadosRemota();

public:
    // ===== MÉTODOS PÚBLICOS =====
    
    // Gerenciamento principal
    static void inicializar();
    static void atualizar();
    static void renderizar();
    static void gerenciarNavegacao();

    // Atualização de status
    static void atualizarTempo();
    static void atualizarTempoBooter(String data, String hora);

    // Gerenciamento de remotas
    static void adicionarRemota(int id, String nome, bool conectada);
    static void atualizarStatusRemota(int id, bool conectada);
    static void definirRefeicao(int idRemota, int indiceRefeicao, int hora, int minuto, int quantidade);
    static void atualizarUltimaExecucao(int idRemota, int indiceRefeicao, String tempo);

    // Configurações
    static void alternarLuzLcd();
    static void resetarSistema();

    // Verificações automáticas
    static void verificarAlertas();

    // Getters
    static TipoTela obterTelaAtual();
    static bool estaEmModoEdicao();

    // Setters de callbacks
    static void definirCallbackResetSistema(void (*callback)());
    static void definirCallbackAtualizacaoRefeicao(void (*callback)(int, int, int, int, int));
};