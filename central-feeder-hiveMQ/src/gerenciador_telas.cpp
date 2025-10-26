#include "gerenciador_telas.h"
#include <config.h>
#include <Preferences.h>
#include "gerenciador_mqtt.h"
#include "sistema_estado.h"

// Inicialização de variáveis estáticas
TipoTela GerenciadorTelas::telaAtual = TipoTela::INICIO;
TipoTela GerenciadorTelas::telaAnterior = TipoTela::INICIO;
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

    irParaTela(TipoTela::INICIO);
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
            irParaTela(TipoTela::INICIO);
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

void GerenciadorTelas::renderizar() {
    Serial.println("[TELAS] Renderizando tela...");
    Display::clear();

    switch (telaAtual) {
        case TipoTela::INICIO:
            Serial.println("[TELAS] Renderizando INICIO");
            renderizarInicio();
            break;
        case TipoTela::INFO_WIFI: renderizarInfoWifi(); break;
        case TipoTela::CONFIG_CENTRAL: renderizarConfigCentral(); break;
        case TipoTela::LISTA_REMOTAS: renderizarListaRemotas(); break;
        case TipoTela::REMOTA_ESPECIFICA: renderizarRemotaEspecifica(); break;
        case TipoTela::CONFIG_REFEICAO: renderizarConfigRefeicao(); break;
        case TipoTela::EDITAR_HORA: renderizarEditarHora(); break;
        case TipoTela::EDITAR_QUANTIDADE: renderizarEditarQuantidade(); break;
        case TipoTela::ALERTA_RACAO_BAIXA: renderizarAlertaRacao(); break;
    }
    Serial.println("[TELAS] Renderização concluída");
}

void GerenciadorTelas::gerenciarNavegacao() {
    switch (telaAtual) {
        case TipoTela::INICIO: navegarInicio(); break;
        case TipoTela::INFO_WIFI: navegarInfoWifi(); break;
        case TipoTela::CONFIG_CENTRAL: navegarConfigCentral(); break;
        case TipoTela::LISTA_REMOTAS: navegarListaRemotas(); break;
        case TipoTela::REMOTA_ESPECIFICA: navegarRemotaEspecifica(); break;
        case TipoTela::CONFIG_REFEICAO: navegarConfigRefeicao(); break;
        case TipoTela::EDITAR_HORA: navegarEditarHora(); break;
        case TipoTela::EDITAR_QUANTIDADE: navegarEditarQuantidade(); break;
        case TipoTela::ALERTA_RACAO_BAIXA: navegarAlertaRacao(); break;
    }
}

// =============================================================================
// RENDERIZAÇÃO DAS TELAS
// =============================================================================

void GerenciadorTelas::renderizarInicio() {
    DadosTempo tempoAtual = GerenciadorTempo::obterTempoAtual();
    Serial.println("[TELAS] Tempo: " + tempoAtual.tempoFormatado);
    centralizarTexto(0, tempoAtual.tempoFormatado);

    String opcoes[] = {"Lista de Remotas", "Info WiFi", "Configuracoes"};

    for (int i = 0; i < 3; i++) {
        Display::setCursor(0, i + 1);
        String linha = (i == opcaoSelecionada ? "> " : "  ") + opcoes[i];
        Serial.println("[TELAS] Linha " + String(i+1) + ": " + linha);
        Display::print(linha);
    }
}

void GerenciadorTelas::renderizarInfoWifi() {
    Display::printAt(0, 0, "Rede: " + estadoSistema.wifiSSID);
    Display::printAt(0, 1, "WiFi: " + String(estadoSistema.wifiConectado ? "Conectado" : "Desconectado"));
    Display::printAt(0, 2, "IP: " + estadoSistema.ipWifi);
    Display::printAt(0, 3, "MQTT: " + String(estadoSistema.mqttConectado ? "Conectado" : "Desconectado"));
}

