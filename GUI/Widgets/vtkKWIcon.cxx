/*=========================================================================

  Module:    vtkKWIcon.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWIcon.h"

#include "vtkObjectFactory.h"
#include "vtkKWResourceUtilities.h"

#include "Resources/vtkKWIconResources.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWIcon );
vtkCxxRevisionMacro(vtkKWIcon, "1.36");

//----------------------------------------------------------------------------
vtkKWIcon::vtkKWIcon()
{
  this->Data         = 0;
  this->Width        = 0;
  this->Height       = 0;
  this->PixelSize    = 0;
}

//----------------------------------------------------------------------------
vtkKWIcon::~vtkKWIcon()
{
  this->SetData(0, 0, 0, 0);
}

//----------------------------------------------------------------------------
void vtkKWIcon::DeepCopy(vtkKWIcon* icon)
{
  this->SetImage(icon);
}

//----------------------------------------------------------------------------
void vtkKWIcon::SetImage(vtkKWIcon* icon)
{
  if (!icon)
    {
    vtkErrorMacro("No icon specified");
    return;
    }

  this->SetData(icon->GetData(), 
                icon->GetWidth(), icon->GetHeight(), 
                icon->GetPixelSize());
}

//----------------------------------------------------------------------------
void vtkKWIcon::SetImage(const unsigned char *data, 
                         int width, int height, int pixel_size, 
                         unsigned long buffer_length,
                         int options)
{
  unsigned long nb_of_raw_bytes = width * height * pixel_size;
  if (!buffer_length)
    {
    buffer_length = nb_of_raw_bytes;
    }

  // Is the data encoded (zlib and/or base64) ?

  unsigned char *decoded_data = NULL;
  if (buffer_length && buffer_length != nb_of_raw_bytes)
    {
    if (!vtkKWResourceUtilities::DecodeBuffer(
          data, buffer_length, &decoded_data, nb_of_raw_bytes))
      {
      vtkErrorMacro("Error while decoding icon pixels");
      return;
      }
    data = decoded_data;
    }

  if (data)
    {
    this->SetData(data, width, height, pixel_size, options);
    }

  if (decoded_data)
    {
    delete [] decoded_data;
    }
}


//----------------------------------------------------------------------------
void vtkKWIcon::SetData(const unsigned char *data, 
                        int width, int height, 
                        int pixel_size,
                        int options)
{
  unsigned long stride = width * pixel_size;
  unsigned long buffer_length = stride * height;
  if (data && buffer_length > 0)
    {
    this->Width  = width;
    this->Height = height;
    this->PixelSize = pixel_size;
    unsigned char *new_data = new unsigned char [buffer_length];
    if (options & vtkKWIcon::ImageOptionFlipVertical)
      {
      const unsigned char *src = data + buffer_length - stride;
      unsigned char *dest = new_data;
      unsigned char *dest_end = dest + buffer_length;
      while (dest < dest_end)
        {
        memcpy(dest, src, stride);
        dest += stride;
        src -= stride;
        }
      }
    else
      {
      memcpy(new_data, data, buffer_length);
      }
    delete [] this->Data;
    this->Data = new_data;
    }
  else
    {
    delete [] this->Data;
    this->Data         = NULL;
    this->Width        = 0;
    this->Height       = 0;
    this->PixelSize    = 0;
    }
}

//----------------------------------------------------------------------------
void vtkKWIcon::SetImage(int image)
{
  this->SetData(0, 0, 0, 0);

  if (image == vtkKWIcon::IconNoIcon)
    {
    return;
    }
  
  switch (image)
    {
    case vtkKWIcon::IconAngleTool:
      this->SetImage(
        image_angle_tool, 
        image_angle_tool_width, image_angle_tool_height,
        image_angle_tool_pixel_size, 
        image_angle_tool_length);
      break;

    case vtkKWIcon::IconBiDimensionalTool:
      this->SetImage(
        image_bidimensional_tool, 
        image_bidimensional_tool_width, image_bidimensional_tool_height,
        image_bidimensional_tool_pixel_size, 
        image_bidimensional_tool_length);
      break;

    case vtkKWIcon::IconBoundingBox:
      this->SetImage(
        image_bounding_box, 
        image_bounding_box_width, image_bounding_box_height,
        image_bounding_box_pixel_size, 
        image_bounding_box_length);
      break;

    case vtkKWIcon::IconCamera:
      this->SetImage(
        image_camera, 
        image_camera_width, image_camera_height,
        image_camera_pixel_size, 
        image_camera_length);
      break;

    case vtkKWIcon::IconColorBarAnnotation:
      this->SetImage(
        image_color_bar_annotation, 
        image_color_bar_annotation_width, image_color_bar_annotation_height,
        image_color_bar_annotation_pixel_size, 
        image_color_bar_annotation_length);
      break;

    case vtkKWIcon::IconColorSquares:
      this->SetImage(
        image_color_squares, 
        image_color_squares_width, image_color_squares_height,
        image_color_squares_pixel_size, 
        image_color_squares_length);
      break;

    case vtkKWIcon::IconConnection:
      this->SetImage(
        image_connection, 
        image_connection_width, image_connection_height,
        image_connection_pixel_size, 
        image_connection_length);
      break;

    case vtkKWIcon::IconContourTool:
      this->SetImage(
        image_contour_tool, 
        image_contour_tool_width, image_contour_tool_height,
        image_contour_tool_pixel_size, 
        image_contour_tool_length);
      break;

    case vtkKWIcon::IconContourSegment:
      this->SetImage(
        image_contour_segment, 
        image_contour_segment_width, image_contour_segment_height,
        image_contour_segment_pixel_size, 
        image_contour_segment_length);
      break;

    case vtkKWIcon::IconCornerAnnotation:
      this->SetImage(
        image_corner_annotation, 
        image_corner_annotation_width, image_corner_annotation_height,
        image_corner_annotation_pixel_size, 
        image_corner_annotation_length);
      break;

    case vtkKWIcon::IconCropTool:
      this->SetImage(
        image_crop_tool, 
        image_crop_tool_width, image_crop_tool_height,
        image_crop_tool_pixel_size, 
        image_crop_tool_length);
      break;

    case vtkKWIcon::IconDistanceTool:
      this->SetImage(
        image_distance_tool, 
        image_distance_tool_width, image_distance_tool_height,
        image_distance_tool_pixel_size, 
        image_distance_tool_length);
      break;

    case vtkKWIcon::IconDocument:
      this->SetImage(
        image_document, 
        image_document_width, image_document_height,
        image_document_pixel_size, 
        image_document_length);
      break;

    case vtkKWIcon::IconEmpty16x16:
      this->SetImage(
        image_empty_16x16, 
        image_empty_16x16_width, image_empty_16x16_height,
        image_empty_16x16_pixel_size, 
        image_empty_16x16_length);
      break;

    case vtkKWIcon::IconEmpty1x1:
      this->SetImage(
        image_empty_1x1, 
        image_empty_1x1_width, image_empty_1x1_height,
        image_empty_1x1_pixel_size, 
        image_empty_1x1_length);
      break;

    case vtkKWIcon::IconError:
      this->SetImage(
        image_error, 
        image_error_width, image_error_height,
        image_error_pixel_size, 
        image_error_length);
      break;

    case vtkKWIcon::IconExpand:
      this->SetImage(
        image_expand, 
        image_expand_width, image_expand_height,
        image_expand_pixel_size, 
        image_expand_length);
      break;

    case vtkKWIcon::IconExpandMini:
      this->SetImage(
        image_expand_mini, 
        image_expand_mini_width, image_expand_mini_height,
        image_expand_mini_pixel_size, 
        image_expand_mini_length);
      break;

    case vtkKWIcon::IconEye:
      this->SetImage(
        image_eye, 
        image_eye_width, image_eye_height,
        image_eye_pixel_size, 
        image_eye_length);
      break;

    case vtkKWIcon::IconFileOpen:
      this->SetImage(
        image_file_open, 
        image_file_open_width, image_file_open_height,
        image_file_open_pixel_size, 
        image_file_open_length);
      break;

    case vtkKWIcon::IconFloppy:
      this->SetImage(
        image_floppy, 
        image_floppy_width, image_floppy_height,
        image_floppy_pixel_size, 
        image_floppy_length);
      break;

    case vtkKWIcon::IconFolder:
      this->SetImage(
        image_folder, 
        image_folder_width, image_folder_height,
        image_folder_pixel_size, 
        image_folder_length);
      break;

    case vtkKWIcon::IconFolderOpen:
      this->SetImage(
        image_folder_open, 
        image_folder_open_width, image_folder_open_height,
        image_folder_open_pixel_size, 
        image_folder_open_length);
      break;

    case vtkKWIcon::IconGridLinear:
      this->SetImage(
        image_grid_linear, 
        image_grid_linear_width, image_grid_linear_height,
        image_grid_linear_pixel_size, 
        image_grid_linear_length);
      break;      

    case vtkKWIcon::IconGridLog:
      this->SetImage(
        image_grid_log, 
        image_grid_log_width, image_grid_log_height,
        image_grid_log_pixel_size, 
        image_grid_log_length);
      break;      

    case vtkKWIcon::IconHeaderAnnotation:
      this->SetImage(
        image_header_annotation, 
        image_header_annotation_width, image_header_annotation_height,
        image_header_annotation_pixel_size, 
        image_header_annotation_length);
      break;

    case vtkKWIcon::IconHelpBubble:
      this->SetImage(
        image_helpbubble, 
        image_helpbubble_width, image_helpbubble_height,
        image_helpbubble_pixel_size, 
        image_helpbubble_length);
      break;      

    case vtkKWIcon::IconInfoMini:
      this->SetImage(
        image_info_mini, 
        image_info_mini_width, image_info_mini_height,
        image_info_mini_pixel_size, 
        image_info_mini_length);
      break;

    case vtkKWIcon::IconLock:
      this->SetImage(
        image_lock, 
        image_lock_width, image_lock_height,
        image_lock_pixel_size, 
        image_lock_length);
      break;

    case vtkKWIcon::IconMagGlass:
      this->SetImage(
        image_mag_glass,
        image_mag_glass_width, image_mag_glass_height,
        image_mag_glass_pixel_size,
        image_mag_glass_length);
      break;

    case vtkKWIcon::IconMinus:
      this->SetImage(
        image_minus, 
        image_minus_width, image_minus_height,
        image_minus_pixel_size, 
        image_minus_length);
      break;      

    case vtkKWIcon::IconMove:
      this->SetImage(
        image_move, 
        image_move_width, image_move_height,
        image_move_pixel_size, 
        image_move_length);
      break;      

    case vtkKWIcon::IconMoveH:
      this->SetImage(
        image_move_h, 
        image_move_h_width, image_move_h_height,
        image_move_h_pixel_size, 
        image_move_h_length);
      break;      

    case vtkKWIcon::IconMoveV:
      this->SetImage(
        image_move_v, 
        image_move_v_width, image_move_v_height,
        image_move_v_pixel_size, 
        image_move_v_length);
      break;      

    case vtkKWIcon::IconOrientationCubeAnnotation:
      this->SetImage(
        image_orientation_cube_annotation, 
        image_orientation_cube_annotation_width, 
        image_orientation_cube_annotation_height,
        image_orientation_cube_annotation_pixel_size, 
        image_orientation_cube_annotation_length);
      break;

    case vtkKWIcon::IconPanHand:
      this->SetImage(
        image_pan_hand, 
        image_pan_hand_width, image_pan_hand_height,
        image_pan_hand_pixel_size, 
        image_pan_hand_length);
      break;      

    case vtkKWIcon::IconPlus:
      this->SetImage(
        image_plus, 
        image_plus_width, image_plus_height,
        image_plus_pixel_size, 
        image_plus_length);
      break;      

    case vtkKWIcon::IconPointFinger:
      this->SetImage(
        image_point_finger, 
        image_point_finger_width, image_point_finger_height,
        image_point_finger_pixel_size, 
        image_point_finger_length);
      break;      

    case vtkKWIcon::IconPresetAdd:
      this->SetImage(
        image_preset_add, 
        image_preset_add_width, image_preset_add_height,
        image_preset_add_pixel_size, 
        image_preset_add_length);
      break;      

    case vtkKWIcon::IconPresetApply:
      this->SetImage(
        image_preset_apply, 
        image_preset_apply_width, image_preset_apply_height,
        image_preset_apply_pixel_size, 
        image_preset_apply_length);
      break;      

    case vtkKWIcon::IconPresetDelete:
      this->SetImage(
        image_preset_delete, 
        image_preset_delete_width, image_preset_delete_height,
        image_preset_delete_pixel_size, 
        image_preset_delete_length);
      break;      

    case vtkKWIcon::IconPresetEmail:
      this->SetImage(
        image_preset_email, 
        image_preset_email_width, image_preset_email_height,
        image_preset_email_pixel_size, 
        image_preset_email_length);
      break;      

    case vtkKWIcon::IconPresetLocate:
      this->SetImage(
        image_preset_locate, 
        image_preset_locate_width, image_preset_locate_height,
        image_preset_locate_pixel_size, 
        image_preset_locate_length);
      break;      

    case vtkKWIcon::IconPresetUpdate:
      this->SetImage(
        image_preset_update, 
        image_preset_update_width, image_preset_update_height,
        image_preset_update_pixel_size, 
        image_preset_update_length);
      break;      

    case vtkKWIcon::IconQuestion:
      this->SetImage(
        image_question, 
        image_question_width, image_question_height,
        image_question_pixel_size, 
        image_question_length);
      break;

    case vtkKWIcon::IconReload:
      this->SetImage(
        image_reload, 
        image_reload_width, image_reload_height,
        image_reload_pixel_size, 
        image_reload_length);
      break;

    case vtkKWIcon::IconRotate:
      this->SetImage(
        image_rotate, 
        image_rotate_width, image_rotate_height,
        image_rotate_pixel_size, 
        image_rotate_length);
      break;

    case vtkKWIcon::IconScaleBarAnnotation:
      this->SetImage(
        image_scale_bar_annotation, 
        image_scale_bar_annotation_width, image_scale_bar_annotation_height,
        image_scale_bar_annotation_pixel_size, 
        image_scale_bar_annotation_length);
      break;

    case vtkKWIcon::IconSideAnnotation:
      this->SetImage(
        image_side_annotation, 
        image_side_annotation_width, image_side_annotation_height,
        image_side_annotation_pixel_size, 
        image_side_annotation_length);
      break;

    case vtkKWIcon::IconSpinDown:
      this->SetImage(
        image_spin_down, 
        image_spin_down_width, image_spin_down_height,
        image_spin_down_pixel_size, 
        image_spin_down_length);
      break;

    case vtkKWIcon::IconSpinLeft:
      this->SetImage(
        image_spin_left, 
        image_spin_left_width, image_spin_left_height,
        image_spin_left_pixel_size, 
        image_spin_left_length);
      break;

    case vtkKWIcon::IconSpinRight:
      this->SetImage(
        image_spin_right, 
        image_spin_right_width, image_spin_right_height,
        image_spin_right_pixel_size, 
        image_spin_right_length);
      break;

    case vtkKWIcon::IconSpinUp:
      this->SetImage(
        image_spin_up, 
        image_spin_up_width, image_spin_up_height,
        image_spin_up_pixel_size, 
        image_spin_up_length);
      break;

    case vtkKWIcon::IconShrink:
      this->SetImage(
        image_shrink, 
        image_shrink_width, image_shrink_height,
        image_shrink_pixel_size, 
        image_shrink_length);
      break;

    case vtkKWIcon::IconErrorMini:
      this->SetImage(
        image_error_mini, 
        image_error_mini_width, image_error_mini_height,
        image_error_mini_pixel_size, 
        image_error_mini_length);
      break;

    case vtkKWIcon::IconErrorRedMini:
      this->SetImage(
        image_error_red_mini, 
        image_error_red_mini_width, image_error_red_mini_height,
        image_error_red_mini_pixel_size, 
        image_error_red_mini_length);
      break;

    case vtkKWIcon::IconStopwatch:
      this->SetImage(
        image_stopwatch, 
        image_stopwatch_width, image_stopwatch_height,
        image_stopwatch_pixel_size, 
        image_stopwatch_length);
      break;
      
    case vtkKWIcon::IconTransportBeginning:
      this->SetImage(
        image_transport_beginning, 
        image_transport_beginning_width, image_transport_beginning_height,
        image_transport_beginning_pixel_size, 
        image_transport_beginning_length);
      break;
      
    case vtkKWIcon::IconTransportEnd:
      this->SetImage(
        image_transport_end, 
        image_transport_end_width, image_transport_end_height,
        image_transport_end_pixel_size, 
        image_transport_end_length);
      break;
      
    case vtkKWIcon::IconTransportFastForward:
      this->SetImage(
        image_transport_fast_forward, 
        image_transport_fast_forward_width, 
        image_transport_fast_forward_height,
        image_transport_fast_forward_pixel_size, 
        image_transport_fast_forward_length);
      break;
      
    case vtkKWIcon::IconTransportFastForwardToKey:
      this->SetImage(
        image_transport_fast_forward_to_key, 
        image_transport_fast_forward_to_key_width, 
        image_transport_fast_forward_to_key_height,
        image_transport_fast_forward_to_key_pixel_size, 
        image_transport_fast_forward_to_key_length);
      break;
      
    case vtkKWIcon::IconTransportLoop:
      this->SetImage(
        image_transport_loop, 
        image_transport_loop_width, 
        image_transport_loop_height,
        image_transport_loop_pixel_size, 
        image_transport_loop_length);
      break;
      
    case vtkKWIcon::IconTransportPause:
      this->SetImage(
        image_transport_pause, 
        image_transport_pause_width, 
        image_transport_pause_height,
        image_transport_pause_pixel_size, 
        image_transport_pause_length);
      break;
      
    case vtkKWIcon::IconTransportPlay:
      this->SetImage(
        image_transport_play, 
        image_transport_play_width, 
        image_transport_play_height,
        image_transport_play_pixel_size, 
        image_transport_play_length);
      break;
      
    case vtkKWIcon::IconTransportPlayToKey:
      this->SetImage(
        image_transport_play_to_key, 
        image_transport_play_to_key_width, 
        image_transport_play_to_key_height,
        image_transport_play_to_key_pixel_size, 
        image_transport_play_to_key_length);
      break;
      
    case vtkKWIcon::IconTransportRewind:
      this->SetImage(
        image_transport_rewind, 
        image_transport_rewind_width, 
        image_transport_rewind_height,
        image_transport_rewind_pixel_size, 
        image_transport_rewind_length);
      break;
      
    case vtkKWIcon::IconTransportRewindToKey:
      this->SetImage(
        image_transport_rewind_to_key, 
        image_transport_rewind_to_key_width, 
        image_transport_rewind_to_key_height,
        image_transport_rewind_to_key_pixel_size, 
        image_transport_rewind_to_key_length);
      break;
      
    case vtkKWIcon::IconTransportStop:
      this->SetImage(
        image_transport_stop, 
        image_transport_stop_width, 
        image_transport_stop_height,
        image_transport_stop_pixel_size, 
        image_transport_stop_length);
      break;
      
    case vtkKWIcon::IconTrashcan:
      this->SetImage(
        image_trashcan, 
        image_trashcan_width, image_trashcan_height,
        image_trashcan_pixel_size, 
        image_trashcan_length);
      break;
      
    case vtkKWIcon::IconTreeClose:
      this->SetImage(
        image_tree_close, 
        image_tree_close_width, image_tree_close_height,
        image_tree_close_pixel_size, 
        image_tree_close_length);
      break;
      
    case vtkKWIcon::IconTreeOpen:
      this->SetImage(
        image_tree_open, 
        image_tree_open_width, image_tree_open_height,
        image_tree_open_pixel_size, 
        image_tree_open_length);
      break;
      
    case vtkKWIcon::IconWarning:
      this->SetImage(
        image_warning, 
        image_warning_width, image_warning_height,
        image_warning_pixel_size, 
        image_warning_length);
      break;

    case vtkKWIcon::IconWarningMini:
      this->SetImage(
        image_warning_mini, 
        image_warning_mini_width, image_warning_mini_height,
        image_warning_mini_pixel_size, 
        image_warning_mini_length);
      break;

    case vtkKWIcon::IconWindowLevel:
      this->SetImage(
        image_window_level, 
        image_window_level_width, image_window_level_height,
        image_window_level_pixel_size, 
        image_window_level_length);
      break;
    }
}

//----------------------------------------------------------------------------
const unsigned char* vtkKWIcon::GetData()
{
  return this->Data;
}

//----------------------------------------------------------------------------
void vtkKWIcon::Fade(double factor)
{
  if (!this->Data || 
      this->Width == 0 || 
      this->Height == 0 || 
      this->PixelSize != 4)
    {
    return;
    }

  unsigned long data_length = this->Width * this->Height * this->PixelSize;
  unsigned char *data_ptr = this->Data;
  const unsigned char *data_ptr_end = this->Data + data_length;

  data_ptr += 3;
  while (data_ptr < data_ptr_end)
    {
    *data_ptr = (unsigned char)((double)(*data_ptr) * factor);
    data_ptr += this->PixelSize;
    }
}

//----------------------------------------------------------------------------
void vtkKWIcon::Flatten(double r, double g, double b)
{
  if (!this->Data || 
      this->Width == 0 || 
      this->Height == 0 || 
      this->PixelSize != 4)
    {
    return;
    }

  unsigned long data_length = this->Width * this->Height * this->PixelSize;
  unsigned char *data_ptr = this->Data;
  const unsigned char *data_ptr_end = this->Data + data_length;

  unsigned long new_data_length = this->Width * this->Height * 3;
  unsigned char *new_data = new unsigned char [new_data_length];
  unsigned char *new_data_ptr = new_data;

  unsigned char rc = (unsigned char)(r * 255.0);
  unsigned char gc = (unsigned char)(g * 255.0);
  unsigned char bc = (unsigned char)(b * 255.0);

  while (data_ptr < data_ptr_end)
    {
    double alpha = static_cast<double>(*(data_ptr + 3)) / 255.0;
    *new_data_ptr++ = 
      static_cast<unsigned char>(rc * (1 - alpha) + *data_ptr++ * alpha);
    *new_data_ptr++ = 
      static_cast<unsigned char>(gc * (1 - alpha) + *data_ptr++ * alpha);
    *new_data_ptr++ = 
      static_cast<unsigned char>(bc * (1 - alpha) + *data_ptr++ * alpha);
    data_ptr++;
    }

  this->SetImage(new_data, this->Width, this->Height, 3, new_data_length);

  delete [] new_data;
}

//----------------------------------------------------------------------------
void vtkKWIcon::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Width:  " << this->GetWidth() << endl
     << indent << "Height: " << this->GetHeight() << endl
     << indent << "PixelSize: " << this->GetPixelSize() << endl;
}



