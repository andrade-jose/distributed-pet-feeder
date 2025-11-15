#include "gerenciador_telas.h"
#include <config.h>
#include <Preferences.h>
#include "gerenciador_mqtt.h"
#include "sistema_estado.h"

// Inicialização de variáveis estáticas
TipoTela GerenciadorTelas::telaAtual = TipoTela::STATUS_GATEWAY;
TipoTela GerenciadorTelas::telaAnterior = TipoTela::STATUS_GATEWAY;
int GerenciadorTelas::opcaoSelecionada = 0;
int GerenciadorTelas::campoEdicao = 0;
bool GerenciadorTelas::estaEditando = false;
bool GerenciadorTelas::precisaRedraw = false;

// Dados das remotas
int GerenciadorTelas::indiceRemotaAtual = 0;
int GerenciadorTelas::indiceRefeicaoAtual = 0;

// Dados do sistema
bool GerenciadorTelas::luzLcd = true;

// Controle de tempo
unsigned long GerenciadorTelas::ultimaAtualizacao = 0;
unsigned long GerenciadorTelas::inicioTela = 0;
bool GerenciadorTelas::timeoutHabilitado = false;
unsigned long GerenciadorTelas::ultimaVerificacaoHorarios = 0;

// Sistema de alerta
bool GerenciadorTelas::alertaRacaoAtivo = false;
int GerenciadorTelas::remotaComAlerta = -1;
unsigned long GerenciadorTelas::ultimaPiscadaAlerta = 0;
bool GerenciadorTelas::estadoPiscaAlerta = false;

// Callbacks
void (*GerenciadorTelas::callbackAtualizacaoRefeicao)(int, int, int, int, int) = nullptr;

// =============================================================================
// INICIALIZAÇÃO E LOOP PRINCIPAL
// =============================================================================

void GerenciadorTelas::inicializar() {
    Serial.println("=== Inicializando Gerenciador de Telas ===");

    Display::init();
    Display::showWelcome();
    delay(2000);


    if (estadoSistema.numRemotas == 0) {
        for (int i = 1; i <= MAX_REMOTAS; i++) {
            estadoSistema.adicionarRemota(i);
        }
    }

    GerenciadorTempo::definirCallbackAtualizacaoTempo([](DadosTempo tempo) {
        GerenciadorTelas::atualizarTempo();
    });

    // ✅ ADICIONAR: Registrar callbacks de eventos
    estadoSistema.onWifiMudou = [](bool conectado) {
        GerenciadorTelas::precisaRedraw = true;
        Serial.printf("[TELAS] WiFi mudou: %s\n", conectado ? "ON" : "OFF");
    };
    
    estadoSistema.onMqttMudou = [](bool conectado) {
        GerenciadorTelas::precisaRedraw = true;
        Serial.printf("[TELAS] MQTT mudou: %s\n", conectado ? "ON" : "OFF");
    };
    
    estadoSistema.onRemotaMudou = [](int id, bool conectada) {
        GerenciadorTelas::precisaRedraw = true;
        Serial.printf("[TELAS] Remota %d mudou: %s\n", id, conectada ? "ON" : "OFF");
    };

    // ✅ MUDANÇA: Ir para STATUS_GATEWAY ao invés de INICIO
    irParaTela(TipoTela::STATUS_GATEWAY);
    Serial.println("Gerenciador de Telas inicializado");
}

void GerenciadorTelas::atualizar() {
    unsigned long agora = millis();
    
    if (agora - ultimaAtualizacao > 100) {
        ultimaAtualizacao = agora;
        
        // Verificações periódicas necessárias
        verificarAlertas();
        
        // Verificar timeout de tela
        if (timeoutHabilitado && verificarTimeout()) {
            irParaTela(TipoTela::STATUS_GATEWAY);
            return;
        }
        
        // Processar navegação
        gerenciarNavegacao();
        
        // Renderizar se necessário
        if (precisaRedraw) {
            renderizar();
            precisaRedraw = false;
        }
    }
}

