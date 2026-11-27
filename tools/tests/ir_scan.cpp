#include "../../src/app/app_09/infrared_scan.h"
#include <cassert>
#include <cstdio>
int main() {
    meow_ir::Scan scan;
    scan.start(5); scan.sent(); scan.sent();
    scan.pause(); assert(scan.index==1 && scan.paused);
    scan.resume(); scan.sent();
    scan.pause(true); assert(scan.index==2); // the current in-flight code, not its predecessor
    scan.move(1); assert(scan.index==3);
    scan.move(99); assert(scan.index==4);
    scan.resume(); scan.sent(); assert(scan.running && scan.paused && scan.index==4);
    scan.start(0); scan.pause(); assert(!scan.running);
    scan.start(1); scan.sent(); assert(scan.running && scan.paused && scan.index==0);
    puts("IR scan checks passed (pause selection, in-flight code, final-code save)");
}
