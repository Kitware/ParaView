/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWIcon.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkKWIcon.h"

#include "vtkImageConstantPad.h"
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkObjectFactory.h"
#include "vtkBase64Utilities.h"
#include "zlib.h"

#include "icons.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWIcon );
vtkCxxRevisionMacro(vtkKWIcon, "1.8");

vtkKWIcon::vtkKWIcon()
{
  this->Data         = 0;
  this->InternalData = 0;
  this->Width        = 0;
  this->Height       = 0;
  this->Internal     = 0;
}

vtkKWIcon::~vtkKWIcon()
{
  this->SetData(0,0,0);
}

void vtkKWIcon::SetImageData(vtkImageData* id)
{
  if (!id )
    {
    vtkErrorMacro("No image data specified");
    return;
    }
  id->Update();

  int *ext = id->GetWholeExtent();
  if ( (ext[5] - ext[4]) > 0 )
    {
    vtkErrorMacro("Can only handle 2D image data");
    return;
    }
  int width  = ext[1] - ext[0]+1;
  int height = ext[3] - ext[2]+1;
  int components = id->GetNumberOfScalarComponents();
  vtkImageData *image = id;
  image->Register(this);
  if ( components < 4 )
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

  vtkImageFlip* flip = vtkImageFlip::New();
  flip->SetInput(image);
  flip->SetFilteredAxis(1);
  flip->Update();
  image->UnRegister(this);

  this->SetData(
    static_cast<unsigned char*>(flip->GetOutput()->GetScalarPointer()),
    width, height);
  flip->Delete();

}

void vtkKWIcon::SetImageData(const unsigned char* pixels, int width, int height, 
                             int pixel_size, unsigned long buffer_length)
{
  unsigned long nb_of_raw_bytes = width * height * pixel_size;
  const unsigned char *data_ptr = pixels;

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
  if ( data_ptr )
    {
    this->SetData(data_ptr, width, height);
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


void vtkKWIcon::SetData(const unsigned char* data, int width, int height)
{
  if ( this->Data || this->InternalData )
    {
    if ( this->Data )
      {
      delete [] this->Data;
      }
    this->Data         = 0;
    this->InternalData = 0;
    this->Width        = 0;
    this->Height       = 0;
    }
  int len = width * height * 4;
  if ( data && len > 0 )
    {
    this->Width  = width;
    this->Height = height;
    this->Data = new unsigned char [ len ];
    memcpy(this->Data, data, len);
    }
}

void vtkKWIcon::SetInternalData(const unsigned char* data, 
                                int width, int height)
{
  if ( this->Data || this->InternalData )
    {
    if ( this->Data )
      {
      delete [] this->Data;
      }
    this->Data         = 0;
    this->InternalData = 0;
    this->Width        = 0;
    this->Height       = 0;
    }
  int len = width * height * 4;
  if ( data && len > 0 )
    {
    this->Width  = width;
    this->Height = height;
    this->InternalData = data;
    }
}

void vtkKWIcon::SetImageData(int image)
{
  if ( this->Internal == image )
    {
    return;
    }
  this->SetData(0,0,0);
  if ( image == vtkKWIcon::ICON_NOICON )
    {
    return;
    }
  
  switch( image )
    {
    case vtkKWIcon::ICON_ANNOTATE:
      this->SetInternalData(image_annotate, image_annotate_width, 
                    image_annotate_height);
      break;
    case vtkKWIcon::ICON_CONTOURS:
      this->SetInternalData(image_contours, image_contours_width, 
                    image_contours_height);
      break;
    case vtkKWIcon::ICON_CUT:
      this->SetInternalData(image_cut, image_cut_width, image_cut_height);
      break;
    case vtkKWIcon::ICON_ERROR:
      this->SetInternalData(image_error, image_error_width, 
                            image_error_height);
      break;
    case vtkKWIcon::ICON_EMPTY:
      this->SetInternalData(image_empty, image_empty_width, 
                            image_empty_height);
      break;
    case vtkKWIcon::ICON_FILTERS:
      this->SetData(image_filters, image_filters_width, image_filters_height);
      break;      
    case vtkKWIcon::ICON_GENERAL:
      this->SetData(image_general, image_general_width, image_general_height);
      break;      
    case vtkKWIcon::ICON_LAYOUT:
      this->SetInternalData(image_layout, image_layout_width, 
                            image_layout_height);
      break;
    case vtkKWIcon::ICON_MACROS:
      this->SetInternalData(image_macros, image_macros_width, 
                            image_macros_height);
      break;      
    case vtkKWIcon::ICON_MATERIAL:
      this->SetInternalData(image_material, image_material_width, 
                            image_material_height);
      break;      
    case vtkKWIcon::ICON_PREFERENCES:
      this->SetInternalData(image_preferences, image_preferences_width, 
                    image_preferences_height);
      break;
    case vtkKWIcon::ICON_QUESTION:
      this->SetInternalData(image_question, image_question_width, 
                    image_question_height);
      break;
    case vtkKWIcon::ICON_TRANSFER:
      this->SetInternalData(image_transfer, image_transfer_width, 
                    image_transfer_height);
      break;
    case vtkKWIcon::ICON_WARNING:
      this->SetInternalData(image_warning, image_warning_width, 
                            image_warning_height);
      break;
    case vtkKWIcon::ICON_SHRINK:
      this->SetInternalData(image_shrink, image_shrink_width, 
                            image_shrink_height);
      break;
    case vtkKWIcon::ICON_EXPAND:
      this->SetInternalData(image_expand, image_expand_width, 
                            image_expand_height);
      break;
    case vtkKWIcon::ICON_SMALLERROR:
      this->SetInternalData(image_smallerror, image_smallerror_width, 
                            image_smallerror_height);
      break;
    case vtkKWIcon::ICON_SMALLERRORRED:
      this->SetInternalData(image_smallerrorred, image_smallerrorred_width, 
                            image_smallerrorred_height);
      break;
    }
  this->Internal = image;
}

const unsigned char* vtkKWIcon::GetData()
{
  if ( this->InternalData )
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
     << indent << "Height: " << this->GetHeight() << endl;
}
