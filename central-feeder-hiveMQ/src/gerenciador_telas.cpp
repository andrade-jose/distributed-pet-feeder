#include "gerenciador_telas.h"
#include <config.h>
#include <Preferences.h>
#include "gerenciador_mqtt.h"

// InicializaÃ§Ã£o de variÃ¡veis estÃ¡ticas
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

// Controle de verificaÃ§Ã£o automÃ¡tica de horÃ¡rios
unsigned long GerenciadorTelas::ultimaVerificacaoHorarios = 0;

// Controle de alerta de ração
bool GerenciadorTelas::alertaRacaoAtivo = false;
int GerenciadorTelas::remotaComAlerta = -1;
unsigned long GerenciadorTelas::ultimaPiscadaAlerta = 0;
bool GerenciadorTelas::estadoPiscaAlerta = false;

// Callbacks
void (*GerenciadorTelas::callbackResetSistema)() = nullptr;
void (*GerenciadorTelas::callbackAtualizacaoRefeicao)(int, int, int, int, int) = nullptr;
// Flag para controle de redraw
bool GerenciadorTelas::precisaRedraw = false;

// Sistema de paginação dinâmica
int GerenciadorTelas::remotasConectadasCache[MAX_REMOTAS];
int GerenciadorTelas::numRemotasConectadasCache = 0;

void GerenciadorTelas::inicializar() {
    Serial.println("=== Inicializando Gerenciador de Telas ===");

    // Carregar dados salvos
    carregarDadosRemota();

    if (numeroRemotas == 0) {
        adicionarRemota(1, "Remota 1", false);
        adicionarRemota(2, "Remota 2", false);
        adicionarRemota(3, "Remota 3", false);
        adicionarRemota(4, "Remota 4", false);

        definirRefeicao(1, 0, 8, 0, 40);
        definirRefeicao(1, 1, 14, 30, 40);
        definirRefeicao(1, 2, 20, 0, 40);
    }

    // Atualizar tempo de boot
    DadosTempo tempoAtual = GerenciadorTempo::obterTempoAtual();
    atualizarTempoBooter(tempoAtual.dataFormatada, tempoAtual.tempoFormatado);

    // ✅ Callback para atualizar tempo automaticamente
    GerenciadorTempo::definirCallbackAtualizacaoTempo([](DadosTempo tempo) {
        GerenciadorTelas::atualizarTempo();
    });

    // ✅ Começar na tela inicial
    irParaTela(TipoTela::INICIO);

    Serial.println("Gerenciador de Telas inicializado");
}


// Alterar as implementações das funções callback para usar const String&:
void GerenciadorTelas::onStatusRemotaRecebido(int idRemota, const String& status) {
    DEBUG_PRINTF("[TELAS] Status recebido - Remota %d: %s\n", idRemota, status.c_str());
    
    for (int i = 0; i < numeroRemotas; i++) {
        if (remotas[i].id == idRemota) {
            bool conectada = (status == "ONLINE" || status == "ALIVE");
            if (remotas[i].conectada != conectada) {
                remotas[i].conectada = conectada;
                precisaRedraw = true;
                DEBUG_PRINTF("[TELAS] Remota %d status atualizado: %s\n", idRemota, conectada ? "ON" : "OFF");
            }
            break;
        }
    }
}

void GerenciadorTelas::onVidaRemotaRecebida(int idRemota, const String& status) {
    bool viva = (status == "ALIVE" || status == "true" || status == "1");
    DEBUG_PRINTF("[TELAS] Heartbeat recebido - Remota %d: %s\n", idRemota, viva ? "VIVA" : "MORTA");
    
    for (int i = 0; i < numeroRemotas; i++) {
        if (remotas[i].id == idRemota) {
            if (viva) {
                remotas[i].ultimoHeartbeat = millis();
                if (!remotas[i].conectada) {
                    remotas[i].conectada = true;
                    precisaRedraw = true;
                }
            } else {
                if (remotas[i].conectada) {
                    remotas[i].conectada = false;
                    precisaRedraw = true;
                }
            }
            break;
        }
    }
}

