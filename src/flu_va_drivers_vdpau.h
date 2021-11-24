#ifndef __FLU_VA_DRIVERS_VDPAU_DRV_VIDEO_H__
#define __FLU_VA_DRIVERS_VDPAU_DRV_VIDEO_H__

#include <va/va.h>
#include <va/va_backend.h>
#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include <string.h>
#include <stdlib.h>
#include "flu_va_drivers_vdpau_vdp_device_impl.h"

typedef struct _FluVaDriversVdpauDriverData FluVaDriversVdpauDriverData;

struct _FluVaDriversVdpauDriverData
{
  VADriverContextP ctx;
  char va_vendor[256];
  FluVaDriversVdpauVdpDeviceImpl vdp_impl;

  char _reserved[16];
};

#endif /* __FLU_VA_DRIVERS_VDPAU_DRV_VIDEO_H__ */
