#include "menu.h"
#include "display.h"
#include "botoes.h"
#include "gerenciador_telas.h"

// Inicializar variáveis estáticas do Menu em português
int Menu::itemAtual = 0;
int Menu::totalItens = 0;
String Menu::itensMenu[MAX_MENU_ITEMS];
void (*Menu::callbacks[MAX_MENU_ITEMS])() = {nullptr};
String Menu::tituloMenu = "";
bool Menu::menuAtivo = false;

void Menu::inicializar() {
    Serial.println("=== Inicializando Menu ===");
    menuAtivo = false;
    itemAtual = 0;
    totalItens = 0;
    tituloMenu = "";
    
    // Limpar arrays
    for (int i = 0; i < MAX_MENU_ITEMS; i++) {
        itensMenu[i] = "";
        callbacks[i] = nullptr;
    }
    
    Serial.println("Menu inicializado");
}

void Menu::definirTitulo(String titulo) {
    tituloMenu = titulo;
}

void Menu::adicionarItem(String nome, void (*callback)()) {
    if (totalItens < MAX_MENU_ITEMS) {
        itensMenu[totalItens] = nome;
        callbacks[totalItens] = callback;
        totalItens++;
    }
}

void Menu::mostrar() {
    if (totalItens == 0) return;
    
    menuAtivo = true;
    Display::clear();
    
    // Mostrar título se definido
    if (tituloMenu.length() > 0) {
        Display::printCentered(0, tituloMenu);
        Display::drawLine(1);
    }
    
    renderizarItens();
}

void Menu::ocultar() {
    menuAtivo = false;
    Display::clear();
}

void Menu::atualizar() {
    if (!menuAtivo || totalItens == 0) return;
    
    // Verificar navegação
    if (Botoes::cimaPressionado()) {
        itemAtual = (itemAtual - 1 + totalItens) % totalItens;
        renderizarItens();
    }
    else if (Botoes::baixoPressionado()) {
        itemAtual = (itemAtual + 1) % totalItens;
        renderizarItens();
    }
    else if (Botoes::enterPressionado()) {
        executarItemSelecionado();
    }
}

void Menu::limpar() {
    totalItens = 0;
    itemAtual = 0;
    tituloMenu = "";
    menuAtivo = false;
    
    for (int i = 0; i < MAX_MENU_ITEMS; i++) {
        itensMenu[i] = "";
        callbacks[i] = nullptr;
    }
}

bool Menu::estaAtivo() {
    return menuAtivo;
}

int Menu::obterItemSelecionado() {
    return itemAtual;
}

void Menu::definirItemSelecionado(int indice) {
    if (indice >= 0 && indice < totalItens) {
        itemAtual = indice;
    }
}

void Menu::renderizarItens() {
    int linhaInicio = (tituloMenu.length() > 0) ? 2 : 0;
    int itensVisiveis = LCD_ROWS - linhaInicio;
    
    // Calcular primeiro item visível
    int primeiroItem = max(0, min(itemAtual - itensVisiveis/2, totalItens - itensVisiveis));
    
    // Limpar área dos itens
    for (int i = linhaInicio; i < LCD_ROWS; i++) {
        Display::printAt(0, i, String(" ").c_str());
    }
    
    // Renderizar itens visíveis
    for (int i = 0; i < itensVisiveis && (primeiroItem + i) < totalItens; i++) {
        int indiceItem = primeiroItem + i;
        int linha = linhaInicio + i;
        
        String prefixo = (indiceItem == itemAtual) ? "> " : "  ";
        String textoCompleto = prefixo + itensMenu[indiceItem];
        
        // Truncar se muito longo
        if (textoCompleto.length() > LCD_COLS) {
            textoCompleto = textoCompleto.substring(0, LCD_COLS - 3) + "...";
        }
        
        Display::printAt(0, linha, textoCompleto);
    }
    
    // Mostrar indicadores de scroll se necessário
    if (totalItens > itensVisiveis) {
        if (primeiroItem > 0) {
            Display::printAt(LCD_COLS - 1, linhaInicio, "↑");
        }
        if (primeiroItem + itensVisiveis < totalItens) {
            Display::printAt(LCD_COLS - 1, LCD_ROWS - 1, "↓");
        }
    }
}

void Menu::executarItemSelecionado() {
    if (itemAtual >= 0 && itemAtual < totalItens && callbacks[itemAtual] != nullptr) {
        callbacks[itemAtual]();
    }
}
