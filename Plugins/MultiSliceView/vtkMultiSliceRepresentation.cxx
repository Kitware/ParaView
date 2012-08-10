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
#include "vtkCompositePolyDataMapper2.h"
#include "vtkCutter.h"
#include "vtkHardwareSelectionPolyDataPainter.h"
#include "vtkMapper.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkThreeSliceFilter.h"
#include "vtkPlane.h"
#include "vtkSelection.h"

vtkStandardNewMacro(vtkMultiSliceRepresentation);
//----------------------------------------------------------------------------
vtkMultiSliceRepresentation::vtkMultiSliceRepresentation()
{
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
void vtkMultiSliceRepresentation::SetSliceX(int index, double value)
{
  this->InternalSliceFilter->SetCutValue(0, index, value);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetNumberOfSliceX(int size)
{
  this->InternalSliceFilter->SetNumberOfSlice(0, size);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceY(int index, double value)
{
  this->InternalSliceFilter->SetCutValue(1, index, value);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetNumberOfSliceY(int size)
{
  this->InternalSliceFilter->SetNumberOfSlice(1, size);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceZ(int index, double value)
{
  this->InternalSliceFilter->SetCutValue(2, index, value);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetNumberOfSliceZ(int size)
{
  this->InternalSliceFilter->SetNumberOfSlice(2, size);
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkMultiSliceRepresentation::GetInternalOutputPort(int port, int conn)
{
  vtkAlgorithmOutput* inputAlgo =
      this->Superclass::GetInternalOutputPort(port, conn);

  this->InternalSliceFilter->SetInputConnection(inputAlgo);

  return this->InternalSliceFilter->GetOutputPort();
}
