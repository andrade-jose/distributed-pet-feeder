#include "botoes.h"
#include "config.h"
#include <Arduino.h>

// Inicializar variáveis estáticas - debounce individual para cada botão
unsigned long Botoes::ultimoPressionamento = 0;
static unsigned long ultimoPressionamentoCima = 0;
static unsigned long ultimoPressionamentoBaixo = 0;
static unsigned long ultimoPressionamentoEnter = 0;

void Botoes::inicializar() {
    Serial.println("=== Inicializando Botões ===");
    
    // Configurar pinos como entrada com pull-up interno
    pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);
    pinMode(ENTER_BUTTON_PIN, INPUT_PULLUP);
    
        Serial.println("Botões inicializados");
    Serial.printf("Pino CIMA: %d\n", UP_BUTTON_PIN);
    Serial.printf("Pino BAIXO: %d\n", DOWN_BUTTON_PIN);
    Serial.printf("Pino ENTER: %d\n", ENTER_BUTTON_PIN);
}

bool Botoes::estaPressionado(int pino) {
    unsigned long agora = millis();

    // Verificar debounce
    if (agora - ultimoPressionamento < delayDebounce) {
        return false;
    }

    // Ler estado do botão (LOW = pressionado devido ao pull-up)
    if (digitalRead(pino) == LOW) {
        ultimoPressionamento = agora;
        return true;
    }

    return false;
}

void Botoes::verificarTodos() {
    if (estaPressionado(UP_BUTTON_PIN)) {
        Serial.println("Botão CIMA pressionado");
    }

    if (estaPressionado(DOWN_BUTTON_PIN)) {
        Serial.println("Botão BAIXO pressionado");
    }

    if (estaPressionado(ENTER_BUTTON_PIN)) {
        Serial.println("Botão ENTER pressionado");
    }
}

bool Botoes::cimaPressionado() {
    unsigned long agora = millis();
    
    // Verificar debounce individual
    if (agora - ultimoPressionamentoCima < delayDebounce) {
        return false;
    }
    
    // Ler estado do botão (LOW = pressionado devido ao pull-up)
    if (digitalRead(UP_BUTTON_PIN) == LOW) {
        ultimoPressionamentoCima = agora;
        Serial.println("DEBUG: Botão CIMA detectado");
        return true;
    }
    
    return false;
}

bool Botoes::baixoPressionado() {
    unsigned long agora = millis();
    
    // Verificar debounce individual
    if (agora - ultimoPressionamentoBaixo < delayDebounce) {
        return false;
    }
    
    // Ler estado do botão (LOW = pressionado devido ao pull-up)
    if (digitalRead(DOWN_BUTTON_PIN) == LOW) {
        ultimoPressionamentoBaixo = agora;
        Serial.println("DEBUG: Botão BAIXO detectado");
        return true;
    }
    
    return false;
}

bool Botoes::enterPressionado() {
    unsigned long agora = millis();
    
    // Verificar debounce individual
    if (agora - ultimoPressionamentoEnter < delayDebounce) {
        return false;
    }
    
    // Ler estado do botão (LOW = pressionado devido ao pull-up)
    if (digitalRead(ENTER_BUTTON_PIN) == LOW) {
        ultimoPressionamentoEnter = agora;
        Serial.println("DEBUG: Botão ENTER detectado");
        return true;
    }
    
    return false;
}
