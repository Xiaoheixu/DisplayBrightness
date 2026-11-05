#include <mach/mach_types.h>
#include <mach/kmod.h>
extern "C" kern_return_t _start(kmod_info_t *,void *);
extern "C" kern_return_t _stop(kmod_info_t *,void *);
extern "C" kern_return_t DisplayBrightness_kern_start(kmod_info_t *,void *);
extern "C" kern_return_t DisplayBrightness_kern_stop(kmod_info_t *,void *);
extern "C" {
KMOD_EXPLICIT_DECL(org.displaybrightness.experimental, "0.0.7", _start, _stop)
__attribute__((visibility("default"))) kmod_start_func_t *_realmain=DisplayBrightness_kern_start;
__attribute__((visibility("default"))) kmod_stop_func_t *_antimain=DisplayBrightness_kern_stop;
__attribute__((visibility("default"))) int _kext_apple_cc=0;
}
