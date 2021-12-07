#ifndef __FLU_VA_DRIVERS_VDPAU_UTILS_H__
#define __FLU_VA_DRIVERS_VDPAU_UTILS_H__

#include <vdpau/vdpau.h>
#include <va/va.h>

VAStatus flu_va_drivers_map_va_profile_to_vdpau_decoder_profile (
    VAProfile va_profile, VdpDecoderProfile *vdp_profile);

#endif /* __FLU_VA_DRIVERS_VDPAU_UTILS_H__ */
