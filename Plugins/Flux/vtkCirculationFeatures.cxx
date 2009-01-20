// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCirculationFeatures.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkCirculationFeatures.h"

#include "vtkCellData.h"
#include "vtkCellDataToPointData.h"
#include "vtkDataSet.h"
#include "vtkFluxVectors.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//=============================================================================
vtkCxxRevisionMacro(vtkCirculationFeatures, "1.1");
vtkStandardNewMacro(vtkCirculationFeatures);

//-----------------------------------------------------------------------------
vtkCirculationFeatures::vtkCirculationFeatures()
{
  this->SetInputArray(vtkDataSetAttributes::SCALARS);
}

vtkCirculationFeatures::~vtkCirculationFeatures()
{
}

void vtkCirculationFeatures::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkCirculationFeatures::SetInputArray(const char *name)
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS,
                               name);
}

void vtkCirculationFeatures::SetInputArray(int fieldAttributeType)
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS,
                               fieldAttributeType);
}

//-----------------------------------------------------------------------------
int vtkCirculationFeatures::RequestData(vtkInformation *vtkNotUsed(request),
                                        vtkInformationVector **inputVector,
                                        vtkInformationVector *outputVector)
{
  vtkDataSet *input = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet *output = vtkDataSet::GetData(outputVector);

  if (!input || !output)
    {
    vtkErrorMacro(<< "Missing input or output?");
    return 0;
    }

  vtkSmartPointer<vtkDataArray> circulation
    = this->GetInputArrayToProcess(0, inputVector);

  if (circulation == NULL)
    {
    vtkDebugMacro("No input circulation.");
    return 1;
    }
  if (circulation->GetName() == NULL)
    {
    vtkErrorMacro("Input array needs a name.");
    return 0;
    }

  vtkSmartPointer<vtkDataSet> inputCopy;
  inputCopy.TakeReference(input->NewInstance());
  inputCopy->ShallowCopy(input);

  if (circulation->GetNumberOfComponents() == 1)
    {
    // Convert scalars to vectors.
    VTK_CREATE(vtkFluxVectors, fluxVectors);
    fluxVectors->SetInput(inputCopy);
    fluxVectors->SetInputArray(circulation->GetName());
    fluxVectors->Update();
    inputCopy = fluxVectors->GetOutput();
    circulation = inputCopy->GetCellData()->GetArray(circulation->GetName());
    }

  if (circulation->GetNumberOfComponents() != 3)
    {
    vtkErrorMacro("Input array needs 1 or 3 components.");
    return 0;
    }

  // Use the cell data to point data filter to get average circulation at each
  // point.
  VTK_CREATE(vtkCellDataToPointData, cell2point);
  cell2point->SetInput(inputCopy);
  cell2point->Update();
  vtkSmartPointer<vtkDataArray> avgPtCirculation
    = cell2point->GetOutput()->GetPointData()->GetArray(circulation->GetName());

  vtkIdType numPoints = input->GetNumberOfPoints();

  VTK_CREATE(vtkUnsignedCharArray, featureMask);
  featureMask->SetName("Features");
  featureMask->SetNumberOfComponents(1);
  featureMask->SetNumberOfTuples(numPoints);

  VTK_CREATE(vtkIdList, ptList);
  ptList->SetNumberOfIds(1);
  VTK_CREATE(vtkIdList, cellList);

  for (vtkIdType ptId = 0; ptId < numPoints; ptId++)
    {
    ptList->SetId(0, ptId);
    input->GetCellNeighbors(-1, ptList, cellList);

    featureMask->SetValue(ptId, 0);
    double avgFlow[3];   avgPtCirculation->GetTuple(ptId, avgFlow);
    for (vtkIdType i = 0; i < cellList->GetNumberOfIds(); i++)
      {
      vtkIdType cellId = cellList->GetId(i);
      double flow[3];  circulation->GetTuple(cellId, flow);
      double dot = vtkMath::Dot(avgFlow, flow);
      if ((dot < 0) || ((dot == 0) && vtkMath::Dot(flow,flow) > 0))
        {
        featureMask->SetValue(ptId, 1);
        break;
        }
      }
    }

  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  output->GetPointData()->AddArray(featureMask);

  return 1;
}

