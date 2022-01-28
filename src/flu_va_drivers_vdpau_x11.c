#include "flu_va_drivers_vdpau_x11.h"

VAStatus
flu_va_drivers_vdpau_destroy_video_mixer (
    VADriverContextP ctx, FluVaDriversVdpauVideoMixerObject *video_mixer_obj)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauContextObject *context_obj;
  VdpStatus vdp_st;

  context_obj = (FluVaDriversVdpauContextObject *) object_heap_lookup (
      &driver_data->context_heap, video_mixer_obj->context_id);
  assert (context_obj != NULL);

  if (video_mixer_obj->vdp_video_mixer != VDP_INVALID_HANDLE)
    vdp_st = driver_data->vdp_impl.vdp_video_mixer_destroy (
        video_mixer_obj->vdp_video_mixer);
  context_obj->video_mixer_id = FLU_VA_DRIVERS_INVALID_ID;
  object_heap_free (
      &driver_data->video_mixer_heap, (object_base_p) video_mixer_obj);

  if (vdp_st != VDP_STATUS_OK)
    return VA_STATUS_ERROR_UNKNOWN;
  return VA_STATUS_SUCCESS;
}

VAStatus
flu_va_drivers_vdpau_create_video_mixer (VADriverContextP ctx,
    VAContextID context, int width, int height, int va_rt_format,
    FluVaDriversID *video_mixer_id)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauVideoMixerObject *video_mixer_obj;
  int video_mixer_obj_id;
  VdpChromaType vdp_chroma_type;
  VAStatus ret = VA_STATUS_SUCCESS;
  VdpVideoMixer vdp_video_mixer;

  ret = flu_va_drivers_map_va_rt_format_to_vdp_chroma_type (
      va_rt_format, &vdp_chroma_type);
  if (ret != VA_STATUS_SUCCESS)
    return ret;

  video_mixer_obj_id = object_heap_allocate (&driver_data->video_mixer_heap);
  if (video_mixer_obj_id == -1)
    return VA_STATUS_ERROR_ALLOCATION_FAILED;
  video_mixer_obj = (FluVaDriversVdpauVideoMixerObject *) object_heap_lookup (
      &driver_data->video_mixer_heap, video_mixer_obj_id);
  assert (video_mixer_obj != NULL);

  video_mixer_obj->context_id = context;
  video_mixer_obj->width = width;
  video_mixer_obj->height = height;
  video_mixer_obj->vdp_chroma_type = vdp_chroma_type;

  static const VdpVideoMixerParameter params[] = {
    VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH,
    VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT,
    VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE
  };
  const void *param_values[] = { &video_mixer_obj->width,
    &video_mixer_obj->height, &video_mixer_obj->vdp_chroma_type };

  if (driver_data->vdp_impl.vdp_video_mixer_create (
          driver_data->vdp_impl.vdp_device, 0, NULL,
          sizeof (params) / sizeof (*params), params, param_values,
          &vdp_video_mixer) != VDP_STATUS_OK)
    goto beach;
  video_mixer_obj->vdp_video_mixer = vdp_video_mixer;

  *video_mixer_id = video_mixer_obj_id;
  return ret;

beach:
  flu_va_drivers_vdpau_destroy_video_mixer (ctx, video_mixer_obj);
  return VA_STATUS_ERROR_UNKNOWN;
}

VAStatus
flu_va_drivers_vdpau_context_ensure_video_mixer (VADriverContextP ctx,
    FluVaDriversVdpauContextObject *context_obj, int width, int height,
    int va_rt_format)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauVideoMixerObject *video_mixer_object;
  VdpChromaType vdp_chroma_type;
  VAStatus va_st;

  va_st = flu_va_drivers_map_va_rt_format_to_vdp_chroma_type (
      va_rt_format, &vdp_chroma_type);
  if (va_st != VA_STATUS_SUCCESS)
    return VA_STATUS_ERROR_UNKNOWN;

  video_mixer_object =
      (FluVaDriversVdpauVideoMixerObject *) object_heap_lookup (
          &driver_data->video_mixer_heap, context_obj->video_mixer_id);
  if (video_mixer_object == NULL)
    goto create;

  // video-mixer is ensured when has same params.
  if (video_mixer_object->width == width &&
      video_mixer_object->height == height &&
      video_mixer_object->vdp_chroma_type == vdp_chroma_type)
    return VA_STATUS_SUCCESS;

  va_st = flu_va_drivers_vdpau_destroy_video_mixer (ctx, video_mixer_object);
  if (va_st != VA_STATUS_SUCCESS)
    return va_st;

create:
  return flu_va_drivers_vdpau_create_video_mixer (ctx, context_obj->base.id,
      width, height, va_rt_format, &context_obj->video_mixer_id);
}