void GerenciadorTelas::renderizarConfigCentral() {
    String opcoes[] = {
        "Luz LCD: " + String(luzLcd ? "ON" : "OFF"),
        "Voltar"
    };

    for (int i = 0; i < 2; i++) {
        Display::setCursor(0, i);
        Display::print((i == opcaoSelecionada ? "> " : "  ") + opcoes[i]);
    }
}

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

void GerenciadorTelas::renderizarRemotaEspecifica() {
    if (indiceRemotaAtual >= estadoSistema.numRemotas) return;
    
    EstadoSistema::EstadoRemota& remota = estadoSistema.remotas[indiceRemotaAtual];
    
    for (int i = 0; i < 3; i++) {
        Display::setCursor(0, i);
        String tempo = formatarHora(remota.refeicoes[i].hora, remota.refeicoes[i].minuto);
        String prefixo = (i == opcaoSelecionada) ? "> " : "  ";
        Display::print(prefixo + "Ref " + String(i+1) + ": " + tempo);
    }
    
    Display::setCursor(0, 3);
    String prefixo = (opcaoSelecionada == 3) ? "> " : "  ";
    Display::print(prefixo + "Voltar");
}


void GerenciadorTelas::renderizarConfigRefeicao() {
    if (indiceRemotaAtual >= estadoSistema.numRemotas || indiceRefeicaoAtual >= 3) return;
    
    EstadoSistema::EstadoRemota::Refeicao& refeicao = estadoSistema.remotas[indiceRemotaAtual].refeicoes[indiceRefeicaoAtual];
    
    String opcoes[] = {
        "Hora: " + formatarHora(refeicao.hora, refeicao.minuto),
        "Quant: " + formatarQuantidade(refeicao.quantidade) + "g",
        "Voltar"
    };
    
    for (int i = 0; i < 3; i++) {
        Display::setCursor(0, i);
        Display::print((i == opcaoSelecionada ? "> " : "  ") + opcoes[i]);
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

void GerenciadorTelas::renderizarAlertaRacao() {
    unsigned long agora = millis();
    
    if (agora - ultimaPiscadaAlerta > ALERTA_PISCADA_INTERVALO) {
        ultimaPiscadaAlerta = agora;
        estadoPiscaAlerta = !estadoPiscaAlerta;
        precisaRedraw = true;
    }

    if (estadoPiscaAlerta) {
        centralizarTexto(0, ALERTA_RACAO_TITULO);
        centralizarTexto(1, ALERTA_RACAO_LINHA1);
        
        char buffer[20];
        sprintf(buffer, ALERTA_RACAO_LINHA2, remotaComAlerta);
        centralizarTexto(2, String(buffer));
        
        centralizarTexto(3, ALERTA_RACAO_LINHA3);
    } else {
        Display::clear();
    }
}
// =============================================================================
// NAVEGAÇÃO DAS TELAS
// =============================================================================

void GerenciadorTelas::navegarInicio() {
    if (Botoes::cimaPressionado()) {
        opcaoSelecionada = (opcaoSelecionada - 1 + 3) % 3;
        precisaRedraw = true;
    }
    else if (Botoes::baixoPressionado()) {
        opcaoSelecionada = (opcaoSelecionada + 1) % 3;
        precisaRedraw = true;
    }
    else if (Botoes::enterPressionado()) {
        switch (opcaoSelecionada) {
            case 0: irParaTela(TipoTela::LISTA_REMOTAS); break;
            case 1: irParaTela(TipoTela::INFO_WIFI); break;
            case 2: irParaTela(TipoTela::CONFIG_CENTRAL); break;
        }
    }
}

void GerenciadorTelas::navegarInfoWifi() {
    if (Botoes::cimaPressionado() || Botoes::baixoPressionado() || Botoes::enterPressionado()) {
        irParaTela(TipoTela::INICIO);
    }
}

void GerenciadorTelas::navegarAlertaRacao() {
    if (!alertaRacaoAtivo && (Botoes::cimaPressionado() || Botoes::baixoPressionado() || Botoes::enterPressionado())) {
        irParaTela(TipoTela::INICIO);
    }
}


void GerenciadorTelas::navegarConfigCentral() {
    if (Botoes::cimaPressionado()) {
        opcaoSelecionada = (opcaoSelecionada - 1 + 2) % 2;
        precisaRedraw = true;
    }
    else if (Botoes::baixoPressionado()) {
        opcaoSelecionada = (opcaoSelecionada + 1) % 2;
        precisaRedraw = true;
    }
    else if (Botoes::enterPressionado()) {
        switch (opcaoSelecionada) {
            case 0: alternarLuzLcd(); break;
            case 1: irParaTela(TipoTela::INICIO); break;
        }
    }
}

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
            // Selecionou uma remota diretamente
            indiceRemotaAtual = opcaoSelecionada;
            irParaTela(TipoTela::REMOTA_ESPECIFICA);
        } else {
            // Selecionou "Voltar"
            irParaTela(TipoTela::INICIO);
        }
    }
}

