/*=========================================================================

  Program:   ParaView
  Module:    vtkExtractCellsAlongLine.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkExtractCellsAlongLine.h"

#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkExtractCellsAlongPolyLine.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLineSource.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkExtractCellsAlongLine);

//----------------------------------------------------------------------------
vtkExtractCellsAlongLine::vtkExtractCellsAlongLine()
{
  this->LineSource->SetResolution(1);
  this->Extractor->SetSourceConnection(this->LineSource->GetOutputPort());
}

//----------------------------------------------------------------------------
int vtkExtractCellsAlongLine::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Check inputs / ouputs
  vtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!outInfo || !inputInfo)
  {
    vtkErrorMacro("Missing input or output information");
    return 0;
  }

  auto input = vtkDataSet::SafeDownCast(inputInfo->Get(vtkDataObject::DATA_OBJECT()));
  auto output = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!output || !input)
  {
    vtkErrorMacro("Missing input or output object");
    return 0;
  }

  this->LineSource->SetPoint1(this->Point1);
  this->LineSource->SetPoint2(this->Point2);

  this->Extractor->SetInputData(input);
  this->Extractor->Update();

  output->ShallowCopy(this->Extractor->GetOutputDataObject(0));

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractCellsAlongLine::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractCellsAlongLine::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Point1: (" << this->Point1[0] << ", " << this->Point1[1] << ", "
     << this->Point1[2] << ")" << std::endl;
  os << indent << "Point2: (" << this->Point2[0] << ", " << this->Point2[1] << ", "
     << this->Point2[2] << ")" << std::endl;
}
