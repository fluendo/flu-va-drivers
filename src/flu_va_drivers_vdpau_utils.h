#ifndef __FLU_VA_DRIVERS_VDPAU_UTILS_H__
#define __FLU_VA_DRIVERS_VDPAU_UTILS_H__

#include <vdpau/vdpau.h>
#include <va/va.h>

typedef enum
{
  FLU_VA_DRIVERS_VDPAU_IMAGE_FORMAT_MAP_ITEM_TYPE_YCBCR,
  FLU_VA_DRIVERS_VDPAU_IMAGE_FORMAT_MAP_ITEM_TYPE_NONE
} FluVaDriversVdpauImageFormatMapItemType;

struct _FluVaDriversVdpauImageFormatMapItem
{
  FluVaDriversVdpauImageFormatMapItemType type;
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

#endif /* __FLU_VA_DRIVERS_VDPAU_UTILS_H__ */
