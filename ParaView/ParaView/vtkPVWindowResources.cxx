/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWindowResources.cxx
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
#include "vtkPVWindow.h"

#include "vtkKWLabel.h"
#include "vtkKWTkUtilities.h"
#include "vtkPVApplication.h"

// Logo

#include "Resources/vtkPVLogoSmall.h"

//----------------------------------------------------------------------------
void vtkPVWindow::UpdateStatusImage()
{
  // Tcl/Tk 8.3 seems to have a bug if you update a photo that already
  // contains pixels. Let's create a second photo and replace the existing
  // one.

#if (TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION < 4)
  this->SetStatusImageName(this->Script("image create photo"));
  this->Script("%s configure -image %s", 
               this->StatusImage->GetWidgetName(),
               this->StatusImageName);
#endif

  // Update status image
  
  if (this->StatusImageName && this->GetPVApplication())
    {
    if (!vtkKWTkUtilities::UpdatePhoto(
      this->GetPVApplication()->GetMainInterp(),
      this->StatusImageName,
      image_PVLogoSmall, 
      image_PVLogoSmall_width, 
      image_PVLogoSmall_height,
      image_PVLogoSmall_pixel_size,
      image_PVLogoSmall_buffer_length))
      {
      vtkWarningMacro("Error updating status image!" << this->StatusImageName);
      return;
      }
    }
}
