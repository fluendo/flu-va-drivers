#ifndef __FLU_VA_DRIVERS_VDPAU_X11_H__
#define __FLU_VA_DRIVERS_VDPAU_X11_H__

#include <va/va.h>
#include <vdpau/vdpau.h>
#include "flu_va_drivers_utils.h"
#include "flu_va_drivers_vdpau.h"
#include "flu_va_drivers_vdpau_utils.h"

VAStatus flu_va_drivers_vdpau_destroy_video_mixer (
    VADriverContextP ctx, FluVaDriversVdpauVideoMixerObject *video_mixer_obj);

VAStatus flu_va_drivers_vdpau_create_video_mixer (VADriverContextP ctx,
    VAContextID context, int width, int height, int va_rt_format,
    FluVaDriversID *video_mixer_id);

VAStatus flu_va_drivers_vdpau_context_ensure_video_mixer (VADriverContextP ctx,
    FluVaDriversVdpauContextObject *context_obj, int width, int height,
    int va_rt_format);

#endif // __FLU_VA_DRIVERS_VDPAU_X11_H__