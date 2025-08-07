#include "gerenciador_telas.h"
#include <config.h>
#include <Preferences.h>

// Inicialização de variáveis estáticas
TipoTela GerenciadorTelas::telaAtual = TipoTela::INICIO;
TipoTela GerenciadorTelas::telaAnterior = TipoTela::INICIO;
int GerenciadorTelas::opcaoSelecionada = 0;
int GerenciadorTelas::paginaAtual = 0;
int GerenciadorTelas::campoEdicao = 0;
bool GerenciadorTelas::estaEditando = false;

// Dados do sistema
String GerenciadorTelas::wifiSSID = "Desconectado";
bool GerenciadorTelas::wifiConectado = false;
String GerenciadorTelas::qualidadeWifi = "Sem sinal";
bool GerenciadorTelas::mqttConectado = false;
bool GerenciadorTelas::luzLcd = true;
String GerenciadorTelas::dataUltimoBooter = "01/08/2025";
String GerenciadorTelas::horaUltimoBooter = "00:00:00";

// Dados das remotas
Remota GerenciadorTelas::remotas[MAX_REMOTAS];
int GerenciadorTelas::remotaAtual = 0;
int GerenciadorTelas::refeicaoAtual = 0;
int GerenciadorTelas::numeroRemotas = 0;

// Controle de tempo
unsigned long GerenciadorTelas::ultimaAtualizacao = 0;
unsigned long GerenciadorTelas::inicioTela = 0;
bool GerenciadorTelas::timeoutHabilitado = false;

// Controle de verificação automática de horários
unsigned long GerenciadorTelas::ultimaVerificacaoHorarios = 0;

// Callbacks
void (*GerenciadorTelas::callbackResetSistema)() = nullptr;
void (*GerenciadorTelas::callbackAtualizacaoRefeicao)(int, int, int, int, int) = nullptr;
// Flag para controle de redraw
bool GerenciadorTelas::precisaRedraw = false;

void GerenciadorTelas::inicializar() {
    Serial.println("=== Inicializando Gerenciador de Telas ===");
    
    // Carregar dados salvos
    carregarDadosRemota();
    
    // Configurar algumas remotas de exemplo se não há dados salvos
    if (numeroRemotas == 0) {
        adicionarRemota(1, "Remota 1", false); // Começar como OFF até receber heartbeat
        adicionarRemota(2, "Remota 2", false); // Começar como OFF até receber heartbeat
        adicionarRemota(3, "Remota 3", false); // Começar como OFF até receber heartbeat
        adicionarRemota(4, "Remota 4", false); // Começar como OFF até receber heartbeat
        
        // Configurar refeições de exemplo
        definirRefeicao(1, 0, 8, 0, 40);    // 08:00, 40g
        definirRefeicao(1, 1, 14, 30, 40);  // 14:30, 40g
        definirRefeicao(1, 2, 20, 0, 40);   // 20:00, 40g
    }
    
    // Atualizar tempo de boot
    DadosTempo tempoAtual = GerenciadorTempo::obterTempoAtual();
    atualizarTempoBooter(tempoAtual.dataFormatada, tempoAtual.tempoFormatado);
    
    // Configurar callbacks MQTT para atualizar status das remotas
    GerenciadorMQTT::definirCallbackStatusRemota(onStatusRemotaRecebido);
    GerenciadorMQTT::definirCallbackVidaRemota(onVidaRemotaRecebida);
    GerenciadorMQTT::definirCallbackRespostaRemota(onRespostaRemotaRecebida);
    
    // Configurar callback de tempo para atualização automática da tela
    GerenciadorTempo::definirCallbackAtualizacaoTempo([](DadosTempo tempo) {
        GerenciadorTelas::atualizarTempo();
    });
    
    // Inicializar na tela INICIO
    irParaTela(TipoTela::INICIO);
    
    Serial.println("Gerenciador de Telas inicializado");
}

void GerenciadorTelas::atualizar() {
    unsigned long agora = millis();
    
    // Atualizar a cada 100ms para responsividade
    if (agora - ultimaAtualizacao > 100) {
        ultimaAtualizacao = agora;
        
        // Verificar timeouts de heartbeat das remotas (a cada 5 segundos)
        static unsigned long ultimaVerificacaoTimeout = 0;
        if (agora - ultimaVerificacaoTimeout > 5000) {
            ultimaVerificacaoTimeout = agora;
            verificarTimeoutRemotas(agora);
        }
        
        // Verificar horários de alimentação automática
        verificarHorariosAutomaticos();
        
        // Verificar timeout se habilitado
        if (timeoutHabilitado && verificarTimeout()) {
            irParaTela(TipoTela::INICIO);
            return;
        }
        
        // Processar navegação e sinalizar redraw se mudou seleção ou tela
        TipoTela telaAntiga = telaAtual;
        int opcaoAntiga = opcaoSelecionada;
        int campoAntigo = campoEdicao;
        bool estaEditandoAntigo = estaEditando;
        
        gerenciarNavegacao();
        
        // Redraw se mudou tela, seleção, campo de edição ou estado de edição
        if (telaAtual != telaAntiga || opcaoSelecionada != opcaoAntiga || 
            campoEdicao != campoAntigo || estaEditando != estaEditandoAntigo || precisaRedraw) {
            precisaRedraw = true;
        }
        
        if (precisaRedraw) {
            renderizar();
            precisaRedraw = false;
        }
        
    }
}

