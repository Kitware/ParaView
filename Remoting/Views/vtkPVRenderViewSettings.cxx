/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderViewSettings.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRenderViewSettings.h"

#include "vtkMapper.h"
#include "vtkObjectFactory.h"

#include <cassert>

vtkSmartPointer<vtkPVRenderViewSettings> vtkPVRenderViewSettings::Instance;

//----------------------------------------------------------------------------
vtkPVRenderViewSettings* vtkPVRenderViewSettings::New()
{
  vtkPVRenderViewSettings* instance = vtkPVRenderViewSettings::GetInstance();
  assert(instance);
  instance->Register(NULL);
  return instance;
}

//----------------------------------------------------------------------------
vtkPVRenderViewSettings* vtkPVRenderViewSettings::GetInstance()
{
  if (!vtkPVRenderViewSettings::Instance)
  {
    vtkPVRenderViewSettings* instance = new vtkPVRenderViewSettings();
    instance->InitializeObjectBase();
    vtkPVRenderViewSettings::Instance.TakeReference(instance);
  }
  return vtkPVRenderViewSettings::Instance;
}

//----------------------------------------------------------------------------
vtkPVRenderViewSettings::vtkPVRenderViewSettings()
  : DefaultInteractionMode(0)
  , OutlineThreshold(250)
  , PointPickingRadius(0)
  , DisableIceT(false)
{
}

//----------------------------------------------------------------------------
vtkPVRenderViewSettings::~vtkPVRenderViewSettings()
{
}

//----------------------------------------------------------------------------
void vtkPVRenderViewSettings::SetResolveCoincidentTopology(int mode)
{
  switch (mode)
  {
    case OFFSET_FACES:
      vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
      vtkMapper::SetResolveCoincidentTopologyPolygonOffsetFaces(1);
      break;

    case OFFSET_LINES_AND_VERTS:
      vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
      vtkMapper::SetResolveCoincidentTopologyPolygonOffsetFaces(0);
      break;

    case ZSHIFT:
      vtkMapper::SetResolveCoincidentTopologyToShiftZBuffer();
      break;

    case DO_NOTHING:
    default:
      vtkMapper::SetResolveCoincidentTopologyToOff();
      break;
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRenderViewSettings::SetPolygonOffsetParameters(double factor, double units)
{
  vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters(factor, units);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRenderViewSettings::SetLineOffsetParameters(double factor, double units)
{
  vtkMapper::SetResolveCoincidentTopologyLineOffsetParameters(factor, units);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRenderViewSettings::SetPointOffsetParameter(double units)
{
  vtkMapper::SetResolveCoincidentTopologyPointOffsetParameter(units);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRenderViewSettings::SetZShift(double val)
{
  vtkMapper::SetResolveCoincidentTopologyZShift(val);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRenderViewSettings::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
