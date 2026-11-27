#include "../../src/app/launcher/idle_power_policy.h"
#include <cassert>
#include <cstdio>

int main() {
    using meowkit::shouldIdlePowerOff;
    // A fully charged USB-connected device is not necessarily charging.
    assert(!shouldIdlePowerOff(300,30,false,true,false));
    assert(!shouldIdlePowerOff(100000,0,false,true,false));
    assert(!shouldIdlePowerOff(300,30,true,false,false));
    assert(!shouldIdlePowerOff(300,30,false,false,true));
    assert(!shouldIdlePowerOff(299,30,false,false,false));
    assert(shouldIdlePowerOff(300,30,false,false,false));
    assert(shouldIdlePowerOff(300,0,false,false,false));
    assert(!shouldIdlePowerOff(1499,600,false,false,false));
    assert(shouldIdlePowerOff(1500,600,false,false,false));
    puts("Idle power checks passed (VBUS without charging, battery timeout, MSC)");
}
