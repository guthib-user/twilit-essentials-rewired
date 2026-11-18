#include <mods/service.hpp>
#include <mods/svc/camera.h>
#include <mods/svc/config.h>
#include <mods/svc/gfx.h>
#include <mods/svc/log.h>
#include <mods/svc/ui.h>
#include "shader.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <string>

DEFINE_MOD();
IMPORT_SERVICE(CameraService, svc_camera);
IMPORT_SERVICE(ConfigService, svc_config);
IMPORT_SERVICE(GfxService, svc_gfx);
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(UiService, svc_ui);

namespace {
ConfigVarHandle styleVar{}, strengthVar{}, widthVar{}, compareVar{}, meshVar{};
GfxStageHookHandle sceneHook{}, hudHook{};
GfxDrawTypeHandle drawType{};
WGPUShaderModule shader{};
WGPUBindGroupLayout bindings{};
WGPUPipelineLayout pipelineLayout{};
WGPURenderPipeline pipeline{};
uint64_t pipelineKey{};
bool cameraValid{};
float nearPlane=1, farPlane=100000;
std::string fxControlPath, lastControl;
struct Uniforms { float settings[4]; float camera[4]; };
struct Draw { WGPUTextureView color, depth; uint32_t offset; };
constexpr const char* Names[] = {
    "Current (original)", "DrawnShade - original FX (no bloom)", "Enhanced Colors", "Better Lighting", "ChromeFX"
};

WGPUStringView str(const char* s) { return {s, WGPU_STRLEN}; }

// Only raw WebGPU calls here: this callback runs on the renderer thread.
void draw(ModContext*, const GfxDrawContext* ctx, const void* payload, size_t, void*) {
    const auto& job=*static_cast<const Draw*>(payload);
    if (!pipeline || pipelineKey!=ctx->layout.key) {
        if (pipeline) wgpuRenderPipelineRelease(pipeline);
        WGPUColorTargetState colors[GFX_MAX_COLOR_ATTACHMENTS];
        auto count=gfx_init_color_target_states(&ctx->layout, colors, nullptr, WGPUColorWriteMask_All);
        WGPUFragmentState fragment=WGPU_FRAGMENT_STATE_INIT;
        fragment.module=shader; fragment.entryPoint=str("fragment_main");
        fragment.targetCount=count; fragment.targets=colors;
        WGPUDepthStencilState depth=WGPU_DEPTH_STENCIL_STATE_INIT;
        depth.format=ctx->layout.depth_stencil_format;
        depth.depthWriteEnabled=WGPUOptionalBool_False;
        depth.depthCompare=WGPUCompareFunction_Always;
        WGPURenderPipelineDescriptor desc=WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
        desc.label=str("Rewired sourced visual presets"); desc.layout=pipelineLayout;
        desc.vertex.module=shader; desc.vertex.entryPoint=str("vertex_main");
        desc.primitive.topology=WGPUPrimitiveTopology_TriangleList;
        desc.multisample.count=ctx->layout.sample_count;
        desc.depthStencil=depth.format==WGPUTextureFormat_Undefined?nullptr:&depth;
        desc.fragment=&fragment;
        pipeline=wgpuDeviceCreateRenderPipeline(ctx->device,&desc);
        pipelineKey=ctx->layout.key;
    }
    if (!pipeline) return;
    WGPUBindGroupEntry entries[3]{};
    entries[0].binding=0;entries[0].textureView=job.color;
    entries[1].binding=1;entries[1].textureView=job.depth;
    entries[2].binding=2;entries[2].buffer=ctx->uniform_buffer;
    entries[2].offset=job.offset;entries[2].size=sizeof(Uniforms);
    WGPUBindGroupDescriptor desc=WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    desc.layout=bindings;desc.entryCount=3;desc.entries=entries;
    auto group=wgpuDeviceCreateBindGroup(ctx->device,&desc);
    wgpuRenderPassEncoderSetPipeline(ctx->pass,pipeline);
    wgpuRenderPassEncoderSetBindGroup(ctx->pass,0,group,0,nullptr);
    wgpuRenderPassEncoderDraw(ctx->pass,3,1,0,0);
    wgpuBindGroupRelease(group);
}

void scene(ModContext*, const GfxStageContext* ctx, void*) {
    CameraInfo camera=CAMERA_INFO_INIT;
    cameraValid=svc_camera->get_camera(mod_ctx,ctx->game_view,&camera)==MOD_OK;
    if(cameraValid){nearPlane=camera.near_plane;farPlane=camera.far_plane;}
}

void before_hud(ModContext*, const GfxStageContext*, void*) {
    int64_t style=0,strength=100,width=1;bool compare=false,mesh=false;
    svc_config->get_int(mod_ctx,styleVar,&style);
    svc_config->get_int(mod_ctx,strengthVar,&strength);
    svc_config->get_int(mod_ctx,widthVar,&width);
    svc_config->get_bool(mod_ctx,compareVar,&compare);
    svc_config->get_bool(mod_ctx,meshVar,&mesh);
    // Current is a true bypass: no resolve, draw or graphics configuration writes.
    if(style<2||style>4||compare||strength<=0||!cameraValid)return;
    GfxDeviceInfo device=GFX_DEVICE_INFO_INIT;
    if(svc_gfx->get_device_info(mod_ctx,&device)!=MOD_OK)return;
    GfxResolveDesc request=GFX_RESOLVE_DESC_INIT;request.depth=false;
    GfxResolvedTargets targets=GFX_RESOLVED_TARGETS_INIT;
    if(svc_gfx->resolve_pass(mod_ctx,&request,&targets)!=MOD_OK||!targets.color)return;
    Uniforms values{{float(style),std::clamp(float(strength)*.01f,0.f,1.f),float(std::clamp<int64_t>(width,1,3)),mesh?1.f:0.f},
        {nearPlane,farPlane,device.uses_reversed_z?1.f:0.f,0}};
    GfxRange range{};
    if(svc_gfx->push_uniform(mod_ctx,&values,sizeof(values),&range)!=MOD_OK)return;
    // A harmless color view fills the unused non-cel depth binding.
    Draw job{targets.color,targets.depth?targets.depth:targets.color,range.offset};
    svc_gfx->push_draw(mod_ctx,drawType,&job,sizeof(job));
}

ModResult build_ui(ModContext* ctx, UiElementHandle pane, void*, ModError*) {
    auto result=svc_ui->pane_add_section(ctx,pane,"Visual style");
    if(result!=MOD_OK)return result;
    UiControlDesc select=UI_CONTROL_DESC_INIT;
    // SELECT requires a tabbed mod window's help pane. The Mods sidebar has
    // no help pane, so it must use the self-contained dropdown control.
    select.kind=UI_CONTROL_DROPDOWN;select.label="Preset";
    select.help_rml="Current bypasses the added processing. DrawnShade runs the original ReShade FX files and preset values, with AmbientLight, OrtonBloom and PD80 Bloom excluded. The other three choices are native color adaptations. Your game bloom and shadow settings stay unchanged.";
    select.binding=UI_BINDING_CONFIG_VAR;select.config_var=styleVar;
    select.options=Names;select.option_count=std::size(Names);
    result=svc_ui->pane_add_control(ctx,pane,&select,nullptr);
    if(result!=MOD_OK)return result;
    UiControlDesc strength=UI_CONTROL_DESC_INIT;
    strength.kind=UI_CONTROL_NUMBER;strength.label="Color preset strength";
    strength.help_rml="Applies to Enhanced Colors, Better Lighting and ChromeFX. DrawnShade uses the original preset strength.";
    strength.binding=UI_BINDING_CONFIG_VAR;strength.config_var=strengthVar;
    strength.min=0;strength.max=100;strength.step=5;strength.suffix="%";
    if((result=svc_ui->pane_add_control(ctx,pane,&strength,nullptr))!=MOD_OK)return result;
    UiControlDesc width=UI_CONTROL_DESC_INIT;
    width.kind=UI_CONTROL_NUMBER;width.label="Cel mesh line width";
    width.help_rml="Original Comic mesh iterations. The author uses 2; 1 can suit lower resolutions. This does not change the other edge detectors.";
    width.binding=UI_BINDING_CONFIG_VAR;width.config_var=widthVar;width.min=1;width.max=3;width.step=1;
    if((result=svc_ui->pane_add_control(ctx,pane,&width,nullptr))!=MOD_OK)return result;
    UiControlDesc mesh=UI_CONTROL_DESC_INIT;
    mesh.kind=UI_CONTROL_TOGGLE;mesh.label="Cel fine mesh detail";
    mesh.help_rml="Runs the original preset's fine mesh edge detector. Enabled by default, as in DrawnShade.";
    mesh.binding=UI_BINDING_CONFIG_VAR;mesh.config_var=meshVar;
    if((result=svc_ui->pane_add_control(ctx,pane,&mesh,nullptr))!=MOD_OK)return result;
    UiControlDesc compare=UI_CONTROL_DESC_INIT;
    compare.kind=UI_CONTROL_TOGGLE;compare.label="Compare original";
    compare.help_rml="Temporarily bypass the selected style without forgetting it.";
    compare.binding=UI_BINDING_CONFIG_VAR;compare.config_var=compareVar;
    if((result=svc_ui->pane_add_control(ctx,pane,&compare,nullptr))!=MOD_OK)return result;
    if(fxControlPath.empty()) svc_ui->pane_add_text(ctx,pane,"DrawnShade is unavailable: launch through the Rewired shortcut with the original FX runtime installed.",nullptr);
    return svc_ui->pane_add_text(ctx,pane,
        "DrawnShade by durchfallalarm: original Comic, Lumenite Kernel/RTAO, EyeAdaption, Depth Cues, ContrastStretch and Vignette. Color adaptations: SwordDeath, Magiike and SweetFX. Original FX also process HUD pixels. See Sources for versions and differences.",nullptr);
}
ModResult integer(const char* name,int value,ConfigVarHandle* handle){
    ConfigVarDesc desc=CONFIG_VAR_DESC_INIT;desc.name=name;desc.type=CONFIG_VAR_INT;desc.default_int=value;
    return svc_config->register_var(mod_ctx,&desc,handle);
}
}

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error) {
    if(const char* path=std::getenv("REWIRED_FX_CONTROL")) fxControlPath=path;
    if(integer("preset",0,&styleVar)!=MOD_OK||integer("strength",100,&strengthVar)!=MOD_OK||
       integer("original-mesh-width",2,&widthVar)!=MOD_OK)return mods::set_error(error,MOD_ERROR,"could not register visual presets");
    ConfigVarDesc compare=CONFIG_VAR_DESC_INIT;compare.name="compare-original";
    if(svc_config->register_var(mod_ctx,&compare,&compareVar)!=MOD_OK)return MOD_ERROR;
    ConfigVarDesc mesh=CONFIG_VAR_DESC_INIT;mesh.name="original-mesh-detail";mesh.default_bool=true;
    if(svc_config->register_var(mod_ctx,&mesh,&meshVar)!=MOD_OK)return MOD_ERROR;
    GfxDeviceInfo device=GFX_DEVICE_INFO_INIT;
    if(svc_gfx->get_device_info(mod_ctx,&device)!=MOD_OK)return mods::set_error(error,MOD_ERROR,"graphics device unavailable");
    WGPUShaderSourceWGSL source=WGPU_SHADER_SOURCE_WGSL_INIT;source.code=str(PresetShader);
    WGPUShaderModuleDescriptor shaderDesc=WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    shaderDesc.label=str("Rewired SweetFX color adaptations");shaderDesc.nextInChain=&source.chain;
    shader=wgpuDeviceCreateShaderModule(device.device,&shaderDesc);
    WGPUBindGroupLayoutEntry entries[3]{};
    for(int i=0;i<2;i++){
        entries[i].binding=i;entries[i].visibility=WGPUShaderStage_Fragment;
        entries[i].texture.sampleType=WGPUTextureSampleType_UnfilterableFloat;
        entries[i].texture.viewDimension=WGPUTextureViewDimension_2D;
    }
    entries[2].binding=2;entries[2].visibility=WGPUShaderStage_Fragment;
    entries[2].buffer.type=WGPUBufferBindingType_Uniform;entries[2].buffer.minBindingSize=sizeof(Uniforms);
    WGPUBindGroupLayoutDescriptor bindingDesc=WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
    bindingDesc.entryCount=3;bindingDesc.entries=entries;
    bindings=wgpuDeviceCreateBindGroupLayout(device.device,&bindingDesc);
    WGPUPipelineLayoutDescriptor layoutDesc=WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    layoutDesc.bindGroupLayoutCount=1;layoutDesc.bindGroupLayouts=&bindings;
    pipelineLayout=wgpuDeviceCreatePipelineLayout(device.device,&layoutDesc);
    GfxDrawTypeDesc drawDesc=GFX_DRAW_TYPE_DESC_INIT;drawDesc.label="Rewired visual styles";drawDesc.draw=draw;
    auto result=svc_gfx->register_draw_type(mod_ctx,&drawDesc,&drawType);
    GfxStageHookDesc hook=GFX_STAGE_HOOK_DESC_INIT;hook.callback=scene;
    if(result==MOD_OK)result=svc_gfx->register_stage_hook(mod_ctx,GFX_STAGE_SCENE_BEGIN,&hook,&sceneHook);
    hook.callback=before_hud;
    if(result==MOD_OK)result=svc_gfx->register_stage_hook(mod_ctx,GFX_STAGE_FRAME_BEFORE_HUD,&hook,&hudHook);
    UiModsPanelDesc panel=UI_MODS_PANEL_DESC_INIT;panel.build=build_ui;
    if(result==MOD_OK)result=svc_ui->register_mods_panel(mod_ctx,&panel);
    return result;
}
MOD_EXPORT ModResult mod_update(ModError*) {
    if(fxControlPath.empty()) return MOD_OK;
    int64_t style=0,width=2; bool compare=false,mesh=true;
    svc_config->get_int(mod_ctx,styleVar,&style);
    svc_config->get_int(mod_ctx,widthVar,&width);
    svc_config->get_bool(mod_ctx,compareVar,&compare);
    svc_config->get_bool(mod_ctx,meshVar,&mesh);
    const auto state="1 "+std::to_string(style==1&&!compare)+" "+
        std::to_string(std::clamp<int64_t>(width,1,3))+" "+std::to_string(mesh)+"\n";
    if(state!=lastControl) {
        const auto temporary=fxControlPath+".next";
        std::ofstream file(temporary); file<<state; file.close();
        if(file && std::rename(temporary.c_str(),fxControlPath.c_str())==0) lastControl=state;
    }
    return MOD_OK;
}
MOD_EXPORT ModResult mod_shutdown(ModError*) {
    if(!fxControlPath.empty()) { std::ofstream file(fxControlPath); file<<"1 0 2 1\n"; }
    if(hudHook)svc_gfx->unregister_stage_hook(mod_ctx,hudHook);
    if(sceneHook)svc_gfx->unregister_stage_hook(mod_ctx,sceneHook);
    if(drawType)svc_gfx->unregister_draw_type(mod_ctx,drawType);
    if(pipeline)wgpuRenderPipelineRelease(pipeline);
    if(pipelineLayout)wgpuPipelineLayoutRelease(pipelineLayout);
    if(bindings)wgpuBindGroupLayoutRelease(bindings);
    if(shader)wgpuShaderModuleRelease(shader);
    return MOD_OK;
}
}
