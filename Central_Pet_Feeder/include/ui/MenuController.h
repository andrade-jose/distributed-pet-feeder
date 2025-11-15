#pragma once
#include <Arduino.h>
#include "ui/LCDRenderer.h"
#include "core/RemoteManager.h"
#include "core/ClockService.h"
#include "hal/Buttons.h"

enum class MenuState {
    STATUS_GATEWAY,      // Tela inicial com status
    REMOTE_LIST,         // Lista de remotas
    MEAL_CONFIG,         // Configuração de refeições
    EDIT_TIME,           // Editar horário
    EDIT_QUANTITY        // Editar quantidade
};

class MenuController {
private:
    LCDRenderer* renderer;
    RemoteManager* remoteManager;
    ClockService* clockService;
    Buttons* buttons;

    MenuState currentState;
    int selectedOption;
    int selectedRemoteIndex;
    int selectedMealIndex;
    int editField;  // 0 = hora, 1 = minuto
    bool isEditing;

    // Callback para enviar configuração via MQTT
    void (*onMealConfigCallback)(int remoteId, int mealIndex, int hour, int minute, int quantity);

    // Métodos de renderização por estado
    void renderStatusGateway();
    void renderRemoteList();
    void renderMealConfig();
    void renderEditTime();
    void renderEditQuantity();

    // Métodos de navegação por estado
    void handleStatusGateway(ButtonEvent event);
    void handleRemoteList(ButtonEvent event);
    void handleMealConfig(ButtonEvent event);
    void handleEditTime(ButtonEvent event);
    void handleEditQuantity(ButtonEvent event);

    // Utilidades
    String formatTime(int hour, int minute);
    void changeState(MenuState newState);

public:
    MenuController(LCDRenderer* r, RemoteManager* rm, ClockService* cs, Buttons* b);

    void init();
    void update();

    void setMealConfigCallback(void (*callback)(int, int, int, int, int));
};