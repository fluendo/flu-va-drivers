#include <va/va.h>
#include <vdpau/vdpau.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "flu_va_drivers_vdpau.h"
#include "flu_va_drivers_utils.h"
#include "flu_va_drivers_vdpau_utils.h"
#include "flu_va_drivers_vdpau_x11.h"

typedef struct ImagePtr
{
  void *planes[3];
  uint32_t pitches[3];
} ImagePtr;

static VAStatus flu_va_drivers_vdpau_CreateSurfaces2 (VADriverContextP ctx,
    unsigned int format, unsigned int width, unsigned int height,
    VASurfaceID *surfaces, unsigned int num_surfaces,
    VASurfaceAttrib *attrib_list, unsigned int num_attribs);
static VAStatus set_image_format (FluVaDriversVdpauImageObject *image_obj,
    const VAImageFormat *format, int width, int height);
static VAStatus get_image_ptr (FluVaDriversVdpauDriverData *driver_data,
    FluVaDriversVdpauImageObject *image_obj, ImagePtr *ptr);

// clang-format off
#define _DEFAULT_OFFSET     24
#define CONFIG_ID_OFFSET    1 << _DEFAULT_OFFSET
#define CONTEXT_ID_OFFSET   2 << _DEFAULT_OFFSET
#define SURFACE_ID_OFFSET   3 << _DEFAULT_OFFSET
#define BUFFER_ID_OFFSET    4 << _DEFAULT_OFFSET
#define IMAGE_ID_OFFSET     5 << _DEFAULT_OFFSET
#define SUBPIC_ID_OFFSET    6 << _DEFAULT_OFFSET
#define VIDEO_MIXER_ID_OFFSET 7 << _DEFAULT_OFFSET
// clang-format on

static VAStatus
flu_va_drivers_vdpau_Terminate (VADriverContextP ctx)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;

  object_heap_terminate (&driver_data->config_heap);
  object_heap_terminate (&driver_data->context_heap);
  object_heap_terminate (&driver_data->surface_heap);
  object_heap_terminate (&driver_data->buffer_heap);
  object_heap_terminate (&driver_data->image_heap);
  object_heap_terminate (&driver_data->subpic_heap);

  if (driver_data->x11_dpy != ctx->native_dpy)
    XCloseDisplay (driver_data->x11_dpy);

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

  if (!flu_va_drivers_vdpau_is_profile_supported (profile))
    return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
  if (!flu_va_drivers_vdpau_is_entrypoint_supported (entrypoint))
    return VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;

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
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauConfigObject *config_obj;
  VAConfigAttrib *attrib;
  VdpStatus vdp_st;
  VAStatus va_st;
  VdpDecoderProfile vdp_profile;
  uint32_t unused_max_level, unused_max_macroblocks, max_width, max_height;
  VdpBool is_profile_supported;

  va_st = flu_va_drivers_map_va_profile_to_vdpau_decoder_profile (
      profile, &vdp_profile);
  if (va_st != VA_STATUS_SUCCESS)
    return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
  /* Check profile hardware support */
  vdp_st = driver_data->vdp_impl.vdp_decoder_query_capabilities (
      driver_data->vdp_impl.vdp_device, vdp_profile, &is_profile_supported,
      &unused_max_level, &unused_max_macroblocks, &max_width, &max_height);
  if (vdp_st != VDP_STATUS_OK || !is_profile_supported)
    return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;

  if (!flu_va_drivers_vdpau_is_entrypoint_supported (entrypoint))
    return VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;

  *config_id = object_heap_allocate (&driver_data->config_heap);
  if (*config_id == -1)
    return VA_STATUS_ERROR_ALLOCATION_FAILED;
  config_obj = (FluVaDriversVdpauConfigObject *) object_heap_lookup (
      &driver_data->config_heap, *config_id);
  assert (config_obj != NULL);

  config_obj->profile = profile;
  config_obj->max_width = max_width;
  config_obj->max_height = max_height;
  config_obj->entrypoint = entrypoint;

  config_obj->num_attribs = 1;
  config_obj->attrib_list[0].type = VAConfigAttribRTFormat;
  config_obj->attrib_list[0].value = VA_RT_FORMAT_YUV420;
  attrib = flu_va_drivers_vdpau_lookup_config_attrib_type (
      attrib_list, num_attribs, VAConfigAttribRTFormat);
  if (attrib && attrib->value != VA_RT_FORMAT_YUV420)
    return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;

  return VA_STATUS_SUCCESS;
}

static VAStatus
flu_va_drivers_vdpau_DestroyConfig (VADriverContextP ctx, VAConfigID config_id)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauConfigObject *config_obj;

  config_obj = (FluVaDriversVdpauConfigObject *) object_heap_lookup (
      &driver_data->config_heap, config_id);
  if (config_obj == NULL)
    return VA_STATUS_ERROR_INVALID_CONFIG;

  object_heap_free (&driver_data->config_heap, (object_base_p) config_obj);

  return VA_STATUS_SUCCESS;
}

