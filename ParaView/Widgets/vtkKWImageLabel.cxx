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
#include "vtkKWImageLabel.h"

#include "vtkKWApplication.h"
#include "vtkKWIcon.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWImageLabel );
vtkCxxRevisionMacro(vtkKWImageLabel, "1.27");

//----------------------------------------------------------------------------
vtkKWImageLabel::vtkKWImageLabel()
{
  this->ImageDataName = 0;
}

//----------------------------------------------------------------------------
vtkKWImageLabel::~vtkKWImageLabel()
{
  this->SetImageDataName(0);
}

//----------------------------------------------------------------------------
void vtkKWImageLabel::Create(vtkKWApplication *app, const char *args)
{
  this->Superclass::Create(app, args);
}

//----------------------------------------------------------------------------
void vtkKWImageLabel::SetImageData(vtkKWIcon* icon)
{
  if (!icon)
    {
    return;
    }

  this->SetImageData(icon->GetData(), 
                     icon->GetWidth(), 
                     icon->GetHeight(), 
                     icon->GetPixelSize());
}

//----------------------------------------------------------------------------
void vtkKWImageLabel::SetImageData(int image_index)
{
  vtkKWIcon *icon = vtkKWIcon::New();
  icon->SetImageData(image_index);
  this->SetImageData(icon);
  icon->Delete();
}

//----------------------------------------------------------------------------
void vtkKWImageLabel::SetImageData(const unsigned char* data, 
                                   int width, int height,
                                   int pixel_size)
{
  if (!this->IsCreated())
  {
  vtkWarningMacro("Image label is not created yet !");
  return;
  }

#if (TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION < 4)
  // This work-around is put here to "fix" what looks like a bug
  // in Tk. Without this, there seems to be some weird problems
  // with Tk picking some alpha values for some colors.
  this->SetImageDataName(0);
#endif

  if (!this->ImageDataName)
    {
    this->Script("image create photo -width %d -height %d", width, height);
    this->SetImageDataName(this->Application->GetMainInterp()->result);
    }

  if (!vtkKWTkUtilities::UpdatePhoto(this->GetApplication()->GetMainInterp(),
                                     this->ImageDataName,
                                     data, 
                                     width, height, pixel_size,
                                     width * height * pixel_size,
                                     this->GetWidgetName()))
    {
    vtkWarningMacro("Error updating Tk photo " << this->ImageDataName);
    return;
    }
}

//----------------------------------------------------------------------------
void vtkKWImageLabel::SetImageDataName(const char* _arg)
{
  if (this->ImageDataName == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->ImageDataName && _arg && (!strcmp(this->ImageDataName, _arg))) 
    {
    return;
    }

  if (this->ImageDataName) 
    { 
    this->Script("catch {destroy %s}", this->ImageDataName);
    delete [] this->ImageDataName; 
    }

  if (_arg)
    {
    this->ImageDataName = new char[strlen(_arg)+1];
    strcpy(this->ImageDataName, _arg);
    }
  else
    {
    this->ImageDataName = NULL;
    }

  this->Modified();

  if (this->IsAlive())
    {
    if (this->ImageDataName)
      {
      this->Script("%s configure -image {%s}", 
                   this->GetWidgetName(), this->ImageDataName);
      }
    else
      {
      this->Script("%s configure -image {}", this->GetWidgetName());
      }
    }
} 

//----------------------------------------------------------------------------
void vtkKWImageLabel::SetLabel(const char* _arg)
{
  this->Superclass::SetLabel(_arg);

  // Whatever the label, -image always takes precedence, unless it's empty
  // so change it accordingly

  this->SetImageDataName(NULL);
}

//----------------------------------------------------------------------------
void vtkKWImageLabel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ImageDataName: " 
     << ( this->ImageDataName ?  this->ImageDataName : "(none)") << endl;
}
