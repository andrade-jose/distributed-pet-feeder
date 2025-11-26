#ifndef SCHEDULE_SERVICE_H
#define SCHEDULE_SERVICE_H

#include <Arduino.h>
#include <Preferences.h>
#include "models.h"

class ClockService;
class FeederService;
class LogService;

class ScheduleService {
public:
    ScheduleService();

    // novo begin() com dependências
    bool begin(ClockService* clock, FeederService* feeder, LogService* log);

    bool load();
    bool save();
    void loop();
    bool checkMeals();
    bool shouldFeedNow(uint8_t hour, uint8_t minute);

    bool setMeal(uint8_t index, uint8_t hour, uint8_t minute,
                 uint16_t qty, bool enabled);

    Meal getMeal(uint8_t index);
    void printMeals();

private:
    Preferences prefs;

    ClockService* clock;
    FeederService* feeder;
    LogService* log;

    Meal meals[3];
    uint32_t lastFeedDay;
    uint8_t lastFeedIndex;
};

extern ScheduleService scheduleService;

#endif
