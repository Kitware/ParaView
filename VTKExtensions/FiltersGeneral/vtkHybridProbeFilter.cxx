/*=========================================================================

  Program:   ParaView
  Module:    vtkHybridProbeFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHybridProbeFilter.h"

#include "vtkCompositeDataSet.h"
#include "vtkExtractSelection.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergeBlocks.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPProbeFilter.h"
#include "vtkPointSource.h"
#include "vtkSelectionNode.h"
#include "vtkSelectionSource.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkHybridProbeFilter);
//----------------------------------------------------------------------------
vtkHybridProbeFilter::vtkHybridProbeFilter()
  : Mode(vtkHybridProbeFilter::INTERPOLATE_AT_LOCATION)
{
  this->Location[0] = this->Location[1] = this->Location[2] = 0.0;
}

//----------------------------------------------------------------------------
vtkHybridProbeFilter::~vtkHybridProbeFilter()
{
}

//----------------------------------------------------------------------------
int vtkHybridProbeFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkHybridProbeFilter::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkHybridProbeFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outputVector, 0);

  switch (this->Mode)
  {
    case INTERPOLATE_AT_LOCATION:
      return this->InterpolateAtLocation(input, output) ? 1 : 0;

    case EXTRACT_CELL_CONTAINING_LOCATION:
      return this->ExtractCellContainingLocation(input, output) ? 1 : 0;
  }

  return 0;
}

//----------------------------------------------------------------------------
bool vtkHybridProbeFilter::InterpolateAtLocation(vtkDataObject* input, vtkUnstructuredGrid* output)
{
  vtkNew<vtkPointSource> pointSource;
  pointSource->SetNumberOfPoints(1);
  pointSource->SetCenter(this->Location);
  pointSource->SetRadius(0.0);
  pointSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  vtkNew<vtkPProbeFilter> probe;
  probe->SetInputConnection(0, pointSource->GetOutputPort());
  probe->SetInputDataObject(1, input);
  probe->Update();

  output->ShallowCopy(probe->GetOutputDataObject(0));
  return true;
}

//----------------------------------------------------------------------------
bool vtkHybridProbeFilter::ExtractCellContainingLocation(
  vtkDataObject* input, vtkUnstructuredGrid* output)
{
  vtkNew<vtkSelectionSource> selSource;
  selSource->AddLocation(this->Location[0], this->Location[1], this->Location[2]);
  selSource->SetContentType(vtkSelectionNode::LOCATIONS);
  selSource->SetFieldType(vtkSelectionNode::CELL);

  vtkNew<vtkExtractSelection> extractor;
  extractor->SetInputDataObject(0, input);
  extractor->SetInputConnection(1, selSource->GetOutputPort());
  extractor->PreserveTopologyOff();
  extractor->Update();

  if (vtkCompositeDataSet::SafeDownCast(input))
  {
    vtkNew<vtkMergeBlocks> merger;
    merger->SetInputDataObject(extractor->GetOutputDataObject(0));
    merger->Update();
    output->ShallowCopy(merger->GetOutputDataObject(0));
  }
  else
  {
    output->ShallowCopy(extractor->GetOutputDataObject(0));
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkHybridProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mode: " << this->Mode << endl;
  os << indent << "Location: " << this->Location[0] << ", " << this->Location[1] << ", "
     << this->Location[2] << endl;
}
