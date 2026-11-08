#include <mach/mach_types.h>
#include <mach/kmod.h>
#include <Headers/plugin_start.hpp>
#include <IOKit/IOLib.h>
#include "BootLog.hpp"
extern "C" kern_return_t _start(kmod_info_t *,void *);
extern "C" kern_return_t _stop(kmod_info_t *,void *);
extern "C" kern_return_t DisplayBrightness_kern_start(kmod_info_t *,void *);
extern "C" kern_return_t DisplayBrightness_kern_stop(kmod_info_t *,void *);
bool dbBootLogEnabled = false;

namespace {
kern_return_t loggedStart(kmod_info_t *info, void *data) {
    dbBootLogEnabled = checkKernelArgument("-dbbootlog");
    if (dbBootLogEnabled)
        IOLog("DisplayBrightness DBBOOT version=%s stage=entry darwin=%u.%u disabledArg=%d\n",
              xStringify(MODULE_VERSION), static_cast<unsigned>(getKernelVersion()),
              static_cast<unsigned>(getKernelMinorVersion()), checkKernelArgument("-dboff"));

    auto result = DisplayBrightness_kern_start(info, data);
    if (dbBootLogEnabled)
        IOLog("DisplayBrightness DBBOOT version=%s stage=entry-exit pluginStartInvoked=%d result=0x%x\n",
              xStringify(MODULE_VERSION), ADDPR(startSuccess), static_cast<unsigned>(result));
    return result;
}
}

extern "C" {
KMOD_EXPLICIT_DECL(org.displaybrightness.experimental, xStringify(MODULE_VERSION), _start, _stop)
__attribute__((visibility("default"))) kmod_start_func_t *_realmain=loggedStart;
__attribute__((visibility("default"))) kmod_stop_func_t *_antimain=DisplayBrightness_kern_stop;
__attribute__((visibility("default"))) int _kext_apple_cc=0;
}