void GerenciadorTelas::onRespostaRemotaRecebida(int idRemota, const String& resposta) {
    DEBUG_PRINTF("[TELAS] Resposta recebida - Remota %d: %s\n", idRemota, resposta.c_str());
}

void GerenciadorTelas::onAlertaRacaoRecebido(int idRemota, const String& alerta) {
    DEBUG_PRINTF("Alerta recebido - Remota %d: %s\n", idRemota, alerta.c_str());
    
    if (alerta == "BAIXO" || alerta.indexOf("baixo") >= 0) {
        alertaRacaoAtivo = true;
        remotaComAlerta = idRemota;
        
        if (!estaEditando) {
            irParaTela(TipoTela::ALERTA_RACAO_BAIXA);
        }
    }
    else if (alerta == "OK" || alerta.indexOf("ok") >= 0) {
        if (remotaComAlerta == idRemota) {
            alertaRacaoAtivo = false;
            remotaAtual = -1;
            
            if (telaAtual == TipoTela::ALERTA_RACAO_BAIXA) {
                irParaTela(TipoTela::INICIO);
            }
        }
    }
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
        
        // Verificar horÃ¡rios de alimentaÃ§Ã£o automÃ¡tica
        verificarHorariosAutomaticos();
        // Verificar alertas de ração
        verificarAlertas();
        
        // Verificar timeout se habilitado
        if (timeoutHabilitado && verificarTimeout()) {
            irParaTela(TipoTela::INICIO);
            return;
        }
        
        // Processar navegaÃ§Ã£o e sinalizar redraw se mudou seleÃ§Ã£o ou tela
        TipoTela telaAntiga = telaAtual;
        int opcaoAntiga = opcaoSelecionada;
        int campoAntigo = campoEdicao;
        bool estaEditandoAntigo = estaEditando;
        
        gerenciarNavegacao();
        
        // Redraw se mudou tela, seleÃ§Ã£o, campo de ediÃ§Ã£o ou estado de ediÃ§Ã£o
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
        case TipoTela::ALERTA_RACAO_BAIXA:
            renderizarAlertaRacao();
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
        case TipoTela::ALERTA_RACAO_BAIXA:
            navegarAlertaRacao();
            break;
    }
}

// =============================================================================
// MÃ‰TODOS DE RENDERIZAÃ‡ÃƒO
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

    // Usar variável membro mqttConectado para evitar acesso a ponteiro
    Display::printAt(0, 3, String("MQTT: ") + (mqttConectado ? "Conectado" : "Desconectado"));
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
    // Contar quantas remotas conectadas existem e criar mapeamento
    int remotasConectadas[MAX_REMOTAS];
    int numConectadas = 0;
    for (int i = 0; i < numeroRemotas && numConectadas < MAX_REMOTAS; i++) {
        if (remotas[i].conectada) {
            remotasConectadas[numConectadas++] = i;
        }
    }

    // Mostrar atÃ© 3 remotas conectadas
    int linhasUsadas = 0;
    for (int i = 0; i < 3 && i < numConectadas; i++) {
        int indiceRemota = remotasConectadas[i];
        Display::setCursor(0, linhasUsadas);
        String linha = remotas[indiceRemota].nome + ": OK";

        if (i == opcaoSelecionada) {
            Display::print("> " + linha);
        } else {
            Display::print("  " + linha);
        }
        linhasUsadas++;
    }

    // Se nÃ£o hÃ¡ remotas conectadas, mostrar mensagem
    if (numConectadas == 0) {
        centralizarTexto(1, "Nenhuma remota");
        centralizarTexto(2, "conectada");
        Display::setCursor(0, 3);
        if (opcaoSelecionada == 0) {
            Display::print("> Voltar");
        } else {
            Display::print("  Voltar");
        }
        return;
    }

    // Mostrar opÃ§Ã£o baseada no nÃºmero de remotas conectadas
    Display::setCursor(0, linhasUsadas);

    if (numConectadas > 3) {
        // HÃ¡ mais remotas conectadas - mostrar "PrÃ³xima pÃ¡gina"
        if (opcaoSelecionada == min(numConectadas, 3)) {
            Display::print("> Proxima pagina");
        } else {
            Display::print("  Proxima pagina");
        }
    } else {
        // NÃ£o hÃ¡ mais remotas conectadas - mostrar "Voltar"
        if (opcaoSelecionada == numConectadas) {
            Display::print("> Voltar");
        } else {
            Display::print("  Voltar");
        }
    }
}

