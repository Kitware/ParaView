/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWindowResources.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
