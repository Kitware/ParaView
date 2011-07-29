/*=========================================================================

  Program:   ParaView
  Module:    vtkPVHardwareSelector.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVHardwareSelector.h"

#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"

vtkStandardNewMacro(vtkPVHardwareSelector);
//----------------------------------------------------------------------------
vtkPVHardwareSelector::vtkPVHardwareSelector()
{
}

//----------------------------------------------------------------------------
vtkPVHardwareSelector::~vtkPVHardwareSelector()
{
}

//----------------------------------------------------------------------------
vtkSelection* vtkPVHardwareSelector::Select(int region[4])
{
  if (this->NeedToRenderForSelection())
    {
    int* size = this->Renderer->GetSize();
    int* origin = this->Renderer->GetOrigin();
    this->SetArea(origin[0], origin[1], origin[0]+size[0]-1,
      origin[1]+size[1]-1);
    if (this->CaptureBuffers() == false)
      {
      this->CaptureTime.Modified();
      return NULL;
      }
    this->CaptureTime.Modified();
    }

  return this->GenerateSelection(region[0], region[1], region[2], region[3]);
}

//----------------------------------------------------------------------------
bool vtkPVHardwareSelector::NeedToRenderForSelection()
{
  // We rely on external logic to ensure that the MTime for the
  // vtkPVHardwareSelector is explicitly modified when some action happens that
  // would result in invalidation of captured buffers.
  return this->CaptureTime < this->GetMTime();
}

//----------------------------------------------------------------------------
void vtkPVHardwareSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
