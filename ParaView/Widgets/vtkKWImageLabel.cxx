/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWImageLabel.cxx
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
#include "vtkKWApplication.h"
#include "vtkKWImageLabel.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWImageLabel );

vtkKWImageLabel::vtkKWImageLabel()
{
  this->ImageDataLabel = 0;
}

vtkKWImageLabel::~vtkKWImageLabel()
{
  this->SetImageDataLabel(0);
}

void vtkKWImageLabel::Create(vtkKWApplication *app, const char *args)
{
  this->vtkKWLabel::Create(app, args);
}

void vtkKWImageLabel::SetImageData(const unsigned char* data, 
				   int width, int height)
{
  this->Script("winfo rgb %s systemButtonFace", this->GetParent()->GetWidgetName());
  int r, g, b;
  sscanf( this->Application->GetMainInterp()->result, "%d %d %d",
	  &r, &g, &b );
  r = (r / 65535.0)*255;
  g = (g / 65535.0)*255;
  b = (b / 65535.0)*255;
  //cout << "RGB: #" << r << ", " << g << ", " << b << endl;
  this->Script("image create photo -height %d -width %d", width, height);
  this->SetImageDataLabel(this->Application->GetMainInterp()->result);
  Tk_PhotoHandle photo;
  Tk_PhotoImageBlock block;

  block.width = width;
  block.height = height;
  block.pixelSize = 4;
  block.pitch = block.width*block.pixelSize;
  block.offset[0] = 0;
  block.offset[1] = 1;
  block.offset[2] = 2;
  block.pixelPtr = new unsigned char [block.pitch*block.height];

  photo = Tk_FindPhoto(this->Application->GetMainInterp(),
		       this->ImageDataLabel);

  unsigned char *pp = block.pixelPtr;
  const unsigned char *dd = data;
  int cc;
  for ( cc=0; cc < block.width * block.height; cc++ )
    {
    /*
    cout << "Color: " << (unsigned int)*(pp) << ", " 
	 << (unsigned int)*(pp) << ", " << (unsigned int)*(pp) << ", " 
	 << (unsigned int)*(pp) << endl;
    */
    if ( *(dd+3) > 0 )
      {
      *(pp)   = *(dd);
      *(pp+1) = *(dd+1);
      *(pp+2) = *(dd+2);
      *(pp+3) = *(dd+3);
      }
    else
      {
      *(pp)   = r;
      *(pp+1) = g;
      *(pp+2) = b;
      *(pp+3) = 0;
      }
    pp+=4;
    dd+=4;
    }
  Tk_PhotoPutBlock(photo, &block, 0, 0, block.width, block.height);
  delete [] block.pixelPtr;
  this->Script("%s configure -image %s", this->GetWidgetName(),
	       this->ImageDataLabel);
}

