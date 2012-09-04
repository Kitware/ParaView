/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiSliceRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiSliceRepresentation.h"

#include "vtkAppendFilter.h"
#include "vtkCommand.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkCutter.h"
#include "vtkHardwareSelectionPolyDataPainter.h"
#include "vtkMapper.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkSelection.h"
#include "vtkThreeSliceFilter.h"

vtkStandardNewMacro(vtkMultiSliceRepresentation);
//----------------------------------------------------------------------------
vtkMultiSliceRepresentation::vtkMultiSliceRepresentation()
{
  this->PortToUse = 0; // All the slices by default
  this->InternalSliceFilter = vtkThreeSliceFilter::New();

  // setup the selection mapper so that we don't need to make any selection
  // conversions after rendering.
  vtkCompositePolyDataMapper2* mapper =
      vtkCompositePolyDataMapper2::SafeDownCast(this->Mapper);
  vtkHardwareSelectionPolyDataPainter* selPainter =
      vtkHardwareSelectionPolyDataPainter::SafeDownCast(
        mapper->GetSelectionPainter()->GetDelegatePainter());
  selPainter->SetPointIdArrayName("-");
  selPainter->SetCellIdArrayName("vtkSliceOriginalCellIds");
}

//----------------------------------------------------------------------------
vtkMultiSliceRepresentation::~vtkMultiSliceRepresentation()
{
  this->InternalSliceFilter->Delete();
  this->InternalSliceFilter = NULL;
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceXNormal(double normal[3])
{
  this->SetCutNormal(0, normal);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceXOrigin(double origin[3])
{
  this->SetCutOrigin(0, origin);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceXNormal(double x, double y, double z)
{
  double normal[3] = {x ,y, z};
  this->SetCutNormal(0, normal);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceXOrigin(double x, double y, double z)
{
  double origin[3] = {x ,y, z};
  this->SetCutOrigin(0, origin);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceX(int index, double value)
{
  this->SetCutValue(0, index, value);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetNumberOfSliceX(int size)
{
  this->SetNumberOfSlice(0, size);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceYNormal(double normal[3])
{
  this->SetCutNormal(1, normal);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceYOrigin(double origin[3])
{
  this->SetCutOrigin(1, origin);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceYNormal(double x, double y, double z)
{
  double normal[3] = {x ,y, z};
  this->SetCutNormal(1, normal);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceYOrigin(double x, double y, double z)
{
  double origin[3] = {x ,y, z};
  this->SetCutOrigin(1, origin);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceY(int index, double value)
{
  this->SetCutValue(1, index, value);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetNumberOfSliceY(int size)
{
  this->SetNumberOfSlice(1, size);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceZNormal(double normal[3])
{
  this->SetCutNormal(2, normal);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceZOrigin(double origin[3])
{
  this->SetCutOrigin(2, origin);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceZNormal(double x, double y, double z)
{
  double normal[3] = {x ,y, z};
  this->SetCutNormal(2, normal);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceZOrigin(double x, double y, double z)
{
  double origin[3] = {x ,y, z};
  this->SetCutOrigin(2, origin);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceZ(int index, double value)
{
  this->SetCutValue(2, index, value);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetNumberOfSliceZ(int size)
{
  this->SetNumberOfSlice(2, size);
}
//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetCutNormal(int cutIndex, double normal[3])
{
  this->InternalSliceFilter->SetCutNormal(cutIndex, normal);
  this->InvokeEvent(vtkCommand::UpdateDataEvent);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetCutOrigin(int cutIndex, double origin[3])
{
  this->InternalSliceFilter->SetCutOrigin(cutIndex, origin);
  this->InvokeEvent(vtkCommand::UpdateDataEvent);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetCutValue(int cutIndex, int index, double value)
{
  this->InternalSliceFilter->SetCutValue(cutIndex, index, value);
  this->InvokeEvent(vtkCommand::UpdateDataEvent);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetNumberOfSlice(int cutIndex, int size)
{
  this->InternalSliceFilter->SetNumberOfSlice(cutIndex, size);
  this->InvokeEvent(vtkCommand::UpdateDataEvent);
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkMultiSliceRepresentation::GetInternalOutputPort(int port, int conn)
{
  vtkAlgorithmOutput* inputAlgo =
      this->Superclass::GetInternalOutputPort(port, conn);

  this->InternalSliceFilter->SetInputConnection(inputAlgo);

  return this->InternalSliceFilter->GetOutputPort(this->PortToUse);
}
//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkMultiSliceRepresentation::GetInternalOutputPort(int port)
{
  return this->Superclass::GetInternalOutputPort(port);
}

//----------------------------------------------------------------------------
vtkThreeSliceFilter* vtkMultiSliceRepresentation::GetInternalVTKFilter()
{
  return this->InternalSliceFilter;
}