// ✅ SIMPLIFICADO: Renderizar apenas telas essenciais
void GerenciadorTelas::renderizar() {
    Display::clear();

    switch (telaAtual) {
        case TipoTela::STATUS_GATEWAY:
            renderizarStatusGateway();
            break;
        case TipoTela::LISTA_REMOTAS:
            renderizarListaRemotas();
            break;
        case TipoTela::CONFIG_REFEICAO:
            renderizarConfigRefeicao();
            break;
        case TipoTela::EDITAR_HORA:
            renderizarEditarHora();
            break;
        case TipoTela::EDITAR_QUANTIDADE:
            renderizarEditarQuantidade();
            break;
    }
}

// ✅ SIMPLIFICADO: Navegar apenas telas essenciais
void GerenciadorTelas::gerenciarNavegacao() {
    switch (telaAtual) {
        case TipoTela::STATUS_GATEWAY: navegarStatusGateway(); break;
        case TipoTela::LISTA_REMOTAS: navegarListaRemotas(); break;
        case TipoTela::CONFIG_REFEICAO: navegarConfigRefeicao(); break;
        case TipoTela::EDITAR_HORA: navegarEditarHora(); break;
        case TipoTela::EDITAR_QUANTIDADE: navegarEditarQuantidade(); break;
    }
}

// =============================================================================
// RENDERIZAÇÃO DAS TELAS
// =============================================================================

// ✅ NOVO: Tela de status do gateway (substitui INICIO)
void GerenciadorTelas::renderizarStatusGateway() {
    // Linha 0: Título
    centralizarTexto(0, "GATEWAY CENTRAL");

    // Linha 1: Status conexão MQTT
    String statusConexao = estadoSistema.mqttConectado ? "ONLINE " : "OFFLINE";
    String iconeConexao = estadoSistema.mqttConectado ? "[V]" : "[X]";
    Display::printAt(0, 1, iconeConexao + " " + statusConexao);

    // Linha 2: Remotas online
    int remotasOnline = 0;
    bool temRacaoBaixa = false;

    for (int i = 0; i < estadoSistema.numRemotas; i++) {
        if (estadoSistema.remotas[i].online) remotasOnline++;
        if (estadoSistema.remotas[i].nivelRacao == "BAIXO") temRacaoBaixa = true;
    }

    Display::printAt(0, 2, "Remotas: " + String(remotasOnline) + "/" + String(estadoSistema.numRemotas));

    // Linha 3: Alerta de ração OU menu
    if (temRacaoBaixa) {
        // Piscar alerta
        unsigned long agora = millis();
        if ((agora / 500) % 2 == 0) {
            Display::printAt(0, 3, "[!] RACAO BAIXA");
        } else {
            Display::printAt(0, 3, "                    ");
        }
        precisaRedraw = true;  // Continuar atualizando para piscar
    } else {
        Display::printAt(0, 3, "> Configurar Remotas");
    }
}

// ✅ Renderização de lista de remotas (mantido)
void GerenciadorTelas::renderizarListaRemotas() {
    centralizarTexto(0, "Remotas");

    // Calcular quantos itens mostrar (remotas + voltar)
    int totalOpcoes = estadoSistema.numRemotas + 1; // +1 apenas para "Voltar"
    int linhasDisponiveis = 3; // Linhas 1, 2 e 3 (linha 0 é o título)

    // Determinar o primeiro item a ser exibido com base na seleção
    int primeiroItem = 0;
    if (opcaoSelecionada >= linhasDisponiveis) {
        primeiroItem = opcaoSelecionada - linhasDisponiveis + 1;
    }

    // Renderizar os itens visíveis
    int linha = 1;
    for (int i = primeiroItem; i < totalOpcoes && linha <= 3; i++) {
        Display::setCursor(0, linha);
        String prefixo = (i == opcaoSelecionada) ? "> " : "  ";
        String texto;

        if (i < estadoSistema.numRemotas) {
            // Mostrar remota - usar remotaAtivaRecente() para verificar se teve sinal nos últimos 10 min
            String nome = "Remota " + String(estadoSistema.remotas[i].id);
            String status = estadoSistema.remotaAtivaRecente(estadoSistema.remotas[i].id) ? "OK" : "OFF";
            texto = prefixo + nome + ": " + status;
        } else {
            // Opção "Voltar"
            texto = prefixo + "Voltar";
        }

        Display::print(texto);
        linha++;
    }
}