void GerenciadorTelas::renderizar() {
    // Limpar antes de redesenhar
    Display::clear();
    switch (telaAtual) {
        case TipoTela::INICIO:
            renderizarInicio();
            break;
        case TipoTela::INFO_WIFI:
            renderizarInfoWifi();
            break;
        case TipoTela::CONFIG_CENTRAL:
            renderizarConfigCentral();
            break;
        case TipoTela::CONFIG_ULTIMO_BOOT:
            renderizarUltimoBooter();
            break;
        case TipoTela::CONFIG_RESETAR:
            renderizarResetar();
            break;
        case TipoTela::LISTA_REMOTAS_P1:
            renderizarListaRemotasP1();
            break;
        case TipoTela::LISTA_REMOTAS_P2:
            renderizarListaRemotasP2();
            break;
        case TipoTela::REMOTA_ESPECIFICA:
            renderizarRemotaEspecifica();
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

void GerenciadorTelas::gerenciarNavegacao() {
    switch (telaAtual) {
        case TipoTela::INICIO:
            navegarInicio();
            break;
        case TipoTela::INFO_WIFI:
            navegarInfoWifi();
            break;
        case TipoTela::CONFIG_CENTRAL:
            navegarConfigCentral();
            break;
        case TipoTela::CONFIG_ULTIMO_BOOT:
            navegarUltimoBooter();
            break;
        case TipoTela::CONFIG_RESETAR:
            navegarResetar();
            break;
        case TipoTela::LISTA_REMOTAS_P1:
            navegarListaRemotasP1();
            break;
        case TipoTela::LISTA_REMOTAS_P2:
            navegarListaRemotasP2();
            break;
        case TipoTela::REMOTA_ESPECIFICA:
            navegarRemotaEspecifica();
            break;
        case TipoTela::CONFIG_REFEICAO:
            navegarConfigRefeicao();
            break;
        case TipoTela::EDITAR_HORA:
            navegarEditarHora();
            break;
        case TipoTela::EDITAR_QUANTIDADE:
            navegarEditarQuantidade();
            break;
    }
}

// =============================================================================
// MÉTODOS DE RENDERIZAÇÃO
// =============================================================================

void GerenciadorTelas::renderizarInicio() {
    
    // Linha 1: Hora atual
    DadosTempo tempoAtual = GerenciadorTempo::obterTempoAtual();
    centralizarTexto(0, tempoAtual.tempoFormatado);
    
    // Linha 2-4: Menu principal
    String opcoes[] = {"Lista de Remotas", "WiFi", "Config Central"};
    
    for (int i = 0; i < 3; i++) {
        Display::setCursor(0, i + 1);
        if (i == opcaoSelecionada) {
            Display::print("> " + opcoes[i]);
        } else {
            Display::print("  " + opcoes[i]);
        }
    }
}

void GerenciadorTelas::renderizarInfoWifi() {
    
    Display::printAt(0, 0, "Rede: " + wifiSSID);
    Display::printAt(0, 1, String("WiFi: ") + (wifiConectado ? "Conectado" : "Desconectado"));
    Display::printAt(0, 2, "Qualidade: " + qualidadeWifi);
    Display::printAt(0, 3, String("MQTT: ") + GerenciadorMQTT::obterStatusConexao());
}

void GerenciadorTelas::renderizarConfigCentral() {
    
    String opcoes[] = {"Ultimo Boot", "Resetar Sistema", String("Luz LCD: ") + (luzLcd ? "ON" : "OFF"), "Voltar"};
    
    for (int i = 0; i < 4; i++) {
        Display::setCursor(0, i);
        if (i == opcaoSelecionada) {
            Display::print("> " + opcoes[i]);
        } else {
            Display::print("  " + opcoes[i]);
        }
    }
}

void GerenciadorTelas::renderizarUltimoBooter() {
    
    centralizarTexto(1, dataUltimoBooter);
    centralizarTexto(2, horaUltimoBooter);
}

void GerenciadorTelas::renderizarResetar() {
    
    centralizarTexto(0, "Confirmar reset?");
    
    String opcoes[] = {"Sim", "Nao"};
    
    for (int i = 0; i < 2; i++) {
        Display::setCursor(0, i + 1);
        if (i == opcaoSelecionada) {
            Display::print("> " + opcoes[i]);
        } else {
            Display::print("  " + opcoes[i]);
        }
    }
}

void GerenciadorTelas::renderizarListaRemotasP1() {
    
    // Mostrar até 3 remotas
    for (int i = 0; i < 3 && i < numeroRemotas; i++) {
        Display::setCursor(0, i);
        String status = remotas[i].conectada ? "OK" : "OFF";
        String linha = remotas[i].nome + ": " + status;
        
        if (i == opcaoSelecionada) {
            Display::print("> " + linha);
        } else {
            Display::print("  " + linha);
        }
    }
    
    // Mostrar opção baseada no número de remotas
    int linhaOpcao = min(numeroRemotas, 3);
    Display::setCursor(0, linhaOpcao);
    
    if (numeroRemotas > 3) {
        // Há mais remotas - mostrar "Próxima página"
        if (opcaoSelecionada == linhaOpcao) {
            Display::print("> Proxima pagina");
        } else {
            Display::print("  Proxima pagina");
        }
    } else {
        // Não há mais remotas - mostrar "Voltar"
        if (opcaoSelecionada == linhaOpcao) {
            Display::print("> Voltar");
        } else {
            Display::print("  Voltar");
        }
    }
}

void GerenciadorTelas::renderizarListaRemotasP2() {
    
    // Mostrar remota 4 se existir (índice 3)
    if (numeroRemotas > 3) {
        Display::setCursor(0, 0);
        String status = remotas[3].conectada ? "OK" : "OFF";
        String linha = remotas[3].nome + ": " + status;
        
        if (opcaoSelecionada == 0) {
            Display::print("> " + linha);
        } else {
            Display::print("  " + linha);
        }
    }
    
    // Opção voltar
    int linhaVoltar = (numeroRemotas > 3) ? 1 : 0;
    Display::setCursor(0, linhaVoltar);
    if (opcaoSelecionada == linhaVoltar) {
        Display::print("> Voltar");
    } else {
        Display::print("  Voltar");
    }
    
    // Indicador de página
    Display::printAt(0, 3, "[Pagina 2/2]");
}

void GerenciadorTelas::renderizarRemotaEspecifica() {
    
    if (remotaAtual < numeroRemotas) {
        Remota& remota = remotas[remotaAtual];
        
        // Mostrar refeições
        for (int i = 0; i < 3; i++) {
            Display::setCursor(0, i);
            String strTempo = formatarHora(remota.refeicoes[i].hora, remota.refeicoes[i].minuto);
            String linha = "Refeicao " + String(i + 1) + ": " + strTempo;
            
            if (i == opcaoSelecionada) {
                Display::print("> " + linha);
            } else {
                Display::print("  " + linha);
            }
        }
        
        // Opção voltar
        Display::setCursor(0, 3);
        if (opcaoSelecionada == 3) {
            Display::print("> Voltar");
        } else {
            Display::print("  Voltar");
        }
    }
}

void GerenciadorTelas::renderizarConfigRefeicao() {
    
    if (remotaAtual < numeroRemotas && refeicaoAtual < REFEICOES_PER_REMOTA) {
        Refeicao& refeicao = remotas[remotaAtual].refeicoes[refeicaoAtual];
        
        String strTempo = formatarHora(refeicao.hora, refeicao.minuto);
        String strQuant = String(refeicao.quantidade);
        if (strQuant.length() < 3) strQuant = "0" + strQuant;
        if (strQuant.length() < 3) strQuant = "0" + strQuant;
        
        String opcoes[] = {
            "Hora: " + strTempo,
            "Quantidade: " + strQuant + "g",
            "Voltar"
        };
        
        for (int i = 0; i < 3; i++) {
            Display::setCursor(0, i);
            if (i == opcaoSelecionada) {
                Display::print("> " + opcoes[i]);
            } else {
                Display::print("  " + opcoes[i]);
            }
        }
    }
}

void GerenciadorTelas::renderizarEditarHora() {
    
    centralizarTexto(0, "Configurar hora:");
    
    if (remotaAtual < numeroRemotas && refeicaoAtual < REFEICOES_PER_REMOTA) {
        Refeicao& refeicao = remotas[remotaAtual].refeicoes[refeicaoAtual];
        
        String strHora = String(refeicao.hora);
        String strMinuto = String(refeicao.minuto);
        
        if (strHora.length() < 2) strHora = "0" + strHora;
        if (strMinuto.length() < 2) strMinuto = "0" + strMinuto;
        
        if (!estaEditando) {
            // Aguardando Enter para começar
            Display::printAt(0, 1, "Pressione Enter");
            Display::printAt(0, 2, "    [" + strHora + "]:[" + strMinuto + "]");
        } else {
            // Editando - destacar campo ativo
            String exibicaoTempo;
            if (campoEdicao == 0) {
                exibicaoTempo = "    >" + strHora + "<:[" + strMinuto + "]";
            } else {
                exibicaoTempo = "    [" + strHora + "]:>" + strMinuto + "<";
            }
            Display::printAt(0, 2, exibicaoTempo);
        }
    }
}

void GerenciadorTelas::renderizarEditarQuantidade() {
    
    centralizarTexto(0, "Quantidade (gramas)");
    
    if (remotaAtual < numeroRemotas && refeicaoAtual < REFEICOES_PER_REMOTA) {
        Refeicao& refeicao = remotas[remotaAtual].refeicoes[refeicaoAtual];
        
        String strQuant = String(refeicao.quantidade);
        if (strQuant.length() < 3) strQuant = "0" + strQuant;
        if (strQuant.length() < 3) strQuant = "0" + strQuant;
        
        String exibicaoQuant = "     [" + strQuant + "]g";
        
        if (estaEditando) {
            exibicaoQuant = "     >" + strQuant + "<g";
        }
        
        Display::printAt(0, 2, exibicaoQuant);
    }
}

// =============================================================================
// MÉTODOS DE NAVEGAÇÃO
// =============================================================================

void GerenciadorTelas::navegarInicio() {
    if (Botoes::cimaPressionado()) {
        opcaoSelecionada = (opcaoSelecionada - 1 + 3) % 3;
    }
    else if (Botoes::baixoPressionado()) {
        opcaoSelecionada = (opcaoSelecionada + 1) % 3;
    }
    else if (Botoes::enterPressionado()) {
        switch (opcaoSelecionada) {
            case 0: // Lista de Remotas
                irParaTela(TipoTela::LISTA_REMOTAS_P1);
                break;
            case 1: // WiFi
                irParaTela(TipoTela::INFO_WIFI);
                break;
            case 2: // Config Central
                irParaTela(TipoTela::CONFIG_CENTRAL);
                break;
        }
    }
}

void GerenciadorTelas::navegarInfoWifi() {
    if (Botoes::cimaPressionado() || Botoes::baixoPressionado() || Botoes::enterPressionado()) {
        irParaTela(TipoTela::INICIO);
    }
}

void GerenciadorTelas::navegarConfigCentral() {
    if (Botoes::cimaPressionado()) {
        opcaoSelecionada = (opcaoSelecionada - 1 + 4) % 4;
    }
    else if (Botoes::baixoPressionado()) {
        opcaoSelecionada = (opcaoSelecionada + 1) % 4;
    }
    else if (Botoes::enterPressionado()) {
        switch (opcaoSelecionada) {
            case 0: // Ultimo Boot
                irParaTela(TipoTela::CONFIG_ULTIMO_BOOT);
                break;
            case 1: // Resetar Sistema
                irParaTela(TipoTela::CONFIG_RESETAR);
                break;
            case 2: // Luz LCD
                alternarLuzLcd();
                break;
            case 3: // Voltar
                irParaTela(TipoTela::INICIO);
                break;
        }
    }
}

void GerenciadorTelas::navegarUltimoBooter() {
    if (Botoes::cimaPressionado() || Botoes::baixoPressionado() || Botoes::enterPressionado()) {
        voltar();
    }
}

void GerenciadorTelas::navegarResetar() {
    if (Botoes::cimaPressionado()) {
        opcaoSelecionada = (opcaoSelecionada - 1 + 2) % 2;
    }
    else if (Botoes::baixoPressionado()) {
        opcaoSelecionada = (opcaoSelecionada + 1) % 2;
    }
    else if (Botoes::enterPressionado()) {
        if (opcaoSelecionada == 0) { // Sim
            resetarSistema();
        }
        voltar();
    }
}

void GerenciadorTelas::navegarListaRemotasP1() {
    int maxOpcoes = min(numeroRemotas, 3) + 1; // +1 para incluir a opção extra (Próxima página ou Voltar)
    
    if (Botoes::cimaPressionado()) {
        opcaoSelecionada = (opcaoSelecionada - 1 + maxOpcoes) % maxOpcoes;
    }
    else if (Botoes::baixoPressionado()) {
        opcaoSelecionada = (opcaoSelecionada + 1) % maxOpcoes;
    }
    else if (Botoes::enterPressionado()) {
        if (opcaoSelecionada < min(numeroRemotas, 3)) {
            // Selecionou uma remota
            remotaAtual = opcaoSelecionada;
            irParaTela(TipoTela::REMOTA_ESPECIFICA);
        } else {
            // Selecionou a opção extra
            if (numeroRemotas > 3) {
                // Há mais remotas - ir para próxima página
                irParaTela(TipoTela::LISTA_REMOTAS_P2);
            } else {
                // Não há mais remotas - voltar para Inicio
                irParaTela(TipoTela::INICIO);
            }
        }
    }
}

void GerenciadorTelas::navegarListaRemotasP2() {
    int maxOpcoes = (numeroRemotas > 3) ? 2 : 1; // Se tem remota 4: Remota4 + Voltar, senão só Voltar
    
    if (Botoes::cimaPressionado()) {
        opcaoSelecionada = (opcaoSelecionada - 1 + maxOpcoes) % maxOpcoes;
    }
    else if (Botoes::baixoPressionado()) {
        opcaoSelecionada = (opcaoSelecionada + 1) % maxOpcoes;
    }
    else if (Botoes::enterPressionado()) {
        if (numeroRemotas > 3 && opcaoSelecionada == 0) {
            // Selecionou Remota 4 (índice 3)
            remotaAtual = 3;
            irParaTela(TipoTela::REMOTA_ESPECIFICA);
        } else {
            // Selecionou Voltar - ir para página 1 das remotas
            irParaTela(TipoTela::LISTA_REMOTAS_P1);
        }
    }
}

void GerenciadorTelas::navegarRemotaEspecifica() {
    if (Botoes::cimaPressionado()) {
        opcaoSelecionada = (opcaoSelecionada - 1 + 4) % 4;
    }
    else if (Botoes::baixoPressionado()) {
        opcaoSelecionada = (opcaoSelecionada + 1) % 4;
    }
    else if (Botoes::enterPressionado()) {
        if (opcaoSelecionada < 3) {
            refeicaoAtual = opcaoSelecionada;
            irParaTela(TipoTela::CONFIG_REFEICAO);
        } else {
            // Voltar para a página correta das remotas
            if (remotaAtual < 3) {
                irParaTela(TipoTela::LISTA_REMOTAS_P1);
            } else {
                irParaTela(TipoTela::LISTA_REMOTAS_P2);
            }
        }
    }
}

void GerenciadorTelas::navegarConfigRefeicao() {
    if (Botoes::cimaPressionado()) {
        opcaoSelecionada = (opcaoSelecionada - 1 + 3) % 3;
    }
    else if (Botoes::baixoPressionado()) {
        opcaoSelecionada = (opcaoSelecionada + 1) % 3;
    }
    else if (Botoes::enterPressionado()) {
        if (opcaoSelecionada == 0) { // Editar Hora
            campoEdicao = 0;
            irParaTela(TipoTela::EDITAR_HORA);
            // NÃO iniciar edição automaticamente - precisa pressionar Enter na tela
        }
        else if (opcaoSelecionada == 1) { // Editar Quantidade
            campoEdicao = 0;
            irParaTela(TipoTela::EDITAR_QUANTIDADE);
        }
        else { // Voltar
            irParaTela(TipoTela::REMOTA_ESPECIFICA);
        }
    }
}

void GerenciadorTelas::navegarEditarHora() {
    if (!estaEditando) {
        // Ainda não começou a editar - esperar Enter para começar
        if (Botoes::enterPressionado()) {
            Serial.println("DEBUG: Enter detectado - iniciando edição");
            estaEditando = true;
            campoEdicao = 0; // começar com a hora
            precisaRedraw = true; // forçar redraw para mostrar modo edição
        } else if (Botoes::cimaPressionado() || Botoes::baixoPressionado()) {
            // Voltar sem editar
            irParaTela(TipoTela::CONFIG_REFEICAO);
        }
    } else {
        // Modo edição ativo
        Refeicao& refeicao = remotas[remotaAtual].refeicoes[refeicaoAtual];
        
        if (Botoes::cimaPressionado()) {
            if (campoEdicao == 0) { // editando hora
                refeicao.hora = (refeicao.hora + 1) % 24;
            } else { // editando minuto
                refeicao.minuto = (refeicao.minuto + 1) % 60;
            }
            precisaRedraw = true; // forçar redraw para mostrar novo valor
        }
        else if (Botoes::baixoPressionado()) {
            if (campoEdicao == 0) { // editando hora
                refeicao.hora = (refeicao.hora - 1 + 24) % 24;
            } else { // editando minuto
                refeicao.minuto = (refeicao.minuto - 1 + 60) % 60;
            }
            precisaRedraw = true; // forçar redraw para mostrar novo valor
        }
        else if (Botoes::enterPressionado()) {
            if (campoEdicao == 0) {
                Serial.println("DEBUG: Enter - passando para minutos");
                // Terminou de editar hora, ir para minutos
                campoEdicao = 1;
                precisaRedraw = true; // redraw para destacar minutos
            } else {
                Serial.println("DEBUG: Enter - salvando e voltando");
                // Terminou de editar minutos, salvar e voltar
                salvarDadosRemota();
                if (callbackAtualizacaoRefeicao) {
                    callbackAtualizacaoRefeicao(remotaAtual, refeicaoAtual, refeicao.hora, refeicao.minuto, refeicao.quantidade);
                }
                estaEditando = false;
                campoEdicao = 0;
                irParaTela(TipoTela::CONFIG_REFEICAO);
            }
        }
    }
}

void GerenciadorTelas::navegarEditarQuantidade() {
    if (!estaEditando) {
        if (Botoes::enterPressionado()) {
            estaEditando = true;
            precisaRedraw = true; // forçar redraw para mostrar modo edição
        } else if (Botoes::cimaPressionado() || Botoes::baixoPressionado()) {
            irParaTela(TipoTela::CONFIG_REFEICAO);
        }
    } else {
        Refeicao& refeicao = remotas[remotaAtual].refeicoes[refeicaoAtual];
        
        if (Botoes::cimaPressionado()) {
            refeicao.quantidade = min(refeicao.quantidade + 10, 990);
            precisaRedraw = true; // forçar redraw para mostrar novo valor
        }
        else if (Botoes::baixoPressionado()) {
            refeicao.quantidade = max(refeicao.quantidade - 10, 0);
            precisaRedraw = true; // forçar redraw para mostrar novo valor
        }
        else if (Botoes::enterPressionado()) {
            // Salvar e voltar para configuração de refeição
            salvarDadosRemota();
            if (callbackAtualizacaoRefeicao) {
                callbackAtualizacaoRefeicao(remotaAtual, refeicaoAtual, refeicao.hora, refeicao.minuto, refeicao.quantidade);
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
    
    // Configurar timeout para telas de informação
    timeoutHabilitado = (tela == TipoTela::INFO_WIFI || 
                        tela == TipoTela::CONFIG_ULTIMO_BOOT);
    
    Display::clear();
    renderizar();  // Desenha imediatamente nova tela
}

void GerenciadorTelas::voltar() {
    irParaTela(telaAnterior);
}

String GerenciadorTelas::formatarHora(int hora, int minuto) {
    String h = String(hora);
    String m = String(minuto);
    
    if (h.length() < 2) h = "0" + h;
    if (m.length() < 2) m = "0" + m;
    
    return h + ":" + m;
}

String GerenciadorTelas::obterStatusRemota(int indice) {
    if (indice < numeroRemotas) {
        return remotas[indice].conectada ? "OK" : "OFF";
    }
    return "N/A";
}

void GerenciadorTelas::atualizarBarraProgresso(int progresso) {
    Display::setCursor(0, 1);
    
    int tamanhoBar = 10;
    int preenchido = (progresso * tamanhoBar) / 100;
    
    String barra = "";
    for (int i = 0; i < tamanhoBar; i++) {
        barra += (i < preenchido) ? "█" : "░";
    }
    
    Display::print(barra + " " + String(progresso) + "%");
}

void GerenciadorTelas::centralizarTexto(int linha, String texto) {
    int espacamento = (LCD_COLS - texto.length()) / 2;
    Display::printAt(espacamento, linha, texto);
}

void GerenciadorTelas::desenharSeletor(int linha) {
    Display::printAt(0, linha, ">");
}

bool GerenciadorTelas::verificarTimeout() {
    return (millis() - inicioTela) > INFO_SCREEN_TIMEOUT;
}

// =============================================================================
// PERSISTÊNCIA DE DADOS
// =============================================================================

void GerenciadorTelas::salvarDadosRemota() {
    Preferences prefs;
    prefs.begin("remotas", false);
    
    prefs.putInt("count", numeroRemotas);
    
    for (int i = 0; i < numeroRemotas; i++) {
        String prefixo = "r" + String(i) + "_";
        
        prefs.putInt((prefixo + "id").c_str(), remotas[i].id);
        prefs.putString((prefixo + "nome").c_str(), remotas[i].nome);
        prefs.putBool((prefixo + "conn").c_str(), remotas[i].conectada);
        
        for (int j = 0; j < REFEICOES_PER_REMOTA; j++) {
            String refPrefixo = prefixo + "ref" + String(j) + "_";
            prefs.putInt((refPrefixo + "hora").c_str(), remotas[i].refeicoes[j].hora);
            prefs.putInt((refPrefixo + "min").c_str(), remotas[i].refeicoes[j].minuto);
            prefs.putInt((refPrefixo + "quant").c_str(), remotas[i].refeicoes[j].quantidade);
            prefs.putString((refPrefixo + "ultima").c_str(), remotas[i].refeicoes[j].ultimaExecucao);
        }
    }
    
    prefs.end();
    Serial.println("Dados das remotas salvos");
}

void GerenciadorTelas::carregarDadosRemota() {
    Preferences prefs;
    prefs.begin("remotas", true);
    
    numeroRemotas = prefs.getInt("count", 0);
    
    for (int i = 0; i < numeroRemotas && i < MAX_REMOTAS; i++) {
        String prefixo = "r" + String(i) + "_";
        
        remotas[i].id = prefs.getInt((prefixo + "id").c_str(), i + 1);
        remotas[i].nome = prefs.getString((prefixo + "nome").c_str(), "Remota " + String(i + 1));
        remotas[i].conectada = prefs.getBool((prefixo + "conn").c_str(), false);
        
        for (int j = 0; j < REFEICOES_PER_REMOTA; j++) {
            String refPrefixo = prefixo + "ref" + String(j) + "_";
            remotas[i].refeicoes[j].hora = prefs.getInt((refPrefixo + "hora").c_str(), 8 + j * 6);
            remotas[i].refeicoes[j].minuto = prefs.getInt((refPrefixo + "min").c_str(), 0);
            remotas[i].refeicoes[j].quantidade = prefs.getInt((refPrefixo + "quant").c_str(), 40);
            remotas[i].refeicoes[j].ultimaExecucao = prefs.getString((refPrefixo + "ultima").c_str(), "Nunca");
        }
    }
    
    prefs.end();
    
    if (numeroRemotas > 0) {
        Serial.println("Dados das remotas carregados: " + String(numeroRemotas) + " remotas");
    }
}

// =============================================================================
// MÉTODOS PÚBLICOS DE ATUALIZAÇÃO
// =============================================================================

void GerenciadorTelas::atualizarStatusWifi(String ssid, bool conectado, String qualidade) {
    wifiSSID = ssid;
    wifiConectado = conectado;
    qualidadeWifi = qualidade;
}

void GerenciadorTelas::atualizarStatusMqtt(bool conectado) {
    mqttConectado = conectado;
}

void GerenciadorTelas::atualizarTempo() {
    // Forçar atualização da tela quando o tempo muda
    // Isso garante que a hora seja atualizada automaticamente
    static String tempoAnterior = "";
    
    DadosTempo tempoAtual = GerenciadorTempo::obterTempoAtual();
    
    // Se o tempo formatado mudou, força redraw da tela
    if (tempoAtual.tempoFormatado != tempoAnterior) {
        tempoAnterior = tempoAtual.tempoFormatado;
        precisaRedraw = true; // Força atualização da tela
        
        // Debug para acompanhar a atualização automática
        Serial.printf("⏰ [DISPLAY] Hora atualizada automaticamente: %s\n", tempoAtual.tempoFormatado.c_str());
    }
}

void GerenciadorTelas::atualizarTempoBooter(String data, String hora) {
    dataUltimoBooter = data;
    horaUltimoBooter = hora;
}

void GerenciadorTelas::adicionarRemota(int id, String nome, bool conectada) {
    if (numeroRemotas < MAX_REMOTAS) {
        remotas[numeroRemotas].id = id;
        remotas[numeroRemotas].nome = nome;
        remotas[numeroRemotas].conectada = conectada;
        remotas[numeroRemotas].ultimoHeartbeat = 0; // Inicializar sem heartbeat
        
        // Inicializar refeições padrão
        for (int i = 0; i < REFEICOES_PER_REMOTA; i++) {
            remotas[numeroRemotas].refeicoes[i].hora = 8 + i * 6;
            remotas[numeroRemotas].refeicoes[i].minuto = 0;
            remotas[numeroRemotas].refeicoes[i].quantidade = 40;
            remotas[numeroRemotas].refeicoes[i].ultimaExecucao = "Nunca";
        }
        
        numeroRemotas++;
        salvarDadosRemota();
    }
}

void GerenciadorTelas::atualizarStatusRemota(int id, bool conectada) {
    for (int i = 0; i < numeroRemotas; i++) {
        if (remotas[i].id == id) {
            remotas[i].conectada = conectada;
            salvarDadosRemota();
            break;
        }
    }
}

void GerenciadorTelas::definirRefeicao(int idRemota, int indiceRefeicao, int hora, int minuto, int quantidade) {
    for (int i = 0; i < numeroRemotas; i++) {
        if (remotas[i].id == idRemota && indiceRefeicao < REFEICOES_PER_REMOTA) {
            remotas[i].refeicoes[indiceRefeicao].hora = hora;
            remotas[i].refeicoes[indiceRefeicao].minuto = minuto;
            remotas[i].refeicoes[indiceRefeicao].quantidade = quantidade;
            salvarDadosRemota();
            break;
        }
    }
}

void GerenciadorTelas::atualizarUltimaExecucao(int idRemota, int indiceRefeicao, String tempo) {
    for (int i = 0; i < numeroRemotas; i++) {
        if (remotas[i].id == idRemota && indiceRefeicao < REFEICOES_PER_REMOTA) {
            remotas[i].refeicoes[indiceRefeicao].ultimaExecucao = tempo;
            salvarDadosRemota();
            break;
        }
    }
}

void GerenciadorTelas::alternarLuzLcd() {
    luzLcd = !luzLcd;
    Display::backlight(luzLcd);
}

void GerenciadorTelas::resetarSistema() {
    if (callbackResetSistema) {
        callbackResetSistema();
    }
}

TipoTela GerenciadorTelas::obterTelaAtual() {
    return telaAtual;
}

bool GerenciadorTelas::estaEmModoEdicao() {
    return estaEditando;
}

void GerenciadorTelas::definirCallbackResetSistema(void (*callback)()) {
    callbackResetSistema = callback;
}

void GerenciadorTelas::definirCallbackAtualizacaoRefeicao(void (*callback)(int, int, int, int, int)) {
    callbackAtualizacaoRefeicao = callback;
}

// ===== CALLBACKS MQTT =====

void GerenciadorTelas::verificarTimeoutRemotas(unsigned long agora) {
    const unsigned long TIMEOUT_HEARTBEAT = 30000; // 30 segundos sem heartbeat = OFF
    
    for (int i = 0; i < numeroRemotas; i++) {
        if (remotas[i].conectada && (agora - remotas[i].ultimoHeartbeat > TIMEOUT_HEARTBEAT)) {
            remotas[i].conectada = false;
            precisaRedraw = true;
            DEBUG_PRINTF("[TELAS] Remota %d marcada como OFF por timeout (sem heartbeat há %lu ms)\n", 
                        remotas[i].id, agora - remotas[i].ultimoHeartbeat);
        }
    }
}

void GerenciadorTelas::onStatusRemotaRecebido(int idRemota, String status) {
    DEBUG_PRINTF("[TELAS] Status recebido - Remota %d: %s\n", idRemota, status.c_str());
    
    // Encontrar a remota pelo ID e atualizar status
    for (int i = 0; i < numeroRemotas; i++) {
        if (remotas[i].id == idRemota) {
            bool conectada = (status == "ONLINE" || status == "ALIVE");
            if (remotas[i].conectada != conectada) {
                remotas[i].conectada = conectada;
                precisaRedraw = true; // Forçar redraw da tela
                DEBUG_PRINTF("[TELAS] Remota %d status atualizado: %s\n", idRemota, conectada ? "ON" : "OFF");
            }
            break;
        }
    }
}

void GerenciadorTelas::onVidaRemotaRecebida(int idRemota, bool viva) {
    DEBUG_PRINTF("[TELAS] Heartbeat recebido - Remota %d: %s\n", idRemota, viva ? "VIVA" : "MORTA");
    
    // Encontrar a remota pelo ID e atualizar status de vida
    for (int i = 0; i < numeroRemotas; i++) {
        if (remotas[i].id == idRemota) {
            if (viva) {
                remotas[i].ultimoHeartbeat = millis(); // Atualizar timestamp
                if (!remotas[i].conectada) {
                    remotas[i].conectada = true;
                    precisaRedraw = true;
                    DEBUG_PRINTF("[TELAS] Remota %d heartbeat atualizado: ON\n", idRemota);
                }
            } else {
                // Heartbeat indica que está morta
                if (remotas[i].conectada) {
                    remotas[i].conectada = false;
                    precisaRedraw = true;
                    DEBUG_PRINTF("[TELAS] Remota %d heartbeat atualizado: OFF\n", idRemota);
                }
            }
            break;
        }
    }
}

void GerenciadorTelas::onRespostaRemotaRecebida(int idRemota, String resposta) {
    DEBUG_PRINTF("[TELAS] Resposta recebida - Remota %d: %s\n", idRemota, resposta.c_str());
    
    // Aqui podemos adicionar lógica adicional para processar respostas específicas
    // Por exemplo, atualizar timestamps de última execução de refeições
}

// =============================================================================
// VERIFICAÇÃO AUTOMÁTICA DE HORÁRIOS DE ALIMENTAÇÃO
// =============================================================================

void GerenciadorTelas::verificarHorariosAutomaticos() {
    unsigned long agora = millis();
    
    // Verificar apenas a cada 30 segundos (para não sobrecarregar)
    const unsigned long INTERVALO_VERIFICACAO = 30000; // 30 segundos
    if (agora - ultimaVerificacaoHorarios < INTERVALO_VERIFICACAO) {
        return;
    }
    
    ultimaVerificacaoHorarios = agora;
    
    // Obter tempo atual
    DadosTempo tempoAtual = GerenciadorTempo::obterTempoAtual();
    int horaAtual = tempoAtual.hora;
    int minutoAtual = tempoAtual.minuto;
    
    // Print de debug para mostrar verificação
    Serial.printf("🕐 [HORARIOS] Verificação automática: %02d:%02d\n", horaAtual, minutoAtual);
    
    // Verificar cada remota cadastrada
    for (int r = 0; r < numeroRemotas; r++) {
        int idRemota = remotas[r].id;
        
        // Só verificar remotas conectadas
        if (!remotas[r].conectada) {
            continue;
        }
        
        // Verificar cada refeição da remota
        for (int ref = 0; ref < REFEICOES_PER_REMOTA; ref++) {
            Refeicao* refeicao = &remotas[r].refeicoes[ref];
            
            // Verificar se o horário programado coincide com o horário atual
            if (refeicao->hora == horaAtual && refeicao->minuto == minutoAtual) {
                
                // Print de debug da flag encontrada
                Serial.printf("🚨 [HORARIOS] FLAG ATIVADA! Remota %d, Refeição %d\n", idRemota, ref + 1);
                Serial.printf("    ⏰ Horário programado: %02d:%02d\n", refeicao->hora, refeicao->minuto);
                Serial.printf("    ⏰ Horário atual: %02d:%02d\n", horaAtual, minutoAtual);
                Serial.printf("    🍽️ Quantidade: %dg\n", refeicao->quantidade);
                
                // Calcular tempo de alimentação (a cada 10g = 1 segundo)
                int tempoSegundos = refeicao->quantidade / 10;
                if (tempoSegundos < 1) tempoSegundos = 1; // Mínimo de 1 segundo
                if (tempoSegundos > 30) tempoSegundos = 30; // Máximo de 30 segundos
                
                Serial.printf("    ⏱️ Tempo calculado: %d segundos (quantidade %dg ÷ 10)\n", 
                             tempoSegundos, refeicao->quantidade);
                
                // Enviar comando MQTT para alimentar
                if (GerenciadorMQTT::estaConectado()) {
                    Serial.printf("📤 [HORARIOS] Enviando comando ALIMENTAR para Remota %d...\n", idRemota);
                    
                    bool resultado = GerenciadorMQTT::enviarComandoGeral("alimentar", tempoSegundos, idRemota);
                    
                    if (resultado) {
                        Serial.printf("✅ [HORARIOS] Comando enviado com sucesso!\n");
                        
                        // Atualizar última execução
                        String tempoFormatado = formatarHora(horaAtual, minutoAtual);
                        refeicao->ultimaExecucao = tempoFormatado;
                        
                        // Salvar dados atualizados
                        salvarDadosRemota();
                        
                        Serial.printf("💾 [HORARIOS] Última execução atualizada: %s\n", tempoFormatado.c_str());
                        
                    } else {
                        Serial.printf("❌ [HORARIOS] Falha ao enviar comando MQTT\n");
                    }
                    
                } else {
                    Serial.printf("❌ [HORARIOS] MQTT não conectado - comando não enviado\n");
                }
                
                Serial.println("🚨 [HORARIOS] ==========================================");
            }
        }
    }
}
