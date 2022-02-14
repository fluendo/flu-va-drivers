#ifndef __FLU_VA_DRIVERS_VDPAU_UTILS_H__
#define __FLU_VA_DRIVERS_VDPAU_UTILS_H__

#include <vdpau/vdpau.h>
#include <va/va.h>
#include "flu_va_drivers_vdpau.h"

struct _FluVaDriversVdpauImageFormatMapItem
{
  FluVaDriversVdpauImageFormatType type;
  uint32_t vdp_image_format;
  VAImageFormat va_image_format;
};

typedef struct _FluVaDriversVdpauImageFormatMapItem
    FluVaDriversVdpauImageFormatMapItem;
typedef const FluVaDriversVdpauImageFormatMapItem
    FluVaDriversVdpauImageFormatMap[];

extern FluVaDriversVdpauImageFormatMap FLU_VA_DRIVERS_VDPAU_IMAGE_FORMAT_MAP;

VAStatus flu_va_drivers_map_va_profile_to_vdpau_decoder_profile (
    VAProfile va_profile, VdpDecoderProfile *vdp_profile);

VAStatus flu_va_drivers_map_va_rt_format_to_vdp_chroma_type (
    int va_rt_format, VdpChromaType *vdp_chroma_type);

VAStatus flu_va_drivers_map_va_flag_to_vdp_video_mixer_picture_structure (
    int va_flag, VdpVideoMixerPictureStructure *vdp_flag);

void flu_va_drivers_map_va_rectangle_to_vdp_rect (
    const VARectangle *va_rect, VdpRect *vdp_rect);

int flu_va_drivers_vdpau_is_profile_supported (VAProfile va_profile);

int flu_va_drivers_vdpau_is_entrypoint_supported (VAEntrypoint va_entrypoint);

int flu_va_drivers_vdpau_is_config_attrib_type_supported (
    VAConfigAttribType va_attrib_type);

VAConfigAttrib *flu_va_drivers_vdpau_lookup_config_attrib_type (
    VAConfigAttrib *attrib_list, int num_attribs,
    VAConfigAttribType attrib_type);

VAStatus flu_va_driver_vdpau_translate_buffer_h264 (VADriverContextP ctx,
    FluVaDriversVdpauContextObject *context_obj,
    FluVaDriversVdpauBufferObject *buffer_obj);

void flu_va_drivers_vdpau_context_object_reset (
    FluVaDriversVdpauContextObject *context_obj);

int flu_va_driver_vdpau_is_buffer_type_supported (VABufferType buffer_type);

#endif /* __FLU_VA_DRIVERS_VDPAU_UTILS_H__ */
