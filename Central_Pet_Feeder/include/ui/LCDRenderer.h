#pragma once
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"

class LCDRenderer {
private:
    LiquidCrystal_I2C* lcd;

public:
    LCDRenderer();

    bool init();
    void clear();
    void setCursor(int col, int row);
    void print(const String& text);
    void printAt(int col, int row, const String& text);
    void printCentered(int row, const String& text);

    // Renderização de 4 linhas (interface principal)
    void render(const String& line0, const String& line1, const String& line2, const String& line3);

    // Utilidades
    String padRight(const String& text, int width);
    String padLeft(const String& text, int width);
    String center(const String& text, int width);

    LiquidCrystal_I2C* getLCD() { return lcd; }
};