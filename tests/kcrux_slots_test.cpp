// Adapted from the earlier Dawnlight workspace; CC0 notice in THIRD_PARTY/.
#include "../rewired-core/src/kcrux_slots.hpp"
#include <cassert>
#include <random>
#include <iostream>
using namespace dawnlight::kcrux;
void unique(const Slots& slots){
    std::array<bool,256> used{};
    for(auto a:slots.values)for(auto item:{a.item,a.mix})if(item!=Empty){assert(item<24);assert(!used[item]);used[item]=true;}
}
int main(){
    for(int source=0;source<5;++source)for(int target=0;target<5;++target){
        Slots slots;for(int i=0;i<5;++i)slots.assign(i,i);
        if(source==target){assert(slots.assign(target,source)==AssignResult::Cleared);assert(slots.values[target].item==Empty);}
        else{assert(slots.assign(target,source)==AssignResult::Swapped);assert(slots.values[target].item==source);assert(slots.values[source].item==target);}
        unique(slots);
        auto bytes=slots.serialize();Slots restored;assert(restored.deserialize(bytes.data(),bytes.size(),24));assert(restored.values==slots.values);
    }
    Slots slots;slots.assign(0,3);assert(slots.assign(4,3)==AssignResult::Swapped);assert(slots.values[0].item==Empty&&slots.values[4].item==3);
    slots.assign(2,4);slots.combine(4,3,4);unique(slots);assert(slots.values[2].item==Empty);
    slots.assign(4,3);assert(slots.values[4].item==Empty);unique(slots);
    slots.values={Assignment{0,1},Assignment{1,2},Assignment{0,Empty},Assignment{23,200},Assignment{22,22}};
    assert(slots.normalize(24)==4);unique(slots);
    auto invalid=slots.serialize();invalid[0]=9;Slots before=slots;assert(!slots.deserialize(invalid.data(),invalid.size(),24));assert(slots.values==before.values);
    std::mt19937 random(9123);slots={};
    for(int n=0;n<100000;++n){int target=random()%5,item=random()%24;const auto before=slots;
        slots.assign(target,item);unique(slots);
        if(before.values[target].contains(item))assert(slots.values[target].item==Empty);
        else assert(slots.values[target].item==item);
        const auto stable=slots.serialize();(void)slots.owner(random()%24);assert(slots.serialize()==stable);
    }
    std::cout<<"PASS: all 25 slot pairs, self-clear, empty moves, combo ownership, migration, versioned round-trip and 100000 invariant operations\n";
}