static VAStatus
flu_va_drivers_vdpau_QueryConfigAttributes (VADriverContextP ctx,
    VAConfigID config_id, VAProfile *profile, VAEntrypoint *entrypoint,
    VAConfigAttrib *attrib_list, int *num_attribs)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauConfigObject *config_obj;

  config_obj = (FluVaDriversVdpauConfigObject *) object_heap_lookup (
      &driver_data->config_heap, config_id);
  if (config_obj == NULL)
    return VA_STATUS_ERROR_INVALID_CONFIG;

  if (profile != NULL)
    *profile = config_obj->profile;

  if (entrypoint != NULL)
    *entrypoint = config_obj->entrypoint;

  if (num_attribs != NULL)
    *num_attribs = config_obj->num_attribs;

  if (attrib_list != NULL)
    memcpy (attrib_list, config_obj->attrib_list,
        config_obj->num_attribs * sizeof (*attrib_list));

  return VA_STATUS_SUCCESS;
}

static VAStatus
flu_va_drivers_vdpau_CreateSurfaces (VADriverContextP ctx, int width,
    int height, int format, int num_surfaces, VASurfaceID *surfaces)
{
  return flu_va_drivers_vdpau_CreateSurfaces2 (
      ctx, format, width, height, surfaces, num_surfaces, NULL, 0);
}

static VAStatus
flu_va_drivers_vdpau_DestroySurfaces (
    VADriverContextP ctx, VASurfaceID *surface_list, int num_surfaces)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  VAStatus ret = VA_STATUS_SUCCESS;
  int i;

  for (i = 0; i < num_surfaces; i++) {
    FluVaDriversVdpauSurfaceObject *surface_obj;
    VdpStatus vdp_st;

    surface_obj = (FluVaDriversVdpauSurfaceObject *) object_heap_lookup (
        &driver_data->surface_heap, surface_list[i]);

    if (surface_obj == NULL) {
      if (ret == VA_STATUS_SUCCESS)
        ret = VA_STATUS_ERROR_INVALID_SURFACE;
      continue;
    }

    vdp_st = driver_data->vdp_impl.vdp_video_surface_destroy (
        surface_obj->vdp_surface);
    if (ret == VA_STATUS_SUCCESS && vdp_st != VDP_STATUS_OK)
      ret = VA_STATUS_ERROR_UNKNOWN;
    object_heap_free (&driver_data->surface_heap, (object_base_p) surface_obj);
  }

  return ret;
}

static VAStatus
flu_va_drivers_vdpau_destroy_context (
    VADriverContextP ctx, FluVaDriversVdpauContextObject *context_obj)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  VAStatus va_st, ret = VA_STATUS_SUCCESS;
  VdpStatus vdp_st;
  FluVaDriversVdpauVideoMixerObject *video_mixer_obj;

  if (context_obj->vdp_decoder != VDP_INVALID_HANDLE) {
    vdp_st =
        driver_data->vdp_impl.vdp_decoder_destroy (context_obj->vdp_decoder);
    if (vdp_st != VDP_STATUS_OK)
      ret = VA_STATUS_ERROR_UNKNOWN;
  }

  video_mixer_obj = (FluVaDriversVdpauVideoMixerObject *) object_heap_lookup (
      &driver_data->video_mixer_heap, context_obj->video_mixer_id);
  if (video_mixer_obj) {
    va_st = flu_va_drivers_vdpau_destroy_video_mixer (ctx, video_mixer_obj);
    if (ret == VA_STATUS_SUCCESS)
      ret = va_st;
  }

  va_st =
      flu_va_drivers_vdpau_context_destroy_presentaton_queue_map (context_obj);
  if (ret == VA_STATUS_SUCCESS)
    ret = va_st;

  va_st = flu_va_drivers_vdpau_destroy_output_surfaces (ctx, context_obj,
      context_obj->vdp_output_surfaces,
      FLU_VA_DRIVERS_VDPAU_NUM_OUTPUT_SURFACES);
  if (ret == VA_STATUS_SUCCESS)
    ret = va_st;

  free (context_obj->render_targets);
  object_heap_free (&driver_data->context_heap, (object_base_p) context_obj);

  return va_st;
}

