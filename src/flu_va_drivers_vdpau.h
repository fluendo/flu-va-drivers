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
#include <sys/queue.h>
#include "flu_va_drivers_vdpau_vdp_device_impl.h"
#include "../ext/intel/intel-vaapi-drivers/object_heap.h"
#include "object_heap/object_heap_utils.h"

// clang-format off
#define FLU_VA_DRIVERS_VDPAU_MAX_PROFILES              3
#define FLU_VA_DRIVERS_VDPAU_MAX_ENTRYPOINTS           1
#define FLU_VA_DRIVERS_VDPAU_MAX_ATTRIBUTES            1
#define FLU_VA_DRIVERS_VDPAU_MAX_SURFACE_ATTRIBUTES    32
// This has been forced to 1 to make va_openDriver to pass.
#define FLU_VA_DRIVERS_VDPAU_MAX_IMAGE_FORMATS         1
// This has been forced to 1 to make va_openDriver to pass.
#define FLU_VA_DRIVERS_VDPAU_MAX_SUBPIC_FORMATS        1
#define FLU_VA_DRIVERS_VDPAU_MAX_DISPLAY_ATTRIBUTES    0
#define FLU_VA_DRIVERS_VDPAU_NUM_OUTPUT_SURFACES       3
// clang-format on

/* HACK: Use alignment values for CFL. */
#define FLU_VA_DRIVERS_DEFAULT_SURFACE_WIDTH_ALIGNMENT 64
#define FLU_VA_DRIVERS_DEFAULT_SURFACE_HEIGHT_ALIGNMENT 16

static const uint8_t NALU_START_CODE[3] = { 0x00, 0x0, 0x01 };
static const uint8_t NALU_START_CODE_4[4] = { 0x00, 0x00, 0x0, 0x01 };

typedef enum
{
  FLU_VA_DRIVERS_VDPAU_IMAGE_FORMAT_TYPE_YCBCR,
  FLU_VA_DRIVERS_VDPAU_IMAGE_FORMAT_TYPE_NONE
} FluVaDriversVdpauImageFormatType;

typedef struct _FluVaDriversVdpauDriverData FluVaDriversVdpauDriverData;

struct _FluVaDriversVdpauDriverData
{
  VADriverContextP ctx;
  char va_vendor[256];
  FluVaDriversVdpauVdpDeviceImpl vdp_impl;
  Display *x11_dpy;
  struct object_heap config_heap;
  struct object_heap context_heap;
  struct object_heap surface_heap;
  struct object_heap buffer_heap;
  struct object_heap image_heap;
  struct object_heap subpic_heap;
  struct object_heap video_mixer_heap;

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

struct _FluVaDriversVdpauSurfaceObject
{
  struct object_base base;
  VAContextID context_id;
  unsigned int format;
  unsigned int width;
  unsigned int height;
  VdpVideoSurface vdp_surface;
};
typedef struct _FluVaDriversVdpauSurfaceObject FluVaDriversVdpauSurfaceObject;

typedef struct _FluVaDriversVdpauPresentationQueueMapEntry
    FluVaDriversVdpauPresentationQueueMapEntry;
SLIST_HEAD (_FluVaDriversVdpauPresentationQueueMap,
    _FluVaDriversVdpauPresentationQueueMapEntry);
typedef struct _FluVaDriversVdpauPresentationQueueMap
    FluVaDriversVdpauPresentationQueueMap;

struct _FluVaDriversVdpauContextObject
{
  struct object_base base;
  VAConfigID config_id;
  int video_mixer_id;
  VdpDecoder vdp_decoder;
  VdpOutputSurface
      vdp_output_surfaces[FLU_VA_DRIVERS_VDPAU_NUM_OUTPUT_SURFACES];
  unsigned int vdp_output_surface_idx;
  FluVaDriversVdpauPresentationQueueMap vdp_presentation_queue_map;
  VdpPresentationQueue vdp_presentation_queue;
  VdpPresentationQueueTarget vdp_presentation_queue_target;
  unsigned int flag;
  int picture_width;
  int picture_height;
  VASurfaceID current_render_target;
  VdpPictureInfoH264 vdp_pic_info;
  VASurfaceID *render_targets;
  unsigned int num_render_targets;
  /* Only one needed, because we support only 1 element per buffer */
  VABufferType last_buffer_type;
  VASliceParameterBufferH264 *last_slice_param;
  VdpBitstreamBuffer *vdp_bs_buf;
  unsigned int num_vdp_bs_buf;
  unsigned int cap_vdp_bs_buf;
};
typedef struct _FluVaDriversVdpauContextObject FluVaDriversVdpauContextObject;

struct _FluVaDriversVdpauBufferObject
{
  struct object_base base;
  VABufferType type;
  void *data;
  size_t size;
  unsigned int num_elements;
};
typedef struct _FluVaDriversVdpauBufferObject FluVaDriversVdpauBufferObject;

struct _FluVaDriversVdpauImageObject
{
  struct object_base base;
  VAImage va_image;
  FluVaDriversVdpauImageFormatType format_type;
  uint32_t vdp_format;
};
typedef struct _FluVaDriversVdpauImageObject FluVaDriversVdpauImageObject;

struct _FluVaDriversVdpauVideoMixerObject
{
  struct object_base base;
  VAContextID context_id;
  VdpVideoMixer vdp_video_mixer;
  VdpChromaType vdp_chroma_type;
  unsigned int width;
  unsigned int height;
};
typedef struct _FluVaDriversVdpauVideoMixerObject
    FluVaDriversVdpauVideoMixerObject;

#endif /* __FLU_VA_DRIVERS_VDPAU_DRV_VIDEO_H__ */
