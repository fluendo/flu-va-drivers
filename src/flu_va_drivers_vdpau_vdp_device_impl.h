#ifndef __FLU_VA_DRIVERS_VDPAU_VDP_IMPL_H__
#define __FLU_VA_DRIVERS_VDPAU_VDP_IMPL_H__

#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>

typedef struct _FluVaDriversVdpauVdpDeviceImpl FluVaDriversVdpauVdpDeviceImpl;

struct _FluVaDriversVdpauVdpDeviceImpl
{
  VdpDevice vdp_device;
  VdpGetApiVersion *vdp_get_api_version;
  VdpGetInformationString *vdp_get_information_string;
  VdpDeviceDestroy *vdp_device_destroy;
  VdpGenerateCSCMatrix *vdp_generate_csc_matrix;
  VdpVideoSurfaceQueryCapabilities *vdp_video_surface_query_capabilities;
  VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities
      *vdp_video_surface_query_get_put_bits_y_cb_cr_capabilities;
  VdpVideoSurfaceCreate *vdp_video_surface_create;
  VdpVideoSurfaceDestroy *vdp_video_surface_destroy;
  VdpVideoSurfaceGetParameters *vdp_video_surface_get_parameters;
  VdpVideoSurfaceGetBitsYCbCr *vdp_video_surface_get_bits_y_cb_cr;
  VdpVideoSurfacePutBitsYCbCr *vdp_video_surface_put_bits_y_cb_cr;
  VdpOutputSurfaceQueryCapabilities *vdp_output_surface_query_capabilities;
  VdpOutputSurfaceQueryGetPutBitsNativeCapabilities
      *vdp_output_surface_query_get_put_bits_native_capabilities;
  VdpOutputSurfaceQueryPutBitsIndexedCapabilities
      *vdp_output_surface_query_put_bits_indexed_capabilities;
  VdpOutputSurfaceQueryPutBitsYCbCrCapabilities
      *vdp_output_surface_query_put_bits_y_cb_cr_capabilities;
  VdpOutputSurfaceCreate *vdp_output_surface_create;
  VdpOutputSurfaceDestroy *vdp_output_surface_destroy;
  VdpOutputSurfaceGetParameters *vdp_output_surface_get_parameters;
  VdpOutputSurfaceGetBitsNative *vdp_output_surface_get_bits_native;
  VdpOutputSurfacePutBitsNative *vdp_output_surface_put_bits_native;
  VdpOutputSurfacePutBitsIndexed *vdp_output_surface_put_bits_indexed;
  VdpOutputSurfacePutBitsYCbCr *vdp_output_surface_put_bits_y_cb_cr;
  VdpBitmapSurfaceQueryCapabilities *vdp_bitmap_surface_query_capabilities;
  VdpBitmapSurfaceCreate *vdp_bitmap_surface_create;
  VdpBitmapSurfaceDestroy *vdp_bitmap_surface_destroy;
  VdpBitmapSurfaceGetParameters *vdp_bitmap_surface_get_parameters;
  VdpBitmapSurfacePutBitsNative *vdp_bitmap_surface_put_bits_native;
  VdpOutputSurfaceRenderOutputSurface
      *vdp_output_surface_render_output_surface;
  VdpOutputSurfaceRenderBitmapSurface
      *vdp_output_surface_render_bitmap_surface;
  VdpDecoderQueryCapabilities *vdp_decoder_query_capabilities;
  VdpDecoderCreate *vdp_decoder_create;
  VdpDecoderDestroy *vdp_decoder_destroy;
  VdpDecoderGetParameters *vdp_decoder_get_parameters;
  VdpDecoderRender *vdp_decoder_render;
  VdpVideoMixerQueryFeatureSupport *vdp_video_mixer_query_feature_support;
  VdpVideoMixerQueryParameterSupport *vdp_video_mixer_query_parameter_support;
  VdpVideoMixerQueryAttributeSupport *vdp_video_mixer_query_attribute_support;
  VdpVideoMixerQueryParameterValueRange
      *vdp_video_mixer_query_parameter_value_range;
  VdpVideoMixerQueryAttributeValueRange
      *vdp_video_mixer_query_attribute_value_range;
  VdpVideoMixerCreate *vdp_video_mixer_create;
  VdpVideoMixerSetFeatureEnables *vdp_video_mixer_set_feature_enables;
  VdpVideoMixerSetAttributeValues *vdp_video_mixer_set_attribute_values;
  VdpVideoMixerGetFeatureSupport *vdp_video_mixer_get_feature_support;
  VdpVideoMixerGetFeatureEnables *vdp_video_mixer_get_feature_enables;
  VdpVideoMixerGetParameterValues *vdp_video_mixer_get_parameter_values;
  VdpVideoMixerGetAttributeValues *vdp_video_mixer_get_attribute_values;
  VdpVideoMixerDestroy *vdp_video_mixer_destroy;
  VdpVideoMixerRender *vdp_video_mixer_render;
  VdpPresentationQueueTargetCreateX11
      *vdp_presentation_queue_target_create_x11;
  VdpPresentationQueueTargetDestroy *vdp_presentation_queue_target_destroy;
  VdpPresentationQueueCreate *vdp_presentation_queue_create;
  VdpPresentationQueueDestroy *vdp_presentation_queue_destroy;
  VdpPresentationQueueSetBackgroundColor
      *vdp_presentation_queue_set_background_color;
  VdpPresentationQueueGetBackgroundColor
      *vdp_presentation_queue_get_background_color;
  VdpPresentationQueueGetTime *vdp_presentation_queue_get_time;
  VdpPresentationQueueDisplay *vdp_presentation_queue_display;
  VdpPresentationQueueBlockUntilSurfaceIdle
      *vdp_presentation_queue_block_until_surface_idle;
  VdpPresentationQueueQuerySurfaceStatus
      *vdp_presentation_queue_query_surface_status;
  VdpPreemptionCallbackRegister *vdp_preemption_callback_register;
};

VdpStatus flu_va_drivers_vdpau_vdp_device_impl_init (
    FluVaDriversVdpauVdpDeviceImpl *self, VdpDevice device,
    VdpGetProcAddress get_proc_addr);

#endif /* __FLU_VA_DRIVERS_VDPAU_VDP_DEVICE_IMPL_H__ */
