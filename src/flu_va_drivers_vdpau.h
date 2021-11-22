#ifndef __FLU_VA_DRIVERS_VDPAU_DRV_VIDEO_H__
#define __FLU_VA_DRIVERS_VDPAU_DRV_VIDEO_H__

#include <va/va.h>
#include <va/va_backend.h>
#include <string.h>
#include <stdlib.h>

typedef struct _FluVaDriversVdpauDriverData FluVaDriversVdpauDriverData;

struct _FluVaDriversVdpauDriverData
{
  char va_vendor[256];
  char _reserved[16];
};

#endif /* __FLU_VA_DRIVERS_VDPAU_DRV_VIDEO_H__ */
