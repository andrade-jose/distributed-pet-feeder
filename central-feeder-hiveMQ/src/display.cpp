#include "display.h"
#include <Arduino.h>

// Inicializar LCD estático
LiquidCrystal_I2C Display::lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

void Display::init()
{
    lcd.init();
    lcd.backlight();
    lcd.clear();

    DEBUG_PRINTLN("=== Display LCD Inicializado ===");
    DEBUG_PRINTF("Endereço I2C: 0x%02X\n", LCD_I2C_ADDR);
    DEBUG_PRINTF("Tamanho: %dx%d\n", LCD_COLS, LCD_ROWS);
    DEBUG_PRINTLN();
}

void Display::clear()
{
    lcd.clear();
}

void Display::backlight(bool on)
{
    if (on)
    {
        lcd.backlight();
    }
    else
    {
        lcd.noBacklight();
    }
}

void Display::setCursor(int col, int row)
{
    lcd.setCursor(col, row);
}

void Display::print(String text)
{
    lcd.print(text);
}

void Display::print(int number)
{
    lcd.print(number);
}

void Display::print(float number)
{
    lcd.print(number);
}

void Display::printAt(int col, int row, String text)
{
    lcd.setCursor(col, row);
    lcd.print(text);
}

void Display::printAt(int col, int row, int number)
{
    lcd.setCursor(col, row);
    lcd.print(number);
}

void Display::printCentered(int row, String text)
{
    int padding = (LCD_COLS - text.length()) / 2;
    if (padding < 0)
        padding = 0;

    lcd.setCursor(padding, row);
    lcd.print(text);
}

void Display::drawLine(int row, char character)
{
    lcd.setCursor(0, row);
    for (int i = 0; i < LCD_COLS; i++)
    {
        lcd.print(character);
    }
}

void Display::showWelcome()
{
    clear();
    printCentered(0, "=================");
    printCentered(1, "Central Alimentacao");
    printCentered(2, "Inicializando...");
    printCentered(3, "=================");
}

void Display::showError(String message)
{
    clear();
    printCentered(0, "*** ERRO ***");
    printCentered(2, message);
}

void Display::showMessage(String line1, String line2, String line3, String line4)
{
    clear();

    if (line1.length() > 0)
        printAt(0, 0, line1);
    if (line2.length() > 0)
        printAt(0, 1, line2);
    if (line3.length() > 0)
        printAt(0, 2, line3);
    if (line4.length() > 0)
        printAt(0, 3, line4);
}
