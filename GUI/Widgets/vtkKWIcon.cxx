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

#include "vtkImageConstantPad.h"
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkObjectFactory.h"
#include "vtkBase64Utilities.h"

#if ((VTK_MAJOR_VERSION <= 4) && (VTK_MINOR_VERSION <= 4))
#include "zlib.h"
#else
#include "vtk_zlib.h"
#endif

#include "Resources/vtkKWIconResources.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWIcon );
vtkCxxRevisionMacro(vtkKWIcon, "1.5");

//----------------------------------------------------------------------------
vtkKWIcon::vtkKWIcon()
{
  this->Data         = 0;
  this->InternalData = 0;
  this->Width        = 0;
  this->Height       = 0;
  this->PixelSize    = 0;
  this->Internal     = 0;
}

//----------------------------------------------------------------------------
vtkKWIcon::~vtkKWIcon()
{
  this->SetData(0, 0, 0, 0);
}

//----------------------------------------------------------------------------
void vtkKWIcon::SetImage(vtkImageData* id)
{
  if (!id )
    {
    vtkErrorMacro("No image data specified");
    return;
    }
  id->Update();

  int *ext = id->GetWholeExtent();
  if ((ext[5] - ext[4]) > 0)
    {
    vtkErrorMacro("Can only handle 2D image data");
    return;
    }

  int width  = ext[1] - ext[0]+1;
  int height = ext[3] - ext[2]+1;
  int components = id->GetNumberOfScalarComponents();

  vtkImageData *image = id;
  image->Register(this);
  if (components < 4)
    {
    image->UnRegister(this);
    vtkImageConstantPad *pad = vtkImageConstantPad::New();
    pad->SetInput(id);
    pad->SetConstant(255);
    pad->SetOutputNumberOfScalarComponents(4);
    pad->Update();
    image = pad->GetOutput();
    image->Register(this);
    pad->Delete();
    }

  vtkImageFlip *flip = vtkImageFlip::New();
  flip->SetInput(image);
  flip->SetFilteredAxis(1);
  flip->Update();
  image->UnRegister(this);

  this->SetData(
    static_cast<unsigned char*>(flip->GetOutput()->GetScalarPointer()),
    width, height, 4);

  flip->Delete();
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
                icon->GetWidth(), icon->GetHeight(), icon->GetPixelSize());
}