void GerenciadorTelas::renderizarListaRemotasP2() {
    // Contar quantas remotas conectadas existem e criar mapeamento
    int remotasConectadas[MAX_REMOTAS];
    int numConectadas = 0;
    for (int i = 0; i < numeroRemotas && numConectadas < MAX_REMOTAS; i++) {
        if (remotas[i].conectada) {
            remotasConectadas[numConectadas++] = i;
        }
    }

    // Mostrar remota 4 em diante (se houver mais de 3 conectadas)
    int linhasUsadas = 0;
    if (numConectadas > 3) {
        int indiceRemota = remotasConectadas[3]; // Quarta remota conectada
        Display::setCursor(0, 0);
        String linha = remotas[indiceRemota].nome + ": OK";

        if (opcaoSelecionada == 0) {
            Display::print("> " + linha);
        } else {
            Display::print("  " + linha);
        }
        linhasUsadas++;
    }

    // OpÃ§Ã£o PÃ¡gina Anterior
    Display::setCursor(0, linhasUsadas);
    if (opcaoSelecionada == (numConectadas > 3 ? 1 : 0)) {
        Display::print("> Pagina Anterior");
    } else {
        Display::print("  Pagina Anterior");
    }
    linhasUsadas++;

    // OpÃ§Ã£o Menu Principal
    Display::setCursor(0, linhasUsadas);
    if (opcaoSelecionada == (numConectadas > 3 ? 2 : 1)) {
        Display::print("> Menu Principal");
    } else {
        Display::print("  Menu Principal");
    }

    // Indicador de pÃ¡gina
    Display::printAt(0, 3, "[Pagina 2/2]");
}

