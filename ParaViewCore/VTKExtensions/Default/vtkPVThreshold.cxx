/*=========================================================================

  Program:   ParaView
  Module:    vtkPVThreshold.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVThreshold.h"

#include "vtkAppendFilter.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridThreshold.h"
#include "vtkInformation.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSetGet.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkPVThreshold);

//----------------------------------------------------------------------------
vtkPVThreshold::vtkPVThreshold()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;
  this->ThresholdMethod = THRESHOLD_BETWEEN;
  this->AllScalars = 1;
  this->AttributeMode = -1;
  this->ComponentMode = VTK_COMPONENT_MODE_USE_SELECTED;
  this->SelectedComponent = 0;
  this->OutputPointsPrecision = DEFAULT_PRECISION;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);

  this->UseContinuousCellRange = 0;
  this->Invert = false;

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPVThreshold::~vtkPVThreshold()
{
}

//----------------------------------------------------------------------------
void vtkPVThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Attribute Mode: " << this->GetAttributeModeAsString() << endl;
  os << indent << "Component Mode: " << this->GetComponentModeAsString() << endl;
  os << indent << "Selected Component: " << this->SelectedComponent << endl;

  os << indent << "All Scalars: " << this->AllScalars << "\n";

  os << indent << "Lower Threshold: " << this->LowerThreshold << "\n";
  os << indent << "Upper Threshold: " << this->UpperThreshold << "\n";
  os << indent << "Precision of the output points: " << this->OutputPointsPrecision << "\n";
  os << indent << "Use Continuous Cell Range: " << this->UseContinuousCellRange << endl;
}

//----------------------------------------------------------------------------
const char* vtkPVThreshold::GetAttributeModeAsString()
{
  if (this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT)
  {
    return "Default";
  }
  else if (this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA)
  {
    return "UsePointData";
  }
  else
  {
    return "UseCellData";
  }
}

//----------------------------------------------------------------------------
void vtkPVThreshold::SetInputData(vtkDataObject* input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
void vtkPVThreshold::ThresholdByUpper(double upper)
{
  if (this->LowerThreshold != -std::numeric_limits<double>::infinity() ||
    this->UpperThreshold != upper)
  {
    this->LowerThreshold = -std::numeric_limits<double>::infinity();
    this->UpperThreshold = upper;
    this->Modified();
  }
  this->ThresholdMethod = THRESHOLD_BY_UPPER;
}

//----------------------------------------------------------------------------
void vtkPVThreshold::ThresholdByLower(double lower)
{
  if (this->LowerThreshold != lower ||
    this->UpperThreshold != std::numeric_limits<double>::infinity())
  {
    this->LowerThreshold = lower;
    this->UpperThreshold = std::numeric_limits<double>::infinity();
    this->Modified();
  }
  this->ThresholdMethod = THRESHOLD_BY_LOWER;
}

//----------------------------------------------------------------------------
void vtkPVThreshold::ThresholdBetween(double lower, double upper)
{
  if (this->LowerThreshold != lower || this->UpperThreshold != upper)
  {
    this->LowerThreshold = lower;
    this->UpperThreshold = upper;
    this->Modified();
  }
  this->ThresholdMethod = THRESHOLD_BETWEEN;
}

//----------------------------------------------------------------------------
const char* vtkPVThreshold::GetComponentModeAsString()
{
  if (this->ComponentMode == VTK_COMPONENT_MODE_USE_SELECTED)
  {
    return "UseSelected";
  }
  else if (this->ComponentMode == VTK_COMPONENT_MODE_USE_ANY)
  {
    return "UseAny";
  }
  else
  {
    return "UseAll";
  }
}

//----------------------------------------------------------------------------
int vtkPVThreshold::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (!inInfo)
  {
    vtkErrorMacro(<< "Failed to get input information.");
    return 0;
  }

  vtkDataObject* inDataObj = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (!inDataObj)
  {
    vtkErrorMacro(<< "Failed to get input data object.");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro(<< "Failed to get output information.");
  }

  vtkDataObject* outDataObj = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!outDataObj)
  {
    vtkErrorMacro(<< "Failed to get output data object.");
  }

  if (vtkDataSet::SafeDownCast(inDataObj))
  {
    vtkNew<vtkThreshold> thresholdFilter;
    thresholdFilter->SetSelectedComponent(this->SelectedComponent);
    switch (this->ThresholdMethod)
    {
      case THRESHOLD_BY_LOWER:
        thresholdFilter->ThresholdByLower(this->LowerThreshold);
        break;
      case THRESHOLD_BY_UPPER:
        thresholdFilter->ThresholdByUpper(this->UpperThreshold);
        break;
      case THRESHOLD_BETWEEN:
        thresholdFilter->ThresholdBetween(this->LowerThreshold, this->UpperThreshold);
        break;
      default:
        vtkErrorMacro(<< "Could not find threshold method");
        return 0;
    }
    thresholdFilter->SetAttributeMode(this->AttributeMode);
    thresholdFilter->SetComponentMode(this->ComponentMode);
    thresholdFilter->SetAllScalars(this->AllScalars);
    thresholdFilter->SetUseContinuousCellRange(this->UseContinuousCellRange);
    thresholdFilter->SetInvert(this->Invert);
    thresholdFilter->SetOutputPointsPrecision(this->OutputPointsPrecision);

    vtkDataObject* inputClone = inDataObj->NewInstance();
    inputClone->ShallowCopy(inDataObj);
    thresholdFilter->SetInputData(0, inputClone);
    inputClone->FastDelete();

    thresholdFilter->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
    thresholdFilter->Update();
    vtkUnstructuredGrid::SafeDownCast(outDataObj)->ShallowCopy(thresholdFilter->GetOutput(0));

    return 1;
  }
  else if (vtkHyperTreeGrid::SafeDownCast(inDataObj))
  {
    vtkNew<vtkHyperTreeGridThreshold> thresholdFilter;
    thresholdFilter->ThresholdBetween(this->LowerThreshold, this->UpperThreshold);

    vtkDataObject* inputClone = inDataObj->NewInstance();
    inputClone->ShallowCopy(inDataObj);
    thresholdFilter->SetInputData(0, inputClone);
    inputClone->FastDelete();

    thresholdFilter->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
    thresholdFilter->Update();
    outDataObj->ShallowCopy(thresholdFilter->GetOutput(0));

    return 1;
  }
  else
  {
    vtkErrorMacro(<< "Failed to process data: needs to be vtkHyperTreeGrid or vtkDataSet");
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVThreshold::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVThreshold::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkHyperTreeGrid* input = vtkHyperTreeGrid::GetData(inInfo);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (input)
  {
    vtkHyperTreeGrid* output = vtkHyperTreeGrid::GetData(outInfo);
    if (!output)
    {
      output = vtkHyperTreeGrid::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->FastDelete();
    }
    return 1;
  }
  else
  {
    vtkDataSet* output = vtkDataSet::GetData(outInfo);
    if (!output)
    {
      output = vtkUnstructuredGrid::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->FastDelete();
    }
    return 1;
  }
}

//----------------------------------------------------------------------------
int vtkPVThreshold::FillInputPortInformation(int, vtkInformation* info)
{
  vtkInformationStringVectorKey::SafeDownCast(
    info->GetKey(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()))
    ->Append(info, "vtkDataSet");
  vtkInformationStringVectorKey::SafeDownCast(
    info->GetKey(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()))
    ->Append(info, "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVThreshold::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

int vtkPVThreshold::GetOutputPointsPrecision() const
{
  return this->OutputPointsPrecision;
}
