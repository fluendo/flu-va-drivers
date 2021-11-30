#include "flu_va_drivers_vdpau_vdp_device_impl.h"

VdpStatus
flu_va_drivers_vdpau_vdp_device_impl_init (
    FluVaDriversVdpauVdpDeviceImpl *self, VdpDevice *device,
    VdpGetProcAddress *get_proc_addr)
{
#define _DEV_FUNC(func_id, func)                                              \
  if (get_proc_addr (self->vdp_device, VDP_FUNC_ID_##func_id,                 \
          (void **) &self->vdp_##func) != VDP_STATUS_OK)                      \
    return VDP_STATUS_ERROR;
  _DEV_FUNC (GET_API_VERSION, get_api_version)
  _DEV_FUNC (GET_INFORMATION_STRING, get_information_string)
  _DEV_FUNC (DEVICE_DESTROY, device_destroy)
  _DEV_FUNC (GENERATE_CSC_MATRIX, generate_csc_matrix)
  _DEV_FUNC (
      VIDEO_SURFACE_QUERY_CAPABILITIES, video_surface_query_capabilities)
  _DEV_FUNC (VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES,
      video_surface_query_get_put_bits_y_cb_cr_capabilities)
  _DEV_FUNC (VIDEO_SURFACE_CREATE, video_surface_create)
  _DEV_FUNC (VIDEO_SURFACE_DESTROY, video_surface_destroy)
  _DEV_FUNC (VIDEO_SURFACE_GET_PARAMETERS, video_surface_get_parameters)
  _DEV_FUNC (VIDEO_SURFACE_GET_BITS_Y_CB_CR, video_surface_get_bits_y_cb_cr)
  _DEV_FUNC (VIDEO_SURFACE_PUT_BITS_Y_CB_CR, video_surface_put_bits_y_cb_cr)
  _DEV_FUNC (
      OUTPUT_SURFACE_QUERY_CAPABILITIES, output_surface_query_capabilities)
  _DEV_FUNC (OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES,
      output_surface_query_get_put_bits_native_capabilities)
  _DEV_FUNC (OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES,
      output_surface_query_put_bits_indexed_capabilities)
  _DEV_FUNC (OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES,
      output_surface_query_put_bits_y_cb_cr_capabilities)
  _DEV_FUNC (OUTPUT_SURFACE_CREATE, output_surface_create)
  _DEV_FUNC (OUTPUT_SURFACE_DESTROY, output_surface_destroy)
  _DEV_FUNC (OUTPUT_SURFACE_GET_PARAMETERS, output_surface_get_parameters)
  _DEV_FUNC (OUTPUT_SURFACE_GET_BITS_NATIVE, output_surface_get_bits_native)
  _DEV_FUNC (OUTPUT_SURFACE_PUT_BITS_NATIVE, output_surface_put_bits_native)
  _DEV_FUNC (OUTPUT_SURFACE_PUT_BITS_INDEXED, output_surface_put_bits_indexed)
  _DEV_FUNC (OUTPUT_SURFACE_PUT_BITS_Y_CB_CR, output_surface_put_bits_y_cb_cr)
  _DEV_FUNC (
      BITMAP_SURFACE_QUERY_CAPABILITIES, bitmap_surface_query_capabilities)
  _DEV_FUNC (BITMAP_SURFACE_CREATE, bitmap_surface_create)
  _DEV_FUNC (BITMAP_SURFACE_DESTROY, bitmap_surface_destroy)
  _DEV_FUNC (BITMAP_SURFACE_GET_PARAMETERS, bitmap_surface_get_parameters)
  _DEV_FUNC (BITMAP_SURFACE_PUT_BITS_NATIVE, bitmap_surface_put_bits_native)
  _DEV_FUNC (OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE,
      output_surface_render_output_surface)
  _DEV_FUNC (OUTPUT_SURFACE_RENDER_BITMAP_SURFACE,
      output_surface_render_bitmap_surface)
  _DEV_FUNC (DECODER_QUERY_CAPABILITIES, decoder_query_capabilities)
  _DEV_FUNC (DECODER_CREATE, decoder_create)
  _DEV_FUNC (DECODER_DESTROY, decoder_destroy)
  _DEV_FUNC (DECODER_GET_PARAMETERS, decoder_get_parameters)
  _DEV_FUNC (DECODER_RENDER, decoder_render)
  _DEV_FUNC (
      VIDEO_MIXER_QUERY_FEATURE_SUPPORT, video_mixer_query_feature_support)
  _DEV_FUNC (
      VIDEO_MIXER_QUERY_PARAMETER_SUPPORT, video_mixer_query_parameter_support)
  _DEV_FUNC (
      VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT, video_mixer_query_attribute_support)
  _DEV_FUNC (VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE,
      video_mixer_query_parameter_value_range)
  _DEV_FUNC (VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE,
      video_mixer_query_attribute_value_range)
  _DEV_FUNC (VIDEO_MIXER_CREATE, video_mixer_create)
  _DEV_FUNC (VIDEO_MIXER_SET_FEATURE_ENABLES, video_mixer_set_feature_enables)
  _DEV_FUNC (
      VIDEO_MIXER_SET_ATTRIBUTE_VALUES, video_mixer_set_attribute_values)
  _DEV_FUNC (VIDEO_MIXER_GET_FEATURE_SUPPORT, video_mixer_get_feature_support)
  _DEV_FUNC (VIDEO_MIXER_GET_FEATURE_ENABLES, video_mixer_get_feature_enables)
  _DEV_FUNC (
      VIDEO_MIXER_GET_PARAMETER_VALUES, video_mixer_get_parameter_values)
  _DEV_FUNC (
      VIDEO_MIXER_GET_ATTRIBUTE_VALUES, video_mixer_get_attribute_values)
  _DEV_FUNC (VIDEO_MIXER_DESTROY, video_mixer_destroy)
  _DEV_FUNC (VIDEO_MIXER_RENDER, video_mixer_render)
  _DEV_FUNC (
      PRESENTATION_QUEUE_TARGET_DESTROY, presentation_queue_target_destroy)
  _DEV_FUNC (PRESENTATION_QUEUE_CREATE, presentation_queue_create)
  _DEV_FUNC (PRESENTATION_QUEUE_DESTROY, presentation_queue_destroy)
  _DEV_FUNC (PRESENTATION_QUEUE_SET_BACKGROUND_COLOR,
      presentation_queue_set_background_color)
  _DEV_FUNC (PRESENTATION_QUEUE_GET_BACKGROUND_COLOR,
      presentation_queue_get_background_color)
  _DEV_FUNC (PRESENTATION_QUEUE_GET_TIME, presentation_queue_get_time)
  _DEV_FUNC (PRESENTATION_QUEUE_DISPLAY, presentation_queue_display)
  _DEV_FUNC (PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE,
      presentation_queue_block_until_surface_idle)
  _DEV_FUNC (PRESENTATION_QUEUE_QUERY_SURFACE_STATUS,
      presentation_queue_query_surface_status)
  _DEV_FUNC (PREEMPTION_CALLBACK_REGISTER, preemption_callback_register)
#undef _DEV_FUNC
  return VDP_STATUS_OK;
}