static VAStatus
flu_va_drivers_vdpau_CreateContext (VADriverContextP ctx, VAConfigID config_id,
    int picture_width, int picture_height, int flag,
    VASurfaceID *render_targets, int num_render_targets, VAContextID *context)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauConfigObject *config_obj;
  FluVaDriversVdpauContextObject *context_obj;
  int i = 0, context_obj_id;

  config_obj = (FluVaDriversVdpauConfigObject *) object_heap_lookup (
      &driver_data->config_heap, config_id);
  if (config_obj == NULL)
    return VA_STATUS_ERROR_INVALID_CONFIG;

  context_obj_id = object_heap_allocate (&driver_data->context_heap);
  if (context_obj_id == -1)
    return VA_STATUS_ERROR_ALLOCATION_FAILED;
  context_obj = (FluVaDriversVdpauContextObject *) object_heap_lookup (
      &driver_data->context_heap, context_obj_id);
  assert (context_obj != NULL);

  if (picture_width > config_obj->max_width ||
      picture_height > config_obj->max_height)
    return VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED;

  context_obj->config_id = config_id;
  context_obj->flag = flag;
  context_obj->picture_width = picture_width;
  context_obj->picture_height = picture_height;
  context_obj->render_targets = NULL;
  context_obj->num_render_targets = num_render_targets;
  context_obj->vdp_bs_buf = NULL;
  context_obj->vdp_decoder = VDP_INVALID_HANDLE;
  context_obj->video_mixer_id = FLU_VA_DRIVERS_INVALID_ID;
  flu_va_drivers_vdpau_context_init_presentaton_queue_map (context_obj);
  context_obj->vdp_presentation_queue = VDP_INVALID_HANDLE;
  context_obj->vdp_presentation_queue_target = VDP_INVALID_HANDLE;
  context_obj->vdp_output_surface_idx = 0;
  flu_va_drivers_vdpau_context_clear_output_surfaces (context_obj);

  flu_va_drivers_vdpau_context_object_reset (context_obj);

  if (num_render_targets == 0)
    goto bye;

  context_obj->render_targets =
      calloc (num_render_targets, sizeof (VASurfaceID));
  do {
    FluVaDriversVdpauSurfaceObject *surface_obj;
    surface_obj = (FluVaDriversVdpauSurfaceObject *) object_heap_lookup (
        &driver_data->surface_heap, render_targets[i]);

    if (surface_obj == NULL || (surface_obj->context_id != VA_INVALID_ID &&
                                   surface_obj->context_id != context_obj_id))
      goto invalid_surface;

    context_obj->render_targets[i] = render_targets[i];
    surface_obj->context_id = (VAContextID) context_obj_id;
  } while (++i < num_render_targets);

bye:
  *context = context_obj_id;
  return VA_STATUS_SUCCESS;

invalid_surface:
  while (i--) {
    FluVaDriversVdpauSurfaceObject *surface_obj;
    surface_obj = (FluVaDriversVdpauSurfaceObject *) object_heap_lookup (
        &driver_data->surface_heap, render_targets[i]);
    assert (surface_obj != NULL);

    surface_obj->context_id = VA_INVALID_ID;
  }
  flu_va_drivers_vdpau_destroy_context (ctx, context_obj);

  return VA_STATUS_ERROR_INVALID_SURFACE;
}

static VAStatus
flu_va_drivers_vdpau_DestroyContext (VADriverContextP ctx, VAContextID context)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauContextObject *context_obj;

  context_obj = (FluVaDriversVdpauContextObject *) object_heap_lookup (
      &driver_data->context_heap, context);
  if (context_obj == NULL)
    return VA_STATUS_ERROR_INVALID_CONFIG;

  return flu_va_drivers_vdpau_destroy_context (
      ctx, (FluVaDriversVdpauContextObject *) context_obj);
}

static VAStatus
flu_va_drivers_vdpau_CreateBuffer (VADriverContextP ctx, VAContextID context,
    VABufferType type, unsigned int size, unsigned int num_elements,
    void *data, VABufferID *buf_id)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauBufferObject *buffer_obj;
  int buffer_obj_id;

  // Support only num_elements == 1, because this is the use-case of most of
  // the programs, including Chromium that is what we target in this project.
  if (size == 0 || num_elements != 1)
    return VA_STATUS_ERROR_INVALID_VALUE;

  switch (type) {
    case VAPictureParameterBufferType:
    case VAIQMatrixBufferType:
    case VASliceParameterBufferType:
    case VASliceDataBufferType:
    case VAImageBufferType:
      break;
    default:
      return VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
  }

  buffer_obj_id = object_heap_allocate (&driver_data->buffer_heap);
  if (buffer_obj_id == -1)
    return VA_STATUS_ERROR_ALLOCATION_FAILED;
  *buf_id = buffer_obj_id;
  buffer_obj = (FluVaDriversVdpauBufferObject *) object_heap_lookup (
      &driver_data->buffer_heap, *buf_id);
  assert (buffer_obj != NULL);

  buffer_obj->type = type;
  buffer_obj->size = size;
  buffer_obj->num_elements = num_elements;
  buffer_obj->data = malloc (buffer_obj->size);
  assert (buffer_obj->data != NULL);
  if (data != NULL) {
    memcpy (
        buffer_obj->data, data, buffer_obj->size * buffer_obj->num_elements);
  }

  return VA_STATUS_SUCCESS;
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
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauBufferObject *buffer_obj;

  if (pbuf == NULL)
    return VA_STATUS_ERROR_INVALID_PARAMETER;

  buffer_obj = (FluVaDriversVdpauBufferObject *) object_heap_lookup (
      &driver_data->buffer_heap, buf_id);
  if (buffer_obj == NULL)
    return VA_STATUS_ERROR_INVALID_BUFFER;

  assert (buffer_obj->data != NULL);
  *pbuf = buffer_obj->data;

  return VA_STATUS_SUCCESS;
}

