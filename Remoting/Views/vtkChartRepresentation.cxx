// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkChartRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkBlockDeliveryPreprocessor.h"
#include "vtkChart.h"
#include "vtkChartSelectionRepresentation.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataObjectTypes.h"
#include "vtkExtractBlockUsingDataAssembly.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkPVMergeTablesComposite.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPlot.h"
#include "vtkReductionFilter.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkTable.h"

#include <set>
#include <sstream>
#include <string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkChartRepresentation);

//----------------------------------------------------------------------------
vtkChartRepresentation::vtkChartRepresentation()
{
  this->SetActiveAssembly("Hierarchy");

  this->LastLocalOutputMTime = 0;
  this->DummyRepresentation = vtkSmartPointer<vtkChartSelectionRepresentation>::New();

  this->SelectionRepresentation = nullptr;
  this->SetSelectionRepresentation(this->DummyRepresentation);

  this->FlattenTable = 1;
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_ROWS;
}

//----------------------------------------------------------------------------
vtkChartRepresentation::~vtkChartRepresentation()
{
  this->SetSelectionRepresentation(nullptr);
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::SetSelectionRepresentation(vtkChartSelectionRepresentation* repr)
{
  if (this->SelectionRepresentation)
  {
    this->SelectionRepresentation->SetChartRepresentation(nullptr);
  }
  this->SelectionRepresentation = repr;
  if (repr)
  {
    repr->SetChartRepresentation(this);
  }
}

//----------------------------------------------------------------------------
int vtkChartRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request, vtkInformation* ininfo, vtkInformation* outinfo)
{
  if (!this->Superclass::ProcessViewRequest(request, ininfo, outinfo))
  {
    return 0;
  }

  if (request == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVView::SetPiece(ininfo, this, this->LocalOutputRequestData);
  }
  else if (request == vtkPVView::REQUEST_RENDER())
  {
    assert(this->ContextView != nullptr);
    // this is called before every render, so don't do expensive things here.
    // Hence, we'll use this check to avoid work unless really needed.
    auto deliveredData =
      vtkDataObjectTree::SafeDownCast(vtkPVView::GetDeliveredPiece(ininfo, this));
    if ((this->LocalOutput != deliveredData) ||
      (deliveredData && deliveredData->GetMTime() != this->LastLocalOutputMTime) ||
      (this->PrepareForRenderingTime < this->GetMTime()))
    {
      // anytime different data is available for rendering, we prep it.
      this->LocalOutput = deliveredData;
      this->PrepareForRendering();
      this->LastLocalOutputMTime = deliveredData ? this->LocalOutput->GetMTime() : 0;
      this->PrepareForRenderingTime.Modified();
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkChartRepresentation::AddToView(vtkView* view)
{
  vtkPVContextView* chartView = vtkPVContextView::SafeDownCast(view);
  if (!chartView || chartView == this->ContextView)
  {
    return false;
  }

  this->ContextView = chartView;
  view->AddRepresentation(this->SelectionRepresentation);
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkChartRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVContextView* chartView = vtkPVContextView::SafeDownCast(view);
  if (!chartView || chartView != this->ContextView)
  {
    return false;
  }
  this->ContextView = nullptr;
  view->RemoveRepresentation(this->SelectionRepresentation);
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
int vtkChartRepresentation::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkChartRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->LocalOutputRequestData = nullptr;
  this->LocalOutput = nullptr;

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkSmartPointer<vtkDataObject> data = vtkDataObject::GetData(inputVector[0], 0);
    data = this->TransformInputData(data);
    if (!data)
    {
      return 0;
    }

    // Prune input dataset to only process blocks on interest.
    // If input is not a multiblock dataset, we make it one so the rest of the
    // pipeline is simple.
    if (data->IsA("vtkDataObjectTree") || data->IsA("vtkUniformGridAMR"))
    {
      vtkNew<vtkExtractBlockUsingDataAssembly> extractBlock;
      extractBlock->SetInputData(data);
      extractBlock->PruneDataAssemblyOff();
      extractBlock->SetAssemblyName(this->ActiveAssembly);
      for (const auto& selector : this->BlockSelectors)
      {
        extractBlock->AddSelector(selector.c_str());
      }
      extractBlock->Update();
      data = extractBlock->GetOutputDataObject(0);
    }
    if (auto pds = vtkPartitionedDataSet::SafeDownCast(data))
    {
      vtkNew<vtkPartitionedDataSetCollection> pdc;
      pdc->SetPartitionedDataSet(0, pds);
      data = pdc.GetPointer();
    }
    else if (data->IsA("vtkDataSet") || data->IsA("vtkTable"))
    {
      vtkNew<vtkPartitionedDataSetCollection> pdc;
      pdc->SetPartition(0, 0, data);
      data = pdc.GetPointer();
    }

    // data must be a multiblock/pdc dataset, no matter what
    assert(data->IsA("vtkMultiBlockDataSet") || data->IsA("vtkPartitionedDataSetCollection"));

    data = this->ReduceDataToRoot(data);
    data = this->TransformTable(data);
    this->LocalOutputRequestData = vtkDataObjectTree::SafeDownCast(data);
  }
  else
  {
    auto placeHolderType = this->PlaceHolderDataType == VTK_MULTIBLOCK_DATA_SET
      ? this->PlaceHolderDataType
      : VTK_PARTITIONED_DATA_SET_COLLECTION;
    auto placeHolder = vtk::TakeSmartPointer(vtkDataObjectTypes::NewDataObject(placeHolderType));
    placeHolder->Initialize();
    this->LocalOutputRequestData = vtkDataObjectTree::SafeDownCast(placeHolder);
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkChartRepresentation::TransformInputData(vtkDataObject* data)
{
  return data;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkChartRepresentation::ReduceDataToRoot(vtkDataObject* data)
{
  // Convert input dataset to vtkTable (rather, multiblock of vtkTable). We
  // can only plot vtkTable.
  vtkNew<vtkBlockDeliveryPreprocessor> preprocessor;
  // tell the preprocessor to flatten multicomponent arrays i.e. extract each
  // component into a separate array.
  preprocessor->SetFlattenTable(this->FlattenTable);
  preprocessor->SetFieldAssociation(this->FieldAssociation);
  preprocessor->SetInputData(data);

  vtkNew<vtkReductionFilter> reductionFilter;
  vtkNew<vtkPVMergeTablesComposite> algo;
  algo->SetMergeStrategy(this->ArraySelectionMode);
  reductionFilter->SetPostGatherHelper(algo.GetPointer());
  reductionFilter->SetInputConnection(preprocessor->GetOutputPort());
  reductionFilter->Update();

  return reductionFilter->GetOutputDataObject(0);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkChartRepresentation::TransformTable(
  vtkSmartPointer<vtkDataObject> data)
{
  return data;
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::MarkModified()
{
  this->SelectionRepresentation->MarkModified();
  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);
  this->SelectionRepresentation->SetVisibility(this->GetVisibility() && visible);
  this->Modified();
}

//----------------------------------------------------------------------------
// Forwarded to vtkBlockDeliveryPreprocessor.
void vtkChartRepresentation::SetFieldAssociation(int v)
{
  if (this->FieldAssociation != v)
  {
    this->FieldAssociation = v;
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::AddBlockSelector(const char* selector)
{
  if (selector != nullptr &&
    std::find(this->BlockSelectors.begin(), this->BlockSelectors.end(), selector) ==
      this->BlockSelectors.end())
  {
    this->BlockSelectors.emplace_back(selector);
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::RemoveAllBlockSelectors()
{
  if (!this->BlockSelectors.empty())
  {
    this->BlockSelectors.clear();
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::SetCompositeDataSetIndex(unsigned int v)
{
  this->CompositeIndices.clear();
  this->CompositeIndices.insert(v);
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::AddCompositeDataSetIndex(unsigned int v)
{
  if (this->CompositeIndices.find(v) == this->CompositeIndices.end())
  {
    this->CompositeIndices.insert(v);
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::ResetCompositeDataSetIndices()
{
  if (!this->CompositeIndices.empty())
  {
    this->CompositeIndices.clear();
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::SetArraySelectionMode(int mode)
{
  if (this->ArraySelectionMode !=
    (mode < MERGED_BLOCKS ? MERGED_BLOCKS : (mode > INDIVIDUAL_BLOCKS ? INDIVIDUAL_BLOCKS : mode)))
  {
    this->ArraySelectionMode =
      (mode < MERGED_BLOCKS ? MERGED_BLOCKS
                            : (mode > INDIVIDUAL_BLOCKS ? INDIVIDUAL_BLOCKS : mode));
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::SetPlaceHolderDataType(int datatype)
{
  if (this->PlaceHolderDataType != datatype)
  {
    this->PlaceHolderDataType = datatype;
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
unsigned int vtkChartRepresentation::Initialize(unsigned int minId, unsigned int maxId)
{
  minId = this->SelectionRepresentation->Initialize(minId, maxId);
  return this->Superclass::Initialize(minId, maxId);
}

//----------------------------------------------------------------------------
vtkTable* vtkChartRepresentation::GetLocalOutput(bool pre_delivery)
{
  auto objTree = pre_delivery ? this->LocalOutputRequestData : this->LocalOutput;
  if (!objTree)
  {
    return nullptr;
  }

  auto iter = vtk::TakeSmartPointer(objTree->NewTreeIterator());
  iter->SkipEmptyNodesOn();
  iter->VisitOnlyLeavesOn();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkTable* table = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
    if (table)
    {
      return table;
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkChartRepresentation::GetLocalOutput(vtkChartRepresentation::MapOfTables& tables)
{
  tables.clear();
  if (!this->LocalOutput)
  {
    return false;
  }

  const vtkNew<vtkDataAssembly> hierarchy;
  if (!vtkDataAssemblyUtilities::GenerateHierarchy(this->LocalOutput, hierarchy))
  {
    return false;
  }
  const auto outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(this->LocalOutput);
  const bool hasAssembly = outputPDC && outputPDC->GetDataAssembly();
  const bool isAssemblySelected =
    this->ActiveAssembly != nullptr && strcmp(this->ActiveAssembly, "Assembly") == 0;
  auto getLabel = [](vtkDataAssembly* assembly, const std::string& selector) -> std::string
  {
    const auto node = assembly->SelectNodes({ selector }).front();
    return assembly->GetAttributeOrDefault(node, "label", assembly->GetNodeName(node));
  };
  auto iter = vtk::TakeSmartPointer(this->LocalOutput->NewTreeIterator());
  iter->SkipEmptyNodesOn();
  iter->VisitOnlyLeavesOn();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkTable* table = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
    if (!table)
    {
      continue;
    }
    std::string blockName;
    if (hasAssembly && isAssemblySelected)
    {
      auto selectors = vtkDataAssemblyUtilities::GetSelectorsForCompositeIds(
        { iter->GetCurrentFlatIndex() }, hierarchy, outputPDC->GetDataAssembly());
      blockName = getLabel(outputPDC->GetDataAssembly(), selectors.front());
    }
    else
    {
      auto selectors = vtkDataAssemblyUtilities::GetSelectorsForCompositeIds(
        { iter->GetCurrentFlatIndex() }, hierarchy);
      blockName = getLabel(hierarchy, selectors.front());
    }
    if (tables.find(blockName) != tables.end())
    {
      vtkWarningMacro("Duplicate table name found: " << blockName << ". It will be skipped.");
    }
    else
    {
      tables[blockName] = { table, iter->GetCurrentFlatIndex() };
    }
  }

  return !tables.empty();
}

//----------------------------------------------------------------------------
std::string vtkChartRepresentation::GetDefaultSeriesLabel(
  const std::string& tableName, const std::string& columnName)
{
  return tableName.empty() ? std::string(columnName)
                           : std::string(columnName + " (" + tableName + ")");
}

//----------------------------------------------------------------------------
bool vtkChartRepresentation::MapSelectionToInput(vtkSelection* sel)
{
  assert(sel != nullptr);
  // note: we don't work very well when there are multiple visible
  // representations in the view and selections are made in it.
  for (unsigned int cc = 0, max = sel->GetNumberOfNodes(); cc < max; ++cc)
  {
    sel->GetNode(cc)->SetFieldType(
      vtkSelectionNode::ConvertAttributeTypeToSelectionField(this->FieldAssociation));
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkChartRepresentation::MapSelectionToView(vtkSelection* sel)
{
  assert(sel != nullptr);
  // note: we don't work very well when there are multiple visible
  // representations in the view and selections are made in it.
  int fieldType = vtkSelectionNode::ConvertAttributeTypeToSelectionField(this->FieldAssociation);
  for (unsigned int cc = sel->GetNumberOfNodes(); cc > 0; --cc)
  {
    vtkSelectionNode* node = sel->GetNode(cc - 1);
    if (node == nullptr || node->GetFieldType() != fieldType)
    {
      sel->RemoveNode(cc - 1);
    }
  }
  return true;
}
