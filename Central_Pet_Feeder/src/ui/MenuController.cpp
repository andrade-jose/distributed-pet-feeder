#include "ui/MenuController.h"

MenuController::MenuController(LCDRenderer* r, RemoteManager* rm, ClockService* cs, Buttons* b)
    : renderer(r), remoteManager(rm), clockService(cs), buttons(b),
      currentState(MenuState::STATUS_GATEWAY),
      selectedOption(0), selectedRemoteIndex(0), selectedMealIndex(0),
      editField(0), isEditing(false), onMealConfigCallback(nullptr) {
}

void MenuController::init() {
    Serial.println("[MenuController] Inicializado");
    changeState(MenuState::STATUS_GATEWAY);
}

void MenuController::update() {
    // Obter evento dos botões
    ButtonEvent event = buttons->getEvent();

    // Renderizar estado atual
    switch (currentState) {
        case MenuState::STATUS_GATEWAY:
            renderStatusGateway();
            if (event != ButtonEvent::NONE) handleStatusGateway(event);
            break;

        case MenuState::REMOTE_LIST:
            renderRemoteList();
            if (event != ButtonEvent::NONE) handleRemoteList(event);
            break;

        case MenuState::MEAL_CONFIG:
            renderMealConfig();
            if (event != ButtonEvent::NONE) handleMealConfig(event);
            break;

        case MenuState::EDIT_TIME:
            renderEditTime();
            if (event != ButtonEvent::NONE) handleEditTime(event);
            break;

        case MenuState::EDIT_QUANTITY:
            renderEditQuantity();
            if (event != ButtonEvent::NONE) handleEditQuantity(event);
            break;
    }
}

void MenuController::setMealConfigCallback(void (*callback)(int, int, int, int, int)) {
    onMealConfigCallback = callback;
}

// ========== RENDERIZAÇÃO ==========

void MenuController::renderStatusGateway() {
    String line0 = renderer->center("GATEWAY CENTRAL", 20);
    String line1 = "Online: OK";
    String line2 = "Remotas: " + String(remoteManager->getOnlineCount()) + "/" + String(remoteManager->getRemoteCount());
    String line3 = remoteManager->hasLowFeed() ? "! RACAO BAIXA !" : "> Configurar";

    renderer->render(line0, line1, line2, line3);
}

void MenuController::renderRemoteList() {
    String line0 = renderer->center("Remotas", 20);

    int totalOptions = remoteManager->getRemoteCount() + 1;  // +1 para Voltar

    String line1 = "";
    String line2 = "";
    String line3 = "";

    // Mostrar 3 opções de cada vez
    int startIdx = selectedOption >= 3 ? selectedOption - 2 : 0;

    for (int i = 0; i < 3; i++) {
        int idx = startIdx + i;
        String* targetLine = (i == 0) ? &line1 : (i == 1) ? &line2 : &line3;

        if (idx >= totalOptions) {
            *targetLine = "";
            continue;
        }

        String prefix = (idx == selectedOption) ? "> " : "  ";

        if (idx < remoteManager->getRemoteCount()) {
            RemoteState* remote = remoteManager->getRemoteByIndex(idx);
            String status = remoteManager->isRemoteActive(remote->id) ? "OK " : "OFF";
            *targetLine = prefix + remote->name + ": " + status;
        } else {
            *targetLine = prefix + "Voltar";
        }
    }

    renderer->render(line0, line1, line2, line3);
}

void MenuController::renderMealConfig() {
    RemoteState* remote = remoteManager->getRemoteByIndex(selectedRemoteIndex);
    if (!remote) return;

    String line0 = renderer->center(remote->name, 20);

    String line1 = "";
    String line2 = "";
    String line3 = "";

    for (int i = 0; i < 3; i++) {
        String prefix = (i == selectedOption) ? "> " : "  ";
        String time = formatTime(remote->meals[i].hour, remote->meals[i].minute);
        String qty = String(remote->meals[i].quantity) + "g";
        String mealLine = prefix + "R" + String(i + 1) + " " + time + " " + qty;

        if (i == 0) line1 = mealLine;
        else if (i == 1) line2 = mealLine;
        else line3 = mealLine;
    }

    renderer->render(line0, line1, line2, line3);
}

void MenuController::renderEditTime() {
    RemoteState* remote = remoteManager->getRemoteByIndex(selectedRemoteIndex);
    if (!remote) return;

    MealSchedule& meal = remote->meals[selectedMealIndex];

    String line0 = renderer->center("Editar Horario", 20);
    String line1 = "";
    String line2 = "";
    String line3 = "";

    char buffer[20];
    if (!isEditing) {
        snprintf(buffer, sizeof(buffer), "   [%02d]:[%02d]", meal.hour, meal.minute);
        line2 = String(buffer);
        line3 = "Enter para editar";
    } else {
        if (editField == 0) {
            snprintf(buffer, sizeof(buffer), "   >%02d<:[%02d]", meal.hour, meal.minute);
        } else {
            snprintf(buffer, sizeof(buffer), "   [%02d]:>%02d<", meal.hour, meal.minute);
        }
        line2 = String(buffer);
        line3 = "Enter para salvar";
    }

    renderer->render(line0, line1, line2, line3);
}

