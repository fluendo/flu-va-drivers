#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "flu_va_drivers_vdpau.h"
#include "flu_va_drivers_utils.h"
#include "flu_va_drivers_vdpau_utils.h"

// clang-format off
#define _DEFAULT_OFFSET     24
#define CONFIG_ID_OFFSET    1 << _DEFAULT_OFFSET
#define CONTEXT_ID_OFFSET   2 << _DEFAULT_OFFSET
#define SURFACE_ID_OFFSET   3 << _DEFAULT_OFFSET
#define BUFFER_ID_OFFSET    4 << _DEFAULT_OFFSET
#define IMAGE_ID_OFFSET     5 << _DEFAULT_OFFSET
#define SUBPIC_ID_OFFSET    6 << _DEFAULT_OFFSET

#define FLU_VA_DRIVERS_VDPAU_MAX_PROFILES              3
#define FLU_VA_DRIVERS_VDPAU_MAX_ENTRYPOINTS           1
#define FLU_VA_DRIVERS_VDPAU_MAX_ATTRIBUTES            1
// This has been forced to 1 to make va_openDriver to pass.
#define FLU_VA_DRIVERS_VDPAU_MAX_IMAGE_FORMATS         1
// This has been forced to 1 to make va_openDriver to pass.
#define FLU_VA_DRIVERS_VDPAU_MAX_SUBPIC_FORMATS        1
#define FLU_VA_DRIVERS_VDPAU_MAX_DISPLAY_ATTRIBUTES    0
// clang-format on

static VAStatus
flu_va_drivers_vdpau_Terminate (VADriverContextP ctx)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;

  object_heap_destroy (&driver_data->config_heap);
  object_heap_destroy (&driver_data->context_heap);
  object_heap_destroy (&driver_data->surface_heap);
  object_heap_destroy (&driver_data->buffer_heap);
  object_heap_destroy (&driver_data->image_heap);
  object_heap_destroy (&driver_data->subpic_heap);

  free (driver_data);

  return VA_STATUS_SUCCESS;
}

static VAStatus
flu_va_drivers_vdpau_QueryConfigProfiles (
    VADriverContextP ctx, VAProfile *profile_list, int *num_profiles)
{
  VAProfile suported_profiles[FLU_VA_DRIVERS_VDPAU_MAX_PROFILES] = {
    VAProfileH264ConstrainedBaseline, VAProfileH264Main, VAProfileH264High
  };
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  int i;

  for (i = 0, *num_profiles = 0; i < FLU_VA_DRIVERS_VDPAU_MAX_PROFILES; i++) {
    VdpDecoderProfile vdp_profile;
    VdpBool is_supported;
    uint32_t unused_max_macroblocks, unused_max_level, unused_max_width,
        unused_max_height;
    VdpStatus vdp_st;
    VAStatus va_st;

    va_st = flu_va_drivers_map_va_profile_to_vdpau_decoder_profile (
        suported_profiles[i], &vdp_profile);
    if (va_st == VA_STATUS_ERROR_UNSUPPORTED_PROFILE)
      continue;

    vdp_st = driver_data->vdp_impl.vdp_decoder_query_capabilities (
        driver_data->vdp_impl.vdp_device, vdp_profile, &is_supported,
        &unused_max_level, &unused_max_macroblocks, &unused_max_width,
        &unused_max_height);

    if (vdp_st != VDP_STATUS_OK)
      goto vdp_query_failed;
    if (!is_supported)
      continue;

    profile_list[(*num_profiles)++] = suported_profiles[i];
  }

  return VA_STATUS_SUCCESS;

vdp_query_failed:
  return VA_STATUS_ERROR_UNKNOWN;
}

static VAStatus
flu_va_drivers_vdpau_QueryConfigEntrypoints (VADriverContextP ctx,
    VAProfile profile, VAEntrypoint *entrypoint_list, int *num_entrypoints)
{
  *num_entrypoints = 0;

  switch (profile) {
    case VAProfileH264ConstrainedBaseline:
    case VAProfileH264Main:
    case VAProfileH264High:
      entrypoint_list[(*num_entrypoints)++] = VAEntrypointVLD;
      break;
    default:
      break;
  }

  return VA_STATUS_SUCCESS;
}

