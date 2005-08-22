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
vtkCxxRevisionMacro(vtkKWIcon, "1.14");

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
  if (this->Data)
    {
    if (this->Data)
      {
      delete [] this->Data;
      }
    this->Data         = 0;
    this->Width        = 0;
    this->Height       = 0;
    this->PixelSize    = 0;
    }

  unsigned long stride = width * pixel_size;
  unsigned long buffer_length = stride * height;
  if (data && buffer_length > 0)
    {
    this->Width  = width;
    this->Height = height;
    this->PixelSize = pixel_size;
    this->Data = new unsigned char [buffer_length];
    if (options & vtkKWIcon::ImageOptionFlipVertical)
      {
      const unsigned char *src = data + buffer_length - stride;
      unsigned char *dest = this->Data;
      unsigned char *dest_end = this->Data + buffer_length;
      while (dest < dest_end)
        {
        memcpy(dest, src, stride);
        dest += stride;
        src -= stride;
        }
      }
    else
      {
      memcpy(this->Data, data, buffer_length);
      }
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
    case vtkKWIcon::IconConnection:
      this->SetImage(
        image_connection, 
        image_connection_width, image_connection_height,
        image_connection_pixel_size, 
        image_connection_length);
      break;

    case vtkKWIcon::IconEmpty16x16:
      this->SetImage(
        image_empty_16x16, 
        image_empty_16x16_width, image_empty_16x16_height,
        image_empty_16x16_pixel_size, 
        image_empty_16x16_length);
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

    case vtkKWIcon::IconPlus:
      this->SetImage(
        image_plus, 
        image_plus_width, image_plus_height,
        image_plus_pixel_size, 
        image_plus_length);
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
void vtkKWIcon::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Width:  " << this->GetWidth() << endl
     << indent << "Height: " << this->GetHeight() << endl
     << indent << "PixelSize: " << this->GetPixelSize() << endl;
}