static VAStatus
flu_va_drivers_vdpau_UnmapBuffer (VADriverContextP ctx, VABufferID buf_id)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauBufferObject *buffer_obj;

  buffer_obj = (FluVaDriversVdpauBufferObject *) object_heap_lookup (
      &driver_data->buffer_heap, buf_id);
  if (buffer_obj == NULL)
    return VA_STATUS_ERROR_INVALID_BUFFER;

  return VA_STATUS_SUCCESS;
}

static VAStatus
flu_va_drivers_vdpau_DestroyBuffer (VADriverContextP ctx, VABufferID buffer_id)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauBufferObject *buffer_obj;

  buffer_obj = (FluVaDriversVdpauBufferObject *) object_heap_lookup (
      &driver_data->buffer_heap, buffer_id);
  if (buffer_obj == NULL)
    return VA_STATUS_ERROR_INVALID_BUFFER;

  assert (buffer_obj->data);
  free (buffer_obj->data);
  object_heap_free (&driver_data->buffer_heap, (object_base_p) buffer_obj);

  return VA_STATUS_SUCCESS;
}

static VAStatus
flu_va_drivers_vdpau_BeginPicture (
    VADriverContextP ctx, VAContextID context, VASurfaceID render_target)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauContextObject *context_obj;
  FluVaDriversVdpauSurfaceObject *surface_obj;

  surface_obj = (FluVaDriversVdpauSurfaceObject *) object_heap_lookup (
      &driver_data->surface_heap, render_target);
  if (surface_obj == NULL || (surface_obj->context_id != VA_INVALID_ID &&
                                 surface_obj->context_id != context))
    return VA_STATUS_ERROR_INVALID_SURFACE;

  context_obj = (FluVaDriversVdpauContextObject *) object_heap_lookup (
      &driver_data->context_heap, context);
  if (context_obj == NULL ||
      context_obj->current_render_target != VA_INVALID_ID)
    return VA_STATUS_ERROR_INVALID_CONTEXT;

  flu_va_drivers_vdpau_context_object_reset (context_obj);
  context_obj->current_render_target = render_target;
  if (surface_obj->context_id == VA_INVALID_ID)
    surface_obj->context_id = context;

  return VA_STATUS_SUCCESS;
}

static VAStatus
flu_va_drivers_vdpau_RenderPicture (VADriverContextP ctx, VAContextID context,
    VABufferID *buffers, int num_buffers)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauContextObject *context_obj;
  FluVaDriversVdpauSurfaceObject *surface_obj;
  int i;

  context_obj = (FluVaDriversVdpauContextObject *) object_heap_lookup (
      &driver_data->context_heap, context);
  if (context_obj == NULL ||
      context_obj->current_render_target == VA_INVALID_ID)
    return VA_STATUS_ERROR_INVALID_CONTEXT;

  surface_obj = (FluVaDriversVdpauSurfaceObject *) object_heap_lookup (
      &driver_data->surface_heap, context_obj->current_render_target);
  if (surface_obj == NULL)
    return VA_STATUS_ERROR_INVALID_SURFACE;
  assert (surface_obj->context_id == VA_INVALID_ID ||
          (surface_obj->context_id != VA_INVALID_ID &&
              surface_obj->context_id == context));

  for (i = 0; i < num_buffers; i++) {
    FluVaDriversVdpauBufferObject *buffer_obj;

    buffer_obj = (FluVaDriversVdpauBufferObject *) object_heap_lookup (
        &driver_data->buffer_heap, buffers[i]);
    if (buffer_obj == NULL ||
        !flu_va_driver_vdpau_is_buffer_type_supported (buffer_obj->type))
      return VA_STATUS_ERROR_INVALID_BUFFER;
  }

  for (i = 0; i < num_buffers; i++) {
    FluVaDriversVdpauBufferObject *buffer_obj;

    buffer_obj = (FluVaDriversVdpauBufferObject *) object_heap_lookup (
        &driver_data->buffer_heap, buffers[i]);
    assert (buffer_obj != NULL);

    if (flu_va_driver_vdpau_translate_buffer_h264 (
            ctx, context_obj, buffer_obj) != VA_STATUS_SUCCESS)
      goto translation_error;
  }

  return VA_STATUS_SUCCESS;

translation_error:
  /* TODO: Execute pending delayed buffer destroy */
  flu_va_drivers_vdpau_context_object_reset (context_obj);
  return VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
}