// ✅ SIMPLIFICADO: Mostrar as 3 refeições da remota selecionada
void GerenciadorTelas::renderizarConfigRefeicao() {
    if (indiceRemotaAtual >= estadoSistema.numRemotas) return;

    EstadoSistema::EstadoRemota& remota = estadoSistema.remotas[indiceRemotaAtual];

    // Linha 0: Título
    centralizarTexto(0, "Remota " + String(remota.id));

    // Linhas 1-3: Refeições
    for (int i = 0; i < 3; i++) {
        Display::setCursor(0, i + 1);
        String tempo = formatarHora(remota.refeicoes[i].hora, remota.refeicoes[i].minuto);
        String qtd = String(remota.refeicoes[i].quantidade) + "g";
        String prefixo = (i == opcaoSelecionada) ? "> " : "  ";
        Display::print(prefixo + "R" + String(i+1) + " " + tempo + " " + qtd);
    }
}

void GerenciadorTelas::renderizarEditarHora() {
    centralizarTexto(0, "Configurar hora:");
    
    if (indiceRemotaAtual >= estadoSistema.numRemotas || indiceRefeicaoAtual >= 3) return;
    
    // ✅ CORRIGIDO: Usar estadoSistema em vez de variáveis antigas
    EstadoSistema::EstadoRemota::Refeicao& refeicao = estadoSistema.remotas[indiceRemotaAtual].refeicoes[indiceRefeicaoAtual];
    String strHora = String(refeicao.hora).length() < 2 ? "0" + String(refeicao.hora) : String(refeicao.hora);
    String strMinuto = String(refeicao.minuto).length() < 2 ? "0" + String(refeicao.minuto) : String(refeicao.minuto);
    
    if (!estaEditando) {
        Display::printAt(0, 1, "Pressione Enter");
        Display::printAt(0, 2, "    [" + strHora + "]:[" + strMinuto + "]");
    } else {
        String display = (campoEdicao == 0) ? 
            "    >" + strHora + "<:[" + strMinuto + "]" : 
            "    [" + strHora + "]:>" + strMinuto + "<";
        Display::printAt(0, 2, display);
    }
}

void GerenciadorTelas::renderizarEditarQuantidade() {
    centralizarTexto(0, "Quantidade (gramas)");
    
    if (indiceRemotaAtual >= estadoSistema.numRemotas || indiceRefeicaoAtual >= 3) return;
    
    // ✅ CORRIGIDO: Usar estadoSistema em vez de variáveis antigas
    EstadoSistema::EstadoRemota::Refeicao& refeicao = estadoSistema.remotas[indiceRemotaAtual].refeicoes[indiceRefeicaoAtual];
    String quant = formatarQuantidade(refeicao.quantidade);
    String display = estaEditando ? "     >" + quant + "<g" : "     [" + quant + "]g";
    
    Display::printAt(0, 2, display);
}

// =============================================================================
// NAVEGAÇÃO DAS TELAS
// =============================================================================

// ✅ NOVO: Navegação da tela de status (substitui navegarInicio)
void GerenciadorTelas::navegarStatusGateway() {
    if (Botoes::enterPressionado()) {
        irParaTela(TipoTela::LISTA_REMOTAS);
    }
}

// ✅ SIMPLIFICADO: Navegação da lista de remotas
void GerenciadorTelas::navegarListaRemotas() {
    int maxOpcoes = estadoSistema.numRemotas + 1; // remotas + voltar

    if (Botoes::cimaPressionado()) {
        opcaoSelecionada = (opcaoSelecionada - 1 + maxOpcoes) % maxOpcoes;
        precisaRedraw = true;
    }
    else if (Botoes::baixoPressionado()) {
        opcaoSelecionada = (opcaoSelecionada + 1) % maxOpcoes;
        precisaRedraw = true;
    }
    else if (Botoes::enterPressionado()) {
        if (opcaoSelecionada < estadoSistema.numRemotas) {
            // ✅ MUDANÇA: Ir direto para configuração (sem tela intermediária)
            indiceRemotaAtual = opcaoSelecionada;
            opcaoSelecionada = 0;  // Resetar para primeira refeição
            irParaTela(TipoTela::CONFIG_REFEICAO);
        } else {
            // Voltar ao status
            irParaTela(TipoTela::STATUS_GATEWAY);
        }
    }
}

