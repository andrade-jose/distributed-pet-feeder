#pragma once
#include <Arduino.h>
#include "config.h"

struct MealSchedule {
    int hour;
    int minute;
    int quantity;  // gramas
    bool enabled;

    MealSchedule() : hour(0), minute(0), quantity(0), enabled(false) {}
};

struct RemoteState {
    int id;
    String name;
    bool online;
    unsigned long lastSeen;
    String feedLevel;  // "OK", "LOW", "EMPTY"
    MealSchedule meals[3];  // Até 3 refeições por dia

    RemoteState() : id(0), name(""), online(false), lastSeen(0), feedLevel("OK") {}
    RemoteState(int _id) : id(_id), name("Remota " + String(_id)), online(false), lastSeen(0), feedLevel("OK") {}
};

class RemoteManager {
private:
    RemoteState remotes[MAX_REMOTAS];
    int remoteCount;

public:
    RemoteManager();

    // Gerenciamento de remotas
    void addRemote(int id);
    RemoteState* getRemote(int id);
    int getRemoteCount() const { return remoteCount; }
    RemoteState* getRemoteByIndex(int index);

    // Atualização de estado
    void updateRemoteStatus(int id, bool online);
    void updateFeedLevel(int id, const String& level);
    void updateLastSeen(int id);
    bool isRemoteActive(int id);  // Verifica se teve sinal nos últimos 10min

    // Configuração de refeições
    bool setMealSchedule(int remoteId, int mealIndex, int hour, int minute, int quantity);
    MealSchedule* getMealSchedule(int remoteId, int mealIndex);

    // Contadores
    int getOnlineCount();
    bool hasLowFeed();
};