static VAStatus
flu_va_drivers_vdpau_EndPicture (VADriverContextP ctx, VAContextID context)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauContextObject *context_obj;
  FluVaDriversVdpauConfigObject *config_obj;
  FluVaDriversVdpauSurfaceObject *surface_obj;
  VdpDecoderProfile vdp_profile;
  VdpStatus vdp_st;
  VAStatus ret;

  context_obj = (FluVaDriversVdpauContextObject *) object_heap_lookup (
      &driver_data->context_heap, context);
  if (context_obj == NULL)
    return VA_STATUS_ERROR_INVALID_CONTEXT;

  config_obj = (FluVaDriversVdpauConfigObject *) object_heap_lookup (
      &driver_data->config_heap, context_obj->config_id);
  if (config_obj == NULL)
    return VA_STATUS_ERROR_INVALID_CONFIG;

  surface_obj = (FluVaDriversVdpauSurfaceObject *) object_heap_lookup (
      &driver_data->surface_heap, context_obj->current_render_target);
  if (surface_obj == NULL)
    return VA_STATUS_ERROR_INVALID_SURFACE;

  if (flu_va_drivers_map_va_profile_to_vdpau_decoder_profile (
          config_obj->profile, &vdp_profile) != VA_STATUS_SUCCESS)
    return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;

  // FIXME: Using 16 as a temporary max_ref_frames, but may lead into
  // waste of memory.
  if (context_obj->vdp_decoder == VDP_INVALID_HANDLE) {
    vdp_st = driver_data->vdp_impl.vdp_decoder_create (
        driver_data->vdp_impl.vdp_device, vdp_profile,
        context_obj->picture_width, context_obj->picture_height, 16,
        &context_obj->vdp_decoder);

    if (vdp_st != VDP_STATUS_OK) {
      context_obj->vdp_decoder = VDP_INVALID_HANDLE;
      ret = VA_STATUS_ERROR_UNKNOWN;
      goto beach;
    }
  }

  /* TODO: Check validity of VdpPictureInfo? */
  vdp_st = driver_data->vdp_impl.vdp_decoder_render (context_obj->vdp_decoder,
      surface_obj->vdp_surface, (VdpPictureInfo *) &context_obj->vdp_pic_info,
      context_obj->num_vdp_bs_buf, context_obj->vdp_bs_buf);

  if (vdp_st != VDP_STATUS_OK) {
    ret = VA_STATUS_ERROR_DECODING_ERROR;
    goto beach;
  }

  ret = VA_STATUS_SUCCESS;
beach:
  flu_va_drivers_vdpau_context_object_reset (context_obj);
  return ret;
}

static VAStatus
flu_va_drivers_vdpau_SyncSurface (
    VADriverContextP ctx, VASurfaceID render_target)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauSurfaceObject *surface_obj;
  FluVaDriversVdpauContextObject *context_obj;

  surface_obj = (FluVaDriversVdpauSurfaceObject *) object_heap_lookup (
      &driver_data->surface_heap, render_target);
  if (surface_obj == NULL)
    return VA_STATUS_ERROR_INVALID_SURFACE;

  context_obj = (FluVaDriversVdpauContextObject *) object_heap_lookup (
      &driver_data->context_heap, surface_obj->context_id);
  if (context_obj == NULL)
    return VA_STATUS_ERROR_INVALID_CONTEXT;

  /* Users of VA-API usually call vaSyncSurface after vaEndPicture */
  assert (context_obj->current_render_target != render_target);

  /* TODO: When vaPutSurface gets implemented, do polling here. */
  return VA_STATUS_SUCCESS;
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
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauContextObject *context_obj;
  FluVaDriversVdpauSurfaceObject *surface_obj;
  FluVaDriversVdpauPresentationQueueMapEntry *vdp_presentation_queue_map_entry;
  VAStatus va_st = VA_STATUS_SUCCESS;
  VdpVideoMixerPictureStructure vdp_field;
  Window win;
  unsigned int width, height, border_width, depth;
  int x, y;
  VARectangle dst_rect = {
    .x = destx, .y = desty, .width = destw, .height = desth
  };

  va_st = flu_va_drivers_map_va_flag_to_vdp_video_mixer_picture_structure (
      flags, &vdp_field);
  if (va_st != VA_STATUS_SUCCESS)
    return va_st;

  if (vdp_field != VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME)
    return VA_STATUS_ERROR_FLAG_NOT_SUPPORTED;

  surface_obj = (FluVaDriversVdpauSurfaceObject *) object_heap_lookup (
      &driver_data->surface_heap, surface);
  if (surface_obj == NULL)
    return VA_STATUS_ERROR_INVALID_SURFACE;

  context_obj = (FluVaDriversVdpauContextObject *) object_heap_lookup (
      &driver_data->context_heap, surface_obj->context_id);
  if (context_obj == NULL)
    return VA_STATUS_ERROR_INVALID_CONTEXT;

  va_st = flu_va_drivers_vdpau_context_ensure_video_mixer (ctx, context_obj,
      surface_obj->width, surface_obj->height, surface_obj->format);
  if (va_st != VA_STATUS_SUCCESS)
    return va_st;

  va_st = flu_va_drivers_vdpau_context_ensure_presentation_queue_map_entry (
      ctx, context_obj, (Drawable) draw, &vdp_presentation_queue_map_entry);
  if (va_st != VA_STATUS_SUCCESS)
    return va_st;

  if (!XGetGeometry (driver_data->x11_dpy, (Drawable) draw, &win, &x, &y,
          &width, &height, &border_width, &depth))
    return VA_STATUS_ERROR_UNKNOWN;

  va_st = flu_va_drivers_vdpau_context_ensure_output_surfaces (
      ctx, context_obj, width, height);
  if (va_st != VA_STATUS_SUCCESS)
    return va_st;

  return flu_va_drivers_vdpau_render (ctx, context_obj, surface_obj,
      (Drawable) draw, vdp_presentation_queue_map_entry, width, height,
      &dst_rect, vdp_field);
}

