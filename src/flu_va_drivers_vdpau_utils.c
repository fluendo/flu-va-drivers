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

#include "flu_va_drivers_vdpau_utils.h"

// clang-format off
FluVaDriversVdpauImageFormatMap FLU_VA_DRIVERS_VDPAU_IMAGE_FORMAT_MAP = {
  {
      FLU_VA_DRIVERS_VDPAU_IMAGE_FORMAT_TYPE_YCBCR,
      VDP_YCBCR_FORMAT_NV12,
      {VA_FOURCC_NV12, VA_LSB_FIRST, 12, },
  },
  { FLU_VA_DRIVERS_VDPAU_IMAGE_FORMAT_TYPE_NONE, 0, {0, 0, 0, } },
};
// clang-format on

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

VAStatus
flu_va_drivers_map_va_rt_format_to_vdp_chroma_type (
    int va_rt_format, VdpChromaType *vdp_chroma_type)
{
  VAStatus ret = VA_STATUS_SUCCESS;
  switch (va_rt_format) {
    case VA_RT_FORMAT_YUV420:
      *vdp_chroma_type = VDP_CHROMA_TYPE_420;
      break;
    default:
      ret = VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;
  }

  return ret;
}

VAStatus
flu_va_drivers_map_va_flag_to_vdp_video_mixer_picture_structure (
    int va_flag, VdpVideoMixerPictureStructure *vdp_flag)
{
  va_flag &= 0xf;

  if (va_flag & VA_TOP_FIELD)
    *vdp_flag = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD;
  else if (va_flag & VA_BOTTOM_FIELD)
    *vdp_flag = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;
  else if (va_flag == VA_FRAME_PICTURE)
    *vdp_flag = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;
  else
    return VA_STATUS_ERROR_UNKNOWN;

  return VA_STATUS_SUCCESS;
}

void
flu_va_drivers_map_va_rectangle_to_vdp_rect (
    const VARectangle *va_rect, VdpRect *vdp_rect)
{
  vdp_rect->x0 = va_rect->x;
  vdp_rect->y0 = va_rect->y;
  vdp_rect->x1 = va_rect->x + va_rect->width;
  vdp_rect->y1 = va_rect->y + va_rect->height;
}

VAConfigAttrib *
flu_va_drivers_vdpau_lookup_config_attrib_type (VAConfigAttrib *attrib_list,
    int num_attribs, VAConfigAttribType attrib_type)
{
  int i;

  for (i = 0; i < num_attribs; i++) {
    if (attrib_list[i].type == attrib_type)
      return &attrib_list[i];
  }
  return NULL;
}

int
flu_va_drivers_vdpau_is_profile_supported (VAProfile va_profile)
{
  VdpDecoderProfile vdp_profile;
  VAStatus st;

  st = flu_va_drivers_map_va_profile_to_vdpau_decoder_profile (
      va_profile, &vdp_profile);

  return st == VA_STATUS_SUCCESS;
}

int
flu_va_drivers_vdpau_is_entrypoint_supported (VAEntrypoint va_entrypoint)
{
  return va_entrypoint == VAEntrypointVLD;
}

void
flu_va_drivers_vdpau_context_object_reset (
    FluVaDriversVdpauContextObject *context_obj)
{
  context_obj->current_render_target = VA_INVALID_ID;
  memset (&context_obj->vdp_pic_info, 0, sizeof (context_obj->vdp_pic_info));
  context_obj->last_slice_param = NULL;
  if (context_obj->vdp_bs_buf != NULL)
    free (context_obj->vdp_bs_buf);
  context_obj->vdp_bs_buf = NULL;
  context_obj->cap_vdp_bs_buf = 0;
  context_obj->num_vdp_bs_buf = 0;
}

static void
flu_va_drivers_vdpau_context_object_free_vdp_bs_buf (
    FluVaDriversVdpauContextObject *context_obj)
{
  if (context_obj->vdp_bs_buf == NULL)
    return;

  free (context_obj->vdp_bs_buf);
  context_obj->vdp_bs_buf = NULL;
  context_obj->num_vdp_bs_buf = 0;
  context_obj->cap_vdp_bs_buf = 0;
}

