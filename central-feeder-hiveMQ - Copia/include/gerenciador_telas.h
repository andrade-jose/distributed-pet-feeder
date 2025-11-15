#pragma once
#include <Arduino.h>
#include "display.h"
#include "botoes.h"
#include "gerenciador_tempo.h"
#include "sistema_estado.h"
#include "config.h"

// ✅ SIMPLIFICADO: Enumeração dos tipos de tela (apenas essenciais)
enum class TipoTela {
    STATUS_GATEWAY,      // Tela principal: status online/offline + flags
    LISTA_REMOTAS,       // Lista de remotas para configurar
    CONFIG_REFEICAO,     // Configurar horários das refeições
    EDITAR_HORA,         // Editar hora específica
    EDITAR_QUANTIDADE    // Editar quantidade específica
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
    static bool luzLcd;

    // Dados das remotas
    static int indiceRemotaAtual; // índice no estadoSistema.remotas[]
    static int indiceRefeicaoAtual;


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
    static void (*callbackAtualizacaoRefeicao)(int, int, int, int, int);

    // ===== MÉTODOS PRIVADOS =====

    // ✅ SIMPLIFICADO: Renderização (apenas telas essenciais)
    static void renderizarStatusGateway();
    static void renderizarListaRemotas();
    static void renderizarConfigRefeicao();
    static void renderizarEditarHora();
    static void renderizarEditarQuantidade();

    // ✅ SIMPLIFICADO: Navegação (apenas telas essenciais)
    static void navegarStatusGateway();
    static void navegarListaRemotas();
    static void navegarConfigRefeicao();
    static void navegarEditarHora();
    static void navegarEditarQuantidade();

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

    // Gerenciamento de remotas
    static void adicionarRemota(int id, String nome, bool conectada);
    static void atualizarStatusRemota(int id, bool conectada);
    static void definirRefeicao(int idRemota, int indiceRefeicao, int hora, int minuto, int quantidade);
    static void atualizarUltimaExecucao(int idRemota, int indiceRefeicao, String tempo);

    // Configurações
    static void alternarLuzLcd();

    // Verificações automáticas
    static void verificarAlertas();

    // Getters
    static TipoTela obterTelaAtual();
    static bool estaEmModoEdicao();

    // Setters de callbacks
    static void definirCallbackAtualizacaoRefeicao(void (*callback)(int, int, int, int, int));
};