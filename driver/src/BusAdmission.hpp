#pragma once
#include <stdint.h>
namespace dbhw {
// Caller serializes admission, release and wakeups with the endpoint queue.
template<unsigned Slots> struct BusAdmission {
    static_assert(Slots > 0 && Slots <= 32, "Bus waiters must fit the mask");
    unsigned owner = Slots;
    uint32_t waiting = 0;

    bool acquire(unsigned slot) {
        if (slot >= Slots || owner == slot) return false;
        if (owner != Slots) {
            waiting |= uint32_t{1} << slot;
            return false;
        }
        owner = slot;
        forget(slot);
        return true;
    }
    bool parked(unsigned slot) const {
        return slot < Slots && (waiting & (uint32_t{1} << slot));
    }
    void forget(unsigned slot) {
        if (slot < Slots) waiting &= ~(uint32_t{1} << slot);
    }
    uint32_t release(unsigned slot) {
        if (slot >= Slots || owner != slot) return 0;
        owner = Slots;
        const uint32_t ready = waiting;
        waiting = 0;
        return ready;
    }
};
}