static VAStatus
flu_va_drivers_vdpau_context_object_alloc_vdp_bs_buf (
    FluVaDriversVdpauContextObject *context_obj, int prepend_nalu_start_code)
{
  if (!prepend_nalu_start_code)
    context_obj->cap_vdp_bs_buf = 1;
  else
    context_obj->cap_vdp_bs_buf = 2;

  context_obj->num_vdp_bs_buf = 0;
  context_obj->vdp_bs_buf =
      malloc (context_obj->cap_vdp_bs_buf * sizeof (VdpBitstreamBuffer));

  if (context_obj->vdp_bs_buf == NULL)
    return VA_STATUS_ERROR_ALLOCATION_FAILED;
  return VA_STATUS_SUCCESS;
}

static VAStatus
flu_va_drivers_vdpau_context_object_try_realloc_vdp_bs_buf (
    FluVaDriversVdpauContextObject *context_obj)
{
  if (context_obj->num_vdp_bs_buf != context_obj->cap_vdp_bs_buf)
    return VA_STATUS_SUCCESS;

  context_obj->cap_vdp_bs_buf *= 2;
  context_obj->vdp_bs_buf = realloc (context_obj->vdp_bs_buf,
      context_obj->cap_vdp_bs_buf * sizeof (VdpBitstreamBuffer));

  if (context_obj->vdp_bs_buf == NULL)
    return VA_STATUS_ERROR_ALLOCATION_FAILED;
  return VA_STATUS_SUCCESS;
}

static void
flu_va_drivers_vdpau_context_object_append_vdp_bs_buf (
    FluVaDriversVdpauContextObject *context_obj, const uint8_t *bs_data,
    uint32_t num_bytes)
{
  context_obj->vdp_bs_buf[context_obj->num_vdp_bs_buf].struct_version =
      VDP_BITSTREAM_BUFFER_VERSION;
  context_obj->vdp_bs_buf[context_obj->num_vdp_bs_buf].bitstream = bs_data;
  context_obj->vdp_bs_buf[context_obj->num_vdp_bs_buf].bitstream_bytes =
      num_bytes;
  context_obj->num_vdp_bs_buf++;
}

VAStatus
flu_va_driver_vdpau_translate_ref_frame_h264 (VADriverContextP ctx,
    VAPictureH264 *va_ref_frame, VdpReferenceFrameH264 *vdp_ref_frame)
{
  FluVaDriversVdpauDriverData *driver_data =
      (FluVaDriversVdpauDriverData *) ctx->pDriverData;
  FluVaDriversVdpauSurfaceObject *surface_obj;

  if (va_ref_frame->picture_id == VA_INVALID_ID) {
    vdp_ref_frame->surface = VDP_INVALID_HANDLE;
    return VA_STATUS_SUCCESS;
  }

  surface_obj = (FluVaDriversVdpauSurfaceObject *) object_heap_lookup (
      &driver_data->surface_heap, va_ref_frame->picture_id);
  if (surface_obj == NULL)
    return VA_STATUS_ERROR_INVALID_SURFACE;

  vdp_ref_frame->surface = surface_obj->vdp_surface;
  vdp_ref_frame->is_long_term =
      (va_ref_frame->flags & VA_PICTURE_H264_LONG_TERM_REFERENCE) != 0;

  if ((va_ref_frame->flags &
          (VA_PICTURE_H264_TOP_FIELD | VA_PICTURE_H264_BOTTOM_FIELD)) == 0) {
    vdp_ref_frame->top_is_reference = VDP_TRUE;
    vdp_ref_frame->bottom_is_reference = VDP_TRUE;

  } else {
    vdp_ref_frame->top_is_reference =
        (va_ref_frame->flags & VA_PICTURE_H264_TOP_FIELD) != 0;
    vdp_ref_frame->bottom_is_reference =
        (va_ref_frame->flags & VA_PICTURE_H264_BOTTOM_FIELD) != 0;
  }

  vdp_ref_frame->field_order_cnt[0] = va_ref_frame->TopFieldOrderCnt;
  vdp_ref_frame->field_order_cnt[1] = va_ref_frame->BottomFieldOrderCnt;
  vdp_ref_frame->frame_idx = va_ref_frame->frame_idx;

  return VA_STATUS_SUCCESS;
}

