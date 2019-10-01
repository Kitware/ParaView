/*=========================================================================

  Program:   ParaView
  Module:    vtkChartRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkChartRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkBlockDeliveryPreprocessor.h"
#include "vtkChart.h"
#include "vtkChartSelectionRepresentation.h"
#include "vtkClientServerMoveData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkContextView.h"
#include "vtkDataObject.h"
#include "vtkExtractBlock.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextView.h"
#include "vtkPVMergeTablesMultiBlock.h"
#include "vtkPlot.h"
#include "vtkProcessModule.h"
#include "vtkReductionFilter.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkTable.h"

#include <set>
#include <sstream>
#include <string>

#define CLASSNAME(a) (a ? a->GetClassName() : "None")

vtkStandardNewMacro(vtkChartRepresentation);
//----------------------------------------------------------------------------
vtkChartRepresentation::vtkChartRepresentation()
{
  this->LastLocalOutputMTime = 0;
  this->DummyRepresentation = vtkSmartPointer<vtkChartSelectionRepresentation>::New();

  this->SelectionRepresentation = 0;
  this->SetSelectionRepresentation(this->DummyRepresentation);

  this->FlattenTable = 1;
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_ROWS;
}

//----------------------------------------------------------------------------
vtkChartRepresentation::~vtkChartRepresentation()
{
  this->SetSelectionRepresentation(NULL);
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::SetSelectionRepresentation(vtkChartSelectionRepresentation* repr)
{
  if (this->SelectionRepresentation)
  {
    this->SelectionRepresentation->SetChartRepresentation(NULL);
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
    assert(this->ContextView != NULL);
    // this is called before every render, so don't do expensive things here.
    // Hence, we'll use this check to avoid work unless really needed.
    auto deliveredData =
      vtkMultiBlockDataSet::SafeDownCast(vtkPVView::GetDeliveredPiece(ininfo, this));
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
  this->ContextView = 0;
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
    if (data->IsA("vtkMultiBlockDataSet"))
    {
      vtkNew<vtkExtractBlock> extractBlock;
      extractBlock->SetInputData(data);
      extractBlock->PruneOutputOff();
      for (std::set<unsigned int>::const_iterator iter = this->CompositeIndices.begin();
           iter != this->CompositeIndices.end(); ++iter)
      {
        extractBlock->AddIndex(*iter);
      }
      extractBlock->Update();
      data = extractBlock->GetOutputDataObject(0);
    }
    else
    {
      vtkNew<vtkMultiBlockDataSet> mbdata;
      mbdata->SetNumberOfBlocks(1);
      mbdata->SetBlock(0, data);
      data = mbdata.GetPointer();
    }

    // data must be a multiblock dataset, no matter what
    assert(data->IsA("vtkMultiBlockDataSet"));

    data = this->ReduceDataToRoot(data);
    data = this->TransformTable(data);
    this->LocalOutputRequestData = vtkMultiBlockDataSet::SafeDownCast(data);
  }
  else
  {
    this->LocalOutputRequestData = vtkSmartPointer<vtkMultiBlockDataSet>::New();
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
  vtkNew<vtkPVMergeTablesMultiBlock> algo;
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
void vtkChartRepresentation::SetCompositeDataSetIndex(unsigned int v)
{
  this->CompositeIndices.clear();
  this->CompositeIndices.insert(v);
  this->MarkModified();
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
  if (this->CompositeIndices.size() > 0)
  {
    this->CompositeIndices.clear();
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
  auto mb = pre_delivery ? this->LocalOutputRequestData : this->LocalOutput;
  if (!mb)
  {
    return NULL;
  }

  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(mb->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkTable* table = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
    if (table)
    {
      return table;
    }
  }
  return NULL;
}

//----------------------------------------------------------------------------
bool vtkChartRepresentation::GetLocalOutput(vtkChartRepresentation::MapOfTables& tables)
{
  tables.clear();
  if (!this->LocalOutput)
  {
    return false;
  }

  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(this->LocalOutput->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkTable* table = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
    if (table)
    {
      std::ostringstream stream;
      if (iter->HasCurrentMetaData() &&
        iter->GetCurrentMetaData()->Has(vtkCompositeDataSet::NAME()))
      {
        stream << iter->GetCurrentMetaData()->Get(vtkCompositeDataSet::NAME());
      }
      else
      {
        stream << iter->GetCurrentFlatIndex();
      }
      if (tables.find(stream.str()) != tables.end())
      {
        // duplicate name, argh! We make it unique by adding a flat index to it.
        stream << "[" << iter->GetCurrentFlatIndex() << "]";
      }
      assert(tables.find(stream.str()) == tables.end());
      tables[stream.str()] = table;
    }
  }

  return (tables.size() > 0);
}

//----------------------------------------------------------------------------
vtkStdString vtkChartRepresentation::GetDefaultSeriesLabel(
  const vtkStdString& tableName, const vtkStdString& columnName)
{
  return tableName.empty() ? vtkStdString(columnName)
                           : vtkStdString(columnName + " (" + tableName + ")");
}

//----------------------------------------------------------------------------
bool vtkChartRepresentation::MapSelectionToInput(vtkSelection* sel)
{
  assert(sel != NULL);
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
  assert(sel != NULL);
  // note: we don't work very well when there are multiple visible
  // representations in the view and selections are made in it.
  int fieldType = vtkSelectionNode::ConvertAttributeTypeToSelectionField(this->FieldAssociation);
  for (unsigned int cc = sel->GetNumberOfNodes(); cc > 0; --cc)
  {
    vtkSelectionNode* node = sel->GetNode(cc - 1);
    if (node == NULL || node->GetFieldType() != fieldType)
    {
      sel->RemoveNode(cc - 1);
    }
  }
  return true;
}
