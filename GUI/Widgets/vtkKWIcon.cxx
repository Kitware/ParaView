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

#if ((VTK_MAJOR_VERSION <= 4) && (VTK_MINOR_VERSION <= 4))
#include "zlib.h"
#else
#include "vtk_zlib.h"
#endif

#include "Resources/vtkKWIconResources.h"

#include <vtksys/Base64.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWIcon );
vtkCxxRevisionMacro(vtkKWIcon, "1.11");

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
  const unsigned char *data_ptr = data;

  // If the buffer_lenth has been provided, and if it's different than the
  // expected size of the raw image buffer, than it might have been compressed
  // using zlib and/or encoded in base64. In that case, decode and/or
  // uncompress the buffer.

  int base64 = 0;
  unsigned char *base64_buffer = 0;

  int zlib = 0;
  unsigned char *zlib_buffer = 0;

  if (buffer_length && buffer_length != nb_of_raw_bytes)
    {
    // Is it a base64 stream (i.e. not zlib for the moment) ?

    if (data_ptr[0] != 0x78 || data_ptr[1] != 0xDA)
      {
      base64_buffer = new unsigned char [buffer_length];
      buffer_length = vtksysBase64_Decode(data_ptr, 0, 
                                         base64_buffer, buffer_length);
      if (buffer_length == 0)
        {
        vtkGenericWarningMacro(<< "Error decoding base64 stream");
        delete [] base64_buffer;
        return;
        }
      base64 = 1;
      data_ptr = base64_buffer;
      }
    
    // Is it zlib ?

    if (buffer_length != nb_of_raw_bytes &&
        data_ptr[0] == 0x78 && data_ptr[1] == 0xDA)
      {
      unsigned long zlib_buffer_length = nb_of_raw_bytes;
      zlib_buffer = new unsigned char [zlib_buffer_length];
      if (uncompress(zlib_buffer, &zlib_buffer_length, 
                     data_ptr, buffer_length) != Z_OK ||
          zlib_buffer_length != nb_of_raw_bytes)
        {
        vtkGenericWarningMacro(<< "Error decoding zlib stream");
        delete [] zlib_buffer;
        if (base64)
          {
          delete [] base64_buffer;
          }
        return;
        }
      zlib = 1;
      data_ptr = zlib_buffer;
      }
    }

  if (data_ptr)
    {
    this->SetData(data_ptr, width, height, pixel_size, options);
    }

  if (base64)
    {
    delete [] base64_buffer;
    }

  if (zlib)
    {
    delete [] zlib_buffer;
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
        image_connection_buffer_length);
      break;

    case vtkKWIcon::IconError:
      this->SetImage(
        image_error, 
        image_error_width, image_error_height,
        image_error_pixel_size, 
        image_error_buffer_length);
      break;

    case vtkKWIcon::IconExpand:
      this->SetImage(
        image_expand, 
        image_expand_width, image_expand_height,
        image_expand_pixel_size, 
        image_expand_buffer_length);
      break;

    case vtkKWIcon::IconFolder:
      this->SetImage(
        image_folder, 
        image_folder_width, image_folder_height,
        image_folder_pixel_size, 
        image_folder_buffer_length);
      break;

    case vtkKWIcon::IconFolderOpen:
      this->SetImage(
        image_folder_open, 
        image_folder_open_width, image_folder_open_height,
        image_folder_open_pixel_size, 
        image_folder_open_buffer_length);
      break;

    case vtkKWIcon::IconGridLinear:
      this->SetImage(
        image_grid_linear, 
        image_grid_linear_width, image_grid_linear_height,
        image_grid_linear_pixel_size, 
        image_grid_linear_buffer_length);
      break;      

    case vtkKWIcon::IconGridLog:
      this->SetImage(
        image_grid_log, 
        image_grid_log_width, image_grid_log_height,
        image_grid_log_pixel_size, 
        image_grid_log_buffer_length);
      break;      

    case vtkKWIcon::IconHelpBubble:
      this->SetImage(
        image_helpbubble, 
        image_helpbubble_width, image_helpbubble_height,
        image_helpbubble_pixel_size, 
        image_helpbubble_buffer_length);
      break;      

    case vtkKWIcon::IconInfoMini:
      this->SetImage(
        image_info_mini, 
        image_info_mini_width, image_info_mini_height,
        image_info_mini_pixel_size, 
        image_info_mini_buffer_length);
      break;

    case vtkKWIcon::IconLock:
      this->SetImage(
        image_lock, 
        image_lock_width, image_lock_height,
        image_lock_pixel_size, 
        image_lock_buffer_length);
      break;

    case vtkKWIcon::IconMagGlass:
      this->SetImage(
        image_mag_glass,
        image_mag_glass_width, image_mag_glass_height,
        image_mag_glass_pixel_size,
        image_mag_glass_buffer_length);
      break;

    case vtkKWIcon::IconMinus:
      this->SetImage(
        image_minus, 
        image_minus_width, image_minus_height,
        image_minus_pixel_size, 
        image_minus_buffer_length);
      break;      

    case vtkKWIcon::IconMove:
      this->SetImage(
        image_move, 
        image_move_width, image_move_height,
        image_move_pixel_size, 
        image_move_buffer_length);
      break;      

    case vtkKWIcon::IconMoveH:
      this->SetImage(
        image_move_h, 
        image_move_h_width, image_move_h_height,
        image_move_h_pixel_size, 
        image_move_h_buffer_length);
      break;      

    case vtkKWIcon::IconMoveV:
      this->SetImage(
        image_move_v, 
        image_move_v_width, image_move_v_height,
        image_move_v_pixel_size, 
        image_move_v_buffer_length);
      break;      

    case vtkKWIcon::IconPlus:
      this->SetImage(
        image_plus, 
        image_plus_width, image_plus_height,
        image_plus_pixel_size, 
        image_plus_buffer_length);
      break;      

    case vtkKWIcon::IconQuestion:
      this->SetImage(
        image_question, 
        image_question_width, image_question_height,
        image_question_pixel_size, 
        image_question_buffer_length);
      break;

    case vtkKWIcon::IconReload:
      this->SetImage(
        image_reload, 
        image_reload_width, image_reload_height,
        image_reload_pixel_size, 
        image_reload_buffer_length);
      break;

    case vtkKWIcon::IconShrink:
      this->SetImage(
        image_shrink, 
        image_shrink_width, image_shrink_height,
        image_shrink_pixel_size, 
        image_shrink_buffer_length);
      break;

    case vtkKWIcon::IconErrorMini:
      this->SetImage(
        image_error_mini, 
        image_error_mini_width, image_error_mini_height,
        image_error_mini_pixel_size, 
        image_error_mini_buffer_length);
      break;

    case vtkKWIcon::IconErrorRedMini:
      this->SetImage(
        image_error_red_mini, 
        image_error_red_mini_width, image_error_red_mini_height,
        image_error_red_mini_pixel_size, 
        image_error_red_mini_buffer_length);
      break;

    case vtkKWIcon::IconStopwatch:
      this->SetImage(
        image_stopwatch, 
        image_stopwatch_width, image_stopwatch_height,
        image_stopwatch_pixel_size, 
        image_stopwatch_buffer_length);
      break;
      
    case vtkKWIcon::IconTransportBeginning:
      this->SetImage(
        image_transport_beginning, 
        image_transport_beginning_width, image_transport_beginning_height,
        image_transport_beginning_pixel_size, 
        image_transport_beginning_buffer_length);
      break;
      
    case vtkKWIcon::IconTransportEnd:
      this->SetImage(
        image_transport_end, 
        image_transport_end_width, image_transport_end_height,
        image_transport_end_pixel_size, 
        image_transport_end_buffer_length);
      break;
      
    case vtkKWIcon::IconTransportFastForward:
      this->SetImage(
        image_transport_fast_forward, 
        image_transport_fast_forward_width, 
        image_transport_fast_forward_height,
        image_transport_fast_forward_pixel_size, 
        image_transport_fast_forward_buffer_length);
      break;
      
    case vtkKWIcon::IconTransportFastForwardToKey:
      this->SetImage(
        image_transport_fast_forward_to_key, 
        image_transport_fast_forward_to_key_width, 
        image_transport_fast_forward_to_key_height,
        image_transport_fast_forward_to_key_pixel_size, 
        image_transport_fast_forward_to_key_buffer_length);
      break;
      
    case vtkKWIcon::IconTransportLoop:
      this->SetImage(
        image_transport_loop, 
        image_transport_loop_width, 
        image_transport_loop_height,
        image_transport_loop_pixel_size, 
        image_transport_loop_buffer_length);
      break;
      
    case vtkKWIcon::IconTransportPause:
      this->SetImage(
        image_transport_pause, 
        image_transport_pause_width, 
        image_transport_pause_height,
        image_transport_pause_pixel_size, 
        image_transport_pause_buffer_length);
      break;
      
    case vtkKWIcon::IconTransportPlay:
      this->SetImage(
        image_transport_play, 
        image_transport_play_width, 
        image_transport_play_height,
        image_transport_play_pixel_size, 
        image_transport_play_buffer_length);
      break;
      
    case vtkKWIcon::IconTransportPlayToKey:
      this->SetImage(
        image_transport_play_to_key, 
        image_transport_play_to_key_width, 
        image_transport_play_to_key_height,
        image_transport_play_to_key_pixel_size, 
        image_transport_play_to_key_buffer_length);
      break;
      
    case vtkKWIcon::IconTransportRewind:
      this->SetImage(
        image_transport_rewind, 
        image_transport_rewind_width, 
        image_transport_rewind_height,
        image_transport_rewind_pixel_size, 
        image_transport_rewind_buffer_length);
      break;
      
    case vtkKWIcon::IconTransportRewindToKey:
      this->SetImage(
        image_transport_rewind_to_key, 
        image_transport_rewind_to_key_width, 
        image_transport_rewind_to_key_height,
        image_transport_rewind_to_key_pixel_size, 
        image_transport_rewind_to_key_buffer_length);
      break;
      
    case vtkKWIcon::IconTransportStop:
      this->SetImage(
        image_transport_stop, 
        image_transport_stop_width, 
        image_transport_stop_height,
        image_transport_stop_pixel_size, 
        image_transport_stop_buffer_length);
      break;
      
    case vtkKWIcon::IconTrashcan:
      this->SetImage(
        image_trashcan, 
        image_trashcan_width, image_trashcan_height,
        image_trashcan_pixel_size, 
        image_trashcan_buffer_length);
      break;
      
    case vtkKWIcon::IconTreeClose:
      this->SetImage(
        image_tree_close, 
        image_tree_close_width, image_tree_close_height,
        image_tree_close_pixel_size, 
        image_tree_close_buffer_length);
      break;
      
    case vtkKWIcon::IconTreeOpen:
      this->SetImage(
        image_tree_open, 
        image_tree_open_width, image_tree_open_height,
        image_tree_open_pixel_size, 
        image_tree_open_buffer_length);
      break;
      
    case vtkKWIcon::IconWarning:
      this->SetImage(
        image_warning, 
        image_warning_width, image_warning_height,
        image_warning_pixel_size, 
        image_warning_buffer_length);
      break;

    case vtkKWIcon::IconWarningMini:
      this->SetImage(
        image_warning_mini, 
        image_warning_mini_width, image_warning_mini_height,
        image_warning_mini_pixel_size, 
        image_warning_mini_buffer_length);
      break;

    case vtkKWIcon::IconWindowLevel:
      this->SetImage(
        image_window_level, 
        image_window_level_width, image_window_level_height,
        image_window_level_pixel_size, 
        image_window_level_buffer_length);
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



