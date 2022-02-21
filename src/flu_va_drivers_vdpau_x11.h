#ifndef __FLU_VA_DRIVERS_VDPAU_X11_H__
#define __FLU_VA_DRIVERS_VDPAU_X11_H__

#include <va/va.h>
#include <vdpau/vdpau.h>
#include <sys/queue.h>
#include "flu_va_drivers_utils.h"
#include "flu_va_drivers_vdpau.h"
#include "flu_va_drivers_vdpau_utils.h"

struct _FluVaDriversVdpauPresentationQueueMapEntry
{
  VADriverContextP ctx;
  Drawable drawable;
  VdpPresentationQueue vdp_presentation_queue;
  VdpPresentationQueueTarget vdp_presentation_queue_target;
  SLIST_ENTRY (_FluVaDriversVdpauPresentationQueueMapEntry) entries;
};

VAStatus flu_va_drivers_vdpau_destroy_video_mixer (
    VADriverContextP ctx, FluVaDriversVdpauVideoMixerObject *video_mixer_obj);

VAStatus flu_va_drivers_vdpau_create_video_mixer (VADriverContextP ctx,
    VAContextID context, int width, int height, int va_rt_format,
    FluVaDriversID *video_mixer_id);

VAStatus flu_va_drivers_vdpau_context_ensure_video_mixer (VADriverContextP ctx,
    FluVaDriversVdpauContextObject *context_obj, int width, int height,
    int va_rt_format);

void flu_va_drivers_vdpau_context_init_presentaton_queue_map (
    FluVaDriversVdpauContextObject *context_obj);

VAStatus flu_va_drivers_vdpau_context_destroy_presentaton_queue_map (
    FluVaDriversVdpauContextObject *context_obj);

VAStatus flu_va_drivers_vdpau_context_ensure_presentation_queue_map_entry (
    VADriverContextP ctx, FluVaDriversVdpauContextObject *context_obj,
    Drawable draw, FluVaDriversVdpauPresentationQueueMapEntry **entry);

void flu_va_drivers_vdpau_context_clear_output_surfaces (
    FluVaDriversVdpauContextObject *context_obj);

VAStatus flu_va_drivers_vdpau_destroy_output_surfaces (VADriverContextP ctx,
    FluVaDriversVdpauContextObject *context_obj,
    VdpOutputSurface *vdp_output_surfaces, int num_output_surfaces);

VAStatus flu_va_drivers_vdpau_context_init_output_surfaces (
    VADriverContextP ctx, FluVaDriversVdpauContextObject *context_obj,
    unsigned int width, unsigned int height);

VAStatus flu_va_drivers_vdpau_context_ensure_output_surfaces (
    VADriverContextP ctx, FluVaDriversVdpauContextObject *context_obj,
    unsigned int width, unsigned int height);

VAStatus flu_va_drivers_vdpau_render (VADriverContextP ctx,
    FluVaDriversVdpauContextObject *context_obj,
    FluVaDriversVdpauSurfaceObject *surface_obj, Drawable draw,
    FluVaDriversVdpauPresentationQueueMapEntry *presentation_queue_map_entry,
    unsigned int draw_width, unsigned int draw_height,
    const VARectangle *dst_rect, VdpVideoMixerPictureStructure vdp_field);

#endif // __FLU_VA_DRIVERS_VDPAU_X11_H__