static VAStatus
flu_va_drivers_vdpau_QueryImageFormats (
    VADriverContextP ctx, VAImageFormat *format_list, int *num_formats)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauVdpDeviceImpl impl = driver_data->vdp_impl;
  const FluVaDriversVdpauImageFormatMapItem *item =
      FLU_VA_DRIVERS_VDPAU_IMAGE_FORMAT_MAP;

  *num_formats = 0;
  while (item->type != FLU_VA_DRIVERS_VDPAU_IMAGE_FORMAT_TYPE_NONE) {
    VdpStatus vdp_st;
    VdpBool is_format_supported = 0;

    switch (item->type) {
      case FLU_VA_DRIVERS_VDPAU_IMAGE_FORMAT_TYPE_YCBCR:
        vdp_st =
            impl.vdp_video_surface_query_get_put_bits_y_cb_cr_capabilities (
                driver_data->vdp_impl.vdp_device, VDP_CHROMA_TYPE_420,
                item->vdp_image_format, &is_format_supported);
        break;
      default:
        vdp_st = VDP_STATUS_NO_IMPLEMENTATION;
        break;
    }

    if (vdp_st == VDP_STATUS_OK && is_format_supported)
      format_list[(*num_formats)++] = item->va_image_format;

    item++;
  }

  return VA_STATUS_SUCCESS;
}

static VAStatus
flu_va_drivers_vdpau_CreateImage (VADriverContextP ctx, VAImageFormat *format,
    int width, int height, VAImage *image)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauImageObject *image_obj;
  VAImage *va_image;
  int image_id;
  VAStatus ret;

  if ((format == NULL) || (image == NULL))
    return VA_STATUS_ERROR_INVALID_PARAMETER;

  image_id = object_heap_allocate (&driver_data->image_heap);
  if (image_id == VA_INVALID_ID)
    return VA_STATUS_ERROR_ALLOCATION_FAILED;

  image_obj = (FluVaDriversVdpauImageObject *) object_heap_lookup (
      &driver_data->image_heap, image_id);
  assert (image_obj != NULL);

  ret = set_image_format (image_obj, format, width, height);
  if (ret != VA_STATUS_SUCCESS)
    goto error;
  va_image = &image_obj->va_image;

  va_image->image_id = image_id;

  ret = flu_va_drivers_vdpau_CreateBuffer (
      ctx, 0, VAImageBufferType, va_image->data_size, 1, NULL, &va_image->buf);
  if (ret != VA_STATUS_SUCCESS)
    goto error;

  *image = *va_image;
  return VA_STATUS_SUCCESS;
error:
  object_heap_free (&driver_data->image_heap, (object_base_p) image_obj);
  return ret;
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
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauImageObject *image_obj;

  image_obj = (FluVaDriversVdpauImageObject *) object_heap_lookup (
      &driver_data->image_heap, image);
  if (image_obj == NULL)
    return VA_STATUS_ERROR_INVALID_IMAGE;

  flu_va_drivers_vdpau_DestroyBuffer (ctx, image_obj->va_image.buf);

  object_heap_free (&driver_data->image_heap, (object_base_p) image_obj);

  return VA_STATUS_SUCCESS;
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
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauSurfaceObject *surface_obj;
  FluVaDriversVdpauImageObject *image_obj;
  ImagePtr img_ptr;
  VdpStatus vdp_st;
  VAStatus ret;

  surface_obj = (FluVaDriversVdpauSurfaceObject *) object_heap_lookup (
      &driver_data->surface_heap, surface);
  if (surface_obj == NULL)
    return VA_STATUS_ERROR_INVALID_SURFACE;

  image_obj = (FluVaDriversVdpauImageObject *) object_heap_lookup (
      &driver_data->image_heap, image);
  if (image_obj == NULL)
    return VA_STATUS_ERROR_INVALID_IMAGE;

  ret = get_image_ptr (driver_data, image_obj, &img_ptr);
  if (ret != VA_STATUS_SUCCESS)
    return ret;

  switch (image_obj->format_type) {
    case FLU_VA_DRIVERS_VDPAU_IMAGE_FORMAT_TYPE_YCBCR:
      // This call can only read the full surface
      if (x != 0 || y != 0 || surface_obj->width != width ||
          surface_obj->height != height) {
        return VA_STATUS_ERROR_INVALID_PARAMETER;
      }

      vdp_st = driver_data->vdp_impl.vdp_video_surface_get_bits_y_cb_cr (
          surface_obj->vdp_surface, image_obj->vdp_format, img_ptr.planes,
          img_ptr.pitches);
      if (vdp_st != VDP_STATUS_OK)
        return VA_STATUS_ERROR_OPERATION_FAILED;
      break;
    default:
      return VA_STATUS_ERROR_INVALID_IMAGE;
  }

  return VA_STATUS_SUCCESS;
}

