#include "hal/ScreenBacklight.h"

ScreenBacklight::ScreenBacklight(LiquidCrystal_I2C* lcdInstance)
    : lcd(lcdInstance), state(true) {
}

void ScreenBacklight::on() {
    if (lcd) {
        lcd->backlight();
        state = true;
    }
}

void ScreenBacklight::off() {
    if (lcd) {
        lcd->noBacklight();
        state = false;
    }
}

void ScreenBacklight::toggle() {
    if (state) {
        off();
    } else {
        on();
    }
}

void ScreenBacklight::set(bool enabled) {
    if (enabled) {
        on();
    } else {
        off();
    }
}