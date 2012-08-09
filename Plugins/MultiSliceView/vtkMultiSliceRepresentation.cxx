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
#include "vtkOrthogonalSliceFilter.h"
#include "vtkPlane.h"
#include "vtkSelection.h"

vtkStandardNewMacro(vtkMultiSliceRepresentation);
//----------------------------------------------------------------------------
vtkMultiSliceRepresentation::vtkMultiSliceRepresentation()
{
  this->InternalSliceFilter = vtkOrthogonalSliceFilter::New();

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
  this->InternalSliceFilter->SetSliceX(index, value);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetNumberOfSliceX(int size)
{
  this->InternalSliceFilter->SetNumberOfSliceX(size);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceY(int index, double value)
{
  this->InternalSliceFilter->SetSliceY(index, value);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetNumberOfSliceY(int size)
{
  this->InternalSliceFilter->SetNumberOfSliceY(size);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceZ(int index, double value)
{
  this->InternalSliceFilter->SetSliceZ(index, value);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetNumberOfSliceZ(int size)
{
  this->InternalSliceFilter->SetNumberOfSliceZ(size);
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkMultiSliceRepresentation::GetInternalOutputPort(int port, int conn)
{
  vtkAlgorithmOutput* inputAlgo =
      this->Superclass::GetInternalOutputPort(port, conn);

  this->InternalSliceFilter->SetInputConnection(inputAlgo);

  return this->InternalSliceFilter->GetOutputPort();
}
