// Adapted from the earlier Dawnlight workspace; CC0 notice in THIRD_PARTY/.
#include "../rewired-core/src/kcrux_input_state.hpp"
#include <cassert>
#include <iostream>
using namespace dawnlight::kcrux;
int main(){
 Button b;b.sample(true,true);assert(!b.held&&!b.pressed);b.sample(true,false);assert(!b.held);b.sample(false,false);b.sample(true,false);assert(b.pressed);b.sample(true,false);assert(b.held&&!b.pressed);
 Trigger t;t.sample(17000,false);assert(t.button.pressed);t.sample(15000,false);assert(t.button.held&&!t.button.pressed);t.sample(8000,false);assert(!t.button.held);t.sample(17000,false);assert(t.button.pressed);
 FireState f;assert(!f.update(1,1,0,false,true,true));assert(f.update(1,1,0,false,true,true));assert(!f.update(1,1,0,false,false,true));
 assert(!f.update(1,1,0,false,true,false));assert(f.pending);assert(f.update(1,1,0,false,false,true));
 f.update(1,1,0,false,true,false);assert(!f.update(1,2,1,false,false,true));assert(!f.pending);
 f.update(1,2,1,false,true,false);f.update(1,2,1,true,false,true);assert(!f.pending&&f.owner==0);
 f.update(1,2,1,false,false,true);f.update(1,2,1,false,true,false);for(int i=0;i<61;++i)f.update(1,2,1,false,false,false);assert(!f.pending);
 f.update(1,2,1,false,true,false);assert(!f.update(2,2,1,false,false,true));assert(!f.pending);
 MapGesture m;
 m.sample(true,false,1);assert(!m.tap&&!m.hold);
 m.sample(false,false,1.2);assert(m.tap&&!m.hold);
 m.sample(false,false,1.3);assert(!m.tap&&!m.hold);
 m.sample(true,false,2);m.sample(true,false,2.56);assert(m.hold&&!m.tap);
 m.sample(true,false,3);assert(!m.hold&&!m.tap);
 m.sample(false,false,3.1);assert(!m.hold&&!m.tap);
 m.sample(true,true,4);m.sample(true,false,5);assert(!m.tap&&!m.hold);
 m.sample(false,false,5.1);assert(!m.tap&&!m.hold);
 m.sample(true,false,6);m.sample(true,true,6.1);m.sample(false,false,6.2);assert(!m.tap&&!m.hold);
 m.sample(true,false,7);m.sample(false,false,7.2);assert(m.tap&&!m.hold);
 std::cout<<"PASS: menu-held suppression, analog hysteresis, independent fire edges, readiness queue, item/actor switch, timeout and cancellation\n";
}
