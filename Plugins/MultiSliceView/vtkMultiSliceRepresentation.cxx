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

#include "vtkObjectFactory.h"
#include "vtkCutter.h"
#include "vtkPlane.h"
#include "vtkNew.h"
#include "vtkAppendFilter.h"

vtkStandardNewMacro(vtkMultiSliceRepresentation);
//----------------------------------------------------------------------------
vtkMultiSliceRepresentation::vtkMultiSliceRepresentation()
{
  this->SliceAlongX = vtkCutter::New();
  this->SliceAlongY = vtkCutter::New();
  this->SliceAlongZ = vtkCutter::New();

  vtkNew<vtkPlane> planeX;
  planeX->SetNormal(1,0,0);
  planeX->SetOrigin(0,0,0);
  this->SliceAlongX->SetCutFunction(planeX.GetPointer());

  vtkNew<vtkPlane> planeY;
  planeY->SetNormal(0,1,0);
  planeY->SetOrigin(0,0,0);
  this->SliceAlongY->SetCutFunction(planeY.GetPointer());

  vtkNew<vtkPlane> planeZ;
  planeZ->SetNormal(0,0,1);
  planeZ->SetOrigin(0,0,0);
  this->SliceAlongZ->SetCutFunction(planeZ.GetPointer());

  this->CombinedFilteredInput = vtkAppendFilter::New();
  this->CombinedFilteredInput->AddInputConnection(this->SliceAlongX->GetOutputPort());
  this->CombinedFilteredInput->AddInputConnection(this->SliceAlongY->GetOutputPort());
  this->CombinedFilteredInput->AddInputConnection(this->SliceAlongZ->GetOutputPort());
}

//----------------------------------------------------------------------------
vtkMultiSliceRepresentation::~vtkMultiSliceRepresentation()
{
  this->SliceAlongX->Delete();
  this->SliceAlongY->Delete();
  this->SliceAlongZ->Delete();
  this->SliceAlongX = this->SliceAlongY = this->SliceAlongZ = NULL;

  this->CombinedFilteredInput->Delete();
  this->CombinedFilteredInput = NULL;
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceX(int index, double value)
{
  this->SetSlice(this->SliceAlongX, index, value);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetNumberOfSliceX(int size)
{
  this->SetNumberOfSlice(this->SliceAlongX, size);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceY(int index, double value)
{
  this->SetSlice(this->SliceAlongY, index, value);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetNumberOfSliceY(int size)
{
  this->SetNumberOfSlice(this->SliceAlongY, size);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSliceZ(int index, double value)
{
  this->SetSlice(this->SliceAlongZ, index, value);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetNumberOfSliceZ(int size)
{
  this->SetNumberOfSlice(this->SliceAlongZ, size);
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetSlice(vtkCutter* slice, int index, double value)
{
  if(slice->GetValue(index) != value)
    {
    slice->SetValue(index, value);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkMultiSliceRepresentation::SetNumberOfSlice(vtkCutter* slice, int size)
{
  if(size != slice->GetNumberOfContours())
    {
    cout  << "Update the number of slices to " << size << endl;
    slice->SetNumberOfContours(size);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkMultiSliceRepresentation::GetInternalOutputPort(int port, int conn)
{
  vtkAlgorithmOutput* inputAlgo =
      this->Superclass::GetInternalOutputPort(port, conn);

  this->SliceAlongX->SetInputConnection(inputAlgo);
  this->SliceAlongY->SetInputConnection(inputAlgo);
  this->SliceAlongZ->SetInputConnection(inputAlgo);

  return this->CombinedFilteredInput->GetOutputPort();
}
