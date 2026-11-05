#pragma once
#include <cstdint>

namespace dawnlight::kcrux {
// A held button must be released after a blocking menu/event before it can act.
struct Button {
    bool held = false;
    bool pressed = false;
    bool suppressed = false;
    void sample(bool down, bool blocked) {
        if (blocked && down) suppressed = true;
        if (!down) suppressed = false;
        down = down && !blocked && !suppressed;
        pressed = down && !held;
        held = down;
    }
};

struct Trigger {
    bool physical = false;
    Button button;
    void sample(int value, bool blocked) {
        // Separate thresholds prevent repeated shots from a noisy analog trigger.
        physical = value >= (physical ? 8192 : 16384);
        button.sample(physical, blocked);
    }
};

// A short release opens the full map. Crossing the hold threshold emits only
// one minimap request; releasing afterwards must not open the full map.
struct MapGesture {
    Button button;
    double started=0;
    bool longFired=false,tap=false,hold=false;
    void sample(bool down,bool blocked,double now){
        const bool previous=button.held;
        button.sample(down,blocked);tap=hold=false;
        if(blocked){started=0;longFired=false;return;}
        if(button.pressed){started=now;longFired=false;}
        if(button.held&&!longFired&&now-started>=.55){hold=true;longFired=true;}
        if(previous&&!button.held&&!down){tap=!longFired;started=0;longFired=false;}
    }
};

struct FireState {
    std::uintptr_t owner = 0;
    unsigned item = 0xffff;
    unsigned slot = 0xff;
    bool pending = false;
    unsigned age = 0;
    void reset() { *this = {}; }
    bool update(std::uintptr_t nextOwner, unsigned nextItem, unsigned nextSlot,
                bool blocked, bool request, bool ready) {
        if (blocked || nextOwner == 0) { reset(); return false; }
        if (owner != nextOwner || item != nextItem || slot != nextSlot) {
            reset();
            owner = nextOwner; item = nextItem; slot = nextSlot;
            // The input equipping a new item is selection, not a second fire.
            return false;
        }
        if (request) { pending = true; age = 0; }
        if (pending && ++age > 60) { pending = false; age = 0; }
        if (pending && ready) { pending = false; age = 0; return true; }
        return false;
    }
};
}
