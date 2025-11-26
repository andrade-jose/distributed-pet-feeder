#ifndef MODELS_H
#define MODELS_H

#include <Arduino.h>

struct Meal {
    uint8_t hour;
    uint8_t minute;
    uint16_t qty;
    bool enabled;
    
    Meal() : hour(8), minute(0), qty(100), enabled(true) {}
};

struct FeedLog {
    uint32_t timestamp;
    uint16_t qty;
    bool delivered;
    char source[12]; // "mqtt", "rtc_auto", "manual"
    
    FeedLog() : timestamp(0), qty(0), delivered(false) {
        strcpy(source, "unknown");
    }
};

#endif