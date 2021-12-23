#ifndef __FLU_VA_DRIVERS_VDPAU_DRV_VIDEO_H__
#define __FLU_VA_DRIVERS_VDPAU_DRV_VIDEO_H__

#include <assert.h>
#include <stdint.h>
#include <va/va.h>
#include <va/va_backend.h>
#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include <string.h>
#include <stdlib.h>
#include "flu_va_drivers_vdpau_vdp_device_impl.h"
#include "../ext/intel/intel-vaapi-drivers/object_heap.h"

typedef struct _FluVaDriversVdpauDriverData FluVaDriversVdpauDriverData;

struct _FluVaDriversVdpauDriverData
{
  VADriverContextP ctx;
  char va_vendor[256];
  FluVaDriversVdpauVdpDeviceImpl vdp_impl;
  struct object_heap config_heap;
  struct object_heap context_heap;
  struct object_heap surface_heap;
  struct object_heap buffer_heap;
  struct object_heap image_heap;
  struct object_heap subpic_heap;

  char _reserved[16];
};

#define FLU_VA_DRIVERS_VDPAU_MAX_ATTRIBUTES 1

struct _FluVaDriversVdpauConfigObject
{
  struct object_base base;
  VAProfile profile;
  uint32_t max_width;
  uint32_t max_height;
  VAEntrypoint entrypoint;
  VAConfigAttrib attrib_list[FLU_VA_DRIVERS_VDPAU_MAX_ATTRIBUTES];
  unsigned int num_attribs;
};

typedef struct _FluVaDriversVdpauConfigObject FluVaDriversVdpauConfigObject;

#endif /* __FLU_VA_DRIVERS_VDPAU_DRV_VIDEO_H__ */