void GerenciadorTelas::navegarRemotaEspecifica() {
    if (Botoes::cimaPressionado()) {
        opcaoSelecionada = (opcaoSelecionada - 1 + 4) % 4;
        precisaRedraw = true;
    }
    else if (Botoes::baixoPressionado()) {
        opcaoSelecionada = (opcaoSelecionada + 1) % 4;
        precisaRedraw = true;
    }
    else if (Botoes::enterPressionado()) {
        if (opcaoSelecionada < 3) {
            // ✅ CORRIGIDO: Usar indiceRefeicaoAtual
            indiceRefeicaoAtual = opcaoSelecionada;
            irParaTela(TipoTela::CONFIG_REFEICAO);
        } else {
            irParaTela(TipoTela::LISTA_REMOTAS);
        }
    }
}

void GerenciadorTelas::navegarConfigRefeicao() {
    if (Botoes::cimaPressionado()) {
        opcaoSelecionada = (opcaoSelecionada - 1 + 3) % 3;
        precisaRedraw = true;
    }
    else if (Botoes::baixoPressionado()) {
        opcaoSelecionada = (opcaoSelecionada + 1) % 3;
        precisaRedraw = true;
    }
    else if (Botoes::enterPressionado()) {
        switch (opcaoSelecionada) {
            case 0: 
                campoEdicao = 0;
                irParaTela(TipoTela::EDITAR_HORA);
                break;
            case 1:
                campoEdicao = 0;
                irParaTela(TipoTela::EDITAR_QUANTIDADE);
                break;
            case 2:
                irParaTela(TipoTela::REMOTA_ESPECIFICA);
                break;
        }
    }
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

    timeoutHabilitado = (tela == TipoTela::INFO_WIFI);
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
    static unsigned long ultimoAlertaPeriodico = 0;
    unsigned long agora = millis();

    if (alertaRacaoAtivo) {
        if (agora - ultimoAlertaPeriodico >= 300000 || ultimoAlertaPeriodico == 0) {
            if (telaAtual != TipoTela::ALERTA_RACAO_BAIXA && !estaEditando) {
                irParaTela(TipoTela::ALERTA_RACAO_BAIXA);
                ultimoAlertaPeriodico = agora;
            }
        }

        static unsigned long inicioTelaAlerta = 0;
        if (telaAtual == TipoTela::ALERTA_RACAO_BAIXA) {
            if (inicioTelaAlerta == 0) inicioTelaAlerta = agora;
            else if (agora - inicioTelaAlerta >= 10000) {
                irParaTela(TipoTela::INICIO);
                inicioTelaAlerta = 0;
            }
        } else {
            inicioTelaAlerta = 0;
        }
    } else {
        ultimoAlertaPeriodico = 0;
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