/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProbeLineFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVProbeLineFilter.h"

#include "vtkDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLineSource.h"
#include "vtkObjectFactory.h"
#include "vtkProbeLineFilter.h"

vtkStandardNewMacro(vtkPVProbeLineFilter);

//----------------------------------------------------------------------------
vtkPVProbeLineFilter::vtkPVProbeLineFilter()
{
  this->LineSource->SetResolution(1);
  this->Prober->SetAggregateAsPolyData(true);
  this->Prober->SetSourceConnection(this->LineSource->GetOutputPort());
}

//----------------------------------------------------------------------------
int vtkPVProbeLineFilter::RequestData(
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

  vtkDataObject* input = inputInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output || !input)
  {
    vtkErrorMacro("Missing input or output object");
    return 0;
  }

  this->LineSource->SetPoint1(this->Point1);
  this->LineSource->SetPoint2(this->Point2);
  this->Prober->SetLineResolution(this->LineResolution);
  this->Prober->SetPassCellArrays(this->PassCellArrays);
  this->Prober->SetPassPointArrays(this->PassPointArrays);
  this->Prober->SetPassFieldArrays(this->PassFieldArrays);
  this->Prober->SetPassPartialArrays(this->PassPartialArrays);
  this->Prober->SetTolerance(this->Tolerance);
  this->Prober->SetComputeTolerance(this->ComputeTolerance);
  this->Prober->SetSamplingPattern(this->SamplingPattern);

  this->Prober->SetInputData(input);
  this->Prober->Update();
  output->ShallowCopy(this->Prober->GetOutputDataObject(0));

  return 1;
}

//----------------------------------------------------------------------------
int vtkPVProbeLineFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
      info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
      info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
      return 1;

    default:
      return 0;
  }
}

//----------------------------------------------------------------------------
void vtkPVProbeLineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  switch (this->SamplingPattern)
  {
    case vtkProbeLineFilter::SAMPLE_LINE_AT_CELL_BOUNDARIES:
      os << indent << "SamplingPattern: SAMPLE_LINE_AT_CELL_BOUNDARIES" << endl;
      break;
    case vtkProbeLineFilter::SAMPLE_LINE_AT_SEGMENT_CENTERS:
      os << indent << "SamplingPattern: SAMPLE_LINE_AT_SEGMENT_CENTERS" << endl;
      break;
    case vtkProbeLineFilter::SAMPLE_LINE_UNIFORMLY:
      os << indent << "SamplingPattern: SAMPLE_LINE_UNIFORMLY" << endl;
      break;
    default:
      os << indent << "SamplingPattern: UNDEFINED" << endl;
      break;
  }
  os << indent << "LineResolution: " << this->LineResolution << endl;
  os << indent << "PassPartialArrays: " << this->PassPartialArrays << endl;
  os << indent << "PassCellArrays: " << this->PassCellArrays << endl;
  os << indent << "PassPointArrays: " << this->PassPointArrays << endl;
  os << indent << "PassFieldArrays: " << this->PassFieldArrays << endl;
  os << indent << "ComputeTolerance: " << this->ComputeTolerance << endl;
  os << indent << "Tolerance: " << this->Tolerance << endl;
}
