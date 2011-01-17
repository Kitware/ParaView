/*=========================================================================

  Program:   ParaView
  Module:    vtkPrismTableToPolyData.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPrismTableToPolyData.h"

#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkCellData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTable.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"

vtkStandardNewMacro(vtkPrismTableToPolyData);
//----------------------------------------------------------------------------
vtkPrismTableToPolyData::vtkPrismTableToPolyData()
{
  this->GobalElementIdColumn = 0;
}

//----------------------------------------------------------------------------
vtkPrismTableToPolyData::~vtkPrismTableToPolyData()
{

  this->SetGobalElementIdColumn(0);
}


//----------------------------------------------------------------------------
int vtkPrismTableToPolyData::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkTable* input = vtkTable::GetData(inputVector[0], 0);
  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);

  if (input->GetNumberOfRows() == 0)
    {
    // empty input.
    return 1;
    }

  vtkDataArray* xarray = NULL;
  vtkDataArray* yarray = NULL;
  vtkDataArray* zarray = NULL;
  vtkDataArray* gobalElementIdArray = NULL;

  if(this->GobalElementIdColumn)
  {
    gobalElementIdArray = vtkDataArray::SafeDownCast(
      input->GetColumnByName(this->GobalElementIdColumn));
  }

  if(this->XColumn && this->YColumn)
    {
    xarray = vtkDataArray::SafeDownCast(
      input->GetColumnByName(this->XColumn));
    yarray = vtkDataArray::SafeDownCast(
      input->GetColumnByName(this->YColumn));
    zarray = vtkDataArray::SafeDownCast(
      input->GetColumnByName(this->ZColumn));
    }
  else if(this->XColumnIndex >= 0)
    {
    xarray = vtkDataArray::SafeDownCast(
      input->GetColumn(this->XColumnIndex));
    yarray = vtkDataArray::SafeDownCast(
      input->GetColumn(this->YColumnIndex));
    zarray = vtkDataArray::SafeDownCast(
      input->GetColumn(this->ZColumnIndex));
    }

  // zarray is optional
  if(this->Create2DPoints)
    {
    if (!xarray || !yarray)
      {
      vtkErrorMacro("Failed to locate  the columns to use for the point"
        " coordinates");
      return 0;
      }
    }
  else
    {
    if (!xarray || !yarray || !zarray)
      {
      vtkErrorMacro("Failed to locate  the columns to use for the point"
        " coordinates");
      return 0;
      }
    }

  vtkPoints* newPoints = vtkPoints::New();

  if (xarray == yarray && yarray == zarray &&
    this->XComponent == 0 &&
    this->YComponent == 1 &&
    this->ZComponent == 2 &&
    xarray->GetNumberOfComponents() == 3)
    {
    newPoints->SetData(xarray);
    }
  else
    {
    // Ideally we determine the smallest data type that can contain the values
    // in all the 3 arrays. For now I am just going with doubles.
    vtkDoubleArray* newData =  vtkDoubleArray::New();
    newData->SetNumberOfComponents(3);
    newData->SetNumberOfTuples(input->GetNumberOfRows());
    vtkIdType numtuples = newData->GetNumberOfTuples();
    if(this->Create2DPoints)
      {
      for (vtkIdType cc=0; cc < numtuples; cc++)
        {
        newData->SetComponent(cc, 0, xarray->GetComponent(cc, this->XComponent));
        newData->SetComponent(cc, 1, yarray->GetComponent(cc, this->YComponent));
        newData->SetComponent(cc, 2, 0.0);
        }
      }
    else
      {
      for (vtkIdType cc=0; cc < numtuples; cc++)
        {
        newData->SetComponent(cc, 0, xarray->GetComponent(cc, this->XComponent));
        newData->SetComponent(cc, 1, yarray->GetComponent(cc, this->YComponent));
        newData->SetComponent(cc, 2, zarray->GetComponent(cc, this->ZComponent));
        }
      }
    newPoints->SetData(newData);
    newData->Delete();
    }

  output->SetPoints(newPoints);
  newPoints->Delete();

  // Now create a vertex cell will all the points.
  vtkIdType numPts = newPoints->GetNumberOfPoints();
  output->Allocate(numPts);
  for (vtkIdType cc=0; cc < numPts; cc++)
    {
    output->InsertNextCell(VTK_VERTEX, 1, &cc);
    }

  // Add all other columns as point data.


  if(gobalElementIdArray)
  {
    vtkDataArray* newArray= vtkDataArray::CreateDataArray(VTK_ID_TYPE);
    newArray->DeepCopy(gobalElementIdArray);
    newArray->SetName("GobalElementId");

    output->GetCellData()->SetGlobalIds(newArray);
    newArray->Delete();
  }

  for (int cc=0; cc < input->GetNumberOfColumns(); cc++)
    {
    vtkAbstractArray* arr = input->GetColumn(cc);
    if (arr != xarray && arr != yarray && arr != zarray && arr != gobalElementIdArray)
      {
      output->GetCellData()->AddArray(arr);
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPrismTableToPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
