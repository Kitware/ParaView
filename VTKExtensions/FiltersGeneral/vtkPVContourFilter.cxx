// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVContourFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkAssume.h"
#include "vtkDataArray.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataObject.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkEventForwarderCommand.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"

#include <cmath>
#include <set>

vtkStandardNewMacro(vtkPVContourFilter);

//-----------------------------------------------------------------------------
vtkPVContourFilter::vtkPVContourFilter() = default;

//-----------------------------------------------------------------------------
vtkPVContourFilter::~vtkPVContourFilter() = default;

//-----------------------------------------------------------------------------
void vtkPVContourFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkPVContourFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    vtkErrorMacro("Failed to get input information.");
    return 1;
  }

  vtkDataObject* inDataObj = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!inDataObj)
  {
    vtkErrorMacro("Failed to get input data object.");
    return 1;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("Failed to get output information.");
    return 1;
  }

  vtkDataObject* outDataObj = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!outDataObj)
  {
    vtkErrorMacro("Failed get output data object.");
    return 1;
  }

  // Check if input is hyper tree grid
  if (vtkHyperTreeGrid::SafeDownCast(inDataObj))
  {
    vtkInformation* inArrayInfo = this->GetInputArrayInformation(0);
    if (!inArrayInfo)
    {
      vtkErrorMacro("Problem getting name of array to process.");
      return 0;
    }

    // WARNING
    // Compute Normals and Generate Triangles do not apply to Hyper Tree Grids

    vtkNew<vtkHyperTreeGridContour> contourFilter;
    contourFilter->SetInputData(0, inDataObj);
    contourFilter->SetInputArrayToProcess(0, inArrayInfo);
    contourFilter->SetStrategy3D(this->HTGStrategy3D);
    contourFilter->SetUseImplicitArrays(this->UseImplicitArraysHTG);
    for (vtkIdType i = 0; i < this->GetNumberOfContours(); ++i)
    {
      contourFilter->SetValue(i, this->GetValue(i));
    }

    contourFilter->Update();
    vtkPolyData::SafeDownCast(outDataObj)->ShallowCopy(contourFilter->GetOutput(0));

    return 1;
  }

  vtkDataArray* array = this->GetInputArrayToProcess(0, inputVector);
  if (!array)
  {
    vtkLog(INFO, "Contour array is null.");
    return 1;
  }

  return this->ContourUsingSuperclass(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVContourFilter::ContourUsingSuperclass(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // instantiate the superclass so as to use the object factory
  vtkNew<Superclass> instance;
  instance->SetNumberOfContours(this->GetNumberOfContours());
  for (int i = 0; i < this->GetNumberOfContours(); ++i)
  {
    instance->SetValue(i, this->GetValue(i));
  }
  instance->SetComputeNormals(this->GetComputeNormals());
  instance->SetComputeGradients(this->GetComputeGradients());
  instance->SetComputeScalars(this->GetComputeScalars());
  instance->SetUseScalarTree(this->GetUseScalarTree());
  instance->SetScalarTree(this->GetScalarTree());
  instance->SetLocator(this->GetLocator());
  instance->SetArrayComponent(this->GetArrayComponent());
  instance->SetGenerateTriangles(this->GetGenerateTriangles());
  instance->SetOutputPointsPrecision(this->GetOutputPointsPrecision());
  instance->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));

  vtkNew<vtkEventForwarderCommand> progressForwarder;
  progressForwarder->SetTarget(this);
  instance->AddObserver(vtkCommand::ProgressEvent, progressForwarder);

  vtkDataSet* inputDS = vtkDataSet::GetData(inputVector[0], 0);
  vtkPolyData* outputPD = vtkPolyData::GetData(outputVector, 0);

  instance->SetInputDataObject(inputDS);
  instance->Update();
  auto polydata = instance->GetOutput();
  this->CleanOutputScalars(polydata->GetPointData()->GetScalars());
  outputPD->ShallowCopy(polydata);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkPVContourFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);

  // According to the documentation this is the way to append additional
  // input data set type since VTK 5.2.
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkNonOverlappingAMR");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}

//-----------------------------------------------------------------------------
namespace
{
struct Cleaner
{
  std::set<double> Values;

  template <typename ArrayT>
  void operator()(ArrayT* scalars) const
  {
    VTK_ASSUME(scalars->GetNumberOfComponents() == 1);
    vtkDataArrayAccessor<ArrayT> accessor(scalars);
    using ValueT = typename vtkDataArrayAccessor<ArrayT>::APIType;

    vtkSMPTools::For(
      0, scalars->GetNumberOfTuples(), [this, &accessor](vtkIdType begin, vtkIdType end) {
        for (vtkIdType cc = begin; cc < end; ++cc)
        {
          accessor.Set(
            cc, 0, static_cast<ValueT>(this->GetClosest(static_cast<double>(accessor.Get(cc, 0)))));
        }
      });
  }

  double GetClosest(const double& val) const
  {
    double delta = VTK_DOUBLE_MAX;
    double closeset_val = val;
    for (const auto& set_val : this->Values)
    {
      const auto curDelta = std::abs(val - set_val);
      if (curDelta < delta)
      {
        closeset_val = set_val;
        delta = curDelta;
      }
    }
    return closeset_val;
  }
};
}

void vtkPVContourFilter::CleanOutputScalars(vtkDataArray* outScalars)
{
  if (outScalars)
  {
    Cleaner worker;
    const auto values = this->GetValues();
    for (int cc = 0, max = this->GetNumberOfContours(); cc < max; ++cc)
    {
      worker.Values.insert(values[cc]);
    }

    using DispatcherT = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
    if (!DispatcherT::Execute(outScalars, worker))
    {
      worker(outScalars);
    }
  }
}
