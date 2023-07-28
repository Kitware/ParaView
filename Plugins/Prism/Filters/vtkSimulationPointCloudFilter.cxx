// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSimulationPointCloudFilter.h"

#include "vtkCellData.h"
#include "vtkCellDataToPointData.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergeVectorComponents.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkSMPTools.h"

#include <numeric>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkSimulationPointCloudFilter);

//------------------------------------------------------------------------------
vtkSimulationPointCloudFilter::vtkSimulationPointCloudFilter()
{
  this->AttributeType = vtkDataObject::CELL;
}

//------------------------------------------------------------------------------
vtkSimulationPointCloudFilter::~vtkSimulationPointCloudFilter() = default;

//------------------------------------------------------------------------------
void vtkSimulationPointCloudFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "AttributeType: " << vtkDataObject::GetAssociationTypeAsString(this->AttributeType) << endl;
}

//------------------------------------------------------------------------------
int vtkSimulationPointCloudFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkSimulationPointCloudFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output
  auto input = vtkDataSet::GetData(inputVector[0], 0);
  auto output = vtkPolyData::GetData(outputVector, 0);
  if (input == nullptr || output == nullptr)
  {
    vtkErrorMacro(<< "Invalid or missing input and/or output!");
    return 0;
  }
  if (input->GetNumberOfPoints() == 0)
  {
    return 1;
  }

  vtkFieldData* inFD = input->GetAttributesAsFieldData(this->AttributeType);

  // create points
  const vtkIdType numberOfOutputPoints = inFD->GetNumberOfTuples();

  // set points
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(1); // this is 1 ON PURPOSE
  output->SetPoints(points);

  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  if (this->AttributeType == vtkDataObject::POINT)
  {
    // copy point data
    outPD->PassData(inPD);
  }
  else // this->AttributeType == vtkDataObject::CELL
  {
    // interpolate point data
    vtkNew<vtkPointDataToCellData> pD2CD;
    pD2CD->SetContainerAlgorithm(this);
    pD2CD->SetInputData(input);
    pD2CD->PassPointDataOff();
    pD2CD->Update();
    outPD->PassData(pD2CD->GetOutput()->GetCellData());
  }

  // create cell array
  const vtkIdType numberOfOutputCells = numberOfOutputPoints;
  vtkNew<vtkIdTypeArray> connectivity;
  connectivity->SetNumberOfValues(numberOfOutputCells);
  vtkSMPTools::For(0, numberOfOutputCells, [&](vtkIdType begin, vtkIdType end) {
    std::iota(connectivity->GetPointer(begin), connectivity->GetPointer(end), begin);
  });
  vtkNew<vtkIdTypeArray> offsets;
  offsets->SetNumberOfValues(numberOfOutputCells + 1);
  vtkSMPTools::For(0, numberOfOutputCells + 1, [&](vtkIdType begin, vtkIdType end) {
    std::iota(offsets->GetPointer(begin), offsets->GetPointer(end), begin);
  });

  // set cell array
  vtkNew<vtkCellArray> cellArray;
  cellArray->SetData(offsets, connectivity);
  output->SetVerts(cellArray);

  // copy cell data
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  if (this->AttributeType == vtkDataObject::POINT)
  {
    // interpolate cell data
    vtkNew<vtkCellDataToPointData> cD2PD;
    cD2PD->SetContainerAlgorithm(this);
    cD2PD->SetInputData(input);
    cD2PD->PassCellDataOff();
    cD2PD->Update();
    outCD->PassData(cD2PD->GetOutput()->GetPointData());
  }
  else // this->AttributeType == vtkDataObject::CELL
  {
    // copy cell data
    outCD->PassData(inCD);
  }

  return 1;
}
