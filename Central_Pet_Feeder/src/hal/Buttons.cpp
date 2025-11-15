#include "hal/Buttons.h"
#include "config.h"

Buttons::Buttons() {
}

void Buttons::initButton(ButtonState& btn, int pin) {
    btn.pin = pin;
    btn.lastState = HIGH;
    btn.currentState = HIGH;
    btn.lastDebounceTime = 0;
    btn.pressed = false;

    pinMode(pin, INPUT_PULLUP);
}

void Buttons::init() {
    Serial.println("[Buttons] Inicializando botões...");

    initButton(btnUp, BTN_UP_PIN);
    initButton(btnDown, BTN_DOWN_PIN);
    initButton(btnOk, BTN_OK_PIN);
    initButton(btnBack, BTN_BACK_PIN);

    Serial.println("[Buttons] Botões inicializados");
}

bool Buttons::updateButton(ButtonState& btn) {
    bool reading = digitalRead(btn.pin);
    bool eventDetected = false;

    // Se mudou de estado, resetar debounce
    if (reading != btn.lastState) {
        btn.lastDebounceTime = millis();
    }

    // Se passou o tempo de debounce
    if ((millis() - btn.lastDebounceTime) > DEBOUNCE_DELAY) {
        // Se o estado mudou de fato
        if (reading != btn.currentState) {
            btn.currentState = reading;

            // Detectar pressionamento (transição HIGH → LOW)
            if (btn.currentState == LOW) {
                btn.pressed = true;
                eventDetected = true;
            } else {
                btn.pressed = false;
            }
        }
    }

    btn.lastState = reading;
    return eventDetected;
}

void Buttons::update() {
    updateButton(btnUp);
    updateButton(btnDown);
    updateButton(btnOk);
    updateButton(btnBack);
}

ButtonEvent Buttons::getEvent() {
    update();

    if (btnUp.pressed) {
        btnUp.pressed = false;
        return ButtonEvent::UP;
    }
    if (btnDown.pressed) {
        btnDown.pressed = false;
        return ButtonEvent::DOWN;
    }
    if (btnOk.pressed) {
        btnOk.pressed = false;
        return ButtonEvent::OK;
    }
    if (btnBack.pressed) {
        btnBack.pressed = false;
        return ButtonEvent::BACK;
    }

    return ButtonEvent::NONE;
}