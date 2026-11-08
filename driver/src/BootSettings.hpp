#pragma once
#include <stdint.h>
namespace dbboot {
struct Settings {
    bool trace = false;
    bool notify = true;
    bool getOnly = false;
    bool hdrTrace = false;
    unsigned slots = 8;
    unsigned coalesceMs = 50;
    unsigned settleMs = 50;
    unsigned replyMs = 40;
    unsigned retries = 2;
    unsigned startupSeconds = 60;
    unsigned startupIntervalMs = 1000;
    unsigned hdrDelayMs = 0;
    unsigned hdrWindowSeconds = 60;

    unsigned initialProbeDelay(bool recovery) const {
        return coalesceMs + (recovery ? hdrDelayMs : 0);
    }
    unsigned probeWindow(bool recovery) const {
        return recovery && hdrWindowSeconds < startupSeconds ? hdrWindowSeconds : startupSeconds;
    }
};
struct NumericOption {
    const char *name;
    unsigned Settings::*member;
    unsigned minimum;
    unsigned maximum;
};
constexpr NumericOption numericOptions[] = {
    {"dbslots", &Settings::slots, 1, 8},
    {"dbcoalesce", &Settings::coalesceMs, 50, 500},
    {"dbsettle", &Settings::settleMs, 50, 500},
    {"dbreply", &Settings::replyMs, 40, 200},
    {"dbretries", &Settings::retries, 0, 2},
    {"dbbootwindow", &Settings::startupSeconds, 5, 60},
    {"dbbootinterval", &Settings::startupIntervalMs, 1000, 5000},
    {"dbhdrdelay", &Settings::hdrDelayMs, 0, 3000},
    {"dbhdrwindow", &Settings::hdrWindowSeconds, 5, 60},
};
inline bool applyNumeric(Settings &settings, const NumericOption &option, uint64_t value) {
    if (value < option.minimum || value > option.maximum) return false;
    settings.*(option.member) = static_cast<unsigned>(value);
    return true;
}
inline bool publishesBrightness(bool hardwareEnabled, bool getOnly) {
    return hardwareEnabled && !getOnly;
}
}