void MenuController::renderEditQuantity() {
    RemoteState* remote = remoteManager->getRemoteByIndex(selectedRemoteIndex);
    if (!remote) return;

    MealSchedule& meal = remote->meals[selectedMealIndex];

    String line0 = renderer->center("Quantidade (g)", 20);
    String line1 = "";
    String line2 = "";
    String line3 = "";

    char buffer[20];
    if (!isEditing) {
        snprintf(buffer, sizeof(buffer), "     [%03d]g", meal.quantity);
        line2 = String(buffer);
        line3 = "Enter para editar";
    } else {
        snprintf(buffer, sizeof(buffer), "     >%03d<g", meal.quantity);
        line2 = String(buffer);
        line3 = "Enter para salvar";
    }

    renderer->render(line0, line1, line2, line3);
}

// ========== NAVEGAÇÃO ==========

void MenuController::handleStatusGateway(ButtonEvent event) {
    if (event == ButtonEvent::OK) {
        changeState(MenuState::REMOTE_LIST);
    }
}

void MenuController::handleRemoteList(ButtonEvent event) {
    int totalOptions = remoteManager->getRemoteCount() + 1;

    if (event == ButtonEvent::UP) {
        selectedOption = (selectedOption - 1 + totalOptions) % totalOptions;
    } else if (event == ButtonEvent::DOWN) {
        selectedOption = (selectedOption + 1) % totalOptions;
    } else if (event == ButtonEvent::OK) {
        if (selectedOption < remoteManager->getRemoteCount()) {
            selectedRemoteIndex = selectedOption;
            selectedOption = 0;
            changeState(MenuState::MEAL_CONFIG);
        } else {
            changeState(MenuState::STATUS_GATEWAY);
        }
    } else if (event == ButtonEvent::BACK) {
        changeState(MenuState::STATUS_GATEWAY);
    }
}

void MenuController::handleMealConfig(ButtonEvent event) {
    if (event == ButtonEvent::UP) {
        selectedOption = (selectedOption - 1 + 3) % 3;
    } else if (event == ButtonEvent::DOWN) {
        selectedOption = (selectedOption + 1) % 3;
    } else if (event == ButtonEvent::OK) {
        selectedMealIndex = selectedOption;
        editField = 0;
        changeState(MenuState::EDIT_TIME);
    } else if (event == ButtonEvent::BACK) {
        changeState(MenuState::REMOTE_LIST);
    }
}

void MenuController::handleEditTime(ButtonEvent event) {
    RemoteState* remote = remoteManager->getRemoteByIndex(selectedRemoteIndex);
    if (!remote) return;

    MealSchedule& meal = remote->meals[selectedMealIndex];

    if (!isEditing) {
        if (event == ButtonEvent::OK) {
            isEditing = true;
        } else if (event == ButtonEvent::BACK) {
            changeState(MenuState::MEAL_CONFIG);
        }
    } else {
        if (event == ButtonEvent::UP) {
            if (editField == 0) {
                meal.hour = (meal.hour + 1) % 24;
            } else {
                meal.minute = (meal.minute + 1) % 60;
            }
        } else if (event == ButtonEvent::DOWN) {
            if (editField == 0) {
                meal.hour = (meal.hour - 1 + 24) % 24;
            } else {
                meal.minute = (meal.minute - 1 + 60) % 60;
            }
        } else if (event == ButtonEvent::OK) {
            if (editField == 0) {
                editField = 1;
            } else {
                isEditing = false;
                editField = 0;
                changeState(MenuState::EDIT_QUANTITY);
            }
        }
    }
}

void MenuController::handleEditQuantity(ButtonEvent event) {
    RemoteState* remote = remoteManager->getRemoteByIndex(selectedRemoteIndex);
    if (!remote) return;

    MealSchedule& meal = remote->meals[selectedMealIndex];

    if (!isEditing) {
        if (event == ButtonEvent::OK) {
            isEditing = true;
        } else if (event == ButtonEvent::BACK) {
            changeState(MenuState::EDIT_TIME);
        }
    } else {
        if (event == ButtonEvent::UP) {
            meal.quantity = min(meal.quantity + 10, 500);
        } else if (event == ButtonEvent::DOWN) {
            meal.quantity = max(meal.quantity - 10, 0);
        } else if (event == ButtonEvent::OK) {
            isEditing = false;
            meal.enabled = (meal.quantity > 0);

            // Chamar callback para enviar via MQTT
            if (onMealConfigCallback) {
                onMealConfigCallback(remote->id, selectedMealIndex,
                                     meal.hour, meal.minute, meal.quantity);
            }

            changeState(MenuState::MEAL_CONFIG);
        }
    }
}

// ========== UTILIDADES ==========

String MenuController::formatTime(int hour, int minute) {
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", hour, minute);
    return String(buffer);
}

void MenuController::changeState(MenuState newState) {
    currentState = newState;
    selectedOption = 0;
    isEditing = false;
    Serial.printf("[MenuController] Estado mudou para: %d\n", (int)newState);
}