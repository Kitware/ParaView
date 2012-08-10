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

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"

#include <map>

class vtkPVHardwareSelector::vtkInternals
{
public:
  typedef std::map<void*, int> PropMapType;
  PropMapType PropMap;
};

vtkStandardNewMacro(vtkPVHardwareSelector);
//----------------------------------------------------------------------------
vtkPVHardwareSelector::vtkPVHardwareSelector()
{
  this->SetUseProcessIdFromData(true);
  this->ProcessID = 0;
  this->UniqueId = 0;
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkPVHardwareSelector::~vtkPVHardwareSelector()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
bool vtkPVHardwareSelector::PassRequired(int pass)
{
  return (pass == PROCESS_PASS?  true : this->Superclass::PassRequired(pass));
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
int vtkPVHardwareSelector::AssignUniqueId(vtkProp* prop)
{
  int id = this->UniqueId;
  this->UniqueId++;
  this->Internals->PropMap[prop] = id;
  return id;
}

//----------------------------------------------------------------------------
int vtkPVHardwareSelector::GetPropID(int vtkNotUsed(idx), vtkProp* prop)
{
  vtkInternals::PropMapType::iterator iter = this->Internals->PropMap.find(prop);
  return (iter != this->Internals->PropMap.end() ?
    iter->second : -1);
}

//----------------------------------------------------------------------------
void vtkPVHardwareSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
