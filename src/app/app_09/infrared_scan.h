#pragma once
#include <algorithm>
#include <cstddef>
namespace meow_ir {
struct Scan {
    size_t index=0,total=0;
    bool running=false,paused=false;
    void start(size_t count) { index=0; total=count; running=count>0; paused=false; }
    void pause(bool inFlight=false) { if (running) { paused=true; if (!inFlight && index) --index; } }
    void resume() { if (running) paused=false; }
    void move(int delta) {
        if (paused && total) index=size_t(std::max(0,std::min(int(total)-1,int(index)+delta)));
    }
    void sent() {
        if (!running || paused) return;
        if (index+1>=total) { paused=true; return; }
        ++index;
    }
};
}
