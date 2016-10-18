/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLineToRectilinearGridFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyLineToRectilinearGridFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"

vtkStandardNewMacro(vtkPolyLineToRectilinearGridFilter);
//-----------------------------------------------------------------------------
vtkPolyLineToRectilinearGridFilter::vtkPolyLineToRectilinearGridFilter()
{
}

//-----------------------------------------------------------------------------
vtkPolyLineToRectilinearGridFilter::~vtkPolyLineToRectilinearGridFilter()
{
}

//-----------------------------------------------------------------------------
int vtkPolyLineToRectilinearGridFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkPolyLineToRectilinearGridFilter::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
int vtkPolyLineToRectilinearGridFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDebugMacro("Executing vtkPolyLineToRectilinearGridFilter");
  vtkInformation* const output_info = outputVector->GetInformationObject(0);
  vtkRectilinearGrid* const output_data =
    vtkRectilinearGrid::SafeDownCast(output_info->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* const input_info = inputVector[0]->GetInformationObject(0);
  vtkDataObject* inputDO = input_info->Get(vtkDataObject::DATA_OBJECT());
  vtkRectilinearGrid* inputRG = vtkRectilinearGrid::SafeDownCast(inputDO);
  if (inputRG)
  {
    // input is already a rectilinear grid. do nothing.
    output_data->ShallowCopy(inputRG);
    return 1;
  }

  vtkPolyData* const input = vtkPolyData::SafeDownCast(inputDO);
  if (!input)
  {
    vtkErrorMacro("Input must be either a vtkPolyData or vtkRectilinearGrid.");
    return 0;
  }

  vtkPointData* outPD = output_data->GetPointData();
  vtkCellData* outCD = output_data->GetCellData();
  vtkPointData* inPD = input->GetPointData();
  vtkCellData* inCD = input->GetCellData();

  vtkCellArray* lines = input->GetLines();
  int num_lines = lines->GetNumberOfCells();
  if (num_lines == 0)
  {
    // vtkWarningMacro("No lines in the input.");
    return 1;
  }

  if (num_lines > 1)
  {
    vtkWarningMacro("Input has more than 1 polyline. Currently this filter only"
                    " uses the first one.");
  }

  vtkIdType number_of_points = 0;
  vtkIdType* points;
  vtkIdType cc;
  lines->GetCell(0, number_of_points, points);

  output_data->SetDimensions(number_of_points, 1, 1);

  vtkDoubleArray* const xcoords = vtkDoubleArray::New();
  xcoords->SetNumberOfComponents(1);
  xcoords->SetNumberOfTuples(number_of_points);
  output_data->SetXCoordinates(xcoords);
  xcoords->Delete();

  vtkDoubleArray* const otherCoords = vtkDoubleArray::New();
  otherCoords->SetNumberOfComponents(1);
  otherCoords->SetNumberOfTuples(1);
  otherCoords->SetTuple1(0, 0.0);
  output_data->SetYCoordinates(otherCoords);
  output_data->SetZCoordinates(otherCoords);
  otherCoords->Delete();

  vtkIdType lineCellId = input->GetNumberOfVerts();
  outCD->CopyAllocate(inCD, 1);
  outCD->CopyData(inCD, lineCellId, 0);

  vtkDoubleArray* pointArray = vtkDoubleArray::New();
  pointArray->SetName("original_coordinates");
  pointArray->SetNumberOfComponents(3);
  pointArray->SetNumberOfTuples(number_of_points);

  vtkDoubleArray* arcLength = vtkDoubleArray::New();
  arcLength->SetName("arc_length");
  arcLength->SetNumberOfComponents(1);
  arcLength->SetNumberOfTuples(number_of_points);
  arcLength->SetValue(0, 0.0);

  outPD->CopyAllocate(inPD, number_of_points);
  double prevPoint[3] = { 0, 0, 0 };
  double curPoint[3] = { 0, 0, 0 };
  for (cc = 0; cc < number_of_points; ++cc)
  {
    xcoords->SetValue(cc, cc);
    outPD->CopyData(inPD, points[(int)cc], cc);
    memcpy(prevPoint, curPoint, sizeof(double) * 3);
    input->GetPoint(points[(int)cc], curPoint);
    pointArray->SetTuple(cc, curPoint);
    if (cc > 0)
    {
      arcLength->SetValue(cc,
        arcLength->GetValue(cc - 1) + sqrt(vtkMath::Distance2BetweenPoints(prevPoint, curPoint)));
    }
  }
  outPD->AddArray(pointArray);
  pointArray->Delete();
  outPD->AddArray(arcLength);
  arcLength->Delete();
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPolyLineToRectilinearGridFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
