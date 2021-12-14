#include "flu_va_drivers_vdpau_utils.h"

VAStatus
flu_va_drivers_map_va_profile_to_vdpau_decoder_profile (
    VAProfile va_profile, VdpDecoderProfile *vdp_profile)
{
  VAStatus ret = VA_STATUS_SUCCESS;

  switch (va_profile) {
    case VAProfileH264ConstrainedBaseline:
      *vdp_profile = VDP_DECODER_PROFILE_H264_BASELINE;
      break;
    case VAProfileH264Main:
      *vdp_profile = VDP_DECODER_PROFILE_H264_MAIN;
      break;
    case VAProfileH264High:
      *vdp_profile = VDP_DECODER_PROFILE_H264_HIGH;
      break;
    default:
      ret = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
  }

  return ret;
}
