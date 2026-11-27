#include "../../src/bsp/sdmmc/SDMMC_Class.hpp"
#include <cassert>
#include <cstdio>
static bool mscActive=false;
extern "C" int usb_msc_is_active() { return mscActive; }
int main() {
    SDMMC_Class card;
    assert(!card.begin());
    sd_fake::inserted=true;
    assert(card.begin()); // SD_MMC expects kHz, not Hz
    assert(card.isPresent() && card.freqKHz()<=10000);
    mscActive=true;
    const auto protectedEnds=sd_fake::ends,protectedAttempts=unsigned(sd_fake::attempts.size());
    assert(!card.begin() && !card.isPresent());
    assert(sd_fake::ends==protectedEnds && sd_fake::attempts.size()==protectedAttempts);
    mscActive=false;
    const auto attempts=sd_fake::attempts.size(); const auto ends=sd_fake::ends;
    assert(card.begin() && sd_fake::attempts.size()==attempts && sd_fake::ends==ends);
    // Repeated calls preserve the active mount and any files owned by the app.
    sd_fake::inserted=false;
    assert(!card.begin()); // the driver's cached cardType must not hide removal
    sd_fake::inserted=true;
    assert(card.begin() && sd_fake::ends>ends);
    // Adopt a healthy mount made through the UI without unmounting it.
    SDMMC_Class another;
    const auto adoptedEnds=sd_fake::ends;
    assert(another.begin() && sd_fake::ends==adoptedEnds);
    another.end(); sd_fake::maximumFrequency=5000;
    assert(another.begin() && another.freqKHz()==5000);
    another.end();
    puts("SD mount checks passed (late insertion, removal, kHz, fallback, mount ownership)");
}
