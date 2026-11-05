#include "config.hpp"
#include "service_imports.hpp"
#include <mods/service.hpp>

namespace dawnlight {
namespace {
ConfigVarHandle s_frameX=0,s_frameY=0,s_frameScale=0;
ModResult add_int(const char* name,int value,ConfigVarHandle* handle) {
    ConfigVarDesc desc=CONFIG_VAR_DESC_INIT;
    desc.name=name;desc.type=CONFIG_VAR_INT;desc.default_int=value;
    return svc_config->register_var(mod_ctx,&desc,handle);
}
int get_int(ConfigVarHandle handle,int fallback) {
    int64_t value=fallback;
    if(handle)svc_config->get_int(mod_ctx,handle,&value);
    return static_cast<int>(value);
}
ModResult build(ModContext* ctx,UiElementHandle pane,void*,ModError*) {
    auto result=svc_ui->pane_add_section(ctx,pane,"Button frame");
    if(result!=MOD_OK)return result;
    struct Entry {const char* label;ConfigVarHandle var;int min,max;};
    const Entry entries[]={{"Horizontal position",s_frameX,-100,100},
        {"Vertical position",s_frameY,-100,100},{"Size (%)",s_frameScale,50,200}};
    for(const auto& entry:entries){
        UiControlDesc control=UI_CONTROL_DESC_INIT;
        control.kind=UI_CONTROL_NUMBER;control.label=entry.label;
        control.binding=UI_BINDING_CONFIG_VAR;control.config_var=entry.var;
        control.min=entry.min;control.max=entry.max;
        result=svc_ui->pane_add_control(ctx,pane,&control,nullptr);
        if(result!=MOD_OK)return result;
    }
    return MOD_OK;
}
}
ModResult register_hud_config(ModError* error) {
    if(add_int("frame-offset-x",0,&s_frameX)!=MOD_OK||
       add_int("frame-offset-y",2,&s_frameY)!=MOD_OK||
       add_int("frame-scale-percent",104,&s_frameScale)!=MOD_OK)
        return mods::set_error(error,MOD_ERROR,"could not register Rewired HUD settings");
    UiModsPanelDesc panel=UI_MODS_PANEL_DESC_INIT;panel.build=build;
    const auto result=svc_ui->register_mods_panel(mod_ctx,&panel);
    return result==MOD_OK?MOD_OK:mods::set_error(error,result,"could not register Rewired HUD panel");
}
int kcrux_frame_offset_x(){return get_int(s_frameX,0);}
int kcrux_frame_offset_y(){return get_int(s_frameY,2);}
int kcrux_frame_scale_percent(){return get_int(s_frameScale,104);}
bool r_jump_enabled(){return true;}
}