static VAStatus
flu_va_drivers_vdpau_GetConfigAttributes (VADriverContextP ctx,
    VAProfile profile, VAEntrypoint entrypoint, VAConfigAttrib *attrib_list,
    int num_attribs)
{
  int i;

  // For vdpau >= 1.2, we should check if the chroma type is supported by
  // calling VdpDecoderQueryProfileCapability.
  for (i = 0; i < num_attribs; i++) {
    switch (attrib_list[i].type) {
      case VAConfigAttribRTFormat:
        attrib_list[i].value = VA_RT_FORMAT_YUV420;
        break;
      default:
        attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
        break;
    }
  }

  return VA_STATUS_SUCCESS;
}

static VAStatus
flu_va_drivers_vdpau_CreateConfig (VADriverContextP ctx, VAProfile profile,
    VAEntrypoint entrypoint, VAConfigAttrib *attrib_list, int num_attribs,
    VAConfigID *config_id)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_DestroyConfig (VADriverContextP ctx, VAConfigID config_id)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_QueryConfigAttributes (VADriverContextP ctx,
    VAConfigID config_id, VAProfile *profile, VAEntrypoint *entrypoint,
    VAConfigAttrib *attrib_list, int *num_attribs)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_CreateSurfaces (VADriverContextP ctx, int width,
    int height, int format, int num_surfaces, VASurfaceID *surfaces)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_DestroySurfaces (
    VADriverContextP ctx, VASurfaceID *surface_list, int num_surfaces)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_CreateContext (VADriverContextP ctx, VAConfigID config_id,
    int picture_width, int picture_height, int flag,
    VASurfaceID *render_targets, int num_render_targets, VAContextID *context)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_DestroyContext (VADriverContextP ctx, VAContextID context)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_CreateBuffer (VADriverContextP ctx, VAContextID context,
    VABufferType type, unsigned int size, unsigned int num_elements,
    void *data, VABufferID *buf_id)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_BufferSetNumElements (
    VADriverContextP ctx, VABufferID buf_id, unsigned int num_elements)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_MapBuffer (
    VADriverContextP ctx, VABufferID buf_id, void **pbuf)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_UnmapBuffer (VADriverContextP ctx, VABufferID buf_id)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_DestroyBuffer (VADriverContextP ctx, VABufferID buffer_id)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_BeginPicture (
    VADriverContextP ctx, VAContextID context, VASurfaceID render_target)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_RenderPicture (VADriverContextP ctx, VAContextID context,
    VABufferID *buffers, int num_buffers)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_EndPicture (VADriverContextP ctx, VAContextID context)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_SyncSurface (
    VADriverContextP ctx, VASurfaceID render_target)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_QuerySurfaceStatus (
    VADriverContextP ctx, VASurfaceID render_target, VASurfaceStatus *status)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_QuerySurfaceError (VADriverContextP ctx,
    VASurfaceID render_target, VAStatus error_status, void **error_info)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_PutSurface (VADriverContextP ctx, VASurfaceID surface,
    void *draw, short srcx, short srcy, unsigned short srcw,
    unsigned short srch, short destx, short desty, unsigned short destw,
    unsigned short desth, VARectangle *cliprects,
    unsigned int number_cliprects, unsigned int flags)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_QueryImageFormats (
    VADriverContextP ctx, VAImageFormat *format_list, int *num_formats)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_CreateImage (VADriverContextP ctx, VAImageFormat *format,
    int width, int height, VAImage *image)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_DeriveImage (
    VADriverContextP ctx, VASurfaceID surface, VAImage *image)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_DestroyImage (VADriverContextP ctx, VAImageID image)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_SetImagePalette (VADriverContextP ctx, VAImageID image,

    unsigned char *palette)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_GetImage (VADriverContextP ctx, VASurfaceID surface,
    int x, int y, unsigned int width, unsigned int height, VAImageID image)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_PutImage (VADriverContextP ctx, VASurfaceID surface,
    VAImageID image, int src_x, int src_y, unsigned int src_width,
    unsigned int src_height, int dest_x, int dest_y, unsigned int dest_width,
    unsigned int dest_height)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_QuerySubpictureFormats (VADriverContextP ctx,
    VAImageFormat *format_list, unsigned int *flags, unsigned int *num_formats)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_CreateSubpicture (
    VADriverContextP ctx, VAImageID image, VASubpictureID *subpicture)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_DestroySubpicture (
    VADriverContextP ctx, VASubpictureID subpicture)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_SetSubpictureImage (
    VADriverContextP ctx, VASubpictureID subpicture, VAImageID image)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_SetSubpictureChromakey (VADriverContextP ctx,
    VASubpictureID subpicture, unsigned int chromakey_min,
    unsigned int chromakey_max, unsigned int chromakey_mask)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_SetSubpictureGlobalAlpha (
    VADriverContextP ctx, VASubpictureID subpicture, float global_alpha)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_AssociateSubpicture (VADriverContextP ctx,
    VASubpictureID subpicture, VASurfaceID *target_surfaces, int num_surfaces,
    short src_x, short src_y, unsigned short src_width,
    unsigned short src_height, short dest_x, short dest_y,
    unsigned short dest_width, unsigned short dest_height, unsigned int flags)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_DeassociateSubpicture (VADriverContextP ctx,
    VASubpictureID subpicture, VASurfaceID *target_surfaces, int num_surfaces)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_QueryDisplayAttributes (
    VADriverContextP ctx, VADisplayAttribute *attr_list, int *num_attributes)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_GetDisplayAttributes (
    VADriverContextP ctx, VADisplayAttribute *attr_list, int num_attributes)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_SetDisplayAttributes (
    VADriverContextP ctx, VADisplayAttribute *attr_list, int num_attributes)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_BufferInfo (VADriverContextP ctx, VABufferID buf_id,
    VABufferType *type, unsigned int *size, unsigned int *num_elements)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_LockSurface (VADriverContextP ctx, VASurfaceID surface,
    unsigned int *fourcc, unsigned int *luma_stride,
    unsigned int *chroma_u_stride, unsigned int *chroma_v_stride,
    unsigned int *luma_offset, unsigned int *chroma_u_offset,
    unsigned int *chroma_v_offset, unsigned int *buffer_name, void **buffer)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_UnlockSurface (VADriverContextP ctx, VASurfaceID surface)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_GetSurfaceAttributes (VADriverContextP dpy,
    VAConfigID config, VASurfaceAttrib *attrib_list, unsigned int num_attribs)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_CreateSurfaces2 (VADriverContextP ctx,
    unsigned int format, unsigned int width, unsigned int height,
    VASurfaceID *surfaces, unsigned int num_surfaces,
    VASurfaceAttrib *attrib_list, unsigned int num_attribs)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_QuerySurfaceAttributes (VADriverContextP dpy,
    VAConfigID config, VASurfaceAttrib *attrib_list, unsigned int *num_attribs)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_AcquireBufferHandle (
    VADriverContextP ctx, VABufferID buf_id, VABufferInfo *buf_info)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_ReleaseBufferHandle (
    VADriverContextP ctx, VABufferID buf_id)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_CreateMFContext (
    VADriverContextP ctx, VAMFContextID *mfe_context)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_MFAddContext (
    VADriverContextP ctx, VAMFContextID mf_context, VAContextID context)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_MFReleaseContext (
    VADriverContextP ctx, VAMFContextID mf_context, VAContextID context)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_MFSubmit (VADriverContextP ctx, VAMFContextID mf_context,
    VAContextID *contexts, int num_contexts)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}
