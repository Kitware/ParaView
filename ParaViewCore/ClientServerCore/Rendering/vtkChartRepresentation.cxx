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
#include "vtkPVCacheKeeper.h"
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
  this->DummyRepresentation = vtkSmartPointer<vtkChartSelectionRepresentation>::New();

  this->SelectionRepresentation = 0;
  this->SetSelectionRepresentation(this->DummyRepresentation);

  this->CacheKeeper = vtkPVCacheKeeper::New();
  this->EnableServerSideRendering = false;
  this->FlattenTable = 1;
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_ROWS;
}

//----------------------------------------------------------------------------
vtkChartRepresentation::~vtkChartRepresentation()
{
  this->CacheKeeper->Delete();
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

  if (request == vtkPVView::REQUEST_RENDER())
  {
    assert(this->ContextView != NULL);
    // this is called before every render, so don't do expensive things here.
    // Hence, we'll use this check to avoid work unless really needed.
    if (this->GetMTime() > this->PrepareForRenderingTime)
    {
      // NOTE: despite the fact that we're only looking at this->MTime,
      // this->PrepareForRendering() will get called even when the
      // representation's upstream pipeline has changed. This is because for
      // representations, when upstream changes, the ServerManager ensures that
      // vtkPVDataRepresentation::MarkModified() gets called.

      this->PrepareForRendering();
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
  this->EnableServerSideRendering = (chartView && chartView->InTileDisplayMode());
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
  if (vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_RENDER_SERVER)
  {
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  // remove the "cached" delivered data.
  this->LocalOutput = NULL;

  // Pass caching information to the cache keeper.
  this->CacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->CacheKeeper->SetCacheTime(this->GetCacheKey());

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int myId = pm->GetPartitionId();
  int numProcs = pm->GetNumberOfLocalPartitions();

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkSmartPointer<vtkDataObject> data;

    // don't process data is the cachekeeper has already cached the result.
    if (!this->CacheKeeper->IsCached())
    {
      data = vtkDataObject::GetData(inputVector[0], 0);
      data = this->TransformInputData(inputVector, data);
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

      // Convert input dataset to vtkTable (rather, multiblock of vtkTable). We
      // can only plot vtkTable.
      vtkNew<vtkBlockDeliveryPreprocessor> preprocessor;
      // tell the preprocessor to flatten multicomponent arrays i.e. extract each
      // component into a separate array.
      preprocessor->SetFlattenTable(this->FlattenTable);
      preprocessor->SetFieldAssociation(this->FieldAssociation);
      preprocessor->SetInputData(data);
      preprocessor->Update();

      data = preprocessor->GetOutputDataObject(0);

      // data must be a multiblock dataset, no matter what.
      assert(data->IsA("vtkMultiBlockDataSet"));

      // now deliver data to the rendering sides:
      // first, reduce it to root node.
      vtkNew<vtkReductionFilter> reductionFilter;
      vtkNew<vtkPVMergeTablesMultiBlock> algo;
      reductionFilter->SetPostGatherHelper(algo.GetPointer());
      reductionFilter->SetController(pm->GetGlobalController());
      reductionFilter->SetInputData(data);
      reductionFilter->Update();

      data = reductionFilter->GetOutputDataObject(0);
      data = this->TransformTable(data);

      if (this->EnableServerSideRendering && numProcs > 1)
      {
        // share the reduction result will all satellites.
        pm->GetGlobalController()->Broadcast(data.GetPointer(), 0);
      }
      this->CacheKeeper->SetInputData(data);
    }
    // here the cachekeeper will either give us the cached result of the result
    // we just processed.
    this->CacheKeeper->Update();
    data = this->CacheKeeper->GetOutputDataObject(0);

    if (myId == 0)
    {
      // send data to client.
      vtkNew<vtkClientServerMoveData> deliver;
      deliver->SetInputData(data);
      deliver->Update();
    }

    this->LocalOutput = vtkMultiBlockDataSet::SafeDownCast(data);
  }
  else
  {
    // receive data on client.
    vtkNew<vtkClientServerMoveData> deliver;
    deliver->Update();
    this->LocalOutput = vtkMultiBlockDataSet::SafeDownCast(deliver->GetOutputDataObject(0));
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkChartRepresentation::TransformInputData(
  vtkInformationVector** inputVector, vtkDataObject* data)
{
  (void)inputVector;
  // Default representation does not transform anything
  return data;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkChartRepresentation::TransformTable(
  vtkSmartPointer<vtkDataObject> data)
{
  return data;
}
//----------------------------------------------------------------------------
bool vtkChartRepresentation::IsCached(double cache_key)
{
  return this->CacheKeeper->IsCached(cache_key);
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::MarkModified()
{
  if (!this->GetUseCache())
  {
    // Cleanup caches when not using cache.
    this->CacheKeeper->RemoveAllCaches();
  }
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
vtkTable* vtkChartRepresentation::GetLocalOutput()
{
  if (!this->LocalOutput)
  {
    return NULL;
  }

  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(this->LocalOutput->NewIterator());
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
