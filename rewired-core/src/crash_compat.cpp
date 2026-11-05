#include "service_imports.hpp"
#include "d/actor/d_a_alink.h"
#include "mods/svc/hook.hpp"

namespace {
DEFINE_HOOK(&daAlink_c::setItemMatrix, RewiredHatAnimationOwner);

HookAction before_item_matrix(ModContext*, void* raw, void*, void*) {
    auto* link=mods::arg<daAlink_c*>(raw,0);
    if(!link || !link->mpLinkHatModel)return HOOK_CONTINUE;
    // Model data can survive a scene transition, but the preceding Link's
    // cutscene calculator belongs to his actor heap. Native setItemMatrix
    // removes it only when the new actor also has a demo animation wrapper.
    // Clear the unowned attachment before recursiveCalc can dereference it.
    // Active demo animations are installed by the original function below.
    if(!link->mpDemoHDTmpBck || !link->mpDemoHDTmpBck->getBckAnm() ||
       link->mClothesChangeWaitTimer!=0) {
        auto* root=link->mpLinkHatModel->getModelData()->getJointNodePointer(0);
        if(root && root->getMtxCalc()) {
            root->setMtxCalc(nullptr);
            svc_log->info(mod_ctx,"Rewired: detached unowned hat cutscene animation");
        }
    }
    return HOOK_CONTINUE;
}
}

ModResult initialize_rewired_crash_compat(ModError* error) {
    HookOptions options=HOOK_OPTIONS_INIT;options.priority=-2000;
    auto result=mods::hook::add_pre<RewiredHatAnimationOwner>(svc_hook,before_item_matrix,&options);
    return result==MOD_OK?MOD_OK:mods::set_error(error,result,"could not install animation lifetime guard");
}
