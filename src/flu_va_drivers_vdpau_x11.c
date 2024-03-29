/* flu-va-drivers
 * Copyright 2022-2023 Fluendo S.A. <support@fluendo.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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

static FluVaDriversVdpauPresentationQueueMapEntry *
flu_va_drivers_vdpau_new_presentation_queue_map_entry (
    VADriverContextP ctx, Drawable draw)
{
  FluVaDriversVdpauPresentationQueueMapEntry *entry;

  entry = malloc (sizeof (FluVaDriversVdpauPresentationQueueMapEntry));
  entry->ctx = ctx;
  entry->drawable = draw;
  entry->vdp_presentation_queue = VDP_INVALID_HANDLE;
  entry->vdp_presentation_queue_target = VDP_INVALID_HANDLE;

  return entry;
}

VAStatus
flu_va_drivers_vdpau_context_destroy_presentaton_queue_entry (
    FluVaDriversVdpauPresentationQueueMapEntry *entry)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) entry->ctx->pDriverData;
  VdpStatus vdp_st;
  VAStatus va_st = VA_STATUS_SUCCESS;

  if (entry->vdp_presentation_queue_target != VDP_INVALID_HANDLE) {
    vdp_st = driver_data->vdp_impl.vdp_presentation_queue_target_destroy (
        entry->vdp_presentation_queue_target);
    entry->vdp_presentation_queue_target = VDP_INVALID_HANDLE;
    if (vdp_st != VDP_STATUS_OK)
      va_st = VA_STATUS_ERROR_UNKNOWN;
  }

  if (entry->vdp_presentation_queue != VDP_INVALID_HANDLE) {
    vdp_st = driver_data->vdp_impl.vdp_presentation_queue_destroy (
        entry->vdp_presentation_queue);
    entry->vdp_presentation_queue = VDP_INVALID_HANDLE;
    if (va_st == VA_STATUS_SUCCESS && vdp_st != VDP_STATUS_OK)
      va_st = VA_STATUS_ERROR_UNKNOWN;
  }

  free (entry);

  return va_st;
}

static FluVaDriversVdpauPresentationQueueMapEntry *
flu_va_drivers_vdpau_context_find_presentation_queue_map_entry (
    FluVaDriversVdpauContextObject *context_obj, Drawable draw)
{
  FluVaDriversVdpauPresentationQueueMapEntry *entry = NULL;

  for (entry = SLIST_FIRST (&context_obj->vdp_presentation_queue_map);
       entry != NULL; entry = SLIST_NEXT (entry, entries)) {
    if (entry->drawable == draw)
      break;
  }
  return entry;
}

void
flu_va_drivers_vdpau_context_init_presentaton_queue_map (
    FluVaDriversVdpauContextObject *context_obj)
{
  SLIST_INIT (&context_obj->vdp_presentation_queue_map);
}

VAStatus
flu_va_drivers_vdpau_context_destroy_presentaton_queue_map (
    FluVaDriversVdpauContextObject *context_obj)
{
  VAStatus ret = VA_STATUS_SUCCESS;

  while (!SLIST_EMPTY (&context_obj->vdp_presentation_queue_map)) {
    VAStatus va_st;
    FluVaDriversVdpauPresentationQueueMapEntry *entry =
        SLIST_FIRST (&context_obj->vdp_presentation_queue_map);
    SLIST_REMOVE_HEAD (&context_obj->vdp_presentation_queue_map, entries);
    va_st =
        flu_va_drivers_vdpau_context_destroy_presentaton_queue_entry (entry);
    if (ret == VA_STATUS_SUCCESS && va_st != VA_STATUS_SUCCESS)
      ret = va_st;
  }

  return ret;
}

VAStatus
flu_va_drivers_vdpau_context_ensure_presentation_queue_map_entry (
    VADriverContextP ctx, FluVaDriversVdpauContextObject *context_obj,
    Drawable draw, FluVaDriversVdpauPresentationQueueMapEntry **entry)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  VdpPresentationQueueTarget vdp_presentation_queue_target;
  VdpPresentationQueue vdp_presentation_queue;
  VAStatus va_st = VA_STATUS_SUCCESS;
  VdpStatus vdp_st;

  *entry = flu_va_drivers_vdpau_context_find_presentation_queue_map_entry (
      context_obj, draw);
  if (*entry != NULL)
    return va_st;

  *entry = flu_va_drivers_vdpau_new_presentation_queue_map_entry (ctx, draw);

  vdp_st = driver_data->vdp_impl.vdp_presentation_queue_target_create_x11 (
      driver_data->vdp_impl.vdp_device, draw, &vdp_presentation_queue_target);
  if (vdp_st != VDP_STATUS_OK)
    goto beach;
  (*entry)->vdp_presentation_queue_target = vdp_presentation_queue_target;

  vdp_st = driver_data->vdp_impl.vdp_presentation_queue_create (
      driver_data->vdp_impl.vdp_device, vdp_presentation_queue_target,
      &vdp_presentation_queue);
  if (va_st == VA_STATUS_SUCCESS && vdp_st != VDP_STATUS_OK)
    goto beach;
  (*entry)->vdp_presentation_queue = vdp_presentation_queue;

  return va_st;
beach:
  flu_va_drivers_vdpau_context_destroy_presentaton_queue_entry (*entry);
  return VA_STATUS_ERROR_UNKNOWN;
}

void
flu_va_drivers_vdpau_context_clear_output_surfaces (
    FluVaDriversVdpauContextObject *context_obj)
{
  int i;
  for (i = 0; i < FLU_VA_DRIVERS_VDPAU_NUM_OUTPUT_SURFACES; i++)
    context_obj->vdp_output_surfaces[i] = VDP_INVALID_HANDLE;
}

VAStatus
flu_va_drivers_vdpau_destroy_output_surfaces (VADriverContextP ctx,
    FluVaDriversVdpauContextObject *context_obj,
    VdpOutputSurface *vdp_output_surfaces, int num_output_surfaces)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  VAStatus va_st = VA_STATUS_SUCCESS;
  int i;

  for (i = 0; i < num_output_surfaces; i++) {
    VdpStatus vdp_st;

    if (vdp_output_surfaces[i] == VDP_INVALID_HANDLE)
      continue;

    vdp_st = driver_data->vdp_impl.vdp_output_surface_destroy (
        vdp_output_surfaces[i]);
    vdp_output_surfaces[i] = VDP_INVALID_HANDLE;
    if (va_st == VA_STATUS_SUCCESS && vdp_st != VDP_STATUS_OK)
      va_st = VA_STATUS_ERROR_UNKNOWN;
  }

  return va_st;
}

VAStatus
flu_va_drivers_vdpau_context_init_output_surfaces (VADriverContextP ctx,
    FluVaDriversVdpauContextObject *context_obj, unsigned int width,
    unsigned int height)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  VAStatus va_st = VA_STATUS_SUCCESS;
  int i;

  for (i = 0; i < FLU_VA_DRIVERS_VDPAU_NUM_OUTPUT_SURFACES; i++) {
    VdpStatus vdp_st;
    vdp_st = driver_data->vdp_impl.vdp_output_surface_create (
        driver_data->vdp_impl.vdp_device, VDP_RGBA_FORMAT_B8G8R8A8, width,
        height, &context_obj->vdp_output_surfaces[i]);
    if (vdp_st != VDP_STATUS_OK)
      goto beach;
  }

  return va_st;

beach:
  return flu_va_drivers_vdpau_destroy_output_surfaces (
      ctx, context_obj, context_obj->vdp_output_surfaces, i);
}

VAStatus
flu_va_drivers_vdpau_context_ensure_output_surfaces (VADriverContextP ctx,
    FluVaDriversVdpauContextObject *context_obj, unsigned int width,
    unsigned int height)
{
  int initialized;

  initialized = context_obj->vdp_output_surfaces[0] != VDP_INVALID_HANDLE;
  if (initialized)
    return VA_STATUS_SUCCESS;

  return flu_va_drivers_vdpau_context_init_output_surfaces (
      ctx, context_obj, width, height);
}

static VAStatus
flu_va_drivers_vdpau_wait_on_current_output_surface (VADriverContextP ctx,
    FluVaDriversVdpauContextObject *context_obj,
    VdpPresentationQueue vdp_presentation_queue)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  VdpOutputSurface vdp_output_surface =
      context_obj->vdp_output_surfaces[context_obj->vdp_output_surface_idx];
  VdpStatus vdp_st;
  VdpTime unused;

  assert (vdp_presentation_queue != VDP_INVALID_HANDLE);
  assert (vdp_output_surface != VDP_INVALID_HANDLE);

  vdp_st =
      driver_data->vdp_impl.vdp_presentation_queue_block_until_surface_idle (
          vdp_presentation_queue, vdp_output_surface, &unused);

  if (vdp_st != VDP_STATUS_OK)
    return VA_STATUS_ERROR_SURFACE_IN_DISPLAYING;
  return VA_STATUS_SUCCESS;
}

VAStatus
flu_va_drivers_vdpau_render (VADriverContextP ctx,
    FluVaDriversVdpauContextObject *context_obj,
    FluVaDriversVdpauSurfaceObject *surface_obj, Drawable draw,
    FluVaDriversVdpauPresentationQueueMapEntry *presentation_queue_map_entry,
    unsigned int draw_width, unsigned int draw_height,
    const VARectangle *dst_rect, VdpVideoMixerPictureStructure vdp_field)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  VdpOutputSurface vdp_output_surface =
      context_obj->vdp_output_surfaces[context_obj->vdp_output_surface_idx];
  FluVaDriversVdpauVideoMixerObject *video_mixer_obj;
  VAStatus va_st;
  VdpStatus vdp_st;
  VdpRect vdp_dst_rect;

  va_st = flu_va_drivers_vdpau_wait_on_current_output_surface (
      ctx, context_obj, presentation_queue_map_entry->vdp_presentation_queue);
  if (va_st != VA_STATUS_SUCCESS)
    return va_st;

  video_mixer_obj = (FluVaDriversVdpauVideoMixerObject *) object_heap_lookup (
      &driver_data->video_mixer_heap, context_obj->video_mixer_id);
  assert (video_mixer_obj != NULL &&
          video_mixer_obj->vdp_video_mixer != VDP_INVALID_HANDLE);

  flu_va_drivers_map_va_rectangle_to_vdp_rect (dst_rect, &vdp_dst_rect);
  vdp_st = driver_data->vdp_impl.vdp_video_mixer_render (
      video_mixer_obj->vdp_video_mixer,
      /* background */
      VDP_INVALID_HANDLE, NULL,
      /* progressive (full-frame), top field or bottom field */
      vdp_field,
      /* past */
      0, NULL,
      /* current */
      surface_obj->vdp_surface,
      /* future */
      0, NULL, NULL,
      /* destination */
      vdp_output_surface, NULL, &vdp_dst_rect,
      /* layers */
      0, NULL);
  if (vdp_st != VDP_STATUS_OK)
    return VA_STATUS_ERROR_SURFACE_IN_DISPLAYING;

  vdp_st = driver_data->vdp_impl.vdp_presentation_queue_display (
      presentation_queue_map_entry->vdp_presentation_queue, vdp_output_surface,
      draw_width, draw_height, 0);
  if (vdp_st != VDP_STATUS_OK)
    return VA_STATUS_ERROR_SURFACE_IN_DISPLAYING;

  context_obj->vdp_output_surface_idx =
      (context_obj->vdp_output_surface_idx + 1) %
      FLU_VA_DRIVERS_VDPAU_NUM_OUTPUT_SURFACES;

  return VA_STATUS_SUCCESS;
}