static VAStatus
get_image_ptr (FluVaDriversVdpauDriverData *driver_data,
    FluVaDriversVdpauImageObject *image_obj, ImagePtr *ptr)
{
  FluVaDriversVdpauBufferObject *buffer_obj;
  uint32_t *offsets;
  uint32_t *pitches;
  uint8_t *data;

  buffer_obj = (FluVaDriversVdpauBufferObject *) object_heap_lookup (
      &driver_data->buffer_heap, image_obj->va_image.buf);
  if (buffer_obj == NULL)
    return VA_STATUS_ERROR_INVALID_BUFFER;

  data = buffer_obj->data;
  offsets = image_obj->va_image.offsets;
  pitches = image_obj->va_image.pitches;

  switch (image_obj->va_image.format.fourcc) {
    case VA_FOURCC_I420:
      ptr->planes[0] = data + offsets[0];
      ptr->pitches[0] = pitches[0];

      ptr->planes[1] = data + offsets[2];
      ptr->pitches[1] = pitches[2];

      ptr->planes[2] = data + offsets[1];
      ptr->pitches[2] = pitches[1];
      break;
    default:
      for (int i = 0; i < image_obj->va_image.num_planes; i++) {
        ptr->planes[i] = data + offsets[i];
        ptr->pitches[i] = pitches[i];
      }
      break;
  }

  return VA_STATUS_SUCCESS;
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
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauBufferObject *buffer_obj;

  buffer_obj = (FluVaDriversVdpauBufferObject *) object_heap_lookup (
      &driver_data->buffer_heap, buf_id);
  if (buffer_obj == NULL)
    return VA_STATUS_ERROR_INVALID_BUFFER;

  *type = buffer_obj->type;
  *size = buffer_obj->size;
  *num_elements = buffer_obj->num_elements;

  return VA_STATUS_SUCCESS;
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
flu_va_drivers_vdpau_create_surface (VADriverContextP ctx, int width,
    int height, int format, VASurfaceAttrib *attrib_list, int num_attribs,
    VASurfaceID *surface_id)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauSurfaceObject *surface_obj;
  VdpVideoSurface vdp_surface;
  VdpStatus vdp_st;
  int surface_obj_id;

  assert (format == VA_RT_FORMAT_YUV420);
  assert (num_attribs <= FLU_VA_DRIVERS_VDPAU_MAX_SURFACE_ATTRIBUTES);

  vdp_st = driver_data->vdp_impl.vdp_video_surface_create (
      driver_data->vdp_impl.vdp_device, VDP_CHROMA_TYPE_420, width, height,
      &vdp_surface);
  if (vdp_st != VDP_STATUS_OK)
    return VA_STATUS_ERROR_OPERATION_FAILED;

  surface_obj_id = object_heap_allocate (&driver_data->surface_heap);
  if (surface_obj_id == -1)
    return VA_STATUS_ERROR_ALLOCATION_FAILED;
  surface_obj = (FluVaDriversVdpauSurfaceObject *) object_heap_lookup (
      &driver_data->surface_heap, surface_obj_id);
  assert (surface_obj != NULL);

  *surface_id = surface_obj_id;
  surface_obj->context_id = VA_INVALID_ID;
  surface_obj->format = format;
  surface_obj->width = width;
  surface_obj->height = height;
  surface_obj->vdp_surface = vdp_surface;

  return VA_STATUS_SUCCESS;
}

static VAStatus
flu_va_drivers_vdpau_CreateSurfaces2 (VADriverContextP ctx,
    unsigned int format, unsigned int width, unsigned int height,
    VASurfaceID *surfaces, unsigned int num_surfaces,
    VASurfaceAttrib *attrib_list, unsigned int num_attribs)
{
  VAStatus va_st = VA_STATUS_SUCCESS;
  int i;

  if (width < 0 || height < 0 || num_surfaces < 0 ||
      num_surfaces > FLU_VA_DRIVERS_VDPAU_MAX_SURFACE_ATTRIBUTES)
    return VA_STATUS_ERROR_INVALID_PARAMETER;

  if (format != VA_RT_FORMAT_YUV420)
    return VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;

  for (i = 0; i < num_surfaces; i++) {
    VAStatus va_st;

    va_st = flu_va_drivers_vdpau_create_surface (
        ctx, width, height, format, attrib_list, num_attribs, &surfaces[i]);

    if (va_st != VA_STATUS_SUCCESS)
      break;
  }

  if (va_st != VA_STATUS_SUCCESS) {
    flu_va_drivers_vdpau_DestroySurfaces (ctx, surfaces, i);
  }

  return va_st;
}

