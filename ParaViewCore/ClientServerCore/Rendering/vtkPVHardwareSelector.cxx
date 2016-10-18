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
#include "vtkPVRenderViewSettings.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"

#include <map>

#include "vtkProcessModule.h"
//#define vtkPVHardwareSelectorDEBUG
#ifdef vtkPVHardwareSelectorDEBUG
#include "vtkImageImport.h"
#include "vtkNew.h"
#include "vtkPNMWriter.h"
#include "vtkProcessModule.h"
#include "vtkWindows.h" // OK on Unix etc
#include <sstream>
#endif

class vtkPVHardwareSelector::vtkInternals
{
public:
  typedef std::map<void*, int> PropMapType;
  PropMapType PropMap;
};

//----------------------------------------------------------------------------
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
void vtkPVHardwareSelector::SetSynchronizedWindows(vtkPVSynchronizedRenderWindows* sw)
{
  if (this->SynchronizedWindows != sw)
  {
    this->SynchronizedWindows = sw;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool vtkPVHardwareSelector::PassRequired(int pass)
{
  if (pass == MIN_KNOWN_PASS + 1)
  {
    // synchronize the MaxAttributeId among all active processes (BUG #141112).
    if (this->SynchronizedWindows && this->SynchronizedWindows->GetEnabled())
    {
      this->SynchronizedWindows->Reduce(
        this->MaxAttributeId, vtkPVSynchronizedRenderWindows::MAX_OP);
    }
  }
  return (pass == PROCESS_PASS ? true : this->Superclass::PassRequired(pass));
}

//----------------------------------------------------------------------------
bool vtkPVHardwareSelector::PrepareSelect()
{
  if (this->NeedToRenderForSelection())
  {
    int* size = this->Renderer->GetSize();
    int* origin = this->Renderer->GetOrigin();
    this->SetArea(origin[0], origin[1], origin[0] + size[0] - 1, origin[1] + size[1] - 1);
    if (this->CaptureBuffers() == false)
    {
      this->CaptureTime.Modified();
      return false;
    }
    this->CaptureTime.Modified();
  }
  return true;
}

//----------------------------------------------------------------------------
vtkSelection* vtkPVHardwareSelector::Select(int region[4])
{
  if (!this->PrepareSelect())
  {
    return NULL;
  }

  vtkSelection* sel = this->GenerateSelection(region[0], region[1], region[2], region[3]);
  if (sel->GetNumberOfNodes() == 0 &&
    this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS && region[0] == region[2] &&
    region[1] == region[3] && vtkPVRenderViewSettings::GetInstance()->GetPointPickingRadius() > 0)
  {
    unsigned int pos[2];
    pos[0] = static_cast<unsigned int>(region[0]);
    pos[1] = static_cast<unsigned int>(region[1]);

    unsigned int out_pos[2];
    vtkHardwareSelector::PixelInformation info = this->GetPixelInformation(
      pos, vtkPVRenderViewSettings::GetInstance()->GetPointPickingRadius(), out_pos);
    if (info.Valid)
    {
      sel->Delete();
      return this->GenerateSelection(out_pos[0], out_pos[1], out_pos[0], out_pos[1]);
    }
  }
  return sel;
}

//----------------------------------------------------------------------------
vtkSelection* vtkPVHardwareSelector::PolygonSelect(int* polygonPoints, vtkIdType count)
{
  if (!this->PrepareSelect())
  {
    return NULL;
  }
  return this->GeneratePolygonSelection(polygonPoints, count);
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
  return (iter != this->Internals->PropMap.end() ? iter->second : -1);
}

//----------------------------------------------------------------------------
void vtkPVHardwareSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVHardwareSelector::BeginRenderProp(vtkRenderWindow* rw)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  this->SetProcessID(pm->GetGlobalController()->GetLocalProcessId());

  this->Superclass::BeginRenderProp(rw);
}

//----------------------------------------------------------------------------
// just add debug output if compiled with vtkOpenGLHardwareSelectorDEBUG
void vtkPVHardwareSelector::SavePixelBuffer(int passNo)
{
  this->Superclass::SavePixelBuffer(passNo);

#ifdef vtkPVHardwareSelectorDEBUG

  vtkNew<vtkImageImport> ii;
  ii->SetImportVoidPointer(this->PixBuffer[passNo], 1);
  ii->SetDataScalarTypeToUnsignedChar();
  ii->SetNumberOfScalarComponents(3);
  ii->SetDataExtent(this->Area[0], this->Area[2], this->Area[1], this->Area[3], 0, 0);
  ii->SetWholeExtent(this->Area[0], this->Area[2], this->Area[1], this->Area[3], 0, 0);

  // hardcode the path so you can find where MPI etc leaves all these files
  std::string fname = "C:/Users/ken.martin/pickbuffer_";

  std::ostringstream toString;
  toString.str("");
  toString.clear();
// is there a platform generic way of doing this?
//  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
//  toString << pm->GetPartitionId();
//  toString << pm->GetGlobalController()->GetLocalProcessId();
#if defined(_WIN32)
  toString << GetCurrentProcessId();
#endif
  fname += toString.str();
  fname += "_";
  fname += ('0' + passNo);
  fname += ".pnm";
  vtkNew<vtkPNMWriter> pw;
  pw->SetInputConnection(ii->GetOutputPort());
  pw->SetFileName(fname.c_str());
  pw->Write();
  cerr << "wrote to " << fname << "\n";
#endif
}
