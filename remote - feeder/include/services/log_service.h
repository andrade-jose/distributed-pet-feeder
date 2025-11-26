#ifndef LOG_SERVICE_H
#define LOG_SERVICE_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "models.h"

class ClockService;

class LogService {
public:
    LogService();

    // begin atualizado com ClockService*
    bool begin(ClockService* clock);

    void addLog(uint32_t timestamp, uint16_t qty, bool delivered, const char* source);
    void saveLogs();
    void loadLogs();
    void sendPendingLogsMQTT();
    void clearLogs();
    void loop();
    int getPendingLogsCount();

private:
    Preferences prefs;
    ClockService* clock;       // dependência

    FeedLog logs[MAX_LOGS];
    int logCount;
    bool hasPendingLogs;
};

extern LogService logService;

#endif
