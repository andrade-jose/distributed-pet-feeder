#include "core/RemoteManager.h"

RemoteManager::RemoteManager() : remoteCount(0) {
    // Inicializar array
    for (int i = 0; i < MAX_REMOTAS; i++) {
        remotes[i] = RemoteState();
    }
}

void RemoteManager::addRemote(int id) {
    if (remoteCount >= MAX_REMOTAS) {
        Serial.println("[RemoteManager] Limite de remotas atingido!");
        return;
    }

    // Verificar se já existe
    for (int i = 0; i < remoteCount; i++) {
        if (remotes[i].id == id) {
            Serial.printf("[RemoteManager] Remota %d já existe\n", id);
            return;
        }
    }

    remotes[remoteCount] = RemoteState(id);
    remoteCount++;
    Serial.printf("[RemoteManager] Remota %d adicionada (%d/%d)\n", id, remoteCount, MAX_REMOTAS);
}

RemoteState* RemoteManager::getRemote(int id) {
    for (int i = 0; i < remoteCount; i++) {
        if (remotes[i].id == id) {
            return &remotes[i];
        }
    }
    return nullptr;
}

RemoteState* RemoteManager::getRemoteByIndex(int index) {
    if (index >= 0 && index < remoteCount) {
        return &remotes[index];
    }
    return nullptr;
}

void RemoteManager::updateRemoteStatus(int id, bool online) {
    RemoteState* remote = getRemote(id);
    if (remote) {
        remote->online = online;
        if (online) {
            remote->lastSeen = millis();
        }
        Serial.printf("[RemoteManager] Remota %d: %s\n", id, online ? "ONLINE" : "OFFLINE");
    }
}

void RemoteManager::updateFeedLevel(int id, const String& level) {
    RemoteState* remote = getRemote(id);
    if (remote) {
        remote->feedLevel = level;
        Serial.printf("[RemoteManager] Remota %d: Nível de ração = %s\n", id, level.c_str());
    }
}

void RemoteManager::updateLastSeen(int id) {
    RemoteState* remote = getRemote(id);
    if (remote) {
        remote->lastSeen = millis();
    }
}

bool RemoteManager::isRemoteActive(int id) {
    RemoteState* remote = getRemote(id);
    if (!remote) return false;

    unsigned long now = millis();
    return (remote->lastSeen > 0 && (now - remote->lastSeen) < REMOTE_TIMEOUT);
}

bool RemoteManager::setMealSchedule(int remoteId, int mealIndex, int hour, int minute, int quantity) {
    if (mealIndex < 0 || mealIndex >= 3) {
        Serial.println("[RemoteManager] Índice de refeição inválido!");
        return false;
    }

    RemoteState* remote = getRemote(remoteId);
    if (!remote) {
        Serial.printf("[RemoteManager] Remota %d não encontrada!\n", remoteId);
        return false;
    }

    remote->meals[mealIndex].hour = hour;
    remote->meals[mealIndex].minute = minute;
    remote->meals[mealIndex].quantity = quantity;
    remote->meals[mealIndex].enabled = (quantity > 0);

    Serial.printf("[RemoteManager] Refeição configurada: Remota %d, R%d = %02d:%02d (%dg)\n",
                  remoteId, mealIndex + 1, hour, minute, quantity);
    return true;
}

MealSchedule* RemoteManager::getMealSchedule(int remoteId, int mealIndex) {
    if (mealIndex < 0 || mealIndex >= 3) return nullptr;

    RemoteState* remote = getRemote(remoteId);
    if (!remote) return nullptr;

    return &remote->meals[mealIndex];
}

int RemoteManager::getOnlineCount() {
    int count = 0;
    for (int i = 0; i < remoteCount; i++) {
        if (isRemoteActive(remotes[i].id)) {
            count++;
        }
    }
    return count;
}

bool RemoteManager::hasLowFeed() {
    for (int i = 0; i < remoteCount; i++) {
        if (remotes[i].feedLevel == "LOW" || remotes[i].feedLevel == "EMPTY") {
            return true;
        }
    }
    return false;
}
