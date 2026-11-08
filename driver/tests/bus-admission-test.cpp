#include "BusAdmission.hpp"
#include "BootstrapWindow.hpp"
#include "RetirePolicy.hpp"
#include <cassert>
#include <cstdio>
int main() {
    dbhw::BusAdmission<8> bus;
    assert(bus.acquire(0));
    assert(!bus.acquire(1) && bus.parked(1));
    assert(!bus.acquire(2) && bus.parked(2));
    assert(!bus.acquire(2)); // duplicate waiters share one wakeup
    assert(bus.release(1) == 0 && bus.parked(1)); // only the owner can release
    assert(dbhw::canRetire(true,true,true,false,bus.parked(1)));
    bus.forget(1); // retired slot will not be scheduled on release
    assert(bus.release(0) == (1U << 2));
    assert(!bus.parked(2) && bus.acquire(2));
    assert(!bus.acquire(2) && !bus.parked(2));
    assert(!bus.acquire(8) && bus.release(8) == 0);
    assert(bus.release(2) == 0 && bus.acquire(1));
    dbstate::BootstrapWindow startup;
    startup.observe(0); // first callback was parked without consuming a read
    assert(startup.rounds == 0);
    assert(!startup.begin(dbstate::BootstrapWindow::durationNs));
    puts("bus admission: parked slots, retirement, owner release, and startup deadline passed");
}
