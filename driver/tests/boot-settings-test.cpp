#include "BootSettings.hpp"
#include <cassert>
#include <cstdio>
int main() {
    using namespace dbboot;
    Settings settings;
    assert(sizeof(numericOptions)/sizeof(numericOptions[0]) == 9);
    for (const auto &option : numericOptions) {
        const unsigned original = settings.*(option.member);
        assert(!applyNumeric(settings, option, static_cast<uint64_t>(option.maximum)+1));
        assert(!applyNumeric(settings, option, UINT64_MAX));
        if (option.minimum) assert(!applyNumeric(settings, option, option.minimum-1));
        assert(settings.*(option.member) == original);
        assert(applyNumeric(settings, option, option.minimum));
        assert(settings.*(option.member) == option.minimum);
        assert(applyNumeric(settings, option, option.maximum));
        assert(settings.*(option.member) == option.maximum);
    }
    assert(publishesBrightness(true, false));
    assert(!publishesBrightness(true, true));
    assert(!publishesBrightness(false, false));
    assert(!publishesBrightness(false, true));
    Settings timing;
    assert(timing.initialProbeDelay(true) == 50 && timing.probeWindow(true) == 60);
    timing.hdrDelayMs = 800;
    timing.hdrWindowSeconds = 10;
    assert(timing.initialProbeDelay(false) == 50 && timing.initialProbeDelay(true) == 850);
    assert(timing.probeWindow(false) == 60 && timing.probeWindow(true) == 10);
    timing.startupSeconds = 5;
    assert(timing.probeWindow(true) == 5);
    puts("boot settings: numeric bounds and read-only publication isolation passed");
}