// ✅ SIMPLIFICADO: Navegação de configuração de refeição
void GerenciadorTelas::navegarConfigRefeicao() {
    int maxOpcoes = 3; // 3 refeições

    if (Botoes::cimaPressionado()) {
        opcaoSelecionada = (opcaoSelecionada - 1 + maxOpcoes) % maxOpcoes;
        precisaRedraw = true;
    }
    else if (Botoes::baixoPressionado()) {
        opcaoSelecionada = (opcaoSelecionada + 1) % maxOpcoes;
        precisaRedraw = true;
    }
    else if (Botoes::enterPressionado()) {
        // Selecionar refeição para editar
        indiceRefeicaoAtual = opcaoSelecionada;
        campoEdicao = 0;
        irParaTela(TipoTela::EDITAR_HORA);
    }
    // ✅ NOVO: Botão BACK/ESC volta para lista de remotas
    // (Se você tiver um botão back, adicione aqui)
}

void GerenciadorTelas::navegarEditarHora() {
    if (indiceRemotaAtual >= estadoSistema.numRemotas || indiceRefeicaoAtual >= 3) return;
    
    EstadoSistema::EstadoRemota::Refeicao& refeicao = estadoSistema.remotas[indiceRemotaAtual].refeicoes[indiceRefeicaoAtual];
    
    if (!estaEditando) {
        if (Botoes::enterPressionado()) {
            estaEditando = true;
            precisaRedraw = true;
        }
        else if (Botoes::cimaPressionado() || Botoes::baixoPressionado()) {
            irParaTela(TipoTela::CONFIG_REFEICAO);
        }
    } else {
        if (Botoes::cimaPressionado()) {
            if (campoEdicao == 0) {
                refeicao.hora = (refeicao.hora + 1) % 24;
            } else {
                refeicao.minuto = (refeicao.minuto + 1) % 60;
            }
            precisaRedraw = true;
        }
        else if (Botoes::baixoPressionado()) {
            if (campoEdicao == 0) {
                refeicao.hora = (refeicao.hora - 1 + 24) % 24;
            } else {
                refeicao.minuto = (refeicao.minuto - 1 + 60) % 60;
            }
            precisaRedraw = true;
        }
        else if (Botoes::enterPressionado()) {
            if (campoEdicao == 0) {
                campoEdicao = 1;
                precisaRedraw = true;
            } else {
                // ✅ CORRIGIDO: Usar o método correto do estadoSistema
                estadoSistema.salvarConfiguracao();
                if (callbackAtualizacaoRefeicao) {
                    callbackAtualizacaoRefeicao(
                        estadoSistema.remotas[indiceRemotaAtual].id, 
                        indiceRefeicaoAtual, 
                        refeicao.hora, 
                        refeicao.minuto, 
                        refeicao.quantidade
                    );
                }
                estaEditando = false;
                campoEdicao = 0;
                irParaTela(TipoTela::CONFIG_REFEICAO);
            }
        }
    }
}

void GerenciadorTelas::navegarEditarQuantidade() {
    if (indiceRemotaAtual >= estadoSistema.numRemotas || indiceRefeicaoAtual >= 3) return;
    
    EstadoSistema::EstadoRemota::Refeicao& refeicao = estadoSistema.remotas[indiceRemotaAtual].refeicoes[indiceRefeicaoAtual];
    
    if (!estaEditando) {
        if (Botoes::enterPressionado()) {
            estaEditando = true;
            precisaRedraw = true;
        }
        else if (Botoes::cimaPressionado() || Botoes::baixoPressionado()) {
            irParaTela(TipoTela::CONFIG_REFEICAO);
        }
    } else {
        if (Botoes::cimaPressionado()) {
            refeicao.quantidade = min(refeicao.quantidade + 10, 990);
            precisaRedraw = true;
        }
        else if (Botoes::baixoPressionado()) {
            refeicao.quantidade = max(refeicao.quantidade - 10, 0);
            precisaRedraw = true;
        }
        else if (Botoes::enterPressionado()) {
            // ✅ CORRIGIDO: Usar o método correto do estadoSistema
            estadoSistema.salvarConfiguracao();
            if (callbackAtualizacaoRefeicao) {
                callbackAtualizacaoRefeicao(
                    estadoSistema.remotas[indiceRemotaAtual].id, 
                    indiceRefeicaoAtual, 
                    refeicao.hora, 
                    refeicao.minuto, 
                    refeicao.quantidade
                );
            }
            estaEditando = false;
            irParaTela(TipoTela::CONFIG_REFEICAO);
        }
    }
}