static VAStatus
set_image_format (FluVaDriversVdpauImageObject *image_obj,
    const VAImageFormat *format, int width, int height)
{
  unsigned int half_width, half_height, quarter_size, size;
  VAImage *va_image = &image_obj->va_image;
  int8_t component_order[4] = { 0 };

  size = width * height;
  half_width = (width + 1) / 2;
  half_height = (height + 1) / 2;
  quarter_size = half_width * half_height;

  switch (format->fourcc) {
    case VA_FOURCC_NV12:
      image_obj->format_type = FLU_VA_DRIVERS_VDPAU_IMAGE_FORMAT_TYPE_YCBCR;
      image_obj->vdp_format = VDP_YCBCR_FORMAT_NV12;

      va_image->num_planes = 2;
      va_image->pitches[0] = width;
      va_image->offsets[0] = 0;
      va_image->pitches[1] = width;
      va_image->offsets[1] = size;
      va_image->data_size = size + (2 * quarter_size);
      va_image->num_palette_entries = 0;
      va_image->entry_bytes = 0;
      break;
    default:
      return VA_STATUS_ERROR_INVALID_IMAGE_FORMAT;
  }

  va_image->format = *format;
  va_image->width = width;
  va_image->height = height;
  memcpy (
      va_image->component_order, component_order, sizeof (component_order));

  return VA_STATUS_SUCCESS;
}

static VAStatus
flu_va_drivers_vdpau_QuerySurfaceAttributes (VADriverContextP ctx,
    VAConfigID config, VASurfaceAttrib *attrib_list, unsigned int *num_attribs)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauConfigObject *config_obj;

  config_obj = (FluVaDriversVdpauConfigObject *) object_heap_lookup (
      &driver_data->config_heap, config);
  if (!config_obj)
    return VA_STATUS_ERROR_INVALID_CONFIG;

  if (num_attribs == NULL)
    return VA_STATUS_ERROR_INVALID_PARAMETER;

  /* vaQuerySurfaceAttributes can be used just to determine the number of
   * supported attributes according the libva reference manual. */
  *num_attribs = 4;
  if (!attrib_list)
    return VA_STATUS_SUCCESS;

  attrib_list[0].type = VASurfaceAttribMaxWidth;
  attrib_list[0].flags = VA_SURFACE_ATTRIB_GETTABLE;
  attrib_list[0].value.type = VAGenericValueTypeInteger;
  attrib_list[0].value.value.i = config_obj->max_width;

  attrib_list[1].type = VASurfaceAttribMaxHeight;
  attrib_list[1].flags = VA_SURFACE_ATTRIB_GETTABLE;
  attrib_list[1].value.type = VAGenericValueTypeInteger;
  attrib_list[1].value.value.i = config_obj->max_height;

  /* We only support VA_FOURCC_NV12 for now. */
  attrib_list[2].type = VASurfaceAttribPixelFormat;
  /* HACK: Don't support to write this attribute, even when docs allow it. */
  attrib_list[2].flags = VA_SURFACE_ATTRIB_GETTABLE;
  attrib_list[2].value.type = VAGenericValueTypeInteger;
  attrib_list[2].value.value.i = VA_FOURCC_NV12;

  /* NOTE: Not sure if this is needed, but include for now. */
  attrib_list[3].type = VASurfaceAttribMemoryType;
  /* HACK: Don't support to write this attribute, even when docs allow it. */
  attrib_list[3].flags = VA_SURFACE_ATTRIB_GETTABLE;
  attrib_list[3].value.type = VAGenericValueTypeInteger;
  attrib_list[3].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_VA;

  return VA_STATUS_SUCCESS;
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
  /* TODO: Use timeout_ns when polling for ready surfaces is added. */
  return flu_va_drivers_vdpau_SyncSurface (ctx, surface);
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
  VdpDevice device = VDP_INVALID_HANDLE;
  int heap_sz = sizeof (struct object_heap);
  const char *x11_dpy_name;

  flu_va_drivers_get_vendor (driver_data->va_vendor);

  if (ctx->display_type != VA_DISPLAY_X11)
    return VA_STATUS_ERROR_INVALID_DISPLAY;

  x11_dpy_name = XDisplayString (ctx->native_dpy);
  driver_data->x11_dpy = XOpenDisplay (x11_dpy_name);
  if (!driver_data->x11_dpy)
    driver_data->x11_dpy = ctx->native_dpy;

  if (vdp_device_create_x11 (driver_data->x11_dpy, ctx->x11_screen, &device,
          &get_proc_address) != VDP_STATUS_OK)
    return VA_STATUS_ERROR_UNKNOWN;

  if (flu_va_drivers_vdpau_vdp_device_impl_init (
          &driver_data->vdp_impl, device, get_proc_address) != VDP_STATUS_OK)
    return VA_STATUS_ERROR_UNKNOWN;

  object_heap_init (&driver_data->config_heap,
      sizeof (FluVaDriversVdpauConfigObject), CONFIG_ID_OFFSET);
  object_heap_init (&driver_data->context_heap,
      sizeof (FluVaDriversVdpauContextObject), CONTEXT_ID_OFFSET);
  object_heap_init (&driver_data->surface_heap,
      sizeof (FluVaDriversVdpauSurfaceObject), SURFACE_ID_OFFSET);
  object_heap_init (&driver_data->buffer_heap,
      sizeof (FluVaDriversVdpauBufferObject), BUFFER_ID_OFFSET);
  object_heap_init (&driver_data->image_heap,
      sizeof (FluVaDriversVdpauImageObject), IMAGE_ID_OFFSET);
  object_heap_init (&driver_data->video_mixer_heap,
      sizeof (FluVaDriversVdpauVideoMixerObject), VIDEO_MIXER_ID_OFFSET);
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
