#pragma once
#include <Arduino.h>
#include <time.h>

class ClockService {
private:
    bool initialized;
    unsigned long lastNTPUpdate;

    int currentHour;
    int currentMinute;
    int currentSecond;
    int currentDay;
    int currentMonth;
    int currentYear;

    void syncWithNTP();

public:
    ClockService();

    bool init();
    void update();

    // Getters
    int getHour() const { return currentHour; }
    int getMinute() const { return currentMinute; }
    int getSecond() const { return currentSecond; }
    int getDay() const { return currentDay; }
    int getMonth() const { return currentMonth; }
    int getYear() const { return currentYear; }

    // Formatação
    String getTimeFormatted();      // HH:MM:SS
    String getDateFormatted();      // DD/MM/YYYY
    String getTimeShort();          // HH:MM

    // Timestamp Unix
    unsigned long getTimestamp() const;

    bool isInitialized() const { return initialized; }
};
