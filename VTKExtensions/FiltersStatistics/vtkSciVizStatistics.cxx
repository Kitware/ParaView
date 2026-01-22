// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSciVizStatistics.h"
#include "vtkSciVizStatisticsPrivate.h"

#include "vtkAlgorithm.h"
#include "vtkCellAttribute.h"
#include "vtkCellData.h"
#include "vtkCellGrid.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkExtractStatisticalModelTables.h"
#include "vtkGenerateStatistics.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkStatisticsAlgorithm.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVariantArray.h"

#include <set>
#include <sstream>

namespace
{

/// Return the number of components the given array name has (or 0 if not found).
int NumberOfArrayComponents(vtkDataObject* data, int fieldAssoc, const char* arrayName)
{
  int nc = 0;
  if (auto* compData = vtkCompositeDataSet::SafeDownCast(data))
  {
    bool foundOne = false;
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(compData->NewIterator());
    iter->SkipEmptyNodesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      auto* leafObj = iter->GetCurrentDataObject();
      if (!vtkCompositeDataSet::SafeDownCast(leafObj))
      {
        int leafNC = NumberOfArrayComponents(leafObj, fieldAssoc, arrayName);
        if (leafNC > 0)
        {
          foundOne = true;
        }
        nc = std::max(nc, leafNC);
      }
    }
    if (!foundOne && nc == 0)
    {
      nc = VTK_INT_MAX;
    }
  }
  else if (auto* dataSet = vtkDataSet::SafeDownCast(data))
  {
    if (auto* dsa = dataSet->GetAttributes(fieldAssoc))
    {
      if (auto* array = dsa->GetArray(arrayName))
      {
        nc = array->GetNumberOfComponents();
      }
    }
  }
  else if (auto* table = vtkTable::SafeDownCast(data))
  {
    if (fieldAssoc != vtkDataObject::FIELD_ASSOCIATION_ROWS)
    {
      vtkGenericWarningMacro(<< "Tables have only row attributes but you asked for an attribute "
                             << " of type " << fieldAssoc << ".");
    }
    if (auto* dsa = table->GetAttributes(fieldAssoc))
    {
      if (auto* array = dsa->GetArray(arrayName))
      {
        nc = array->GetNumberOfComponents();
      }
    }
  }
  else if (auto* cellGrid = vtkCellGrid::SafeDownCast(data))
  {
    if (fieldAssoc != vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
      vtkGenericWarningMacro(
        << "Cell grids have only cell attributes but you asked for an attribute "
        << " of type " << fieldAssoc << ".");
    }
    else
    {
      if (auto* cellAtt = cellGrid->GetCellAttributeByName(arrayName))
      {
        nc = cellAtt->GetNumberOfComponents();
      }
    }
  }
  else
  {
    // Data is not present on this rank; report a large value.
    nc = VTK_INT_MAX;
  }
  return nc;
}

}

vtkCxxSetObjectMacro(vtkSciVizStatistics, Controller, vtkMultiProcessController);

vtkInformationKeyMacro(vtkSciVizStatistics, MULTIPLE_MODELS, Integer);

vtkSciVizStatistics::vtkSciVizStatistics()
{
  this->P = new vtkSciVizStatisticsP;
  this->AttributeMode = vtkDataObject::POINT;
  this->TrainingFraction = 0.1;
  this->Task = MODEL_AND_ASSESS;
  this->SetNumberOfInputPorts(2);  // data + optional model
  this->SetNumberOfOutputPorts(2); // model + assessed input
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

vtkSciVizStatistics::~vtkSciVizStatistics()
{
  this->SetController(nullptr);
  delete this->P;
}

void vtkSciVizStatistics::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Task: " << this->Task << "\n";
  os << indent << "AttributeMode: " << this->AttributeMode << "\n";
  os << indent << "TrainingFraction: " << this->TrainingFraction << "\n";
}

int vtkSciVizStatistics::GetNumberOfAttributeArrays()
{
  vtkDataObject* dobj = this->GetInputDataObject(0, 0); // First input is always the leader
  if (!dobj)
  {
    return 0;
  }

  vtkFieldData* fdata = dobj->GetAttributesAsFieldData(this->AttributeMode);
  if (!fdata)
  {
    return 0;
  }

  return fdata->GetNumberOfArrays();
}

const char* vtkSciVizStatistics::GetAttributeArrayName(int n)
{
  vtkDataObject* dobj = this->GetInputDataObject(0, 0); // First input is always the leader
  if (!dobj)
  {
    return nullptr;
  }

  vtkFieldData* fdata = dobj->GetAttributesAsFieldData(this->AttributeMode);
  if (!fdata)
  {
    return nullptr;
  }

  int numArrays = fdata->GetNumberOfArrays();
  if (n < 0 || n > numArrays)
  {
    return nullptr;
  }

  vtkAbstractArray* arr = fdata->GetAbstractArray(n);
  if (!arr)
  {
    return nullptr;
  }

  return arr->GetName();
}

int vtkSciVizStatistics::GetAttributeArrayStatus(const char* arrName)
{
  return this->P->Has(arrName) ? 1 : 0;
}

void vtkSciVizStatistics::EnableAttributeArray(const char* arrName)
{
  if (arrName)
  {
    if (this->P->SetBufferColumnStatus(arrName, 1))
    {
      this->Modified();
    }
  }
}

void vtkSciVizStatistics::ClearAttributeArrays()
{
  bool hadRequests = this->P->GetNumberOfRequests() > 0;
  if (this->P->ResetBuffer() || hadRequests)
  {
    this->P->ResetRequests();
    this->Modified();
  }
}