static VAStatus
flu_va_drivers_vdpau_CreateBuffer2 (VADriverContextP ctx, VAContextID context,
    VABufferType type, unsigned int width, unsigned int height,
    unsigned int *unit_size, unsigned int *pitch, VABufferID *buf_id)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_QueryProcessingRate (VADriverContextP ctx,
    VAConfigID config_id, VAProcessingRateParameter *proc_buf,
    unsigned int *processing_rate)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_ExportSurfaceHandle (VADriverContextP ctx,
    VASurfaceID surface_id, uint32_t mem_type, uint32_t flags,
    void *descriptor)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}
static VAStatus
flu_va_drivers_vdpau_SyncSurface2 (
    VADriverContextP ctx, VASurfaceID surface, uint64_t timeout_ns)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_SyncBuffer (
    VADriverContextP ctx, VABufferID buf_id, uint64_t timeout_ns)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_Copy (VADriverContextP ctx, VACopyObject *dst,
    VACopyObject *src, VACopyOption option)
{
  return VA_STATUS_ERROR_UNIMPLEMENTED;
}

static VAStatus
flu_va_drivers_vdpau_data_init (FluVaDriversVdpauDriverData *driver_data)
{
  VADriverContextP ctx = driver_data->ctx;
  VdpGetProcAddress *get_proc_address;
  VdpDevice device;
  int heap_sz = sizeof (struct object_heap);

  flu_va_drivers_get_vendor (driver_data->va_vendor);

  device = VDP_INVALID_HANDLE;
  if (vdp_device_create_x11 (ctx->native_dpy, ctx->x11_screen, &device,
          &get_proc_address) != VDP_STATUS_OK)
    return VA_STATUS_ERROR_UNKNOWN;

  if (flu_va_drivers_vdpau_vdp_device_impl_init (
          &driver_data->vdp_impl, device, get_proc_address) != VDP_STATUS_OK)
    return VA_STATUS_ERROR_UNKNOWN;

  object_heap_init (&driver_data->config_heap, heap_sz, CONFIG_ID_OFFSET);
  object_heap_init (&driver_data->context_heap, heap_sz, CONTEXT_ID_OFFSET);
  object_heap_init (&driver_data->surface_heap, heap_sz, SURFACE_ID_OFFSET);
  object_heap_init (&driver_data->buffer_heap, heap_sz, BUFFER_ID_OFFSET);
  object_heap_init (&driver_data->image_heap, heap_sz, IMAGE_ID_OFFSET);
  object_heap_init (&driver_data->subpic_heap, heap_sz, SUBPIC_ID_OFFSET);

  return VA_STATUS_SUCCESS;
}

