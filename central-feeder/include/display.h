#pragma once
#include <LiquidCrystal_I2C.h>
#include "config.h"

// Classe para gerenciar o display LCD
class Display
{
private:
    static LiquidCrystal_I2C lcd;

public:
    // Inicializar LCD
    static void init();

    // Controle básico
    static void clear();
    static void backlight(bool on = true);
    static void setCursor(int col, int row);

    // Escrita de texto
    static void print(String text);
    static void print(int number);
    static void print(float number);

    // Escrita com posição
    static void printAt(int col, int row, String text);
    static void printAt(int col, int row, int number);

    // Centralizar texto em uma linha
    static void printCentered(int row, String text);

    // Desenhar linha horizontal
    static void drawLine(int row, char character = '-');

    // Mostrar tela de boas-vindas
    static void showWelcome();

    // Mostrar mensagem de erro
    static void showError(String message);

    // Mostrar mensagem temporária
    static void showMessage(String line1, String line2 = "", String line3 = "", String line4 = "");
};
