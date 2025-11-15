#pragma once
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

class ScreenBacklight {
private:
    LiquidCrystal_I2C* lcd;
    bool state;

public:
    ScreenBacklight(LiquidCrystal_I2C* lcdInstance);

    void on();
    void off();
    void toggle();
    void set(bool enabled);
    bool isOn() const { return state; }
};