bool vtkSciVizStatistics::PrepareInputArrays(
  vtkDataObject* inData, vtkGenerateStatistics* modelData)
{
  // I. Generate a map of array names to numbers of components locally.
  std::map<vtkStringToken::Hash, int> arrayNameToNumberOfComponents;
  this->P->AddBufferToRequests();
  for (const auto& req : this->P->Requests)
  {
    for (const auto& arrayName : req)
    {
      if (arrayName.empty())
      {
        continue;
      }
      vtkStringToken arrayToken(arrayName);
      int localNumComps = NumberOfArrayComponents(inData, this->AttributeMode, arrayName.c_str());
      arrayNameToNumberOfComponents[arrayToken.GetId()] = localNumComps;
    }
  }
  // II. Flatten the map to an array of numbers of components. Because all ranks have
  //     identical requests, the vectors will all be sized and ordered identically.
  std::vector<int> numComps;
  numComps.reserve(arrayNameToNumberOfComponents.size());
  for (auto [key, value] : arrayNameToNumberOfComponents)
  {
    (void)key;
    numComps.push_back(value);
  }

  // II. Take the minimum number of components globally for each array. (Reduce with MIN_OP.)
  //     Then update the map to reflect the global number of components.
  //     For single-process runs, the map does not need to be updated.
  if (this->Controller)
  {
    std::vector<int> globalNumComps;
    globalNumComps.resize(numComps.size());
    this->Controller->AllReduce(numComps.data(), globalNumComps.data(),
      static_cast<vtkIdType>(numComps.size()), vtkCommunicator::StandardOperations::MIN_OP);
    int idx = 0;
    for (auto& entry : arrayNameToNumberOfComponents)
    {
      entry.second = globalNumComps[idx++];
      if (entry.second == VTK_INT_MAX)
      {
        // No ranks reported any data. Discard this array.
        entry.second = 0;
      }
    }
  }
  // III. Build requests using globally available numbers of components, not those available
  // locally.
  int arrayIdx = 0;
  for (const auto& req : this->P->Requests)
  {
    for (const auto& arrayName : req)
    {
      if (arrayName.empty())
      {
        continue;
      }
      vtkStringToken arrayToken(arrayName);
      int numCompsForArray = arrayNameToNumberOfComponents[arrayToken.GetId()];
      if (numCompsForArray <= 0)
      {
        continue;
      }
      for (int cc = (numCompsForArray > 1 ? -2 : 0); cc < numCompsForArray; ++cc)
      {
        modelData->SetInputArrayToProcess(
          arrayIdx++, 0, 0, this->AttributeMode, arrayName.c_str(), cc);
        if (cc == -2)
        {
          ++cc; /* skip the L₁ norm for now */
        }
      }
    }
  }
  return true;
}

int vtkSciVizStatistics::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  if (port == 0)
  {
    return 1;
  }
  else if (port >= 1 && port <= 2)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
  }
  return 0;
}

int vtkSciVizStatistics::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSetCollection");
    return 1;
  }
  return this->Superclass::FillOutputPortInformation(port, info);
}

int vtkSciVizStatistics::RequestData(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** input, vtkInformationVector* output)
{
  double trainingFraction = 1.;
  switch (this->Task)
  {
    case Tasks::MODEL_INPUT:
      break;
    case Tasks::CREATE_MODEL:
      trainingFraction = this->TrainingFraction;
      break;
    default:
    case Tasks::ASSESS_INPUT:
    case Tasks::MODEL_AND_ASSESS:
      vtkErrorMacro("Tasks that involve assessing data are no longer supported.");
      return 0;
      break;
  }

  // Input 0: Data for learning/assessment.
  // If this is composite data, both outputs must be composite datasets with the same structure.
  vtkInformation* iinfo = input[0]->GetInformationObject(0);
  vtkDataObject* inData = iinfo->Get(vtkDataObject::DATA_OBJECT());

  // Output 0: Model
  // The output model type must be a partitioned dataset collection
  vtkInformation* oinfom = output->GetInformationObject(0);
  auto* ouModel = vtkPartitionedDataSetCollection::GetData(oinfom);

  vtkNew<vtkGenerateStatistics> modelData;
  modelData->SetController(this->Controller);
  modelData->SetInputDataObject(0, inData);
  modelData->SetTrainingFraction(this->Task == Tasks::MODEL_INPUT ? 1.0 : this->TrainingFraction);

  // Ensure the same arrays+components are requested across all ranks, even
  // those with no data.
  this->PrepareInputArrays(inData, modelData);

  // Subclasses override this method and call modelData->SetStatisticsAlgorithm()
  // inside it with a properly-configured algorithm.
  this->PrepareAlgorithm(modelData);

  vtkNew<vtkExtractStatisticalModelTables> modelToTables;
  modelToTables->SetInputConnection(0, modelData->GetOutputPort());
  modelToTables->UpdatePiece(this->Controller ? this->Controller->GetLocalProcessId() : 0,
    this->Controller ? this->Controller->GetNumberOfProcesses() : 1,
    /* ghost levels */ 0);
  ouModel->CompositeShallowCopy(
    vtkCompositeDataSet::SafeDownCast(modelToTables->GetOutputDataObject(0)));

  // II. Create/update the output sci-viz data
  vtkDataObject* dataObjOu = vtkDataObject::GetData(output, 1);
  vtkDataObject* dataObjIn = vtkDataObject::GetData(input[0], 0);
  if (auto* cdou = vtkCompositeDataSet::SafeDownCast(dataObjIn))
  {
    cdou->CompositeShallowCopy(vtkCompositeDataSet::SafeDownCast(dataObjIn));
  }
  else
  {
    dataObjOu->ShallowCopy(dataObjIn);
  }

  return 1;
}
