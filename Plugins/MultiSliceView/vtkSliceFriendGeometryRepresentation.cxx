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
#include "vtkHardwareSelectionPolyDataPainter.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSliceFriendGeometryRepresentation);
//----------------------------------------------------------------------------
vtkSliceFriendGeometryRepresentation::vtkSliceFriendGeometryRepresentation() : vtkGeometryRepresentationWithFaces()
{
  this->AllowInputConnectionSetting = true;
}
//----------------------------------------------------------------------------
vtkSliceFriendGeometryRepresentation::~vtkSliceFriendGeometryRepresentation()
{
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
    }
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::SetInputConnection(vtkAlgorithmOutput* input)
{
  if(this->AllowInputConnectionSetting)
    {
    this->Superclass::SetInputConnection(input);
    }
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::AddInputConnection(int port, vtkAlgorithmOutput* input)
{
  if(this->AllowInputConnectionSetting)
    {
    this->Superclass::AddInputConnection(port, input);
    }
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::AddInputConnection(vtkAlgorithmOutput* input)
{
  if(this->AllowInputConnectionSetting)
    {
    this->Superclass::AddInputConnection(input);
    }
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::RemoveInputConnection(int port, vtkAlgorithmOutput* input)
{
  if(this->AllowInputConnectionSetting)
    {
    this->Superclass::RemoveInputConnection(port, input);
    }
}

//----------------------------------------------------------------------------
void vtkSliceFriendGeometryRepresentation::RemoveInputConnection(int port, int idx)
{
  if(this->AllowInputConnectionSetting)
    {
    this->Superclass::RemoveInputConnection(port, idx);
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
}
