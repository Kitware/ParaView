/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVContourFilter.h"

#include "vtkAMRDualContour.h"
#include "vtkAppendPolyData.h"
#include "vtkArrayDispatch.h"
#include "vtkAssume.h"
#include "vtkCompositeDataIterator.h"
#include "vtkContour3DLinearGrid.h"
#include "vtkDataArray.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataObject.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkEventForwarderCommand.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridContour.h"
#include "vtkInformation.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include <cmath>
#include <set>

vtkStandardNewMacro(vtkPVContourFilter);

//-----------------------------------------------------------------------------
vtkPVContourFilter::vtkPVContourFilter()
  : vtkContourFilter()
{
}

//-----------------------------------------------------------------------------
vtkPVContourFilter::~vtkPVContourFilter() = default;

//-----------------------------------------------------------------------------
void vtkPVContourFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkPVContourFilter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
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

  // Check if input is AMR data.
  if (vtkHierarchicalBoxDataSet::SafeDownCast(inDataObj))
  {
    // This is a lot to go through to get the name of the array to process.
    vtkInformation* inArrayInfo = this->GetInputArrayInformation(0);
    if (!inArrayInfo)
    {
      vtkErrorMacro("Problem getting name of array to process.");
      return 0;
    }
    int fieldAssociation = -1;
    if (!inArrayInfo->Has(vtkDataObject::FIELD_ASSOCIATION()))
    {
      vtkErrorMacro("Unable to query field association for the scalar.");
      return 0;
    }
    fieldAssociation = inArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());
    if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
      vtkSmartPointer<vtkAMRDualContour> amrDC(vtkSmartPointer<vtkAMRDualContour>::New());

      amrDC->SetInputData(0, inDataObj);
      amrDC->SetInputArrayToProcess(0, inArrayInfo);
      amrDC->SetEnableCapping(1);
      amrDC->SetEnableDegenerateCells(1);
      amrDC->SetEnableMultiProcessCommunication(1);
      amrDC->SetSkipGhostCopy(1);
      amrDC->SetTriangulateCap(1);
      amrDC->SetEnableMergePoints(1);

      for (int i = 0; i < this->GetNumberOfContours(); ++i)
      {
        vtkSmartPointer<vtkMultiBlockDataSet> out(vtkSmartPointer<vtkMultiBlockDataSet>::New());
        amrDC->SetIsoValue(this->GetValue(i));
        amrDC->Update();
        out->ShallowCopy(amrDC->GetOutput(0));
        vtkMultiBlockDataSet::SafeDownCast(outDataObj)->SetBlock(i, out);
      }
      return 1;
    }
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

  // See if we can delegate to the faster vtkContour3DLinearGrid for this dataset and settings
  // Note: vtkContour3DLinearGrid does not support the ComputeScalars option.
  bool useLinear3DContour = this->ComputeScalars == 0 &&
    vtkContour3DLinearGrid::CanFullyProcessDataObject(inDataObj, array->GetName());

  if (useLinear3DContour)
  {
    vtkNew<vtkContour3DLinearGrid> linear3DContour;
    linear3DContour->SetNumberOfContours(this->GetNumberOfContours());
    for (int i = 0; i < this->GetNumberOfContours(); ++i)
    {
      linear3DContour->SetValue(i, this->GetValue(i));
    }
    linear3DContour->SetMergePoints(this->GetLocator() != nullptr);
    linear3DContour->SetInterpolateAttributes(true);
    linear3DContour->SetComputeNormals(this->GetComputeNormals());
    linear3DContour->SetOutputPointsPrecision(this->GetOutputPointsPrecision());
    linear3DContour->SetUseScalarTree(this->GetUseScalarTree());
    linear3DContour->SetScalarTree(this->GetScalarTree());
    linear3DContour->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
    vtkNew<vtkEventForwarderCommand> progressForwarder;
    progressForwarder->SetTarget(this);
    linear3DContour->AddObserver(vtkCommand::ProgressEvent, progressForwarder);
    auto retval = linear3DContour->ProcessRequest(request, inputVector, outputVector);

    return retval;
  }

  return this->ContourUsingSuperclass(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
int vtkPVContourFilter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkHierarchicalBoxDataSet* input = vtkHierarchicalBoxDataSet::GetData(inInfo);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (input)
  {
    vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outInfo);
    if (!output)
    {
      output = vtkMultiBlockDataSet::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->Delete();
    }
    return 1;
  }
  else
  {
    vtkDataSet* output = vtkDataSet::GetData(outInfo);
    if (!output)
    {
      output = vtkPolyData::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->Delete();
    }
    return 1;
  }
}

//----------------------------------------------------------------------------
int vtkPVContourFilter::ContourUsingSuperclass(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  vtkCompositeDataSet* inputCD = vtkCompositeDataSet::SafeDownCast(inputDO);
  if (!inputCD)
  {
    auto retval = this->Superclass::RequestData(request, inputVector, outputVector);
    if (retval)
    {
      if (auto polydata = vtkPolyData::GetData(outputVector, 0))
      {
        this->CleanOutputScalars(polydata->GetPointData()->GetScalars());
      }
    }
    return retval;
  }

  vtkCompositeDataSet* outputCD = vtkCompositeDataSet::SafeDownCast(outputDO);
  outputCD->CopyStructure(inputCD);

  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(inputCD->NewIterator());

  // for input.
  vtkSmartPointer<vtkInformationVector> newInInfoVec = vtkSmartPointer<vtkInformationVector>::New();
  vtkSmartPointer<vtkInformation> newInInfo = vtkSmartPointer<vtkInformation>::New();
  newInInfoVec->SetInformationObject(0, newInInfo);

  // for output.
  vtkSmartPointer<vtkInformationVector> newOutInfoVec =
    vtkSmartPointer<vtkInformationVector>::New();
  vtkSmartPointer<vtkInformation> newOutInfo = vtkSmartPointer<vtkInformation>::New();
  newOutInfoVec->SetInformationObject(0, newOutInfo);

  // Loop over all the datasets.
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    newInInfo->Set(vtkDataObject::DATA_OBJECT(), iter->GetCurrentDataObject());
    vtkPolyData* polydata = vtkPolyData::New();
    newOutInfo->Set(vtkDataObject::DATA_OBJECT(), polydata);
    polydata->FastDelete();

    vtkInformationVector* newInInfoVecPtr = newInInfoVec.GetPointer();
    if (!this->Superclass::RequestData(request, &newInInfoVecPtr, newOutInfoVec.GetPointer()))
    {
      return 0;
    }
    this->CleanOutputScalars(polydata->GetPointData()->GetScalars());
    outputCD->SetDataSet(iter, polydata);
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkPVContourFilter::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkPVContourFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);

  // According to the documentation this is the way to append additional
  // input data set type since VTK 5.2.
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHierarchicalBoxDataSet");
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
