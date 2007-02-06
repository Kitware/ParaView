/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorWidget1D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransferFunctionEditorWidget1D.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDataSetHistogramFilter.h"
#include "vtkDoubleArray.h"
#include "vtkImageAccumulate.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkTransferFunctionEditorRepresentation1D.h"

vtkCxxRevisionMacro(vtkTransferFunctionEditorWidget1D, "1.1");

//----------------------------------------------------------------------------
vtkTransferFunctionEditorWidget1D::vtkTransferFunctionEditorWidget1D()
{
  this->Histogram = vtkDoubleArray::New();
  this->Histogram->SetNumberOfComponents(1);
}

//----------------------------------------------------------------------------
vtkTransferFunctionEditorWidget1D::~vtkTransferFunctionEditorWidget1D()
{
  this->Histogram->Delete();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget1D::ComputeHistogram()
{
  if (!this->Input)
    {
    return;
    }

  vtkDataSetAttributes *dsa;
  if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    dsa = this->Input->GetPointData();
    }
  else
    {
    dsa = this->Input->GetCellData();
    }

  vtkDataArray *array;
  if (this->ArrayName)
    {
    array = dsa->GetArray(this->ArrayName);
    }
  else
    {
    array = dsa->GetScalars();
    }

  int scalarType = array->GetDataType();

  double range[2];
  array->GetRange(range);

  if (this->Input->IsA("vtkImageData"))
    {
    vtkImageAccumulate *accum = vtkImageAccumulate::New();
    accum->SetInput(this->Input);
    if (scalarType == VTK_FLOAT || scalarType == VTK_DOUBLE)
      {
      accum->SetComponentExtent(0, this->NumberOfScalarBins-1, 0, 0, 0, 0);
      accum->SetComponentSpacing(
        (range[1]-range[0])/(double)(this->NumberOfScalarBins), 0, 0);
      }
    else
      {
      accum->SetComponentExtent(0, range[1]-range[0], 0, 0, 0, 0);
      accum->SetComponentSpacing(1, 0, 0);
      }
    accum->SetComponentOrigin(range[0], 0, 0);
    accum->Update();
    this->Histogram->DeepCopy(
      accum->GetOutput()->GetPointData()->GetScalars());
    accum->Delete();
    }
  else
    {
    vtkDataSetHistogramFilter *hist = vtkDataSetHistogramFilter::New();
    hist->SetInput(this->Input);
    if (scalarType == VTK_FLOAT || scalarType == VTK_DOUBLE)
      {
      hist->SetOutputExtent(0, this->NumberOfScalarBins-1);
      hist->SetOutputSpacing(
        (range[1]-range[0])/(double)(this->NumberOfScalarBins));
      }
    else
      {
      hist->SetOutputExtent(0, range[1]-range[0]);
      hist->SetOutputSpacing(1);
      }
    hist->SetOutputOrigin(range[0]);
    hist->Update();
    this->Histogram->DeepCopy(
      hist->GetOutput()->GetPointData()->GetScalars());
    hist->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget1D::SetEnabled(int enabled)
{
  this->Superclass::SetEnabled(enabled);

  vtkTransferFunctionEditorRepresentation1D *rep =
    vtkTransferFunctionEditorRepresentation1D::SafeDownCast(this->WidgetRep);
  if (enabled && rep)
    {
    rep->SetHistogram(this->Histogram);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget1D::PrintSelf(ostream& os,
                                                  vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
