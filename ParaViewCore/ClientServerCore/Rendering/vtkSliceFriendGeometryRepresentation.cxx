/*=========================================================================

  Program:   ParaView
  Module:    vtkSliceFriendGeometryRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSliceFriendGeometryRepresentation.h"

#include "vtkCompositePolyDataMapper2.h"
#include "vtkCubeAxesRepresentation.h"
#include "vtkGarbageCollector.h"
#include "vtkHardwareSelectionPolyDataPainter.h"
#include "vtkObjectFactory.h"
#include "vtkView.h"

vtkStandardNewMacro(vtkSliceFriendGeometryRepresentation);
//----------------------------------------------------------------------------
vtkSliceFriendGeometryRepresentation::vtkSliceFriendGeometryRepresentation() : vtkGeometryRepresentationWithFaces()
{
  this->CubeAxesVisibility = false;
  this->AllowInputConnectionSetting = true;
  this->CubeAxesRepresentation = vtkCubeAxesRepresentation::New();
}
//----------------------------------------------------------------------------
vtkSliceFriendGeometryRepresentation::~vtkSliceFriendGeometryRepresentation()
{
  // Managed by GC: this->CubeAxesRepresentation->Delete();
  if(this->CubeAxesRepresentation)
    {
    this->CubeAxesRepresentation->Delete();
    this->CubeAxesRepresentation = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::SetInputConnection(int port, vtkAlgorithmOutput* input)
{
  if(this->AllowInputConnectionSetting)
    {
    this->Superclass::SetInputConnection(port, input);
    this->CubeAxesRepresentation->SetInputConnection(port, input);
    }
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::SetInputConnection(vtkAlgorithmOutput* input)
{
  if(this->AllowInputConnectionSetting)
    {
    this->Superclass::SetInputConnection(input);
    this->CubeAxesRepresentation->SetInputConnection(input);
    }
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::AddInputConnection(int port, vtkAlgorithmOutput* input)
{
  if(this->AllowInputConnectionSetting)
    {
    this->Superclass::AddInputConnection(port, input);
    this->CubeAxesRepresentation->AddInputConnection(port, input);
    }
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::AddInputConnection(vtkAlgorithmOutput* input)
{
  if(this->AllowInputConnectionSetting)
    {
    this->Superclass::AddInputConnection(input);
    this->CubeAxesRepresentation->AddInputConnection(input);
    }
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::RemoveInputConnection(int port, vtkAlgorithmOutput* input)
{
  if(this->AllowInputConnectionSetting)
    {
    this->Superclass::RemoveInputConnection(port, input);
    this->CubeAxesRepresentation->RemoveInputConnection(port, input);
    }
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::RemoveInputConnection(int port, int idx)
{
  if(this->AllowInputConnectionSetting)
    {
    this->Superclass::RemoveInputConnection(port, idx);
    this->CubeAxesRepresentation->RemoveInputConnection(port, idx);
    }
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::InitializeMapperForSliceSelection()
{
  // setup the selection mapper so that we don't need to make any selection
  // conversions after rendering.
  vtkCompositePolyDataMapper2* mapper =
      vtkCompositePolyDataMapper2::SafeDownCast(this->Mapper);
  vtkHardwareSelectionPolyDataPainter* selPainter =
      vtkHardwareSelectionPolyDataPainter::SafeDownCast(
        mapper->GetSelectionPainter()->GetDelegatePainter());
  selPainter->SetPointIdArrayName("-");
  selPainter->SetCellIdArrayName("vtkSliceOriginalCellIds");
  selPainter->SetCompositeIdArrayName("vtkSliceCompositeIndex");
}

//----------------------------------------------------------------------------
vtkDataObject* vtkSliceFriendGeometryRepresentation::GetRenderedDataObject(int port)
{
  if(this->RepresentationForRenderedDataObject)
    {
    return this->RepresentationForRenderedDataObject->GetRenderedDataObject(port);
    }

  return this->Superclass::GetRenderedDataObject(port);
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::SetRepresentationForRenderedDataObject(vtkPVDataRepresentation* rep)
{
  this->RepresentationForRenderedDataObject = rep;
}
//----------------------------------------------------------------------------
bool vtkSliceFriendGeometryRepresentation::AddToView(vtkView* view)
{
  this->CubeAxesRepresentation->AddToView(view);
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkSliceFriendGeometryRepresentation::RemoveFromView(vtkView* view)
{
  this->CubeAxesRepresentation->RemoveFromView(view);
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::SetCubeAxesVisibility(bool visible)
{
  this->CubeAxesVisibility = visible;
  this->CubeAxesRepresentation->SetVisibility(this->GetVisibility() && visible);
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);
  this->SetCubeAxesVisibility(this->CubeAxesVisibility);
}

//----------------------------------------------------------------------------
unsigned int vtkSliceFriendGeometryRepresentation::Initialize(unsigned int minIdAvailable,
                                                    unsigned int maxIdAvailable)
{
  unsigned int minId = minIdAvailable;
  minId = this->CubeAxesRepresentation->Initialize(minId, maxIdAvailable);

  return  this->Superclass::Initialize(minId, maxIdAvailable);
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::MarkModified()
{
  this->CubeAxesRepresentation->MarkModified();
  this->Superclass::MarkModified();
}
//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->CubeAxesRepresentation, "CubeAxesRepresentation");
}
