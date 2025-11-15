#pragma once
#include <Arduino.h>

enum class ButtonEvent {
    NONE,
    UP,
    DOWN,
    OK,
    BACK
};

class Buttons {
private:
    static const unsigned long DEBOUNCE_DELAY = 50;

    struct ButtonState {
        int pin;
        bool lastState;
        bool currentState;
        unsigned long lastDebounceTime;
        bool pressed;
    };

    ButtonState btnUp;
    ButtonState btnDown;
    ButtonState btnOk;
    ButtonState btnBack;

    void initButton(ButtonState& btn, int pin);
    bool updateButton(ButtonState& btn);

public:
    Buttons();

    void init();
    void update();

    // Eventos
    ButtonEvent getEvent();

    // Verificação individual (para compatibilidade)
    bool isUpPressed() { return btnUp.pressed; }
    bool isDownPressed() { return btnDown.pressed; }
    bool isOkPressed() { return btnOk.pressed; }
    bool isBackPressed() { return btnBack.pressed; }
};