VAStatus
FLU_VA_DRIVERS_DRIVER_INIT (VADriverContextP ctx)
{
  VAStatus res;
  FluVaDriversVdpauDriverData *driver_data;

  driver_data = calloc (1, sizeof (FluVaDriversVdpauDriverData));
  driver_data->ctx = ctx;
  ctx->pDriverData = driver_data;

  res = flu_va_drivers_vdpau_data_init (driver_data);
  if (res != VA_STATUS_SUCCESS) {
    flu_va_drivers_vdpau_Terminate (ctx);
    return res;
  };

  ctx->version_major = VA_MAJOR_VERSION;
  ctx->version_minor = VA_MINOR_VERSION;
  ctx->max_profiles = FLU_VA_DRIVERS_VDPAU_MAX_PROFILES;
  ctx->max_entrypoints = FLU_VA_DRIVERS_VDPAU_MAX_ENTRYPOINTS;
  ctx->max_attributes = FLU_VA_DRIVERS_VDPAU_MAX_ATTRIBUTES;
  ctx->max_image_formats = FLU_VA_DRIVERS_VDPAU_MAX_IMAGE_FORMATS;
  ctx->max_subpic_formats = FLU_VA_DRIVERS_VDPAU_MAX_SUBPIC_FORMATS;
  ctx->max_display_attributes = FLU_VA_DRIVERS_VDPAU_MAX_DISPLAY_ATTRIBUTES;
  ctx->str_vendor = driver_data->va_vendor;

  memset (ctx->vtable, 0, sizeof (*ctx->vtable));

  ctx->vtable->vaTerminate = flu_va_drivers_vdpau_Terminate;
  ctx->vtable->vaQueryConfigProfiles =
      flu_va_drivers_vdpau_QueryConfigProfiles;
  ctx->vtable->vaQueryConfigEntrypoints =
      flu_va_drivers_vdpau_QueryConfigEntrypoints;
  ctx->vtable->vaGetConfigAttributes =
      flu_va_drivers_vdpau_GetConfigAttributes;
  ctx->vtable->vaCreateConfig = flu_va_drivers_vdpau_CreateConfig;
  ctx->vtable->vaDestroyConfig = flu_va_drivers_vdpau_DestroyConfig;
  ctx->vtable->vaQueryConfigAttributes =
      flu_va_drivers_vdpau_QueryConfigAttributes;
  ctx->vtable->vaCreateSurfaces = flu_va_drivers_vdpau_CreateSurfaces;
  ctx->vtable->vaDestroySurfaces = flu_va_drivers_vdpau_DestroySurfaces;
  ctx->vtable->vaCreateContext = flu_va_drivers_vdpau_CreateContext;
  ctx->vtable->vaDestroyContext = flu_va_drivers_vdpau_DestroyContext;
  ctx->vtable->vaCreateBuffer = flu_va_drivers_vdpau_CreateBuffer;
  ctx->vtable->vaBufferSetNumElements =
      flu_va_drivers_vdpau_BufferSetNumElements;
  ctx->vtable->vaMapBuffer = flu_va_drivers_vdpau_MapBuffer;
  ctx->vtable->vaUnmapBuffer = flu_va_drivers_vdpau_UnmapBuffer;
  ctx->vtable->vaDestroyBuffer = flu_va_drivers_vdpau_DestroyBuffer;
  ctx->vtable->vaBeginPicture = flu_va_drivers_vdpau_BeginPicture;
  ctx->vtable->vaRenderPicture = flu_va_drivers_vdpau_RenderPicture;
  ctx->vtable->vaEndPicture = flu_va_drivers_vdpau_EndPicture;
  ctx->vtable->vaSyncSurface = flu_va_drivers_vdpau_SyncSurface;
  ctx->vtable->vaQuerySurfaceStatus = flu_va_drivers_vdpau_QuerySurfaceStatus;
  ctx->vtable->vaQuerySurfaceError = flu_va_drivers_vdpau_QuerySurfaceError;
  ctx->vtable->vaPutSurface = flu_va_drivers_vdpau_PutSurface;
  ctx->vtable->vaQueryImageFormats = flu_va_drivers_vdpau_QueryImageFormats;
  ctx->vtable->vaCreateImage = flu_va_drivers_vdpau_CreateImage;
  ctx->vtable->vaDeriveImage = flu_va_drivers_vdpau_DeriveImage;
  ctx->vtable->vaDestroyImage = flu_va_drivers_vdpau_DestroyImage;
  ctx->vtable->vaSetImagePalette = flu_va_drivers_vdpau_SetImagePalette;
  ctx->vtable->vaGetImage = flu_va_drivers_vdpau_GetImage;
  ctx->vtable->vaPutImage = flu_va_drivers_vdpau_PutImage;
  ctx->vtable->vaQuerySubpictureFormats =
      flu_va_drivers_vdpau_QuerySubpictureFormats;
  ctx->vtable->vaCreateSubpicture = flu_va_drivers_vdpau_CreateSubpicture;
  ctx->vtable->vaDestroySubpicture = flu_va_drivers_vdpau_DestroySubpicture;
  ctx->vtable->vaSetSubpictureImage = flu_va_drivers_vdpau_SetSubpictureImage;
  ctx->vtable->vaSetSubpictureChromakey =
      flu_va_drivers_vdpau_SetSubpictureChromakey;
  ctx->vtable->vaSetSubpictureGlobalAlpha =
      flu_va_drivers_vdpau_SetSubpictureGlobalAlpha;
  ctx->vtable->vaAssociateSubpicture =
      flu_va_drivers_vdpau_AssociateSubpicture;
  ctx->vtable->vaDeassociateSubpicture =
      flu_va_drivers_vdpau_DeassociateSubpicture;
  ctx->vtable->vaQueryDisplayAttributes =
      flu_va_drivers_vdpau_QueryDisplayAttributes;
  ctx->vtable->vaGetDisplayAttributes =
      flu_va_drivers_vdpau_GetDisplayAttributes;
  ctx->vtable->vaSetDisplayAttributes =
      flu_va_drivers_vdpau_SetDisplayAttributes;
  ctx->vtable->vaBufferInfo = flu_va_drivers_vdpau_BufferInfo;
  ctx->vtable->vaLockSurface = flu_va_drivers_vdpau_LockSurface;
  ctx->vtable->vaUnlockSurface = flu_va_drivers_vdpau_UnlockSurface;
  ctx->vtable->vaGetSurfaceAttributes =
      flu_va_drivers_vdpau_GetSurfaceAttributes;
  ctx->vtable->vaCreateSurfaces2 = flu_va_drivers_vdpau_CreateSurfaces2;
  ctx->vtable->vaQuerySurfaceAttributes =
      flu_va_drivers_vdpau_QuerySurfaceAttributes;
  ctx->vtable->vaAcquireBufferHandle =
      flu_va_drivers_vdpau_AcquireBufferHandle;
  ctx->vtable->vaReleaseBufferHandle =
      flu_va_drivers_vdpau_ReleaseBufferHandle;
  ctx->vtable->vaCreateMFContext = flu_va_drivers_vdpau_CreateMFContext;
  ctx->vtable->vaMFAddContext = flu_va_drivers_vdpau_MFAddContext;
  ctx->vtable->vaMFReleaseContext = flu_va_drivers_vdpau_MFReleaseContext;
  ctx->vtable->vaMFSubmit = flu_va_drivers_vdpau_MFSubmit;
  ctx->vtable->vaCreateBuffer2 = flu_va_drivers_vdpau_CreateBuffer2;
  ctx->vtable->vaQueryProcessingRate =
      flu_va_drivers_vdpau_QueryProcessingRate;
  ctx->vtable->vaExportSurfaceHandle =
      flu_va_drivers_vdpau_ExportSurfaceHandle;
  ctx->vtable->vaSyncSurface2 = flu_va_drivers_vdpau_SyncSurface2;
  ctx->vtable->vaSyncBuffer = flu_va_drivers_vdpau_SyncBuffer;
  ctx->vtable->vaCopy = flu_va_drivers_vdpau_Copy;

  return VA_STATUS_SUCCESS;
}
