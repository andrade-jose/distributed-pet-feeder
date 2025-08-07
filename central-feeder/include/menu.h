#pragma once
#include <Arduino.h>
#include "config.h"

// Número máximo de itens no menu
#define MAX_MENU_ITEMS 10

// Classe para gerenciar menus em português
class Menu {
private:
    static int itemAtual;
    static int totalItens;
    static String itensMenu[MAX_MENU_ITEMS];
    static void (*callbacks[MAX_MENU_ITEMS])();
    static String tituloMenu;
    static bool menuAtivo;
    
    // Métodos privados
    static void renderizarItens();
    static void executarItemSelecionado();

public:
    // Inicialização
    static void inicializar();
    
    // Configuração do menu
    static void definirTitulo(String titulo);
    static void adicionarItem(String nome, void (*callback)());
    static void limpar();
    
    // Controle de exibição
    static void mostrar();
    static void ocultar();
    static void atualizar();
    
    // Estado atual
    static bool estaAtivo();
    static int obterItemSelecionado();
    static void definirItemSelecionado(int indice);
};