#define _MAP_FIELD(FIELD) vdp_pic_info->FIELD = param->FIELD;
VAStatus
flu_va_driver_vdpau_translate_buffer_h264 (VADriverContextP ctx,
    FluVaDriversVdpauContextObject *context_obj,
    FluVaDriversVdpauBufferObject *buffer_obj)
{
  VAStatus ret = VA_STATUS_SUCCESS;
  VdpPictureInfoH264 *vdp_pic_info = &context_obj->vdp_pic_info;

  assert (buffer_obj->num_elements == 1);

  switch (buffer_obj->type) {
    case VAPictureParameterBufferType:
#define _MAP_BITS_FIELD(FIELD, BITS_FIELD)                                    \
  vdp_pic_info->BITS_FIELD = param->FIELD.bits.BITS_FIELD
    {
      VAPictureParameterBufferH264 *param =
          (VAPictureParameterBufferH264 *) buffer_obj->data;
      int i;

      /* Note: Rec. ITU-T H.264. Section: 6.2: For now only 4:2:0 support. */
      if (param->seq_fields.bits.chroma_format_idc != 1 ||
          param->seq_fields.bits.residual_colour_transform_flag != 0) {
        ret = VA_STATUS_ERROR_UNKNOWN;
        break;
      }

      vdp_pic_info->field_order_cnt[0] = param->CurrPic.TopFieldOrderCnt;
      vdp_pic_info->field_order_cnt[1] = param->CurrPic.TopFieldOrderCnt;
      // _MAP_BITS_FIELD (seq_fields, chroma_format_idc);
      // _MAP_BITS_FIELD (seq_fields, gaps_in_frame_num_value_allowed_flag);
      _MAP_BITS_FIELD (seq_fields, frame_mbs_only_flag);
      _MAP_BITS_FIELD (seq_fields, mb_adaptive_frame_field_flag);
      _MAP_BITS_FIELD (seq_fields, direct_8x8_inference_flag);
      // _MAP_BITS_FIELD (seq_fields, MinLumaBiPredSize8x8);
      _MAP_BITS_FIELD (seq_fields, log2_max_frame_num_minus4);
      _MAP_BITS_FIELD (seq_fields, pic_order_cnt_type);
      _MAP_BITS_FIELD (seq_fields, log2_max_pic_order_cnt_lsb_minus4);
      _MAP_BITS_FIELD (seq_fields, delta_pic_order_always_zero_flag);

      _MAP_BITS_FIELD (pic_fields, entropy_coding_mode_flag);
      _MAP_BITS_FIELD (pic_fields, weighted_pred_flag);
      _MAP_BITS_FIELD (pic_fields, weighted_bipred_idc);
      _MAP_BITS_FIELD (pic_fields, transform_8x8_mode_flag);
      _MAP_BITS_FIELD (pic_fields, field_pic_flag);

      _MAP_BITS_FIELD (pic_fields, constrained_intra_pred_flag);
      _MAP_BITS_FIELD (pic_fields, pic_order_present_flag);
      _MAP_BITS_FIELD (pic_fields, deblocking_filter_control_present_flag);
      _MAP_BITS_FIELD (pic_fields, redundant_pic_cnt_present_flag);
      vdp_pic_info->is_reference = param->pic_fields.bits.reference_pic_flag;

      vdp_pic_info->bottom_field_flag =
          param->pic_fields.bits.field_pic_flag &&
          (param->CurrPic.flags & VA_PICTURE_H264_BOTTOM_FIELD) != 0;
      _MAP_FIELD (num_ref_frames);
      _MAP_FIELD (chroma_qp_index_offset);
      _MAP_FIELD (second_chroma_qp_index_offset);
      _MAP_FIELD (pic_init_qp_minus26);

      for (i = 0; ret == VA_STATUS_SUCCESS && i < 16; i++)
        ret = flu_va_driver_vdpau_translate_ref_frame_h264 (ctx,
            &param->ReferenceFrames[i], &vdp_pic_info->referenceFrames[i]);
      if (ret != VA_STATUS_SUCCESS)
        break;

      vdp_pic_info->frame_num = param->frame_num;
      break;
    }
#undef _MAP_BITS_FIELD
    case VAIQMatrixBufferType: {
      VAIQMatrixBufferH264 *iq_matrix =
          (VAIQMatrixBufferH264 *) buffer_obj->data;

      memcpy (vdp_pic_info->scaling_lists_4x4, iq_matrix->ScalingList4x4,
          sizeof (iq_matrix->ScalingList4x4));
      memcpy (vdp_pic_info->scaling_lists_8x8, iq_matrix->ScalingList8x8,
          sizeof (iq_matrix->ScalingList8x8));
      break;
    }
    case VASliceParameterBufferType: {
      VASliceParameterBufferH264 *param =
          (VASliceParameterBufferH264 *) buffer_obj->data;

      if (context_obj->last_buffer_type != VASliceDataBufferType)
        flu_va_drivers_vdpau_context_object_free_vdp_bs_buf (context_obj);

      vdp_pic_info->slice_count += buffer_obj->num_elements;
      _MAP_FIELD (num_ref_idx_l0_active_minus1);
      _MAP_FIELD (num_ref_idx_l1_active_minus1);
      context_obj->last_slice_param = param;
      break;
    }
    case VASliceDataBufferType: {
      uint8_t *buf;
      int prepend_nalu_start_code = 0;

      // TODO: vaEndPicture or vaRenderPicture vfunc should ensure to set
      // vdp_bs_buf back to NULL and num_vdp_bs_buf to 0, so the next sequence
      // of calls to vaBeginPicture, vaRenderPicture, vaEndPicture will have an
      // empty vdp_bs_buf.

      assert (
          (context_obj->vdp_bs_buf != NULL &&
              context_obj->num_vdp_bs_buf > 0 &&
              context_obj->num_vdp_bs_buf <= context_obj->cap_vdp_bs_buf) ||
          (context_obj->vdp_bs_buf == NULL &&
              context_obj->num_vdp_bs_buf == 0 &&
              context_obj->cap_vdp_bs_buf == 0));
      // This driver supports only if a VASliceDataBufferType buffer follows a
      // a VASliceParameterBufferType buffer. Also, this driver supports only
      // one buffer of this type between vaBeginPicture and vaEndPicture
      // lifetime as it seems to be the most common.
      if (context_obj->last_slice_param == NULL ||
          context_obj->last_buffer_type != VASliceParameterBufferType) {
        ret = VA_STATUS_ERROR_UNKNOWN;
        break;
      }

      buf =
          buffer_obj->data + context_obj->last_slice_param->slice_data_offset;
      prepend_nalu_start_code =
          memcmp (buf, NALU_START_CODE, sizeof (NALU_START_CODE)) != 0 &&
          memcmp (buf, NALU_START_CODE_4, sizeof (NALU_START_CODE_4)) != 0;

      if (context_obj->vdp_bs_buf == NULL) {
        ret = flu_va_drivers_vdpau_context_object_alloc_vdp_bs_buf (
            context_obj, prepend_nalu_start_code);
        if (ret != VA_STATUS_SUCCESS)
          break;
      }
      if ((ret = flu_va_drivers_vdpau_context_object_try_realloc_vdp_bs_buf (
               context_obj)) != VA_STATUS_SUCCESS)
        break;

      if (prepend_nalu_start_code)
        flu_va_drivers_vdpau_context_object_append_vdp_bs_buf (
            context_obj, NALU_START_CODE, sizeof (NALU_START_CODE));

      flu_va_drivers_vdpau_context_object_append_vdp_bs_buf (
          context_obj, buf, context_obj->last_slice_param->slice_data_size);

      break;
    }
    default:
      ret = VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
      break;
  }

  context_obj->last_buffer_type = buffer_obj->type;
  return ret;
}
#undef _MAP_FIELD

int
flu_va_driver_vdpau_is_buffer_type_supported (VABufferType buffer_type)
{
  switch (buffer_type) {
    case VAPictureParameterBufferType:
    case VAIQMatrixBufferType:
    case VASliceParameterBufferType:
    case VASliceDataBufferType:
      return 1;
    default:
      return 0;
  }
}