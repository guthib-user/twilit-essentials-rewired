#pragma once
#include <array>
#include <cstdint>

namespace dawnlight::kcrux {
constexpr std::uint8_t Empty=255;
constexpr int SlotCount=5;
struct Assignment {
    std::uint8_t item=Empty,mix=Empty;
    bool contains(std::uint8_t index)const{return index!=Empty&&(item==index||mix==index);}
    bool operator==(const Assignment&)const=default;
};
enum class AssignResult {Unchanged,Assigned,Swapped,Cleared};
struct Slots {
    std::array<Assignment,SlotCount> values{};
    int owner(std::uint8_t item)const {
        for(int i=0;i<SlotCount;++i)if(values[i].contains(item))return i;
        return -1;
    }
    AssignResult assign(int target,std::uint8_t item) {
        if(target<0||target>=SlotCount||item==Empty)return AssignResult::Unchanged;
        if(values[target].contains(item)){values[target]={};return AssignResult::Cleared;}
        const int source=owner(item);const auto previous=values[target];
        values[target]={item,Empty};
        if(source>=0){values[source]=previous;return AssignResult::Swapped;}
        return AssignResult::Assigned;
    }
    // Combination ownership includes both inventory entries, across all five slots.
    void combine(int target,std::uint8_t base,std::uint8_t component) {
        if(target<0||target>=SlotCount||base==Empty||component==Empty||base==component)return;
        for(int i=0;i<SlotCount;++i)if(i!=target&&
            (values[i].contains(base)||values[i].contains(component)))values[i]={};
        values[target]={component,base};
    }
    unsigned normalize(unsigned inventorySize) {
        unsigned changed=0;std::array<bool,256> seen{};
        for(auto& a:values){const auto previous=a;
            if(a.item>=inventorySize||seen[a.item])a={};
            else {seen[a.item]=true;
                if(a.mix>=inventorySize||a.mix==a.item||seen[a.mix])a.mix=Empty;
                else seen[a.mix]=true;
            }
            changed+=a!=previous;
        }
        return changed;
    }
    static constexpr unsigned SerializedSize=12;
    std::array<std::uint8_t,SerializedSize> serialize()const {
        std::array<std::uint8_t,SerializedSize> bytes{1,SlotCount};
        for(int i=0;i<SlotCount;++i){bytes[2+i*2]=values[i].item;bytes[3+i*2]=values[i].mix;}
        return bytes;
    }
    bool deserialize(const std::uint8_t* bytes,unsigned size,unsigned inventorySize) {
        if(size!=SerializedSize||bytes[0]!=1||bytes[1]!=SlotCount)return false;
        for(int i=0;i<SlotCount;++i)values[i]={bytes[2+i*2],bytes[3+i*2]};
        normalize(inventorySize);return true;
    }
};
}
