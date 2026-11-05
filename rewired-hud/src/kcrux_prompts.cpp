#include "kcrux_prompts.hpp"
#include "kcrux_controls.hpp"
#include "kcrux_hud_layout.hpp"
#include "JSystem/JKernel/JKRArchive.h"
#include "config.hpp"
#include "hud_fade.hpp"
#include "service_imports.hpp"
#include "global.h"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2.h"
#include "d/d_meter2_draw.h"
#include "d/d_meter_button.h"
#include "d/d_meter2_info.h"
#include "d/d_meter_HIO.h"
#include "d/d_menu_ring.h"
#include "d/d_menu_collect.h"
#include "d/d_meter_map.h"
#include "d/d_pane_class.h"
#include "d/d_select_cursor.h"
#include "d/d_kantera_icon_meter.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_graphic.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "JSystem/J2DGraph/J2DPictureEx.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#include "JSystem/J2DGraph/J2DOrthoGraph.h"
#include "JSystem/JUtility/JUTFont.h"
#include "JSystem/JUtility/JUTResFont.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "mods/svc/hook.hpp"
#include "mods/svc/resource.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>


namespace dawnlight {
namespace {
DEFINE_HOOK(&dMeter2Draw_c::draw,CustomHud);
DEFINE_HOOK(&dMeter2Draw_c::draw,RewiredMidnaMask);
DEFINE_HOOK(&dMeter2_c::_delete,DeleteHud);
DEFINE_HOOK(&J2DScreen::draw,CustomScreen);
DEFINE_HOOK(&dMenu_Ring_c::_draw,CustomRing);
DEFINE_HOOK(&dMenu_Ring_c::drawSelectItem,LegacyRingSelection);
DEFINE_HOOK(&dMenu_Ring_c::drawNumber,RewiredRingNumber);
DEFINE_HOOK(&dMenu_Ring_c::setMixMessage,RewiredRingMixMessage);
DEFINE_HOOK(&dMenu_Collect_c::draw,RewiredCollectionFrame);
DEFINE_HOOK(&dSelect_cursor_c::draw,RewiredCollectionCursor);
DEFINE_HOOK(static_cast<void (J2DPicture::*)(f32,f32,Mtx*)>(&J2DPicture::drawSelf),RewiredPictureSelf);
DEFINE_HOOK(&J2DPictureEx::drawSelf,RewiredPictureExSelf);
DEFINE_HOOK(&JUTFont::drawString_size_scale,RewiredMenuText);
DEFINE_HOOK(&JUTResFont::drawChar_scale,RewiredMenuGlyph);
DEFINE_HOOK(&dMeterMap_c::draw,RewiredMapDraw);
DEFINE_HOOK(static_cast<void (J2DPicture::*)(f32,f32,f32,f32,bool,bool,bool)>(&J2DPicture::draw),RewiredPictureDraw);
DEFINE_HOOK(static_cast<void (J2DPictureEx::*)(f32,f32,f32,f32,bool,bool,bool)>(&J2DPictureEx::draw),RewiredPictureExDraw);
DEFINE_HOOK(&dMeterButton_c::draw,ContextHelper);
DEFINE_HOOK(static_cast<void (dMeter2Draw_c::*)(f32,f32,f32*,f32,JUtility::TColor,JUtility::TColor,
    JUtility::TColor,JUtility::TColor,f32,u8,f32)>(&dMeter2Draw_c::drawPikari),CustomButtonGlow);
enum Texture{Neutral,Active,ArrowLeft,ArrowRight,ArrowUp,ArrowDown,FaceA,FaceB,FaceX,FaceY,Shoulder,OpaqueMask,TextureCount};
struct alignas(32) TextureData{ResTIMG header{};std::array<u8,64*64*4> pixels{};};
std::array<TextureData,TextureCount> s_textures;
struct alignas(32) BumperTexture {
    ResTIMG header{};
    alignas(32) std::array<u8,1056*616*4> pixels{};
};
BumperTexture s_bumperTexture;
alignas(32) std::array<u8,32+256*256*2> s_frameData{};
bool s_frameReady=false;
std::array<J2DPicture*,TextureCount> s_shapes{};
J2DPicture* s_digit=nullptr;
J2DTextBox* s_text=nullptr;
std::array<J2DPicture*,3> s_nativeLights{};
J2DPicture* s_bumper=nullptr;
J2DPicture* s_frame=nullptr;
J2DPicture* s_midna=nullptr;
J2DPicture* s_stickButton=nullptr;
float s_mapTop=0,s_navY=0;
bool s_mapVisible=false;
double s_navTime=0;
u8 s_collectionSelection=255;
bool legacy_footer(float x,float y);
bool s_renderingCustom=false;
struct CustomDrawScope {bool previous=s_renderingCustom;CustomDrawScope(){s_renderingCustom=true;}~CustomDrawScope(){s_renderingCustom=previous;}};
std::array<const ResTIMG*,4> s_legacyZTextures{};
void cache_legacy_z(){
    if(s_legacyZTextures[0])return;
    auto* archive=dComIfGp_getMain2DArchive();if(!archive)return;
    constexpr const char* names[]={"im_zelda_button_z_base.bti","im_zelda_button_z_text.bti","im_z_button_02.bti","textures/hud_theme/switch_zl.bti"};
    for(unsigned i=0;i<4;++i)s_legacyZTextures[i]=static_cast<ResTIMG*>(archive->getResource('TIMG',names[i]));
}

dMeterMap_c* s_mapDrawing=nullptr;
float s_savedMapW=0,s_savedMapH=0;
J2DPane* s_nativeMidnaPane=nullptr;
u8 s_nativeMidnaAlpha=0,s_nativeMidnaColorAlpha=0;
float s_nativeMidnaZAlpha=0;
struct MenuCell {float x,y,w,h;};
std::array<MenuCell,20> s_menuCells{};
unsigned s_menuCellCount=0;
J2DPicture* s_menuCellFrame=nullptr;
unsigned s_bagDrawIndex=0;
struct PictureColor {void* call;J2DPicture* picture;JUtility::TColor black,white;std::array<JUtility::TColor,4> corners;};
std::vector<PictureColor> s_pictureColors;
J2DPicture* s_wave=nullptr;
std::array<u8,6> s_contextKeys{};
std::array<bool,6> s_contextActive{};
std::array<double,6> s_contextSince{},s_lastNotice{};
daAlink_c* s_contextOwner=nullptr;
bool s_contextWolf=false;
dMeterButton_c* s_contextHelper=nullptr;
dMeterButton_c* s_pickupHelper=nullptr;
u32 s_pickupHelperMask=0;
JKRExpHeap* s_guiHeap=nullptr;
struct HeapScope {
    JKRHeap* previous;
    explicit HeapScope(JKRHeap* heap):previous(heap->becomeCurrentHeap()){}
    ~HeapScope(){if(previous)previous->becomeCurrentHeap();}
};
struct Art {
    J2DPicture* pictures[4]{};
    alignas(32) u8 buffer[2][4][0xC00]{};
    u8 item=255,page=0;int layers=0;
};
std::array<Art,6> s_art;
std::array<dKantera_icon_c*,5> s_oil{};
std::array<u8,5> s_lastInventory{255,255,255,255,255};
std::array<u8,5> s_lastMix{255,255,255,255,255};
std::array<double,5> s_flashUntil{};
dMeter2Draw_c* s_hud=nullptr;
dMenu_Ring_c* s_ring=nullptr;
bool s_drawing=false;
struct Hidden {J2DPane* pane;bool visible;};
std::array<Hidden,32> s_hidden{};unsigned s_hiddenCount=0;
std::array<u8,6> s_ammoAlpha{};
std::array<float,2> s_oilAlpha{};
bool s_ammoSuppressed=false;

double now(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
void make_texture(int which){
    auto& t=s_textures[which];auto& h=t.header;h.format=GX_TF_RGBA8;h.alphaEnabled=1;h.width=64;h.height=64;
    h.minFilter=GX_LINEAR;h.magFilter=GX_LINEAR;h.mipmapCount=1;h.imageOffset=32;
    constexpr int colors[4][3]={{56,112,28},{147,43,34},{37,89,145},{157,119,25}};
    for(int y=0;y<64;++y)for(int x=0;x<64;++x){
        const float px=(x-31.5f)/64,py=(y-31.5f)/64,r=std::sqrt(px*px+py*py);
        float alpha=std::clamp((.48f-r)*64,0.f,1.f);int red=34,green=36,blue=32;
        if(which==Shoulder){
            // A broad, slightly sloped shoulder-button silhouette, with room for RT.
            const float qx=std::abs(px+py*.12f)-.34f,qy=std::abs(py)-.19f;
            const float distance=std::hypot(std::max(qx,0.f),std::max(qy,0.f))+std::min(std::max(qx,qy),0.f)-.11f;
            alpha=std::clamp(-distance*64,0.f,1.f);
            const int shade=distance>-.04f?91:25-py*20;red=shade;green=shade+2;blue=shade;
        }
        else if(which==Active){red=237;green=200;blue=102;alpha*=std::clamp((r-.414f)*100,0.f,1.f);}
        else {
            if(which>=FaceA&&which<=FaceY){red=colors[which-FaceA][0];green=colors[which-FaceA][1];blue=colors[which-FaceA][2];}
            const float light=1.07f-py*.46f;red*=light;green*=light;blue*=light;
            if(r>.43f){red=12;green=16;blue=10;}
            else if(r>.405f){red=std::min(255,red+40);green=std::min(255,green+40);blue=std::min(255,blue+30);}
            if(which>=ArrowLeft&&which<=ArrowDown){
                float a=px,b=py;if(which==ArrowRight)a=-px;
                if(which==ArrowUp){a=py;b=px;}if(which==ArrowDown){a=-py;b=px;}
                const bool ink=(a>=-.31f&&a<=.03f&&std::abs(b)<(a+.31f)*.9f)||(a>=0&&a<.29f&&std::abs(b)<.085f);
                if(ink)red=green=blue=244;
            }
        }
        if(which==OpaqueMask){alpha=1.f;red=16;green=13;blue=10;}
        const unsigned tile=((y/4)*16+x/4)*64,offset=((y%4)*4+x%4)*2;
        t.pixels[tile+offset]=alpha*255;t.pixels[tile+offset+1]=red;t.pixels[tile+32+offset]=green;t.pixels[tile+32+offset+1]=blue;
    }
}
void release(){
    if(s_wave)JKR_DELETE(s_wave);s_wave=nullptr;s_contextOwner=nullptr;s_contextKeys={};s_contextActive={};s_contextSince={};s_lastNotice={};s_contextHelper=nullptr;s_pickupHelper=nullptr;s_pickupHelperMask=0;
    if(s_bumper)JKR_DELETE(s_bumper);s_bumper=nullptr;
    if(s_frame)JKR_DELETE(s_frame);s_frame=nullptr;
    if(s_midna)JKR_DELETE(s_midna);s_midna=nullptr;
    if(s_stickButton)JKR_DELETE(s_stickButton);s_stickButton=nullptr;
    s_mapTop=s_navY=0;s_mapVisible=false;s_navTime=0;
    s_legacyZTextures={};s_renderingCustom=false;s_collectionSelection=255;s_menuCellCount=0;s_menuCellFrame=nullptr;s_pictureColors.clear();
    for(auto*& p:s_nativeLights){if(p)JKR_DELETE(p);p=nullptr;}
    for(auto*& p:s_shapes){if(p)JKR_DELETE(p);p=nullptr;}
    for(auto& art:s_art){for(auto*& p:art.pictures){if(p)JKR_DELETE(p);p=nullptr;}art.item=255;art.layers=0;}
    for(auto*& p:s_oil){if(p)JKR_DELETE(p);p=nullptr;}
    if(s_digit)JKR_DELETE(s_digit);if(s_text)JKR_DELETE(s_text);s_digit=nullptr;s_text=nullptr;
    s_hud=nullptr;s_drawing=false;s_hiddenCount=0;s_ammoSuppressed=false;
    if(s_guiHeap){if(JKRHeap::getCurrentHeap()==s_guiHeap)JKRHeap::getSystemHeap()->becomeCurrentHeap();
        s_guiHeap->destroy();s_guiHeap=nullptr;}
}
bool ensure_resources(){
    if(s_text)return true;
    auto* font=mDoExt_getMesgFont();if(!font)return false;
    if(!s_guiHeap)s_guiHeap=JKRExpHeap::create(512*1024,mDoExt_getGameHeap(),false);
    if(!s_guiHeap)return false;
    auto* heap=s_guiHeap;HeapScope scope(heap);
    for(int i=0;i<TextureCount;++i){s_shapes[i]=JKR_NEW_ARGS(heap,32) J2DPicture(&s_textures[i].header);if(!s_shapes[i]){release();return false;}}
    if(s_frameReady)s_frame=JKR_NEW_ARGS(heap,32) J2DPicture(reinterpret_cast<ResTIMG*>(s_frameData.data()));
    s_digit=JKR_NEW_ARGS(heap,32) J2DPicture(&s_textures[Neutral].header);
    s_wave=JKR_NEW_ARGS(heap,32) J2DPicture(&s_textures[Active].header);
    s_text=JKR_NEW_ARGS(heap,32) J2DTextBox(0,JGeometry::TBox2<f32>(0,0,180,22),font->getResFont(),"",128,HBIND_LEFT,VBIND_TOP);
    if(!s_text||!s_digit||!s_wave){release();return false;}
    return true;
}
void text(const char* value,float x,float y,float size,u8 alpha=255,JUtility::TColor color=JUtility::TColor(245,244,226,255)){
    alpha=static_cast<u8>(alpha*hud_standalone_alpha());
    if(!alpha)return;
    if(!s_text||!value||!*value)return;
    // The native setter reallocates on every call. Reuse the owned buffer:
    // rendering must never consume a frame/solid heap or churn allocations.
    auto string=s_text->getStringPtr();
    std::snprintf(string.buffer,string.size,"%s",value);s_text->setFontSize(size,size*1.15f);
    const float start=x-dMeter2Info_getStringLength(s_text,const_cast<char*>(value))*.5f;
    const float baseline=y+size*.35f;
    s_text->setFontColor(JUtility::TColor(0,0,0,alpha),JUtility::TColor(0,0,0,alpha));s_text->draw(start+1,baseline+1);
    color.a=alpha;s_text->setFontColor(color,color);s_text->draw(start,baseline);
}
void shape(int texture,float x,float y,float size,u8 alpha){auto* p=s_shapes[texture];p->setAlpha(alpha);p->draw(x-size*.5f,y-size*.5f,size,size,false,false,false);}
void native_light(int index,float x,float y,float size,u8 alpha){
    if(!s_hud)return;
    auto* source=index==2?s_hud->mpLightB:s_hud->mpLightXY[index];
    if(!source)return;
    auto* picture=static_cast<J2DPicture*>(source->getPanePtr());
    auto* texture=picture?picture->getTexture(0):nullptr;
    if(!texture||!texture->getTexInfo())return;
    // Borrow the game's own item-light art. The owned picture dies before its
    // meter archive, without modifying the native pane's state or texture.
    auto*& p=s_nativeLights[index];
    if(!p)p=JKR_NEW_ARGS(s_guiHeap,32) J2DPicture(texture->getTexInfo());
    if(p){p->setAlpha(alpha);p->draw(x-size*.5f,y-size*.5f,size,size,false,false,false);}
}
void frame_hint(float x,float y,u8 alpha){
    if(!s_frame||!s_hud||!s_hud->mpUzu)return;
    auto* pane=s_hud->mpUzu->getPanePtr()->search(MULTI_CHAR('wire'));
    if(!pane||pane->getTypeID()!=18)return;
    auto* native=static_cast<J2DPicture*>(pane);
    // Use the original ornament's color treatment and standalone HUD fade.
    // This BTI has four equal circular openings for the modern diamond.
    s_frame->setBlackWhite(native->getBlack(),native->getWhite());
    s_frame->setCornerColor(native->corner(0),native->corner(1),native->corner(2),native->corner(3));
    s_frame->setAlpha(static_cast<u8>(alpha*.48f));
    const float scale=kcrux_frame_scale_percent()/100.f;
    const float width=100.f*scale,height=98.f*scale;
    const float centerX=x+kcrux_frame_offset_x(),centerY=y+kcrux_frame_offset_y();
    s_frame->draw(centerX-width*.5f,centerY-height*.5f,width,height,false,false,false);
}
void bumper_hint(const char* button,const char* label,kcrux::HudPoint point,u8 alpha){
    if(!s_bumper)s_bumper=JKR_NEW_ARGS(s_guiHeap,32) J2DPicture(&s_bumperTexture.header);
    if(!s_bumper)return;
    // Existing Xbox prompt-pack shoulder silhouette, not a round face button.
    s_bumper->setAlpha(alpha);s_bumper->draw(point.x-12.6f,point.y-7.2f,25.2f,14.4f,button[0]=='L',false,false);
    text(button,point.x,point.y,7.2f,alpha);
    text(label,point.x+30,point.y,8.1f,alpha);
}
void midna_hint(const kcrux::HudLayout& layout,u8 alpha){
    if(!s_hud||!s_hud->mpButtonMidona||!s_hud->mpButtonMidona->isVisible())return;
    auto* archive=dComIfGp_getMain2DArchive();if(!archive)return;
    if(!s_midna){auto* texture=static_cast<ResTIMG*>(archive->getResource('TIMG',"midona64.bti"));
        if(texture)s_midna=JKR_NEW_ARGS(s_guiHeap,32) J2DPicture(texture);}
    if(!s_stickButton){auto* texture=static_cast<ResTIMG*>(archive->getResource('TIMG',"tt_zelda_button_x_base.bti"));
        if(texture)s_stickButton=JKR_NEW_ARGS(s_guiHeap,32) J2DPicture(texture);}
    // Use Midna's native face and the existing shaded controller cap. Keep the
    // native availability distinction; this hint does not unlock an action.
    const u8 opacity=alpha*(s_hud->mButtonZAlpha>.1f?1.f:.5f);
    if(s_midna){s_midna->setAlpha(opacity);s_midna->draw(layout.midna.x-20,layout.midna.y-20,40,40,false,false,false);}
    if(s_stickButton){s_stickButton->setAlpha(opacity);s_stickButton->draw(layout.midnaButton.x-12,layout.midnaButton.y-18,24,36,false,false,false);}
    text("L3",layout.midnaButton.x,layout.midnaButton.y,8,opacity);
}
float pane_vertical_edge(CPaneMgr* manager,bool bottom){
    auto* pane=manager->getPanePtr();
    Mtx matrix;
    const auto left=manager->getGlobalVtx(pane,&matrix,bottom?2:0,false,0);
    const auto right=manager->getGlobalVtx(pane,&matrix,bottom?3:1,false,0);
    return bottom?std::max(left.y,right.y):std::min(left.y,right.y);
}
void place_midna_below_tears(kcrux::HudLayout& layout,float top,float bottom){
    if(!s_hud||!s_hud->mpLightDropParent||!s_hud->mpRupeeParent[0]||
       s_hud->mpLightDropParent->getAlphaRate()<=.05f||
       !s_hud->mpLightDropParent->getPanePtr()->isVisible())return;
    float tearsBottom=top;
    for(int i=0;i<16;++i){
        if(auto* part=s_hud->mpSIParts[i][0])
            tearsBottom=std::max(tearsBottom,pane_vertical_edge(part,true));
    }
    const float rupeesTop=pane_vertical_edge(s_hud->mpRupeeParent[0],false);
    // The native meter and rupee panes include user HUD scale and any saved
    // Dawnlight layout transforms. Keep the paired Midna hint centered in
    // their actual free space without crowding either element.
    if(!std::isfinite(tearsBottom)||!std::isfinite(rupeesTop)||
       tearsBottom<top||rupeesTop>bottom||rupeesTop-tearsBottom<48.f)return;
    layout.midna.y=layout.midnaButton.y=(tearsBottom+rupeesTop)*.5f;
}
bool pickup_action(u8 status){
    switch(status){
    case BUTTON_STATUS_PICK_UP:case BUTTON_STATUS_LIFT:case BUTTON_STATUS_TAKE:
    case BUTTON_STATUS_PICK:case BUTTON_STATUS_UNK_31:case BUTTON_STATUS_UNK_52:
    case BUTTON_STATUS_UNK_57:return true;
    default:return false;
    }
}
bool minor_carry_action(u8 status){
    return status==BUTTON_STATUS_PLACE||status==BUTTON_STATUS_THROW;
}
bool carry_action(u8 status){return pickup_action(status)||minor_carry_action(status);}
bool pickup_activity(daAlink_c* link){
    if(!link)return false;
    switch(link->mProcID){
    case daAlink_c::PROC_GRAB_READY:case daAlink_c::PROC_GRAB_UP:case daAlink_c::PROC_PICK_UP:
    case daAlink_c::PROC_WOLF_GRAB_UP:return true;
    default:return false;
    }
}
bool minor_carry_activity(daAlink_c* link){
    if(!link)return false;
    switch(link->mProcID){
    case daAlink_c::PROC_GRAB_THROW:case daAlink_c::PROC_GRAB_PUT:case daAlink_c::PROC_PICK_PUT:
    case daAlink_c::PROC_WOLF_GRAB_PUT:case daAlink_c::PROC_WOLF_GRAB_THROW:
        return true;
    default:return false;
    }
}
u32 pickup_helper_buttons(u8 action,u8 sword,u8 object){
    return (carry_action(action)?1u<<dMeterButton_c::BUTTON_A_e:0)|
        (carry_action(sword)?1u<<dMeterButton_c::BUTTON_B_e:0)|
        (carry_action(object)?1u<<dMeterButton_c::BUTTON_R_e:0);
}
bool context_action(u8 status,bool emphasis=false){
    if(minor_carry_action(status))return false;
    switch(status){
    case BUTTON_STATUS_NONE:case BUTTON_STATUS_PUT_AWAY:case BUTTON_STATUS_BACK:
    case BUTTON_STATUS_ATTACK:case BUTTON_STATUS_ROLL:case BUTTON_STATUS_DASH:
    case BUTTON_STATUS_DEFEND:case BUTTON_STATUS_LOCK:case BUTTON_STATUS_DRAW:
    case BUTTON_STATUS_UNK_121:case BUTTON_STATUS_UNK_134:case BUTTON_STATUS_SWIM:
    case BUTTON_STATUS_PADDLE:case BUTTON_STATUS_BLANK:
    case BUTTON_STATUS_UNK_37:case BUTTON_STATUS_UNK_38:case BUTTON_STATUS_UNK_39:
    case BUTTON_STATUS_UNK_79:return false; // Native blank/default sword and canoe states.
    case BUTTON_STATUS_SENSE:case BUTTON_STATUS_DIG:case BUTTON_STATUS_JUMP:return emphasis;
    default:return true;
    }
}
constexpr double kContextDuration=.65,kContextCooldown=.75,kContextPeriod=kContextDuration+kContextCooldown;
std::array<bool,6> context_activity(daAlink_c* link){
    std::array<bool,6> active{};
    if(!link)return active;
    if(link->checkWolf())active[3]=link->checkWolfEyeUp()!=0;
    if(link->checkFmChainGrabAnime())active[2]=true;
    // Native procedures confirm the action actually started. An available
    // prompt, a held button or failed input is insufficient.
    switch(link->mProcID){
    case daAlink_c::PROC_GRAB_READY:case daAlink_c::PROC_GRAB_UP:case daAlink_c::PROC_PICK_UP:
        active[0]=true;break;
    case daAlink_c::PROC_WOLF_GRAB_UP:active[2]=true;break;
    case daAlink_c::PROC_PUSH_PULL_WAIT:case daAlink_c::PROC_PUSH_MOVE:case daAlink_c::PROC_PULL_MOVE:
    case daAlink_c::PROC_CHAIN_UP:case daAlink_c::PROC_CHAIN_STRONG_PULL:
    case daAlink_c::PROC_WOLF_CHAIN_UP:case daAlink_c::PROC_WOLF_CHAIN_READY:case daAlink_c::PROC_WOLF_CHAIN_WAIT:
        active[2]=true;break;
    case daAlink_c::PROC_WOLF_DIG:case daAlink_c::PROC_WOLF_DIG_THROUGH:active[4]=true;break;
    case daAlink_c::PROC_HORSE_GETOFF:active[5]=true;break;
    case daAlink_c::PROC_CRAWL_START:case daAlink_c::PROC_CRAWL_MOVE:case daAlink_c::PROC_CRAWL_AUTO_MOVE:case daAlink_c::PROC_CRAWL_END:
    case daAlink_c::PROC_HANG_UP:case daAlink_c::PROC_HANG_CLIMB:case daAlink_c::PROC_HANG_LEVER_DOWN:
    case daAlink_c::PROC_LADDER_UP_START:case daAlink_c::PROC_LADDER_UP_END:
    case daAlink_c::PROC_LADDER_DOWN_START:case daAlink_c::PROC_LADDER_DOWN_END:case daAlink_c::PROC_LADDER_MOVE:
    case daAlink_c::PROC_CLIMB_UP_START:case daAlink_c::PROC_CLIMB_DOWN_START:
    case daAlink_c::PROC_CLIMB_MOVE_UPDOWN:case daAlink_c::PROC_CLIMB_MOVE_SIDE:case daAlink_c::PROC_CLIMB_TO_ROOF:
    case daAlink_c::PROC_CUT_DOWN:case daAlink_c::PROC_CUT_DOWN_LAND:
    case daAlink_c::PROC_CUT_HEAD:case daAlink_c::PROC_CUT_HEAD_LAND:case daAlink_c::PROC_WOLF_DOWN_ATTACK:
    case daAlink_c::PROC_DIVE_JUMP:
    case daAlink_c::PROC_DOOR_OPEN:case daAlink_c::PROC_TALK:case daAlink_c::PROC_INSECT_CATCH:
    case daAlink_c::PROC_GOAT_CATCH:case daAlink_c::PROC_GOAT_STROKE:case daAlink_c::PROC_HORSE_RIDE:case daAlink_c::PROC_CANOE_RIDE:
    case daAlink_c::PROC_WOLF_HOWL_DEMO:case daAlink_c::PROC_WOLF_HOWL:case daAlink_c::PROC_WOLF_GET_SMELL:
    case daAlink_c::PROC_WOLF_TAG_JUMP:case daAlink_c::PROC_WOLF_TAG_JUMP_LAND:
        active[0]=true;break;
    default:break;
    }
    return active;
}
void update_context_cues(daAlink_c* link){
    const bool wolf=link->checkWolf();
    if(link!=s_contextOwner||wolf!=s_contextWolf){s_contextKeys={};s_contextActive={};s_contextSince={};s_lastNotice={};}
    s_contextOwner=link;s_contextWolf=wolf;
    const u8 a=dComIfGp_getDoStatusForce()?dComIfGp_getDoStatusForce():dComIfGp_getDoStatus();
    const u8 x=dComIfGp_getAStatusForce()?dComIfGp_getAStatusForce():dComIfGp_getAStatus();
    const u8 cancel=kcrux_cancel_action(link),push=kcrux_push_action(link);
    const bool emphasisA=dComIfGp_isDoSetFlag(BUTTON_STATUS_FLAG_EMPHASIS)||
        (dComIfGp_getDoSetFlagForce()&BUTTON_STATUS_FLAG_EMPHASIS);
    const bool emphasisX=dComIfGp_isASetFlag(BUTTON_STATUS_FLAG_EMPHASIS)||
        (dComIfGp_getASetFlagForce()&BUTTON_STATUS_FLAG_EMPHASIS);
    // A, X, Y, wolf Up, wolf Down, B. Routine actions never raise a notice.
    const std::array<u8,6> keys={
        u8(a!=BUTTON_STATUS_DISMOUNT&&context_action(a,emphasisA)?a:0),
        u8(context_action(x,emphasisX)?x:0),u8(push&&!minor_carry_action(push)?BUTTON_STATUS_GRAB:0),
        u8(wolf&&dComIfGp_isXSetFlag(BUTTON_STATUS_FLAG_EMPHASIS)?dComIfGp_getXStatus():0),
        u8(wolf&&dComIfGp_isYSetFlag(BUTTON_STATUS_FLAG_EMPHASIS)?dComIfGp_getYStatus():0),
        u8(cancel==BUTTON_STATUS_DISMOUNT?cancel:0)};
    const double stamp=now();
    const auto previousActive=s_contextActive;s_contextActive=context_activity(link);
    for(int i=0;i<6;++i){
        if(s_contextActive[i]){s_contextKeys[i]=keys[i];s_contextSince[i]=0;continue;}
        if(keys[i]!=s_contextKeys[i]||previousActive[i]){
            if(keys[i]){
                // A fast leave/re-enter waits out the cooldown instead of
                // losing its reminder until the player leaves again.
                s_contextSince[i]=previousActive[i]?stamp+kContextCooldown:std::max(stamp,s_lastNotice[i]+kContextCooldown);
            }
            if(!keys[i])s_contextSince[i]=0;
            s_contextKeys[i]=keys[i];
        }
        if(keys[i]&&stamp>=s_contextSince[i])
            s_lastNotice[i]=s_contextSince[i]+std::floor((stamp-s_contextSince[i])/kContextPeriod)*kContextPeriod;
    }
}
float context_phase(double age){
    if(age<0)return -1;
    const double phase=std::fmod(age,kContextPeriod);
    return phase<kContextDuration?static_cast<float>(phase/kContextDuration):-1;
}
void active_action_glow(float x,float y,float size,u8 alpha,float intensity=.6f){
    if(!s_hud||!s_hud->mpPikariParent)return;
    // Reuse the existing opening flash at its bright frame. A local clock,
    // zero animation speed and non-audible draw mode leave native cues alone.
    auto& style=g_drawHIO.mEmpButton;
    auto frontOuter=style.mPikariFrontOuter,frontInner=style.mPikariFrontInner;
    auto backOuter=style.mPikariBackOuter,backInner=style.mPikariBackInner;
    for(auto* color:{&frontOuter,&frontInner,&backOuter,&backInner})color->a*=intensity;
    auto* parent=s_hud->mpPikariParent;const float previous=parent->getAlphaRate();
    parent->setAlphaRate(previous*alpha/255.f);
    f32 frame=18.f;
    s_hud->drawPikari(x,y,&frame,style.mPikariScale*.7f*size/29.f,
        frontOuter,frontInner,backOuter,backInner,0.f,0,0.f);
    parent->setAlphaRate(previous);
}
void context_flare(int channel,float x,float y,float size,u8 alpha,JUtility::TColor color){
    if(s_contextActive[channel]){active_action_glow(x,y,size,alpha);return;}
    const double age=now()-s_contextSince[channel];
    const float t=context_phase(age);
    if(!s_wave||!s_contextKeys[channel]||!s_contextSince[channel]||t<0)return;
    const float eased=1-(1-t)*(1-t);
    const float diameter=size+5+size*.65f*eased;
    // Keep the same 0.65-second ring, with a quiet gap before each reminder.
    color.a=255;s_wave->setBlackWhite(JUtility::TColor(color.r,color.g,color.b,0),color);
    s_wave->setAlpha(alpha*.65f*(1-t)*(1-t));
    s_wave->draw(x-diameter*.5f,y-diameter*.5f,diameter,diameter,false,false,false);
}
void art(int index,u8 item,float x,float y,float size,u8 alpha){
    if(item==255||item==0)return;
    if(item==dItemNo_LIGHT_ARROW_e||item==dItemNo_HAWK_ARROW_e)item=dItemNo_BOW_e;
    auto& a=s_art[index];
    if(!a.pictures[0])for(auto*& p:a.pictures)p=JKR_NEW_ARGS(s_guiHeap,32) J2DPicture(&s_textures[Neutral].header);
    for(auto* p:a.pictures)if(!p)return;
    if(a.item!=item){a.page^=1;a.layers=dMeter2Info_readItemTexture(item,a.buffer[a.page][0],a.pictures[0],
        a.buffer[a.page][1],a.pictures[1],a.buffer[a.page][2],a.pictures[2],a.buffer[a.page][3],a.pictures[3],-1);a.item=item;}
    for(int i=0;i<std::min(a.layers,4);++i){auto* p=a.pictures[i];const auto* image=p->getTexture(0)->getTexInfo();if(!image)continue;
        const float w=image->width,h=image->height,scale=size/std::max(w,h);
        p->setAlpha(alpha);p->draw(x-w*scale*.5f,y-h*scale*.5f,w*scale,h*scale,false,false,false);}
}
void digits(int count,int maximum,float x,float y,u8 alpha){
    if(count<0||maximum<=0||!s_hud||!s_digit)return;
    count=std::clamp(count,0,999);char number[8];std::snprintf(number,sizeof(number),"%d",count);
    const JUtility::TColor color=count==0?JUtility::TColor(170,170,170,255):count>=maximum?JUtility::TColor(255,210,95,255):JUtility::TColor(255,255,255,255);
    s_digit->setBlackWhite(JUtility::TColor(0,0,0,0),color);s_digit->setAlpha(alpha);
    for(int i=0;number[i];++i){auto* image=s_hud->getNumberTexture(number[i]-'0');if(image){s_digit->changeTexture(image,0);s_digit->draw(x+i*8,y,9,11,false,false,false);}}
}
void badge(int slot,float x,float y,float size,u8 alpha){
    if(slot<4)shape(ArrowLeft+slot,x,y,size,alpha);
    else{shape(Shoulder,x,y,size*1.5f,alpha);text("RT",x,y,size*.48f,alpha);}
}
void item_slot(int slot,float x,float y,u8 alpha,bool wheel,bool showOil=true){
    constexpr float size=kcrux::HudLayout::itemSize;
    const auto v=kcrux_slot_view(slot);const bool active=kcrux_active_slot()==slot&&!wheel;
    if(s_lastInventory[slot]!=v.inventory||s_lastMix[slot]!=v.mix){s_lastInventory[slot]=v.inventory;s_lastMix[slot]=v.mix;s_flashUntil[slot]=now()+.4;}
    const float remaining=std::clamp(static_cast<float>((s_flashUntil[slot]-now())/.4),0.f,1.f);
    const float pulse=1.f+.16f*std::sin(remaining*3.14159265f);
    if(active)shape(Active,x,y,size+5,alpha);
    if(remaining>0)shape(Active,x,y,size+5+10*(1-remaining),alpha*remaining);
    shape(Neutral,x,y,size*pulse,v.available?alpha:alpha*.45f);
    art(slot,v.item,x,y-2,size*pulse,v.available?alpha:alpha*.45f);
    badge(slot,x+15,y+13,15,alpha);
    digits(v.count,v.maximum,x-17,y+12,alpha);
    if(showOil&&v.oil&&dComIfGs_getMaxOil()>0){
        if(!s_oil[slot])s_oil[slot]=JKR_NEW_ARGS(s_guiHeap,32) dKantera_icon_c();
        if(s_oil[slot]){auto* p=s_oil[slot];p->setPos(x+5,y+18);p->setScale(.36f,.36f);p->setNowGauge(dComIfGs_getMaxOil(),dComIfGs_getOil());p->setAlphaRate(alpha/255.f);p->drawSelf();}
    }
}
void wolf_action(int slot,float x,float y,u8 status,bool active,u8 alpha){
    constexpr float size=kcrux::HudLayout::wolfSize;
    const u8 opacity=status?alpha:static_cast<u8>(alpha*.45f);
    if(active)shape(Active,x,y,size+5,alpha);
    shape(Neutral,x,y,size,opacity);
    native_light(slot,x,y,size+6,opacity*(active?.8f:.45f));
    const char* label=s_hud&&status?s_hud->getActionString(status,1,nullptr):nullptr;
    if(!label||!*label)label=slot==0?"Senses":"Dig";
    text(label,x,y,8.5f,opacity);badge(slot+2,x+13,y+12,14,alpha);
    context_flare(slot+3,x,y,size,alpha,slot==0?JUtility::TColor(120,220,190,255):JUtility::TColor(235,210,130,255));
}
void draw_layout(dMenu_Ring_c* ring=nullptr){
    if(!kcrux_controls_enabled()||!ensure_resources())return;
    HeapScope scope(s_guiHeap);
    CustomDrawScope customDraw;
    auto* link=daAlink_getAlinkActorClass();if(!link||(!ring&&link->checkEventRun()))return;
    const int window=dMeter2Info_getWindowStatus();if(!ring&&window!=0)return;
    auto* port=dComIfGp_getCurrentGrafPort();if(!port)return;port->setup2D();
    auto bounds=*port->getBounds();if(port->getGrafType()==1)bounds=*static_cast<J2DOrthoGraph*>(port)->getOrtho();
    auto layout=kcrux::make_hud_layout(bounds.i.x,bounds.i.y,bounds.f.x,bounds.f.y);
    if(!ring)place_midna_below_tears(layout,bounds.i.y,bounds.f.y);
    const float rate=ring?std::clamp(ring->mAlphaRate,0.f,1.f):s_hud&&s_hud->mpButtonParent?std::clamp(s_hud->mpButtonParent->getAlphaRate(),0.f,1.f):1.f;
    const u8 alpha=rate*255;if(!alpha)return;
    const bool wolf=link->checkWolf();
    if(!ring)update_context_cues(link);
    if(wolf){
        wolf_action(0,layout.wolf[0].x,layout.wolf[0].y,dComIfGp_getXStatus(),link->checkWolfEyeUp()!=0,alpha);
        wolf_action(1,layout.wolf[1].x,layout.wolf[1].y,dComIfGp_getYStatus(),link->mProcID==daAlink_c::PROC_WOLF_DIG||
            link->mProcID==daAlink_c::PROC_WOLF_DIG_THROUGH,alpha);
    }else for(int i=0;i<5;++i)item_slot(i,layout.items[i].x,layout.items[i].y,alpha,ring);
    if(ring){
        // Each ring item gets its actual five-slot ownership badge, including RT.
        for(int i=0;!wolf&&i<ring->mItemsTotal;++i){const int owner=kcrux_item_owner(ring->mItemSlots[i]);
            if(owner>=0)badge(owner,ring->mItemSlotPosX[i]+ring->mCenterPosX+17,ring->mItemSlotPosY[i]+ring->mCenterPosY+15,16,alpha);}
        const int combo=ring->mCurrentSlot<ring->mItemsTotal?svc_kcrux_controls->combination_state(ring->mItemSlots[ring->mCurrentSlot]):0;
        if(combo&&!wolf){
            const float cx=bounds.f.x-154.f,cy=bounds.f.y-48.f;
            shape(FaceY,cx,cy,20,alpha);text("Y",cx,cy,11,alpha);
            text(combo==2?"Combo Off":"Combine",cx+43,cy,12,alpha);
        }
        return;
    }
    const auto a=layout.face[0],x=layout.face[1],y=layout.face[2],b=layout.face[3];
    const bool aiming=kcrux_aiming_item(link);
    const u8 push=kcrux_push_action(link);
    frame_hint(y.x,x.y,alpha);
    shape(FaceY,y.x,y.y,29,push||r_jump_enabled()?alpha:alpha*.45f);text("Y",y.x,y.y,16,alpha);
    text(push&&s_hud?s_hud->getActionString(push,1,nullptr):"Jump",y.x,y.y-27,9,push||r_jump_enabled()?alpha:alpha*.45f);
    shape(FaceX,x.x,x.y,29,alpha);
    const u8 sword=dComIfGs_getSelectEquipSword();
    if(!wolf&&sword!=255&&sword!=0){
        art(5,sword,x.x,x.y-3,31,alpha);
        shape(FaceX,x.x+12,x.y+11,13,alpha);text("X",x.x+12,x.y+11,8,alpha);
    }else{
        native_light(2,x.x,x.y,36,alpha*.55f);
        text("X",x.x,x.y,16,alpha);
    }
    const u8 attackStatus=dComIfGp_getAStatusForce()?dComIfGp_getAStatusForce():dComIfGp_getAStatus();
    text(s_hud&&attackStatus?s_hud->getActionString(attackStatus,1,nullptr):(wolf?"Attack":"Sword"),x.x,x.y+17,8,alpha);
    const u8 cancel=kcrux_cancel_action(link);
    shape(FaceB,b.x,b.y,29,cancel?alpha:alpha*.55f);text("B",b.x,b.y,16,alpha);
    if(s_hud&&cancel)text(s_hud->getActionString(cancel,1,nullptr),b.x,b.y+17,8,alpha);
    shape(FaceA,a.x,a.y,29,alpha);text("A",a.x,a.y,16,alpha);
    const u8 status=dComIfGp_getDoStatusForce()?dComIfGp_getDoStatusForce():dComIfGp_getDoStatus();
    if(s_hud&&status&&status!=BUTTON_STATUS_PUT_AWAY&&status!=BUTTON_STATUS_DISMOUNT&&
        !(aiming&&status==BUTTON_STATUS_BACK))text(s_hud->getActionString(status,1,nullptr),a.x,a.y+17,9,alpha);
    // Place/Throw stay Minor; initial pickup availability gets the Major cue.
    if(minor_carry_action(status)||(!status&&minor_carry_activity(link)))active_action_glow(a.x,a.y,29,alpha,.14f);
    if(minor_carry_action(attackStatus))active_action_glow(x.x,x.y,29,alpha,.14f);
    if(minor_carry_action(push))active_action_glow(y.x,y.y,29,alpha,.14f);
    context_flare(0,a.x,a.y,29,alpha,JUtility::TColor(125,215,85,255));
    context_flare(1,x.x,x.y,29,alpha,JUtility::TColor(100,180,240,255));
    context_flare(2,y.x,y.y,29,alpha,JUtility::TColor(240,210,115,255));
    context_flare(5,b.x,b.y,29,alpha,JUtility::TColor(235,125,105,255));
    const double stamp=now();
    const float target=s_mapVisible?std::min(layout.bumpers[0].y,s_mapTop-35):layout.bumpers[0].y;
    if(!s_navTime)s_navY=target;
    else s_navY+=(target-s_navY)*(1-std::exp(-std::clamp(stamp-s_navTime,0.,.1)/.075));
    s_navTime=stamp;
    bumper_hint("LB","Map",{layout.bumpers[0].x,s_navY},alpha);
    bumper_hint("RB","Items",{layout.bumpers[1].x,s_navY+17.5f},alpha);
    midna_hint(layout,alpha);
}

void hide(J2DPane* pane){if(!pane||s_hiddenCount>=s_hidden.size())return;s_hidden[s_hiddenCount++]={pane,pane->isVisible()};pane->hide();}
void restore_hidden(){while(s_hiddenCount){auto state=s_hidden[--s_hiddenCount];if(state.visible)state.pane->show();else state.pane->hide();}}
void hide_pickup_helper(dMeterButton_c* helper,u32 mask){
    if(!helper)return;
    if((mask&(1u<<dMeterButton_c::BUTTON_A_e))&&helper->mpButtonA)hide(helper->mpButtonA->getPanePtr());
    if((mask&(1u<<dMeterButton_c::BUTTON_B_e))&&helper->mpButtonB)hide(helper->mpButtonB->getPanePtr());
    if((mask&(1u<<dMeterButton_c::BUTTON_R_e))&&helper->mpButtonR)hide(helper->mpButtonR->getPanePtr());
    for(int i=0;i<2;++i)if(helper->field_0x4be[i]<32&&(mask&(1u<<helper->field_0x4be[i]))&&helper->mpText[i])
        hide(helper->mpText[i]->getPanePtr());
}
HookAction before_screen(ModContext*,void* args,void*,void*){
    auto* screen=mods::arg<J2DScreen*>(args,0);
    // Essentials' optional third-slot bridge can draw its cross cluster from
    // a separate screen before the standard meter draw hook runs.
    if(screen&&kcrux_controls_enabled())hide(screen->search(MULTI_CHAR('juji_n')));
    // Hide only redundant central callouts for remapped object/wolf actions. Their update/draw logic
    // still runs, preserving every native animation timer and sound cue.
    if(s_contextHelper&&(screen==s_contextHelper->mpButtonScreen||screen==s_contextHelper->field_0x00c))
        return HOOK_SKIP_ORIGINAL;
    if(s_pickupHelper&&(screen==s_pickupHelper->mpButtonScreen||screen==s_pickupHelper->field_0x00c))
        hide_pickup_helper(s_pickupHelper,s_pickupHelperMask);
    if(s_drawing&&s_hud&&kcrux_controls_enabled()&&mods::arg<J2DScreen*>(args,0)==s_hud->getMainScreenPtr()){
        hide(s_hud->mpButtonParent?s_hud->mpButtonParent->getPanePtr():nullptr);
        hide(s_hud->mpButtonCrossParent?s_hud->mpButtonCrossParent->getPanePtr():nullptr);
        hide(s_hud->mpButtonMidona?s_hud->mpButtonMidona->getPanePtr():nullptr);
    }
    if(s_ring&&kcrux_controls_enabled()&&mods::arg<J2DScreen*>(args,0)==s_ring->mpScreen){
        for(u64 tag:{MULTI_CHAR('x_btn_n'),MULTI_CHAR('y_btn_n'),MULTI_CHAR('r_btn_n'),
            MULTI_CHAR('yx_te_s1'),MULTI_CHAR('yx_te_s2'),MULTI_CHAR('yx_te_s3'),MULTI_CHAR('yx_te_s4'),MULTI_CHAR('yx_text'),
            MULTI_CHAR('fyx_te_1'),MULTI_CHAR('fyx_te_2'),MULTI_CHAR('fyx_te_3'),MULTI_CHAR('fyx_te_4'),MULTI_CHAR('fyx_tex')})
            hide(s_ring->mpScreen->search(tag));
    }
    return HOOK_CONTINUE;
}
void draw_collection_overlay();
void after_screen(ModContext*,void* args,void*,void*){
    restore_hidden();
    auto* screen=mods::arg<J2DScreen*>(args,0);
    if(s_hud&&screen==s_hud->getMainScreenPtr())draw_collection_overlay();
}
HookAction before_hud(ModContext*,void* args,void*,void*){
    s_hud=mods::arg<dMeter2Draw_c*>(args,0);s_drawing=kcrux_controls_enabled();
    if(s_drawing){
        // Rewired owns these controls for the entire boot session. Essentials
        // may redraw its third-slot icon after the main screen draw hook, so
        // suppress the native cluster before any of its HUD passes begin.
        if(auto* screen=s_hud->getMainScreenPtr())
            if(auto* cross=screen->search(MULTI_CHAR('juji_n')))cross->hide();
        if(s_hud->mpButtonMidona)s_hud->mpButtonMidona->getPanePtr()->hide();
        for(int i=0;i<2;++i){for(int j=0;j<3;++j)if(auto* p=s_hud->mpItemNumTex[i][j]){s_ammoAlpha[i*3+j]=p->getAlpha();p->setAlpha(0);}
            if(auto* p=s_hud->mpKanteraMeter[i]){s_oilAlpha[i]=p->mpParent?p->mpParent->getAlphaRate():0;p->setAlphaRate(0);}}
        s_ammoSuppressed=true;
    }
    return HOOK_CONTINUE;
}
HookAction mask_midna_before_essentials(ModContext*,void* args,void*,void*){
    if(!kcrux_controls_enabled())return HOOK_CONTINUE;
    cache_legacy_z();
    auto* hud=mods::arg<dMeter2Draw_c*>(args,0);
    s_nativeMidnaPane=hud->mpButtonMidona?hud->mpButtonMidona->getPanePtr():nullptr;
    s_nativeMidnaZAlpha=hud->mButtonZAlpha;hud->mButtonZAlpha=0;
    if(s_nativeMidnaPane){s_nativeMidnaAlpha=s_nativeMidnaPane->getAlpha();s_nativeMidnaColorAlpha=s_nativeMidnaPane->mColorAlpha;
        s_nativeMidnaPane->setAlpha(0);s_nativeMidnaPane->mColorAlpha=0;}
    return HOOK_CONTINUE;
}
void after_hud(ModContext*,void*,void*,void*){
    if(s_drawing&&s_hud){s_hud->mButtonZAlpha=s_nativeMidnaZAlpha;
        if(s_nativeMidnaPane){s_nativeMidnaPane->setAlpha(s_nativeMidnaAlpha);s_nativeMidnaPane->mColorAlpha=s_nativeMidnaColorAlpha;}}
    s_nativeMidnaPane=nullptr;
    if(s_drawing)draw_layout();
    if(s_ammoSuppressed&&s_hud){for(int i=0;i<2;++i){for(int j=0;j<3;++j)if(auto* p=s_hud->mpItemNumTex[i][j])p->setAlpha(s_ammoAlpha[i*3+j]);
        if(auto* p=s_hud->mpKanteraMeter[i])p->setAlphaRate(s_oilAlpha[i]);}}
    s_drawing=false;s_ammoSuppressed=false;
}
HookAction before_delete(ModContext*,void*,void*,void*){release();return HOOK_CONTINUE;}
HookAction before_context_helper(ModContext*,void* args,void*,void*){
    auto* helper=mods::arg<dMeterButton_c*>(args,0);auto* meter=dMeter2Info_getMeterClass();
    auto* link=daAlink_getAlinkActorClass();
    const bool wolfAction=link&&link->checkWolf()&&
        ((dComIfGp_isYSetFlag(BUTTON_STATUS_FLAG_EMPHASIS)&&dComIfGp_getYStatus()==BUTTON_STATUS_DIG)||
         (dComIfGp_isXSetFlag(BUTTON_STATUS_FLAG_EMPHASIS)&&dComIfGp_getXStatus()==BUTTON_STATUS_SENSE));
    const bool owned=kcrux_controls_enabled()&&meter&&helper==meter->mpEmpButton&&link&&
        !link->checkEventRun()&&dMeter2Info_getWindowStatus()==0;
    const u8 push=owned?kcrux_push_action(link):0;
    s_contextHelper=owned&&((push&&!minor_carry_action(push))||wolfAction)?helper:nullptr;
    const u8 action=dComIfGp_getDoStatusForce()?dComIfGp_getDoStatusForce():dComIfGp_getDoStatus();
    s_pickupHelperMask=owned?pickup_helper_buttons(
        !action&&(minor_carry_activity(link)||pickup_activity(link))?static_cast<u8>(BUTTON_STATUS_PICK_UP):action,
        dComIfGp_getAStatusForce()?dComIfGp_getAStatusForce():dComIfGp_getAStatus(),
        dComIfGp_getRStatusForce()?dComIfGp_getRStatusForce():dComIfGp_getRStatus()):0;
    s_pickupHelper=s_pickupHelperMask?helper:nullptr;
    return HOOK_CONTINUE;
}
void after_context_helper(ModContext*,void*,void*,void*){s_contextHelper=nullptr;s_pickupHelper=nullptr;s_pickupHelperMask=0;}
HookAction before_button_glow(ModContext*,void* args,void*,void*){
    auto* hud=mods::arg<dMeter2Draw_c*>(args,0);
    if(!kcrux_controls_enabled())return HOOK_CONTINUE;
    if(legacy_footer(mods::arg<f32>(args,1),mods::arg<f32>(args,2))){
        for(int i=5;i<=8;++i)mods::arg_ref<JUtility::TColor>(args,i).a=0;
        // Mode 2 plays the combination-available ping as the flare loops.
        // The replaced footer stays silent; actual combine/separate audio is
        // emitted independently by the canonical assignment transaction.
        if(mods::arg<u8>(args,10)==2)mods::arg_ref<u8>(args,10)=0;
        return HOOK_CONTINUE;
    }
    auto* frame=mods::arg<f32*>(args,3);
    // The old native anchor remains hidden. Keep its clock/audio, with no
    // stray glow at the former D-pad location.
    if(hud&&frame==&hud->field_0x738){
        for(int i=5;i<=8;++i)mods::arg_ref<JUtility::TColor>(args,i).a=0;
        return HOOK_CONTINUE;
    }
    int helperButton=-1;
    auto* helper=s_contextHelper?s_contextHelper:s_pickupHelper;
    if(helper)for(int i=0;i<2;++i)if(frame==&helper->field_0x2e8[i])helperButton=helper->field_0x4be[i];
    const auto* link=daAlink_getAlinkActorClass();const bool wolf=link&&link->checkWolf();
    const bool own=s_drawing&&hud==s_hud;
    const bool a=(own&&frame==&hud->field_0x608)||helperButton==dMeterButton_c::BUTTON_A_e;
    const bool b=(own&&frame==&hud->field_0x60c)||helperButton==dMeterButton_c::BUTTON_B_e;
    const bool y=helperButton==dMeterButton_c::BUTTON_R_e;
    const int item=(own&&frame==&hud->field_0x620[0])||(wolf&&helperButton==dMeterButton_c::BUTTON_X_e)?0:
        (own&&frame==&hud->field_0x620[1])||(wolf&&helperButton==dMeterButton_c::BUTTON_Y_e)?1:-1;
    if(!a&&!b&&!y&&item<0)return HOOK_CONTINUE; // Hearts, tears and Midna keep their native effects.
    auto* port=dComIfGp_getCurrentGrafPort();if(!port)return HOOK_CONTINUE;
    auto bounds=*port->getBounds();if(port->getGrafType()==1)bounds=*static_cast<J2DOrthoGraph*>(port)->getOrtho();
    const auto layout=kcrux::make_hud_layout(bounds.i.x,bounds.i.y,bounds.f.x,bounds.f.y);
    const u8 action=dComIfGp_getDoStatusForce()?dComIfGp_getDoStatusForce():dComIfGp_getDoStatus();
    const u8 swordAction=dComIfGp_getAStatusForce()?dComIfGp_getAStatusForce():dComIfGp_getAStatus();
    const bool quietCarry=(a&&minor_carry_action(action))||(b&&minor_carry_action(swordAction))||
        (y&&minor_carry_action(kcrux_push_action(daAlink_getAlinkActorClass())));
    const int channel=a?0:b?1:y?2:item>=0?item+3:-1;
    const bool sustained=channel>=0&&context_activity(daAlink_getAlinkActorClass())[channel];
    const auto point=a?layout.face[0]:b?layout.face[1]:y?layout.face[2]:wolf?layout.wolf[item]:layout.items[item];
    mods::arg_ref<f32>(args,1)=point.x;
    mods::arg_ref<f32>(args,2)=point.y;
    mods::arg_ref<f32>(args,4)*=.7f*(item>=0?(wolf?kcrux::HudLayout::wolfSize:kcrux::HudLayout::itemSize)/42.f:1.f);
    // Keep native timers and sound calls running; only Place/Throw stay quiet.
    for(int i=5;i<=8;++i)mods::arg_ref<JUtility::TColor>(args,i).a*=quietCarry||sustained?0.f:.6f;
    return HOOK_CONTINUE;
}
HookAction skip_legacy_selection(ModContext*,void*,void*,void*){return kcrux_controls_enabled()?HOOK_SKIP_ORIGINAL:HOOK_CONTINUE;}
HookAction ring_number(ModContext*,void* args,void*,void*){
    if(!kcrux_controls_enabled())return HOOK_CONTINUE;
    if(!ensure_resources())return HOOK_CONTINUE;
    HeapScope heap(s_guiHeap);CustomDrawScope customDraw;
    const auto* ring=mods::arg<dMenu_Ring_c*>(args,0);
    const float opacity=ring->mPlayerIsWolf?g_ringHIO.mItemIconAlpha_Wolf:g_ringHIO.mItemIconAlpha;
    // Native number coordinates point at the icon's lower midpoint. Use its
    // lower-left corner and our HUD digit size, leaving the right badge clear.
    digits(mods::arg<int>(args,1),mods::arg<int>(args,2),mods::arg<f32>(args,3)-24.f,
        mods::arg<f32>(args,4)-11.f,std::clamp(ring->mAlphaRate,0.f,1.f)*opacity);
    return HOOK_SKIP_ORIGINAL;
}
HookAction ring_mix_message(ModContext*,void* args,void*,void*){
    if(!kcrux_controls_enabled())return HOOK_CONTINUE;
    auto* ring=mods::arg<dMenu_Ring_c*>(args,0);
    if(ring->mpTextParent[4])ring->mpTextParent[4]->hide();
    ring->mPikariFlashingSpeed=0.f;
    // Our canonical prompt replaces the native R label, availability ping,
    // and independently drawn gradient. Combine/separate audio lives in core.
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_ring(ModContext*,void* args,void*,void*){
    s_ring=mods::arg<dMenu_Ring_c*>(args,0);
    return HOOK_CONTINUE;
}
void after_ring(ModContext*,void* args,void*,void*){
    auto* ring=mods::arg<dMenu_Ring_c*>(args,0);
    if(ring&&ring->mDrawFlag==0)draw_layout(ring);s_ring=nullptr;
}
void draw_collection_overlay(){
    if(kcrux_collection_page()!=1||s_menuCellCount!=20||!ensure_resources())return;
    HeapScope scope(s_guiHeap);
    CustomDrawScope customDraw;
    auto* port=dComIfGp_getCurrentGrafPort();
    if(!port)return;
    port->setup2D();
    const auto bounds=port->getGrafType()==1?*static_cast<J2DOrthoGraph*>(port)->getOrtho():*port->getBounds();
    const float slotY=s_menuCells[19].y+s_menuCells[19].h+32.f;
    const float footerY=bounds.f.y-34.f;
    for(int i=0;i<5;++i)item_slot(i,s_menuCells[i].x+s_menuCells[i].w*.5f,slotY,255,false,false);
    // TE's 5x4 inventory grid retains its native item art and cursor. Overlay
    // the canonical owner on each item so the menu no longer advertises X/Y/Z.
    constexpr u8 inventoryOrder[20]={0,1,2,3,4,5,6,8,10,11,12,13,14,15,16,17,20,21,22,23};
    for(int index=0;index<20;++index){
        int owner=svc_kcrux_controls->owner_of_inventory(inventoryOrder[index]);
        if(index==8&&owner<0)owner=svc_kcrux_controls->owner_of_inventory(9);
        if(owner>=0&&owner<5)badge(owner,s_menuCells[index].x+7,s_menuCells[index].y+7,12.f,255);
    }
    const int combo=svc_kcrux_controls->combination_state(s_collectionSelection);
    if(combo){
        shape(FaceA,bounds.f.x-226,footerY+12,15,255);text("A",bounds.f.x-226,footerY+12,9,255);
        text(combo==2?"Combo Off":"Combine",bounds.f.x-187,footerY+12,11,255);
    }
    port->setPort();
}
HookAction before_collection_cursor(ModContext*,void* args,void*,void*){
    if(!kcrux_controls_enabled()||dMeter2Info_getWindowStatus()!=3||kcrux_collection_page()!=1||s_menuCellCount!=20)return HOOK_CONTINUE;
    auto* cursor=mods::arg<dSelect_cursor_c*>(args,0);
    const float x=cursor->getPositionX(),y=cursor->getPositionY();
    constexpr u8 inventoryOrder[20]={0,1,2,3,4,5,6,8,10,11,12,13,14,15,16,17,20,21,22,23};
    for(unsigned i=0;i<20;++i){const auto& c=s_menuCells[i];
        if(x>=c.x-2&&x<=c.x+c.w+2&&y>=c.y-2&&y<=c.y+c.h+2){
            u8 inventory=inventoryOrder[i];
            if(i==8&&dComIfGs_getItem(inventory,false)==dItemNo_NONE_e)inventory=9;
            s_collectionSelection=inventory;svc_kcrux_controls->assign_collection_selection(inventory);break;
        }
    }
    return HOOK_CONTINUE;
}
HookAction begin_collection_frame(ModContext*,void*,void*,void*){
    s_menuCellCount=0;s_menuCellFrame=nullptr;s_bagDrawIndex=0;return HOOK_CONTINUE;
}
bool legacy_footer(float x,float y){
    if(!kcrux_controls_enabled()||s_renderingCustom||kcrux_collection_page()!=1)return false;
    auto* port=dComIfGp_getCurrentGrafPort();if(!port)return false;
    const auto b=port->getGrafType()==1?*static_cast<J2DOrthoGraph*>(port)->getOrtho():*port->getBounds();
    return y>b.f.y-42&&x>b.i.x+240&&x<b.f.x-90;
}
HookAction before_menu_glyph(ModContext*,void* args,void*,void*){
    if(!kcrux_controls_enabled()||s_renderingCustom||kcrux_collection_page()!=1)return HOOK_CONTINUE;
    auto* port=dComIfGp_getCurrentGrafPort();if(!port)return HOOK_CONTINUE;
    const auto b=port->getGrafType()==1?*static_cast<J2DOrthoGraph*>(port)->getOrtho():*port->getBounds();
    const float x=mods::arg<f32>(args,1),y=mods::arg<f32>(args,2);
    if(y>b.f.y-35&&x>b.i.x+240&&x<b.f.x-90)mods::arg_ref<f32>(args,2)=-10000.f;
    return HOOK_CONTINUE;
}
HookAction observe_menu_text(ModContext*,void* args,void*,void*){
    if(!kcrux_controls_enabled()||dMeter2Info_getWindowStatus()!=3)return HOOK_CONTINUE;
    const char* label=mods::arg<const char*>(args,5);if(!label)return HOOK_CONTINUE;
    auto* port=dComIfGp_getCurrentGrafPort();if(!port)return HOOK_CONTINUE;
    auto bounds=port->getGrafType()==1?*static_cast<J2DOrthoGraph*>(port)->getOrtho():*port->getBounds();
    if(mods::arg<f32>(args,1)>bounds.i.x+40||mods::arg<f32>(args,2)>65||mods::arg<f32>(args,4)<20)return HOOK_CONTINUE;
    int page=-1;
    if(!std::strcmp(label,"Items"))page=1;
    else if(!std::strcmp(label,"Equipment"))page=0;
    else if(!std::strcmp(label,"Collection"))page=2;
    else if(!std::strcmp(label,"Expansions"))page=3;
    else if(!std::strcmp(label,"Settings"))page=4;
    if(page>=0)svc_kcrux_controls->report_collection_page(page);
    return HOOK_CONTINUE;
}
HookAction before_map_draw(ModContext*,void* args,void*,void*){
    if(!kcrux_controls_enabled())return HOOK_CONTINUE;
    s_mapDrawing=mods::arg<dMeterMap_c*>(args,0);
    s_savedMapW=s_mapDrawing->mSizeW;s_savedMapH=s_mapDrawing->mSizeH;
    s_mapDrawing->mSizeW*=.74f;s_mapDrawing->mSizeH*=.74f;
    return HOOK_CONTINUE;
}
void after_map_draw(ModContext*,void*,void*,void*){
    if(!s_mapDrawing)return;
    s_mapDrawing->mSizeW=s_savedMapW;s_mapDrawing->mSizeH=s_savedMapH;
    s_mapDrawing=nullptr;
}
HookAction before_picture_self(ModContext*,void* args,void*,void*){
    if(!kcrux_controls_enabled()||s_renderingCustom)return HOOK_CONTINUE;
    auto* picture=mods::arg<J2DPicture*>(args,0);
    if(picture&&picture->mInfoTag==MULTI_CHAR('j_light1'))return HOOK_SKIP_ORIGINAL;
    const auto* texture=picture?picture->getTexture(0):nullptr;
    const auto* image=texture?texture->getTexInfo():nullptr;
    for(const auto* legacy:s_legacyZTextures)if(legacy&&image==legacy)return HOOK_SKIP_ORIGINAL;
    return HOOK_CONTINUE;
}
HookAction before_picture_draw(ModContext*,void* args,void*,void*){
    if(!kcrux_controls_enabled())return HOOK_CONTINUE;
    if(legacy_footer(mods::arg<f32>(args,1),mods::arg<f32>(args,2)))return HOOK_SKIP_ORIGINAL;
    auto* picture=mods::arg<J2DPicture*>(args,0);
    if(picture&&!s_renderingCustom){
        if(picture->mInfoTag==MULTI_CHAR('j_light1'))return HOOK_SKIP_ORIGINAL;
        const auto* texture=picture->getTexture(0);const auto* image=texture?texture->getTexInfo():nullptr;
        for(const auto* legacy:s_legacyZTextures)if(legacy&&image==legacy)return HOOK_SKIP_ORIGINAL;
        if(s_hud){
            for(int i=0;i<2;++i)for(int j=0;j<3;++j)
                if(picture==s_hud->mpItemNumTex[i][j])return HOOK_SKIP_ORIGINAL;
            auto* port=dComIfGp_getCurrentGrafPort();
            const int window=dMeter2Info_getWindowStatus();
            if(image&&port&&(window==0||window==2)){
                const auto bounds=port->getGrafType()==1?*static_cast<J2DOrthoGraph*>(port)->getOrtho():*port->getBounds();
                // Essentials' extra-channel counters are detached pictures. The
                // entire native item cluster at this anchor is replaced by ours.
                if(mods::arg<f32>(args,1)>bounds.f.x-150&&mods::arg<f32>(args,2)<bounds.i.y+145)
                    for(int digit=0;digit<10;++digit)if(image==s_hud->getNumberTexture(digit))return HOOK_SKIP_ORIGINAL;
            }
        }
    }
    if(kcrux_collection_page()==3&&picture){
        const auto* texture=picture->getTexture(0);const auto* image=texture?texture->getTexInfo():nullptr;
        if(image&&image->width==40&&image->height==54&&mods::arg<f32>(args,2)>190&&mods::arg<f32>(args,2)<280){
            const unsigned bag=s_bagDrawIndex++;
            if(bag<3&&g_dComIfG_gameInfo.info.getPlayer().getItem().mItems[15+bag]!=dItemNo_NONE_e){
                s_pictureColors.push_back({args,picture,picture->getBlack(),picture->getWhite(),
                    {picture->corner(0),picture->corner(1),picture->corner(2),picture->corner(3)}});
                picture->setBlackWhite(JUtility::TColor(0,0,0,0),JUtility::TColor(255,255,255,255));
                std::array<JUtility::TColor,4> colors;
                for(int i=0;i<4;++i){colors[i]=picture->corner(i);colors[i].r=colors[i].g=colors[i].b=255;}
                picture->setCornerColor(colors[0],colors[1],colors[2],colors[3]);
            }
        }
    }
    if(kcrux_collection_page()==1&&picture&&s_menuCellCount<20){
        const float x=mods::arg<f32>(args,1),y=mods::arg<f32>(args,2),w=mods::arg<f32>(args,3),h=mods::arg<f32>(args,4);
        const auto* texture=picture->getTexture(0);const auto* image=texture?texture->getTexInfo():nullptr;
        if(!s_menuCellFrame&&image&&image->width==64&&image->height==64&&w>40&&w<70&&std::abs(w-h)<.1f&&y>80&&y<140)
            s_menuCellFrame=picture;
        if(picture==s_menuCellFrame)s_menuCells[s_menuCellCount++]={x,y,w,h};
    }
    // Essentials explicitly draws these native portrait panes outside their
    // original parent. Visibility on that parent cannot suppress the duplicate.
    if(picture&&(picture->mInfoTag==MULTI_CHAR('midona')||picture->mInfoTag==MULTI_CHAR('midona_s')))
        return HOOK_SKIP_ORIGINAL;
    if(!s_mapDrawing||picture!=s_mapDrawing->mMapJ2DPicture)return HOOK_CONTINUE;
    auto* port=dComIfGp_getCurrentGrafPort();if(!port)return HOOK_CONTINUE;
    auto bounds=*port->getBounds();
    if(port->getGrafType()==1)bounds=*static_cast<J2DOrthoGraph*>(port)->getOrtho();
    mods::arg_ref<f32>(args,1)=bounds.i.x+26+2*s_mapDrawing->mSlidePositionOffset;
    mods::arg_ref<f32>(args,2)=bounds.f.y-154-mods::arg<f32>(args,4);
    kcrux_minimap_layout(mods::arg<f32>(args,2),s_mapDrawing->isDispPosInsideFlg()||
        s_mapDrawing->mSlidePositionOffset>s_mapDrawing->getDispPosOutSide_OffsetX()+1);
    return HOOK_CONTINUE;
}
void after_picture_draw(ModContext*,void* args,void*,void*){
    if(s_pictureColors.empty()||s_pictureColors.back().call!=args)return;
    const auto state=s_pictureColors.back();s_pictureColors.pop_back();
    state.picture->setBlackWhite(state.black,state.white);
    state.picture->setCornerColor(state.corners[0],state.corners[1],state.corners[2],state.corners[3]);
}
}
void kcrux_minimap_layout(float top,bool visible){s_mapTop=top;s_mapVisible=visible;}
ModResult initialize_kcrux_prompts(ModError* error){
    for(int i=0;i<TextureCount;++i)make_texture(i);
    ResourceBuffer buffer=RESOURCE_BUFFER_INIT;
    const auto loaded=svc_resource->load(mod_ctx,"kcrux/xbox-bumper.rgba8",&buffer);
    const bool valid=loaded==MOD_OK&&buffer.data&&buffer.size==s_bumperTexture.pixels.size();
    if(valid)std::memcpy(s_bumperTexture.pixels.data(),buffer.data,buffer.size);
    svc_resource->free(mod_ctx,&buffer);
    if(!valid)return mods::set_error(error,MOD_ERROR,"could not load existing Xbox bumper artwork");
    ResourceBuffer frame=RESOURCE_BUFFER_INIT;
    const auto frameLoad=svc_resource->load(mod_ctx,"kcrux/hud-ornament.bti",&frame);
    s_frameReady=frameLoad==MOD_OK&&frame.data&&frame.size==s_frameData.size();
    if(s_frameReady){
        std::memcpy(s_frameData.data(),frame.data,frame.size);
        const auto* image=reinterpret_cast<const ResTIMG*>(s_frameData.data());
        s_frameReady=image->format==GX_TF_RGB5A3&&image->width==256&&image->height==256&&image->imageOffset==32;
    }
    svc_resource->free(mod_ctx,&frame);
    if(!s_frameReady)svc_log->warn(mod_ctx,"Rewired HUD frame artwork unavailable; continuing without ornament");
    auto& header=s_bumperTexture.header;header={};header.format=GX_TF_RGBA8;header.alphaEnabled=1;
    header.width=1056;header.height=616;header.wrapS=header.wrapT=GX_CLAMP;
    header.minFilter=header.magFilter=GX_LINEAR;header.mipmapCount=1;
    header.imageOffset=offsetof(BumperTexture,pixels);
    HookOptions last=HOOK_OPTIONS_INIT;last.priority=-1000;
    auto result=mods::hook::add_pre<CustomHud>(svc_hook,before_hud,&last);
    if(result==MOD_OK)result=mods::hook::add_post<CustomHud>(svc_hook,after_hud,&last);
    if(result==MOD_OK)result=mods::hook::add_pre<CustomScreen>(svc_hook,before_screen,&last);
    if(result==MOD_OK)result=mods::hook::add_post<CustomScreen>(svc_hook,after_screen,&last);
    if(result==MOD_OK)result=mods::hook::add_pre<DeleteHud>(svc_hook,before_delete);
    if(result==MOD_OK)result=mods::hook::add_pre<LegacyRingSelection>(svc_hook,skip_legacy_selection);
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredRingNumber>(svc_hook,ring_number,&last);
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredRingMixMessage>(svc_hook,ring_mix_message,&last);
    if(result==MOD_OK)result=mods::hook::add_pre<CustomRing>(svc_hook,before_ring,&last);
    if(result==MOD_OK)result=mods::hook::add_post<CustomRing>(svc_hook,after_ring,&last);
    if(result==MOD_OK)result=mods::hook::add_pre<CustomButtonGlow>(svc_hook,before_button_glow,&last);
    if(result==MOD_OK)result=mods::hook::add_pre<ContextHelper>(svc_hook,before_context_helper,&last);
    if(result==MOD_OK)result=mods::hook::add_post<ContextHelper>(svc_hook,after_context_helper,&last);
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredMapDraw>(svc_hook,before_map_draw,&last);
    if(result==MOD_OK)result=mods::hook::add_post<RewiredMapDraw>(svc_hook,after_map_draw,&last);
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredPictureSelf>(svc_hook,before_picture_self,&last);
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredPictureExSelf>(svc_hook,before_picture_self,&last);
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredPictureDraw>(svc_hook,before_picture_draw,&last);
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredPictureExDraw>(svc_hook,before_picture_draw,&last);
    if(result==MOD_OK)result=mods::hook::add_post<RewiredPictureDraw>(svc_hook,after_picture_draw,&last);
    if(result==MOD_OK)result=mods::hook::add_post<RewiredPictureExDraw>(svc_hook,after_picture_draw,&last);
    HookOptions first=HOOK_OPTIONS_INIT;first.priority=2000;
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredMidnaMask>(svc_hook,mask_midna_before_essentials,&first);
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredCollectionFrame>(svc_hook,begin_collection_frame,&first);
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredMenuGlyph>(svc_hook,before_menu_glyph,&last);
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredMenuText>(svc_hook,observe_menu_text,&first);
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredCollectionCursor>(svc_hook,before_collection_cursor,&first);
    if(result!=MOD_OK)return mods::set_error(error,result,"could not install five-slot native HUD");
    svc_log->info(mod_ctx,"Rewired HUD revision 25: compact wheel counts and quiet left-Y combination prompt");return MOD_OK;
}
void shutdown_kcrux_prompts(){
    mods::hook::uninstall<RewiredRingNumber>(svc_hook);mods::hook::uninstall<RewiredRingMixMessage>(svc_hook);
    mods::hook::uninstall<RewiredPictureSelf>(svc_hook);mods::hook::uninstall<RewiredPictureExSelf>(svc_hook);
    mods::hook::uninstall<RewiredMapDraw>(svc_hook);mods::hook::uninstall<RewiredPictureDraw>(svc_hook);
    mods::hook::uninstall<RewiredPictureExDraw>(svc_hook);
    mods::hook::uninstall<RewiredCollectionFrame>(svc_hook);mods::hook::uninstall<RewiredMenuGlyph>(svc_hook);mods::hook::uninstall<RewiredMenuText>(svc_hook);
    mods::hook::uninstall<RewiredMidnaMask>(svc_hook);mods::hook::uninstall<RewiredCollectionCursor>(svc_hook);
    mods::hook::uninstall<CustomHud>(svc_hook);mods::hook::uninstall<CustomScreen>(svc_hook);
    mods::hook::uninstall<DeleteHud>(svc_hook);mods::hook::uninstall<CustomRing>(svc_hook);mods::hook::uninstall<LegacyRingSelection>(svc_hook);
    mods::hook::uninstall<CustomButtonGlow>(svc_hook);mods::hook::uninstall<ContextHelper>(svc_hook);
    restore_hidden();release();
}
}
