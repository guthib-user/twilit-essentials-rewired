#pragma once
#include <array>

namespace dawnlight::kcrux {
struct HudPoint { float x,y; };
struct HudLayout {
    static constexpr float faceSize=29,itemSize=36,wolfSize=36;
    std::array<HudPoint,4> face; // A, X, Y, B, matching cue channels.
    std::array<HudPoint,5> items; // Left, Right, Up, Down, RT.
    std::array<HudPoint,2> wolf; // Up Senses, Down Dig.
    std::array<HudPoint,2> bumpers; // LB Map, RB Items.
    HudPoint midna,midnaButton;
};
inline HudLayout make_hud_layout(float left,float top,float right,float bottom){
    const float fx=right-83,fy=top+67,cx=left+74,cy=bottom-88;
    return {
        .face={{{fx,fy+58},{fx-27,fy+29},{fx,fy},{fx+27,fy+29}}},
        .items={{{cx-35,cy},{cx+35,cy},{cx,cy-35},{cx,cy+35},{right-44,top+46}}},
        .wolf={{{cx-35,bottom-78},{cx-35,bottom-34}}},
        .bumpers={{{cx-35,(top+bottom)*.5f+6},{cx-35,(top+bottom)*.5f+25}}},
        // Center the 12-unit gap, rather than the unequal icon centers, under A/Y.
        .midna={fx+28,fy+110},.midnaButton={fx-18,fy+110},
    };
}
static_assert(HudLayout::itemSize>=HudLayout::faceSize&&HudLayout::wolfSize>=HudLayout::faceSize);
}