void GerenciadorTelas::renderizarRemotaEspecifica() {
    
    if (remotaAtual < numeroRemotas) {
        Remota& remota = remotas[remotaAtual];
        
        // Mostrar refeiÃ§Ãµes
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
        
        // OpÃ§Ã£o voltar
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
            // Aguardando Enter para comeÃ§ar
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
// MÃ‰TODOS DE NAVEGAÃ‡ÃƒO
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
    // Contar remotas conectadas e criar mapeamento
    int remotasConectadas[MAX_REMOTAS];
    int numConectadas = 0;
    for (int i = 0; i < numeroRemotas && numConectadas < MAX_REMOTAS; i++) {
        if (remotas[i].conectada) {
            remotasConectadas[numConectadas++] = i;
        }
    }

    // Se nÃ£o hÃ¡ remotas conectadas, apenas voltar
    if (numConectadas == 0) {
        if (Botoes::enterPressionado() || Botoes::cimaPressionado() || Botoes::baixoPressionado()) {
            irParaTela(TipoTela::INICIO);
        }
        return;
    }

    int maxOpcoes = min(numConectadas, 3) + 1; // +1 para opÃ§Ã£o extra

    if (Botoes::cimaPressionado()) {
        opcaoSelecionada = (opcaoSelecionada - 1 + maxOpcoes) % maxOpcoes;
    }
    else if (Botoes::baixoPressionado()) {
        opcaoSelecionada = (opcaoSelecionada + 1) % maxOpcoes;
    }
    else if (Botoes::enterPressionado()) {
        if (opcaoSelecionada < min(numConectadas, 3)) {
            // Selecionou uma remota conectada - mapear para Ã­ndice real
            remotaAtual = remotasConectadas[opcaoSelecionada];
            irParaTela(TipoTela::REMOTA_ESPECIFICA);
        } else {
            // Selecionou a opÃ§Ã£o extra
            if (numConectadas > 3) {
                // HÃ¡ mais remotas conectadas - ir para prÃ³xima pÃ¡gina
                irParaTela(TipoTela::LISTA_REMOTAS_P2);
            } else {
                // NÃ£o hÃ¡ mais remotas - voltar para Inicio
                irParaTela(TipoTela::INICIO);
            }
        }
    }
}

void GerenciadorTelas::navegarListaRemotasP2() {
    // Contar remotas conectadas e criar mapeamento
    int remotasConectadas[MAX_REMOTAS];
    int numConectadas = 0;
    for (int i = 0; i < numeroRemotas && numConectadas < MAX_REMOTAS; i++) {
        if (remotas[i].conectada) {
            remotasConectadas[numConectadas++] = i;
        }
    }

    int maxOpcoes = (numConectadas > 3) ? 3 : 2; // Remota4 conectada + PÃ¡gina Anterior + Menu Principal

    if (Botoes::cimaPressionado()) {
        opcaoSelecionada = (opcaoSelecionada - 1 + maxOpcoes) % maxOpcoes;
    }
    else if (Botoes::baixoPressionado()) {
        opcaoSelecionada = (opcaoSelecionada + 1) % maxOpcoes;
    }
    else if (Botoes::enterPressionado()) {
        if (numConectadas > 3 && opcaoSelecionada == 0) {
            // Selecionou quarta remota conectada - mapear para Ã­ndice real
            remotaAtual = remotasConectadas[3];
            irParaTela(TipoTela::REMOTA_ESPECIFICA);
        } else if (opcaoSelecionada == 1 || (numConectadas <= 3 && opcaoSelecionada == 0)) {
            // PÃ¡gina Anterior
            irParaTela(TipoTela::LISTA_REMOTAS_P1);
        } else {
            // Menu Principal
            irParaTela(TipoTela::INICIO);
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
            // Remover a verificaÃ§Ã£o restritiva de conexÃ£o
            // Permitir ediÃ§Ã£o sempre, pois a web interface jÃ¡ confirma que funciona
            refeicaoAtual = opcaoSelecionada;
            irParaTela(TipoTela::CONFIG_REFEICAO);
        } else {
            // Voltar para a pÃ¡gina correta das remotas
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
            // NÃƒO iniciar ediÃ§Ã£o automaticamente - precisa pressionar Enter na tela
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
        // Ainda nÃ£o comeÃ§ou a editar - esperar Enter para comeÃ§ar
        if (Botoes::enterPressionado()) {
            Serial.println("DEBUG: Enter detectado - iniciando ediÃ§Ã£o");
            estaEditando = true;
            campoEdicao = 0; // comeÃ§ar com a hora
            precisaRedraw = true; // forÃ§ar redraw para mostrar modo ediÃ§Ã£o
        } else if (Botoes::cimaPressionado() || Botoes::baixoPressionado()) {
            // Voltar sem editar
            irParaTela(TipoTela::CONFIG_REFEICAO);
        }
    } else {
        // Modo ediÃ§Ã£o ativo
        Refeicao& refeicao = remotas[remotaAtual].refeicoes[refeicaoAtual];
        
        if (Botoes::cimaPressionado()) {
            if (campoEdicao == 0) { // editando hora
                refeicao.hora = (refeicao.hora + 1) % 24;
            } else { // editando minuto
                refeicao.minuto = (refeicao.minuto + 1) % 60;
            }
            precisaRedraw = true; // forÃ§ar redraw para mostrar novo valor
        }
        else if (Botoes::baixoPressionado()) {
            if (campoEdicao == 0) { // editando hora
                refeicao.hora = (refeicao.hora - 1 + 24) % 24;
            } else { // editando minuto
                refeicao.minuto = (refeicao.minuto - 1 + 60) % 60;
            }
            precisaRedraw = true; // forÃ§ar redraw para mostrar novo valor
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
            precisaRedraw = true; // forÃ§ar redraw para mostrar modo ediÃ§Ã£o
        } else if (Botoes::cimaPressionado() || Botoes::baixoPressionado()) {
            irParaTela(TipoTela::CONFIG_REFEICAO);
        }
    } else {
        Refeicao& refeicao = remotas[remotaAtual].refeicoes[refeicaoAtual];
        
        if (Botoes::cimaPressionado()) {
            refeicao.quantidade = min(refeicao.quantidade + 10, 990);
            precisaRedraw = true; // forÃ§ar redraw para mostrar novo valor
        }
        else if (Botoes::baixoPressionado()) {
            refeicao.quantidade = max(refeicao.quantidade - 10, 0);
            precisaRedraw = true; // forÃ§ar redraw para mostrar novo valor
        }
        else if (Botoes::enterPressionado()) {
            // Salvar e voltar para configuraÃ§Ã£o de refeiÃ§Ã£o
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
// MÃ‰TODOS UTILITÃRIOS
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
    
    // Configurar timeout para telas de informaÃ§Ã£o
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
        barra += (i < preenchido) ? "â–ˆ" : "â–‘";
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
// PERSISTÃŠNCIA DE DADOS
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
// MÃ‰TODOS PÃšBLICOS DE ATUALIZAÃ‡ÃƒO
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
    // ForÃ§ar atualizaÃ§Ã£o da tela quando o tempo muda
    // Isso garante que a hora seja atualizada automaticamente
    static String tempoAnterior = "";
    
    DadosTempo tempoAtual = GerenciadorTempo::obterTempoAtual();
    
    // Se o tempo formatado mudou, forÃ§a redraw da tela
    if (tempoAtual.tempoFormatado != tempoAnterior) {
        tempoAnterior = tempoAtual.tempoFormatado;
        precisaRedraw = true; // ForÃ§a atualizaÃ§Ã£o da tela
        
        // Debug para acompanhar a atualizaÃ§Ã£o automÃ¡tica
        Serial.printf("â° [DISPLAY] Hora atualizada automaticamente: %s\n", tempoAtual.tempoFormatado.c_str());
    }
    
    // VerificaÃ§Ã£o de mudanÃ§as nas Preferences a cada 10 segundos
    static unsigned long ultimaVerificacaoPrefs = 0;
    unsigned long agora = millis();
    
    if (agora - ultimaVerificacaoPrefs > 3000) {
        ultimaVerificacaoPrefs = agora;
        sincronizarComPreferences();
    }
}

void GerenciadorTelas::atualizarTempoBooter(String data, String hora) {
    dataUltimoBooter = data;
    horaUltimoBooter = hora;
}
void GerenciadorTelas::sincronizarComPreferences() {
    Preferences prefs;
    prefs.begin("remotas", true);
    
    bool precisaRecarregar = false;
    
    // Verificar TODAS as remotas e TODAS as refeiÃ§Ãµes
    for (int i = 0; i < numeroRemotas; i++) {
        for (int j = 0; j < REFEICOES_PER_REMOTA; j++) {
            String prefixo = "r" + String(i) + "_";
            String refPrefixo = prefixo + "ref" + String(j) + "_";
            
            int horaPrefs = prefs.getInt((refPrefixo + "hora").c_str(), -1);
            int minutoPrefs = prefs.getInt((refPrefixo + "min").c_str(), -1);
            int quantidadePrefs = prefs.getInt((refPrefixo + "quant").c_str(), -1);
            
            // Se os dados sÃ£o diferentes, precisa recarregar
            if (horaPrefs != -1 && (
                horaPrefs != remotas[i].refeicoes[j].hora ||
                minutoPrefs != remotas[i].refeicoes[j].minuto ||
                quantidadePrefs != remotas[i].refeicoes[j].quantidade)) {
                
                precisaRecarregar = true;
                DEBUG_PRINTF("[TELAS] MudanÃ§a detectada via web: Remota %d, RefeiÃ§Ã£o %d\n", i+1, j+1);
                DEBUG_PRINTF("   MemÃ³ria: %02d:%02d %dg\n", 
                           remotas[i].refeicoes[j].hora, 
                           remotas[i].refeicoes[j].minuto, 
                           remotas[i].refeicoes[j].quantidade);
                DEBUG_PRINTF("   Preferences: %02d:%02d %dg\n", horaPrefs, minutoPrefs, quantidadePrefs);
                break;
            }
        }
        if (precisaRecarregar) break;
    }
    
    prefs.end();
    
    if (precisaRecarregar) {
        DEBUG_PRINTLN("[TELAS] Recarregando todos os dados das Preferences...");
        carregarDadosRemota();
        precisaRedraw = true; // ForÃ§ar redraw da tela
        DEBUG_PRINTLN("[TELAS] SincronizaÃ§Ã£o completa - tela atualizada");
    }
}
void GerenciadorTelas::adicionarRemota(int id, String nome, bool conectada) {
    if (numeroRemotas < MAX_REMOTAS) {
        remotas[numeroRemotas].id = id;
        remotas[numeroRemotas].nome = nome;
        remotas[numeroRemotas].conectada = conectada;
        remotas[numeroRemotas].ultimoHeartbeat = 0; // Inicializar sem heartbeat
        
        // Inicializar refeiÃ§Ãµes padrÃ£o
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
            DEBUG_PRINTF("[TELAS] Remota %d marcada como OFF por timeout (sem heartbeat hÃ¡ %lu ms)\n", 
                        remotas[i].id, agora - remotas[i].ultimoHeartbeat);
        }
    }
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
                if (GerenciadorMQTT::instance && GerenciadorMQTT::instance->estaConectado()) {
                    Serial.printf("📤 [HORARIOS] Enviando comando ALIMENTAR para Remota %d...\n", idRemota);

                    bool resultado = GerenciadorMQTT::instance->enviarComandoGeral("alimentar", tempoSegundos, idRemota);

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

// =============================================================================
// SISTEMA DE ALERTA DE RAÇÃO BAIXA
// =============================================================================

void GerenciadorTelas::renderizarAlertaRacao() {
    unsigned long agora = millis();

    // Piscada a cada 500ms
    if (agora - ultimaPiscadaAlerta > ALERTA_PISCADA_INTERVALO) {
        ultimaPiscadaAlerta = agora;
        estadoPiscaAlerta = !estadoPiscaAlerta;
        precisaRedraw = true;
    }

    if (estadoPiscaAlerta) {
        // Mostrar alerta (piscada visível)
        centralizarTexto(0, ALERTA_RACAO_TITULO);
        centralizarTexto(1, ALERTA_RACAO_LINHA1);

        char buffer[20];
        sprintf(buffer, ALERTA_RACAO_LINHA2, remotaComAlerta);
        centralizarTexto(2, String(buffer));

        centralizarTexto(3, ALERTA_RACAO_LINHA3);
    } else {
        // Tela "vazia" (piscada invisível)
        Display::clear();
    }
}

void GerenciadorTelas::navegarAlertaRacao() {
    // Qualquer botão volta para o menu principal quando alerta não está mais ativo
    if (!alertaRacaoAtivo && (Botoes::cimaPressionado() || Botoes::baixoPressionado() || Botoes::enterPressionado())) {
        irParaTela(TipoTela::INICIO);
    }
}

void GerenciadorTelas::verificarAlertas() {
    static unsigned long ultimoAlertaPeriodico = 0;
    unsigned long agora = millis();

    // Se há alerta ativo, verificar se é hora de mostrar o alerta novamente
    if (alertaRacaoAtivo) {
        // Mostrar alerta a cada 5 minutos ou se acabou de ser ativado
        if (agora - ultimoAlertaPeriodico >= 300000 || ultimoAlertaPeriodico == 0) { // 5 minutos = 300000ms
            if (telaAtual != TipoTela::ALERTA_RACAO_BAIXA && !estaEditando) {
                DEBUG_ALERTA_PRINTLN("Mostrando alerta periódico");
                irParaTela(TipoTela::ALERTA_RACAO_BAIXA);
                ultimoAlertaPeriodico = agora;
            }
        }

        // Se estamos na tela de alerta há mais de 10 segundos, voltar para tela inicial
        static unsigned long inicioTelaAlerta = 0;
        if (telaAtual == TipoTela::ALERTA_RACAO_BAIXA) {
            if (inicioTelaAlerta == 0) {
                inicioTelaAlerta = agora;
            } else if (agora - inicioTelaAlerta >= 10000) { // 10 segundos
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