//----------------------------------------------------------------------------
void vtkKWIcon::SetImage(const unsigned char *data, 
                         int width, int height, int pixel_size, 
                         unsigned long buffer_length)
{
  unsigned long nb_of_raw_bytes = width * height * pixel_size;
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
      buffer_length = vtkBase64Utilities::Decode(data_ptr, 0, 
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
    this->SetData(data_ptr, width, height, pixel_size);
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
                        int width, int height, int pixel_size)
{
  if (this->Data || this->InternalData)
    {
    if (this->Data)
      {
      delete [] this->Data;
      }
    this->Data         = 0;
    this->InternalData = 0;
    this->Width        = 0;
    this->Height       = 0;
    this->PixelSize    = 0;
    }

  int len = width * height * pixel_size;
  if (data && len > 0)
    {
    this->Width  = width;
    this->Height = height;
    this->PixelSize = pixel_size;
    this->Data = new unsigned char [len];
    memcpy(this->Data, data, len);
    }
}

//----------------------------------------------------------------------------
void vtkKWIcon::SetInternalData(const unsigned char *data, 
                                int width, int height, int pixel_size)
{
  if (this->Data || this->InternalData)
    {
    if (this->Data)
      {
      delete [] this->Data;
      }
    this->Data         = 0;
    this->InternalData = 0;
    this->Width        = 0;
    this->Height       = 0;
    this->PixelSize    = 0;
    }

  int len = width * height * pixel_size;
  if (data && len > 0)
    {
    this->Width  = width;
    this->Height = height;
    this->PixelSize = pixel_size;
    this->InternalData = data;
    }
}

//----------------------------------------------------------------------------
void vtkKWIcon::SetImage(int image)
{
  if (this->Internal == image)
    {
    return;
    }

  this->SetData(0, 0, 0, 0);

  if (image == vtkKWIcon::ICON_NOICON)
    {
    return;
    }
  
  switch (image)
    {
    case vtkKWIcon::ICON_ANNOTATE:
      this->SetImage(
        image_annotate, 
        image_annotate_width, image_annotate_height,
        image_annotate_pixel_size, 
        image_annotate_buffer_length);
      break;

    case vtkKWIcon::ICON_AXES:
      this->SetImage(
        image_axes, 
        image_axes_width, image_axes_height,
        image_axes_pixel_size, 
        image_axes_buffer_length);
      break;

    case vtkKWIcon::ICON_CONNECTION:
      this->SetImage(
        image_connection, 
        image_connection_width, image_connection_height,
        image_connection_pixel_size, 
        image_connection_buffer_length);
      break;

    case vtkKWIcon::ICON_CONTOURS:
      this->SetImage(
        image_contours, 
        image_contours_width, image_contours_height,
        image_contours_pixel_size, 
        image_contours_buffer_length);
      break;

    case vtkKWIcon::ICON_CUT:
      this->SetImage(
        image_cut, 
        image_cut_width, image_cut_height,
        image_cut_pixel_size, 
        image_cut_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_AIF:
      this->SetImage(
        image_doc_aif, 
        image_doc_aif_width, image_doc_aif_height,
        image_doc_aif_pixel_size, 
        image_doc_aif_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_ASF:
      this->SetImage(
        image_doc_asf, 
        image_doc_asf_width, image_doc_asf_height,
        image_doc_asf_pixel_size, 
        image_doc_asf_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_AVI:
      this->SetImage(
        image_doc_avi, 
        image_doc_avi_width, image_doc_avi_height,
        image_doc_avi_pixel_size, 
        image_doc_avi_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_BMP:
      this->SetImage(
        image_doc_bmp, 
        image_doc_bmp_width, image_doc_bmp_height,
        image_doc_bmp_pixel_size, 
        image_doc_bmp_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_CHM:
      this->SetImage(
        image_doc_chm, 
        image_doc_chm_width, image_doc_chm_height,
        image_doc_chm_pixel_size, 
        image_doc_chm_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_EXE:
      this->SetImage(
        image_doc_exe, 
        image_doc_exe_width, image_doc_exe_height,
        image_doc_exe_pixel_size, 
        image_doc_exe_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_GIF:
      this->SetImage(
        image_doc_gif, 
        image_doc_gif_width, image_doc_gif_height,
        image_doc_gif_pixel_size, 
        image_doc_gif_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_HLP:
      this->SetImage(
        image_doc_hlp, 
        image_doc_hlp_width, image_doc_hlp_height,
        image_doc_hlp_pixel_size, 
        image_doc_hlp_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_HTTP:
      this->SetImage(
        image_doc_http, 
        image_doc_http_width, image_doc_http_height,
        image_doc_http_pixel_size, 
        image_doc_http_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_JPG:
      this->SetImage(
        image_doc_jpg, 
        image_doc_jpg_width, image_doc_jpg_height,
        image_doc_jpg_pixel_size, 
        image_doc_jpg_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_MP3:
      this->SetImage(
        image_doc_mp3, 
        image_doc_mp3_width, image_doc_mp3_height,
        image_doc_mp3_pixel_size, 
        image_doc_mp3_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_MPEG:
      this->SetImage(
        image_doc_mpeg, 
        image_doc_mpeg_width, image_doc_mpeg_height,
        image_doc_mpeg_pixel_size, 
        image_doc_mpeg_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_MSI:
      this->SetImage(
        image_doc_msi, 
        image_doc_msi_width, image_doc_msi_height,
        image_doc_msi_pixel_size, 
        image_doc_msi_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_PDF:
      this->SetImage(
        image_doc_pdf, 
        image_doc_pdf_width, image_doc_pdf_height,
        image_doc_pdf_pixel_size, 
        image_doc_pdf_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_PNG:
      this->SetImage(
        image_doc_png, 
        image_doc_png_width, image_doc_png_height,
        image_doc_png_pixel_size, 
        image_doc_png_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_POSTSCRIPT:
      this->SetImage(
        image_doc_postscript, 
        image_doc_postscript_width, image_doc_postscript_height,
        image_doc_postscript_pixel_size, 
        image_doc_postscript_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_POWERPOINT:
      this->SetImage(
        image_doc_powerpoint, 
        image_doc_powerpoint_width, image_doc_powerpoint_height,
        image_doc_powerpoint_pixel_size, 
        image_doc_powerpoint_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_QUICKTIME:
      this->SetImage(
        image_doc_quicktime, 
        image_doc_quicktime_width, image_doc_quicktime_height,
        image_doc_quicktime_pixel_size, 
        image_doc_quicktime_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_REALAUDIO:
      this->SetImage(
        image_doc_realaudio, 
        image_doc_realaudio_width, image_doc_realaudio_height,
        image_doc_realaudio_pixel_size, 
        image_doc_realaudio_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_TGA:
      this->SetImage(
        image_doc_tga, 
        image_doc_tga_width, image_doc_tga_height,
        image_doc_tga_pixel_size, 
        image_doc_tga_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_TIF:
      this->SetImage(
        image_doc_tif, 
        image_doc_tif_width, image_doc_tif_height,
        image_doc_tif_pixel_size, 
        image_doc_tif_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_TXT:
      this->SetImage(
        image_doc_txt, 
        image_doc_txt_width, image_doc_txt_height,
        image_doc_txt_pixel_size, 
        image_doc_txt_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_WAV:
      this->SetImage(
        image_doc_wav, 
        image_doc_wav_width, image_doc_wav_height,
        image_doc_wav_pixel_size, 
        image_doc_wav_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_WORD:
      this->SetImage(
        image_doc_word, 
        image_doc_word_width, image_doc_word_height,
        image_doc_word_pixel_size, 
        image_doc_word_buffer_length);
      break;

    case vtkKWIcon::ICON_DOC_ZIP:
      this->SetImage(
        image_doc_zip, 
        image_doc_zip_width, image_doc_zip_height,
        image_doc_zip_pixel_size, 
        image_doc_zip_buffer_length);
      break;

    case vtkKWIcon::ICON_ERROR:
      this->SetImage(
        image_error, 
        image_error_width, image_error_height,
        image_error_pixel_size, 
        image_error_buffer_length);
      break;

    case vtkKWIcon::ICON_EXPAND:
      this->SetImage(
        image_expand, 
        image_expand_width, image_expand_height,
        image_expand_pixel_size, 
        image_expand_buffer_length);
      break;

    case vtkKWIcon::ICON_FILTERS:
      this->SetImage(
        image_filters, 
        image_filters_width, image_filters_height,
        image_filters_pixel_size, 
        image_filters_buffer_length);
      break;      

    case vtkKWIcon::ICON_FOLDER:
      this->SetImage(
        image_folder, 
        image_folder_width, image_folder_height,
        image_folder_pixel_size, 
        image_folder_buffer_length);
      break;

    case vtkKWIcon::ICON_FOLDER_OPEN:
      this->SetImage(
        image_folder_open, 
        image_folder_open_width, image_folder_open_height,
        image_folder_open_pixel_size, 
        image_folder_open_buffer_length);
      break;

    case vtkKWIcon::ICON_GENERAL:
      this->SetImage(
        image_general, 
        image_general_width, image_general_height,
        image_general_pixel_size, 
        image_general_buffer_length);
      break;      

    case vtkKWIcon::ICON_GRID_LINEAR:
      this->SetImage(
        image_grid_linear, 
        image_grid_linear_width, image_grid_linear_height,
        image_grid_linear_pixel_size, 
        image_grid_linear_buffer_length);
      break;      

    case vtkKWIcon::ICON_GRID_LOG:
      this->SetImage(
        image_grid_log, 
        image_grid_log_width, image_grid_log_height,
        image_grid_log_pixel_size, 
        image_grid_log_buffer_length);
      break;      

    case vtkKWIcon::ICON_HELPBUBBLE:
      this->SetImage(
        image_helpbubble, 
        image_helpbubble_width, image_helpbubble_height,
        image_helpbubble_pixel_size, 
        image_helpbubble_buffer_length);
      break;      

    case vtkKWIcon::ICON_INFO_MINI:
      this->SetImage(
        image_info_mini, 
        image_info_mini_width, image_info_mini_height,
        image_info_mini_pixel_size, 
        image_info_mini_buffer_length);
      break;

    case vtkKWIcon::ICON_LAYOUT:
      this->SetImage(
        image_layout, 
        image_layout_width, image_layout_height,
        image_layout_pixel_size, 
        image_layout_buffer_length);
      break;

    case vtkKWIcon::ICON_LOCK:
      this->SetImage(
        image_lock, 
        image_lock_width, image_lock_height,
        image_lock_pixel_size, 
        image_lock_buffer_length);
      break;

    case vtkKWIcon::ICON_MACROS:
      this->SetImage(
        image_macros, 
        image_macros_width, image_macros_height,
        image_macros_pixel_size, 
        image_macros_buffer_length);
      break;      

    case vtkKWIcon::ICON_MAG_GLASS:
      this->SetImage(
        image_mag_glass,
        image_mag_glass_width, image_mag_glass_height,
        image_mag_glass_pixel_size,
        image_mag_glass_buffer_length);
      break;

    case vtkKWIcon::ICON_MATERIAL:
      this->SetImage(
        image_material, 
        image_material_width, image_material_height,
        image_material_pixel_size, 
        image_material_buffer_length);
      break;      

    case vtkKWIcon::ICON_MINUS:
      this->SetImage(
        image_minus, 
        image_minus_width, image_minus_height,
        image_minus_pixel_size, 
        image_minus_buffer_length);
      break;      

    case vtkKWIcon::ICON_MOVE:
      this->SetImage(
        image_move, 
        image_move_width, image_move_height,
        image_move_pixel_size, 
        image_move_buffer_length);
      break;      

    case vtkKWIcon::ICON_MOVE_H:
      this->SetImage(
        image_move_h, 
        image_move_h_width, image_move_h_height,
        image_move_h_pixel_size, 
        image_move_h_buffer_length);
      break;      

    case vtkKWIcon::ICON_MOVE_V:
      this->SetImage(
        image_move_v, 
        image_move_v_width, image_move_v_height,
        image_move_v_pixel_size, 
        image_move_v_buffer_length);
      break;      

    case vtkKWIcon::ICON_PLUS:
      this->SetImage(
        image_plus, 
        image_plus_width, image_plus_height,
        image_plus_pixel_size, 
        image_plus_buffer_length);
      break;      

    case vtkKWIcon::ICON_PREFERENCES:
      this->SetImage(
        image_preferences, 
        image_preferences_width, image_preferences_height,
        image_preferences_pixel_size, 
        image_preferences_buffer_length);
      break;

    case vtkKWIcon::ICON_QUESTION:
      this->SetImage(
        image_question, 
        image_question_width, image_question_height,
        image_question_pixel_size, 
        image_question_buffer_length);
      break;

    case vtkKWIcon::ICON_RELOAD:
      this->SetImage(
        image_reload, 
        image_reload_width, image_reload_height,
        image_reload_pixel_size, 
        image_reload_buffer_length);
      break;

    case vtkKWIcon::ICON_SHRINK:
      this->SetImage(
        image_shrink, 
        image_shrink_width, image_shrink_height,
        image_shrink_pixel_size, 
        image_shrink_buffer_length);
      break;

    case vtkKWIcon::ICON_SMALLERROR:
      this->SetImage(
        image_smallerror, 
        image_smallerror_width, image_smallerror_height,
        image_smallerror_pixel_size, 
        image_smallerror_buffer_length);
      break;

    case vtkKWIcon::ICON_SMALLERRORRED:
      this->SetImage(
        image_smallerrorred, 
        image_smallerrorred_width, image_smallerrorred_height,
        image_smallerrorred_pixel_size, 
        image_smallerrorred_buffer_length);
      break;

    case vtkKWIcon::ICON_STOPWATCH:
      this->SetImage(
        image_stopwatch, 
        image_stopwatch_width, image_stopwatch_height,
        image_stopwatch_pixel_size, 
        image_stopwatch_buffer_length);
      break;
      
    case vtkKWIcon::ICON_TRANSFER:
      this->SetImage(
        image_transfer, 
        image_transfer_width, image_transfer_height,
        image_transfer_pixel_size, 
        image_transfer_buffer_length);
      break;
      
    case vtkKWIcon::ICON_TRANSPORT_BEGINNING:
      this->SetImage(
        image_transport_beginning, 
        image_transport_beginning_width, image_transport_beginning_height,
        image_transport_beginning_pixel_size, 
        image_transport_beginning_buffer_length);
      break;
      
    case vtkKWIcon::ICON_TRANSPORT_END:
      this->SetImage(
        image_transport_end, 
        image_transport_end_width, image_transport_end_height,
        image_transport_end_pixel_size, 
        image_transport_end_buffer_length);
      break;
      
    case vtkKWIcon::ICON_TRANSPORT_FAST_FORWARD:
      this->SetImage(
        image_transport_fast_forward, 
        image_transport_fast_forward_width, 
        image_transport_fast_forward_height,
        image_transport_fast_forward_pixel_size, 
        image_transport_fast_forward_buffer_length);
      break;
      
    case vtkKWIcon::ICON_TRANSPORT_FAST_FORWARD_TO_KEY:
      this->SetImage(
        image_transport_fast_forward_to_key, 
        image_transport_fast_forward_to_key_width, 
        image_transport_fast_forward_to_key_height,
        image_transport_fast_forward_to_key_pixel_size, 
        image_transport_fast_forward_to_key_buffer_length);
      break;
      
    case vtkKWIcon::ICON_TRANSPORT_LOOP:
      this->SetImage(
        image_transport_loop, 
        image_transport_loop_width, 
        image_transport_loop_height,
        image_transport_loop_pixel_size, 
        image_transport_loop_buffer_length);
      break;
      
    case vtkKWIcon::ICON_TRANSPORT_PAUSE:
      this->SetImage(
        image_transport_pause, 
        image_transport_pause_width, 
        image_transport_pause_height,
        image_transport_pause_pixel_size, 
        image_transport_pause_buffer_length);
      break;
      
    case vtkKWIcon::ICON_TRANSPORT_PLAY:
      this->SetImage(
        image_transport_play, 
        image_transport_play_width, 
        image_transport_play_height,
        image_transport_play_pixel_size, 
        image_transport_play_buffer_length);
      break;
      
    case vtkKWIcon::ICON_TRANSPORT_PLAY_TO_KEY:
      this->SetImage(
        image_transport_play_to_key, 
        image_transport_play_to_key_width, 
        image_transport_play_to_key_height,
        image_transport_play_to_key_pixel_size, 
        image_transport_play_to_key_buffer_length);
      break;
      
    case vtkKWIcon::ICON_TRANSPORT_REWIND:
      this->SetImage(
        image_transport_rewind, 
        image_transport_rewind_width, 
        image_transport_rewind_height,
        image_transport_rewind_pixel_size, 
        image_transport_rewind_buffer_length);
      break;
      
    case vtkKWIcon::ICON_TRANSPORT_REWIND_TO_KEY:
      this->SetImage(
        image_transport_rewind_to_key, 
        image_transport_rewind_to_key_width, 
        image_transport_rewind_to_key_height,
        image_transport_rewind_to_key_pixel_size, 
        image_transport_rewind_to_key_buffer_length);
      break;
      
    case vtkKWIcon::ICON_TRANSPORT_STOP:
      this->SetImage(
        image_transport_stop, 
        image_transport_stop_width, 
        image_transport_stop_height,
        image_transport_stop_pixel_size, 
        image_transport_stop_buffer_length);
      break;
      
    case vtkKWIcon::ICON_TRASHCAN:
      this->SetImage(
        image_trashcan, 
        image_trashcan_width, image_trashcan_height,
        image_trashcan_pixel_size, 
        image_trashcan_buffer_length);
      break;
      
    case vtkKWIcon::ICON_TREE_CLOSE:
      this->SetImage(
        image_tree_close, 
        image_tree_close_width, image_tree_close_height,
        image_tree_close_pixel_size, 
        image_tree_close_buffer_length);
      break;
      
    case vtkKWIcon::ICON_TREE_OPEN:
      this->SetImage(
        image_tree_open, 
        image_tree_open_width, image_tree_open_height,
        image_tree_open_pixel_size, 
        image_tree_open_buffer_length);
      break;
      
    case vtkKWIcon::ICON_WARNING:
      this->SetImage(
        image_warning, 
        image_warning_width, image_warning_height,
        image_warning_pixel_size, 
        image_warning_buffer_length);
      break;

    case vtkKWIcon::ICON_WARNING_MINI:
      this->SetImage(
        image_warning_mini, 
        image_warning_mini_width, image_warning_mini_height,
        image_warning_mini_pixel_size, 
        image_warning_mini_buffer_length);
      break;

    case vtkKWIcon::ICON_WINDOW_LEVEL:
      this->SetImage(
        image_window_level, 
        image_window_level_width, image_window_level_height,
        image_window_level_pixel_size, 
        image_window_level_buffer_length);
      break;
    }
  this->Internal = image;
}

//----------------------------------------------------------------------------
const unsigned char* vtkKWIcon::GetData()
{
  if (this->InternalData)
    {
    return this->InternalData;
    }

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



