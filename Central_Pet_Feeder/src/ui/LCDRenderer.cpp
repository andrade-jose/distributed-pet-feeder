#include "ui/LCDRenderer.h"

LCDRenderer::LCDRenderer() {
    lcd = new LiquidCrystal_I2C(LCD_ADDRESS, LCD_COLS, LCD_ROWS);
}

bool LCDRenderer::init() {
    Serial.println("[LCDRenderer] Inicializando display...");

    lcd->init();
    lcd->backlight();
    lcd->clear();

    Serial.println("[LCDRenderer] Display inicializado (20x4)");
    return true;
}

void LCDRenderer::clear() {
    lcd->clear();
}

void LCDRenderer::setCursor(int col, int row) {
    lcd->setCursor(col, row);
}

void LCDRenderer::print(const String& text) {
    lcd->print(text);
}

void LCDRenderer::printAt(int col, int row, const String& text) {
    lcd->setCursor(col, row);
    lcd->print(text);
}

void LCDRenderer::printCentered(int row, const String& text) {
    String centered = center(text, LCD_COLS);
    printAt(0, row, centered);
}

void LCDRenderer::render(const String& line0, const String& line1, const String& line2, const String& line3) {
    clear();
    printAt(0, 0, padRight(line0, LCD_COLS));
    printAt(0, 1, padRight(line1, LCD_COLS));
    printAt(0, 2, padRight(line2, LCD_COLS));
    printAt(0, 3, padRight(line3, LCD_COLS));
}

String LCDRenderer::padRight(const String& text, int width) {
    String result = text;
    while (result.length() < width) {
        result += " ";
    }
    if (result.length() > width) {
        result = result.substring(0, width);
    }
    return result;
}

String LCDRenderer::padLeft(const String& text, int width) {
    String result = text;
    while (result.length() < width) {
        result = " " + result;
    }
    if (result.length() > width) {
        result = result.substring(0, width);
    }
    return result;
}

String LCDRenderer::center(const String& text, int width) {
    if (text.length() >= width) {
        return text.substring(0, width);
    }

    int padding = (width - text.length()) / 2;
    String result = "";
    for (int i = 0; i < padding; i++) {
        result += " ";
    }
    result += text;

    while (result.length() < width) {
        result += " ";
    }

    return result;
}