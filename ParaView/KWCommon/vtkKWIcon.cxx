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
#include "vtkObjectFactory.h"

#include "icons.h"

#include "vtkPNGWriter.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWIcon );

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

  this->SetData(static_cast<unsigned char*>(image->GetScalarPointer()), 
                width, height);

  image->UnRegister(this);
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
