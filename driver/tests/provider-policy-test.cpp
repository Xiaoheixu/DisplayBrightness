#include "ProviderPolicy.hpp"
#include <cassert>
#include <cstdio>
int main() {
    using dbhw::BrightnessProvider;
    using dbhw::brightnessProvider;
    using dbhw::allowsTransport;

    // No provider at preparation. A native provider appeared before actual I/O.
    assert(allowsTransport(BrightnessProvider::Missing, false, false, true));
    assert(!allowsTransport(BrightnessProvider::Foreign, false, false, true));
    assert(!allowsTransport(BrightnessProvider::Foreign, true, false, false));
    assert(!allowsTransport(BrightnessProvider::Owned, true, false, false));
    assert(allowsTransport(BrightnessProvider::Owned, true, true, false));
    assert(!allowsTransport(BrightnessProvider::Missing, true, false, false));

    // Similar shape or marker alone must never grant ownership.
    assert(brightnessProvider(false, true, true) == BrightnessProvider::Missing);
    assert(brightnessProvider(true, false, true) == BrightnessProvider::Foreign);
    assert(brightnessProvider(true, true, false) == BrightnessProvider::Foreign);
    assert(brightnessProvider(true, true, true) == BrightnessProvider::Owned);
    puts("provider policy: native takeover stops bootstrap and writes");
}