// =============================================================================
// MÉTODOS UTILITÁRIOS
// =============================================================================

void GerenciadorTelas::resetarSelecao() {
    opcaoSelecionada = 0;
    campoEdicao = 0;
    estaEditando = false;
}

void GerenciadorTelas::irParaTela(TipoTela tela) {
    telaAnterior = telaAtual;
    telaAtual = tela;
    inicioTela = millis();
    resetarSelecao();
    timeoutHabilitado = false; // Não usamos mais timeout automático
    precisaRedraw = true;
}

void GerenciadorTelas::voltar() {
    irParaTela(telaAnterior);
}

String GerenciadorTelas::formatarHora(int hora, int minuto) {
    return GerenciadorTempo::formatarDoisDigitos(hora) + ":" +
           GerenciadorTempo::formatarDoisDigitos(minuto);
}

String GerenciadorTelas::formatarQuantidade(int quantidade) {
    return GerenciadorTempo::formatarNumeroComZeros(quantidade, 3);
}

void GerenciadorTelas::centralizarTexto(int linha, String texto) {
    int espacamento = (LCD_COLS - texto.length()) / 2;
    if (espacamento < 0) espacamento = 0;
    Display::printAt(espacamento, linha, texto);
}


bool GerenciadorTelas::verificarTimeout() {
    return (millis() - inicioTela) > INFO_SCREEN_TIMEOUT;
}

// =============================================================================
// MÉTODOS PÚBLICOS DE ATUALIZAÇÃO
// =============================================================================

void GerenciadorTelas::atualizarTempo() {
    static String tempoAnterior = "";
    DadosTempo tempoAtual = GerenciadorTempo::obterTempoAtual();
    
    if (tempoAtual.tempoFormatado != tempoAnterior) {
        tempoAnterior = tempoAtual.tempoFormatado;
        precisaRedraw = true;
    }
    
    // ✅ CORRIGIDO: Usar estadoSistema para verificar atualizações
    if (estadoSistema.verificarELimparFlagRefeicoes()) {
        precisaRedraw = true;
        DEBUG_PRINTLN("[TELAS] Refeições atualizadas por evento externo");
    }
}

// =============================================================================
// VERIFICAÇÕES AUTOMÁTICAS
// =============================================================================

void GerenciadorTelas::verificarAlertas() {

    bool temRacaoBaixa = false;
    for (int i = 0; i < estadoSistema.numRemotas; i++) {
        if (estadoSistema.remotas[i].nivelRacao == "BAIXO") {
            temRacaoBaixa = true;
            break;
        }
    }

    // ✅ ATUALIZAR: Forçar redraw se houver alerta na tela principal
    if (temRacaoBaixa && telaAtual == TipoTela::STATUS_GATEWAY) {
        precisaRedraw = true;
    }
}
// =============================================================================
// MÉTODOS DE CONFIGURAÇÃO
// =============================================================================

void GerenciadorTelas::alternarLuzLcd() {
    luzLcd = !luzLcd;
    Display::backlight(luzLcd);
    precisaRedraw = true;
}

// =============================================================================
// GETTERS E SETTERS
// =============================================================================

TipoTela GerenciadorTelas::obterTelaAtual() {
    return telaAtual;
}

bool GerenciadorTelas::estaEmModoEdicao() {
    return estaEditando;
}

void GerenciadorTelas::definirCallbackAtualizacaoRefeicao(void (*callback)(int, int, int, int, int)) {
    callbackAtualizacaoRefeicao = callback;
}