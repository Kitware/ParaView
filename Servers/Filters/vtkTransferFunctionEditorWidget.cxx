/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransferFunctionEditorWidget.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkTransferFunctionEditorRepresentation.h"

vtkCxxRevisionMacro(vtkTransferFunctionEditorWidget, "1.5");

//----------------------------------------------------------------------------
vtkTransferFunctionEditorWidget::vtkTransferFunctionEditorWidget()
{
  this->Input = NULL;
  this->ArrayName = NULL;
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  this->NumberOfScalarBins = 10000;
  this->WholeScalarRange[0] = this->VisibleScalarRange[0] = 1;
  this->WholeScalarRange[1] = this->VisibleScalarRange[1] = 0;
  this->ModificationType = OPACITY;
  this->OpacityFunction = vtkPiecewiseFunction::New();
}

//----------------------------------------------------------------------------
vtkTransferFunctionEditorWidget::~vtkTransferFunctionEditorWidget()
{
  this->SetInput(NULL);
  this->SetArrayName(NULL);
  this->OpacityFunction->Delete();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget::SetVisibleScalarRange(double min,
                                                            double max)
{
  if (min == this->VisibleScalarRange[0] && max == this->VisibleScalarRange[1])
    {
    return;
    }

  if (this->WholeScalarRange[0] > this->WholeScalarRange[1])
    {
    this->SetWholeScalarRange(min, max);
    }

  this->VisibleScalarRange[0] = min;
  this->VisibleScalarRange[1] = max;
  this->Modified();

  vtkTransferFunctionEditorRepresentation *rep =
    vtkTransferFunctionEditorRepresentation::SafeDownCast(this->WidgetRep);
  if (rep)
    {
    if (this->Input)
      {
      double inputScalarRange[2];
      vtkDataSetAttributes *dsa;
      if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
        {
        dsa = this->Input->GetPointData();
        }
      else
        {
        dsa = this->Input->GetCellData();
        }
      vtkDataArray *dataArray;
      if (this->ArrayName)
        {
        dataArray = dsa->GetArray(this->ArrayName);
        }
      else
        {
        dataArray = dsa->GetScalars();
        }
      dataArray->GetRange(inputScalarRange);
      int scalarType = dataArray->GetDataType();
      if (scalarType != VTK_FLOAT && scalarType != VTK_DOUBLE)
        {
        rep->SetScalarBinRange(static_cast<int>(min - inputScalarRange[0]),
                               static_cast<int>(max - inputScalarRange[0]));
        }
      else
        {
        rep->SetScalarBinRange(
          static_cast<int>(
            (min - inputScalarRange[0]) * this->NumberOfScalarBins /
            (inputScalarRange[1] - inputScalarRange[0])),
          static_cast<int>(
            (max - inputScalarRange[0]) * this->NumberOfScalarBins /
            (inputScalarRange[1] - inputScalarRange[0])));
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget::ShowWholeScalarRange()
{
  if (this->Input)
    {
    double range[2];
    vtkDataSetAttributes *dsa;
    vtkDataArray *dataArray;
    if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
      {
      dsa = this->Input->GetPointData();
      }
    else
      {
      dsa = this->Input->GetCellData();
      }
    if (this->ArrayName)
      {
      dataArray = dsa->GetArray(this->ArrayName);
      }
    else
      {
      dataArray = dsa->GetScalars();
      }
    if (dataArray)
      {
      dataArray->GetRange(range);
      this->SetVisibleScalarRange(range);
      }
    }
  else
    {
    this->SetVisibleScalarRange(this->WholeScalarRange);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget::SetInput(vtkDataSet *input)
{
  if (input != this->Input)
    {
    vtkDataSet *tmpInput = this->Input;
    this->Input = input;
    if (this->Input != NULL)
      {
      this->Input->Register(this);
      if (this->VisibleScalarRange[0] > this->VisibleScalarRange[1])
        {
        double range[2];
        vtkDataSetAttributes *dsa;
        vtkDataArray *array;
        if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
          {
          dsa = this->Input->GetPointData();
          }
        else
          {
          dsa = this->Input->GetCellData();
          }
        if (this->ArrayName)
          {
          array = dsa->GetArray(this->ArrayName);
          }
        else
          {
          array = dsa->GetScalars();
          }
        if (array)
          {
          array->GetRange(range);
          this->SetVisibleScalarRange(range);
          }
        }
      this->ComputeHistogram();
      }
    if (tmpInput != NULL)
      {
      tmpInput->UnRegister(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget::SetArrayName(const char *name)
{
  if (this->ArrayName == NULL && name == NULL)
    {
    return;
    }
  if (this->ArrayName && name && !strcmp(this->ArrayName, name))
    {
    return;
    }
  if (this->ArrayName)
    {
    delete [] this->ArrayName;
    }
  if (name)
    {
    size_t len = strlen(name) + 1;
    char *str1 = new char[len];
    const char *str2 = name;
    this->ArrayName = str1;
    do
      {
      *str1++ = *str2++;
      }
    while (--len);
    this->ComputeHistogram();
    }
  else
    {
    this->ArrayName = NULL;
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget::SetFieldAssociation(int assoc)
{
  int clampedVal =
    (assoc < vtkDataObject::FIELD_ASSOCIATION_POINTS) ? 
    vtkDataObject::FIELD_ASSOCIATION_POINTS : assoc;
  clampedVal =
    (assoc > vtkDataObject::FIELD_ASSOCIATION_CELLS) ? 
    vtkDataObject::FIELD_ASSOCIATION_CELLS : assoc;
  
  if (this->FieldAssociation != clampedVal)
    {
    this->FieldAssociation = clampedVal;
    this->ComputeHistogram();
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget::Configure(int size[2])
{
  vtkTransferFunctionEditorRepresentation *rep =
    vtkTransferFunctionEditorRepresentation::SafeDownCast(this->WidgetRep);
  if (rep)
    {
    rep->SetDisplaySize(size);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget::InputModified()
{
  this->ComputeHistogram();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget::SetEnabled(int enable)
{
  this->Superclass::SetEnabled(enable);

  if (enable)
    {
    this->ComputeHistogram();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "VisibleScalarRange: " << this->VisibleScalarRange[0] << " "
     << this->VisibleScalarRange[1] << endl;
  os << indent << "WholeScalarRange: " << this->WholeScalarRange[0] << " "
     << this->WholeScalarRange[1] << endl;
}
