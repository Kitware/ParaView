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
#include "vtkAnnotationLink.h"
#include "vtkBlockDeliveryPreprocessor.h"
#include "vtkChart.h"
#include "vtkChartNamedOptions.h"
#include "vtkChartSelectionRepresentation.h"
#include "vtkClientServerMoveData.h"
#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkContextView.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVContextView.h"
#include "vtkPVMergeTablesMultiBlock.h"
#include "vtkPlot.h"
#include "vtkProcessModule.h"
#include "vtkReductionFilter.h"
#include "vtkScatterPlotMatrix.h"
#include "vtkTable.h"

#include <vtksys/ios/sstream>
#include <set>
#include <string>

#define CLASSNAME(a) (a? a->GetClassName() : "None")


vtkStandardNewMacro(vtkChartRepresentation);
vtkCxxSetObjectMacro(vtkChartRepresentation, Options, vtkChartNamedOptions);
//----------------------------------------------------------------------------
vtkChartRepresentation::vtkChartRepresentation()
{
  this->LocalOutputCacheTimeStamp = 0;
  this->SelectionRepresentation = 0;
  vtkNew<vtkChartSelectionRepresentation> selectionRepr;
  this->SetSelectionRepresentation(selectionRepr.GetPointer());

  this->Options = 0;

  this->Preprocessor = vtkBlockDeliveryPreprocessor::New();
  // Should the table be flattened (multicomponent columns split to single
  // component)?
  this->Preprocessor->SetFlattenTable(1);
  this->CacheKeeper = vtkPVCacheKeeper::New();

  this->ReductionFilter = vtkReductionFilter::New();
  vtkPVMergeTablesMultiBlock* post_gather_algo = vtkPVMergeTablesMultiBlock::New();
  this->ReductionFilter->SetPostGatherHelper(post_gather_algo);
  this->ReductionFilter->SetController(vtkMultiProcessController::GetGlobalController());

  this->CacheKeeper->SetInputConnection(this->Preprocessor->GetOutputPort());
  this->ReductionFilter->SetInputConnection(this->CacheKeeper->GetOutputPort());

  post_gather_algo->FastDelete();
  this->DeliveryFilter = vtkClientServerMoveData::New();
  this->DeliveryFilter->SetOutputDataType(VTK_MULTIBLOCK_DATA_SET);

  this->EnableServerSideRendering = false;
}

//----------------------------------------------------------------------------
vtkChartRepresentation::~vtkChartRepresentation()
{
  this->SetOptions(0);

  this->Preprocessor->Delete();
  this->CacheKeeper->Delete();
  this->ReductionFilter->Delete();
  this->DeliveryFilter->Delete();
  this->SetSelectionRepresentation(NULL);
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::SetSelectionRepresentation(
  vtkChartSelectionRepresentation* repr)
{
  if (this->SelectionRepresentation && (repr != this->SelectionRepresentation))
    {
    this->SelectionRepresentation->SetChartRepresentation(NULL);
    }
  vtkSetObjectBodyMacro(SelectionRepresentation,
    vtkChartSelectionRepresentation, repr);
  if (repr)
    {
    repr->SetChartRepresentation(this);
    }
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
  this->EnableServerSideRendering = (chartView &&
    chartView->InTileDisplayMode());

  if (this->Options)
    {
    if(vtkChart* pChart = vtkChart::SafeDownCast(chartView->GetContextItem()))
      {
      this->Options->SetChart(pChart);
      }
    else if(vtkScatterPlotMatrix* plotMatrix =
      vtkScatterPlotMatrix::SafeDownCast(chartView->GetContextItem()))
      {
      this->Options->SetPlotMatrix(plotMatrix);
      }
    this->Options->SetTableVisibility(this->GetVisibility());
    }

  view->AddRepresentation(this->SelectionRepresentation);
  return true;
}

//----------------------------------------------------------------------------
bool vtkChartRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVContextView* chartView = vtkPVContextView::SafeDownCast(view);
  if (!chartView || chartView != this->ContextView)
    {
    return false;
    }

  if (this->Options)
    {
    this->Options->RemovePlotsFromChart();
    this->Options->SetChart(0);
    this->Options->SetPlotMatrix(0);
    }
  this->ContextView = 0;
  view->RemoveRepresentation(this->SelectionRepresentation);
  return true;
}

//----------------------------------------------------------------------------
int vtkChartRepresentation::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkChartRepresentation::RequestUpdateExtent(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  return this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkChartRepresentation::GetLocalOutput(std::vector<vtkTable*>& tables,
                                            std::vector<std::string> *blockNames /*= NULL*/)
{
  tables.clear();
  if(blockNames)
    {
    blockNames->clear();
    }
  vtkCompositeDataSet* compositeOut;
  if (this->LocalOutput)
    {
    compositeOut = this->LocalOutput;
    }
  else
    {
    vtkDataObject* out = this->DeliveryFilter->GetOutputDataObject(0);
    compositeOut = vtkCompositeDataSet::SafeDownCast(out);
    }

  // Use cache if possible
  if(compositeOut && this->LocalOutputCacheTimeStamp == compositeOut->GetMTime())
    {
    for(size_t i = 0; i < this->LocalOutputTableCache.size(); ++i)
      {
      tables.push_back(this->LocalOutputTableCache[i]);
      if(blockNames) blockNames->push_back(this->LocalOutputBlockNameCache[i]);
      }
    return tables.size()>0;
    }

  // Access the real data as the cache is not valid anymore...
  if(compositeOut)
    {
    // Clear previous cache value
    this->LocalOutputCacheTimeStamp = compositeOut->GetMTime();
    this->LocalOutputBlockNameCache.clear();
    this->LocalOutputTableCache.clear();

    // If 0, which corresponds to the root node for a multiblock dataset, is
    // present in the set of indices, then we simply pass all the blocks.
    bool pass_all_blocks = (this->CompositeIndices.find(0) !=
      this->CompositeIndices.end());

    // Let's traverse the vtkMultiBlockDataSet
    vtkMultiBlockDataSet* tree = vtkMultiBlockDataSet::SafeDownCast(compositeOut);
    if(tree != NULL)
      {
      std::vector<std::string> tmpNames;
      std::vector<vtkTable*>   tmpTables;
      this->FillTableList(tree, tmpTables, &tmpNames, "");

      // Filter based on which block is selected
      std::set<vtkTable*> tableToRemove;
      vtkSmartPointer<vtkCompositeDataIterator> iter;
      iter.TakeReference(compositeOut->NewIterator());
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
        int compositeIndex = iter->GetCurrentFlatIndex();
        if(this->CompositeIndices.find(compositeIndex)==this->CompositeIndices.end())
          {
          vtkTable* inputObj = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
          if(inputObj != NULL)
            {
            tableToRemove.insert(inputObj);
            }
          }
        }
      for(size_t idx = 0; idx < tmpTables.size(); ++idx)
        {
        if( tableToRemove.find(tmpTables[idx]) == tableToRemove.end() )
          {
          this->LocalOutputBlockNameCache.push_back(tmpNames[idx]);
          this->LocalOutputTableCache.push_back(tmpTables[idx]);
          }
        }

      // Update values based on the cache
      tables = this->LocalOutputTableCache;
      if(blockNames)
        {
        (*blockNames) = this->LocalOutputBlockNameCache;
        }
      }
    else // Another type of composite so let's use the iterator
      {
      vtkSmartPointer<vtkCompositeDataIterator> iter;
      iter.TakeReference(compositeOut->NewIterator());
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
        int compositeIndex = iter->GetCurrentFlatIndex();
        if (pass_all_blocks ||
            this->CompositeIndices.find(compositeIndex)!=this->CompositeIndices.end())
          {
          vtkDataObject* inputObj = iter->GetCurrentDataObject();
          if(vtkTable::SafeDownCast(inputObj))
            {
            vtkTable* table = vtkTable::SafeDownCast(inputObj);
            tables.push_back(table);
            this->LocalOutputTableCache.push_back(table);

            // Fill blockname if requested
            std::ostringstream blockName;
            if(iter->GetCurrentMetaData()->Get(vtkCompositeDataSet::NAME()))
              {
              blockName << iter->GetCurrentMetaData()->Get(vtkCompositeDataSet::NAME());
              }
            else
              {
              blockName << "Block " << compositeIndex;
              }
            this->LocalOutputBlockNameCache.push_back(blockName.str());

            if(blockNames)
              {
              blockNames->push_back(blockName.str());
              }
            }
          }
        }
      }
    }

  return tables.size()>0;
}

//----------------------------------------------------------------------------
int vtkChartRepresentation::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_RENDER_SERVER)
    {
    return this->Superclass::RequestData(request, inputVector, outputVector);
    }

  this->ReductionFilter->Modified();
  this->DeliveryFilter->Modified();

  // remove the "cached" delivered data.
  if (this->LocalOutput)
    {
    this->LocalOutput->Initialize();
    }

  // Pass caching information to the cache keeper.
  this->CacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->CacheKeeper->SetCacheTime(this->GetCacheKey());

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int myId = pm->GetPartitionId();
  int numProcs = pm->GetNumberOfLocalPartitions();

  if (inputVector[0]->GetNumberOfInformationObjects()==1)
    {
    vtkDataObject* input =  vtkDataObject::GetData(inputVector[0], 0);
    vtkSmartPointer<vtkMultiBlockDataSet> inputMB =  vtkMultiBlockDataSet::GetData(inputVector[0], 0);
    if(!inputMB)
      {
      inputMB = vtkSmartPointer<vtkMultiBlockDataSet>::New();
      if(input)
        {
        vtkDataObject* input0 = input->NewInstance();
        input0->ShallowCopy(input);
        inputMB->SetNumberOfBlocks(1);
        inputMB->SetBlock(0,input);
        input0->Delete();
        }
      else
        {
        vtkErrorMacro("No input");
        }
      }
    this->Preprocessor->SetInputData(inputMB);
    this->Preprocessor->Update();
    this->DeliveryFilter->SetInputConnection(this->ReductionFilter->GetOutputPort());
    this->ReductionFilter->Update();
    if (this->EnableServerSideRendering)
      {
      // Due to the way vtkChartNamedOptions works with vtkPlot, we need to
      // ensure that the vtkTable instance is "updated" and not replaced
      // otherwise the new vtkTable is not propagated to the plot correctly.
      if (this->LocalOutput == NULL)
        {
        this->LocalOutput = vtkSmartPointer<vtkMultiBlockDataSet>::New();
        }
      if (myId == 0)
        {
        // clone the output of reduction filter on all satellites.
        this->LocalOutput->ShallowCopy( this->ReductionFilter->GetOutputDataObject(0));
        }
      if (numProcs > 1)
        {
        pm->GetGlobalController()->Broadcast(this->LocalOutput, 0);
        }
      }
    }
  else
    {
    this->Preprocessor->RemoveAllInputs();
    this->DeliveryFilter->RemoveAllInputs();
    }
  this->DeliveryFilter->Update();
  this->UpdateSeriesNames();

#ifdef DEBUGME
  cout<<"Composite indices: ";
  for(std::set<int>::iterator i = this->CompositeIndices.begin(); i!=this->CompositeIndices.end();i++)
    {
    cout<<(*i)<<" ";
    }
  cout<<endl;
  if (inputVector[0]->GetNumberOfInformationObjects()>=1)
    {
    cout<<"Preprocess output type "<<CLASSNAME(this->Preprocessor->GetOutputDataObject(0))<<endl;
    cout<<"Cache Keeper output type "<<CLASSNAME(this->CacheKeeper->GetOutputDataObject(0))<<endl;
    cout<<"Reduction filter output type: "<<CLASSNAME(this->ReductionFilter->GetOutputDataObject(0))<<endl;
    cout<<"Delivery output type: "<<CLASSNAME(this->DeliveryFilter->GetOutputDataObject(0))<<endl;
    }
#endif
 
  if (this->Options)
    {
    std::vector<vtkTable*> output;
    std::vector<std::string> blockNames;
    this->GetLocalOutput(output, &blockNames);

    const char** charBlockNames = new const char*[output.size()];
    for(size_t i=0;i<output.size();++i)
      {
      charBlockNames[i] = blockNames[i].c_str();
      }

    this->Options->SetTables( output.empty() ? NULL : &output[0],
                              charBlockNames,
                              static_cast<int>(output.size()));
    delete[] charBlockNames;
    }
  return this->Superclass::RequestData(request, inputVector, outputVector);
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
  if (this->Options)
    {
    this->Options->SetTableVisibility(visible);
    }
  this->SelectionRepresentation->SetVisibility(
    this->GetVisibility() && visible);
}

//----------------------------------------------------------------------------
int vtkChartRepresentation::GetNumberOfSeries()
{
  this->UpdateSeriesNames();
  return static_cast<int>(this->SeriesNames.size());
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::RescaleChart()
{
  if (this->Options && this->Options->GetChart())
    {
    this->Options->GetChart()->RecalculateBounds();
    }
}

//----------------------------------------------------------------------------
const char* vtkChartRepresentation::GetSeriesName(int col)
{
  if(col<0 && col >= static_cast<int>(this->SeriesNames.size()))
    {
    return NULL;
    }
  return this->SeriesNames[col].c_str();
}

// *************************************************************************
// Forwarded to vtkBlockDeliveryPreprocessor.
void vtkChartRepresentation::SetFieldAssociation(int v)
{
  this->Preprocessor->SetFieldAssociation(v);
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::SetCompositeDataSetIndex(unsigned int v)
{
  if(v<=0)
    {
#ifdef DEBUGME
    cout<<"Ignore invalid composite index"<<endl;
#endif
    return;
    }
  this->CompositeIndices.clear();
  this->CompositeIndices.insert(v);
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::GetSeriesNames(std::vector<const char*>& seriesNames)
{
  this->UpdateSeriesNames();
  seriesNames.clear();
  size_t size = this->SeriesNames.size();
  for(size_t i=0; i < size; ++i)
    {
    seriesNames.push_back(this->SeriesNames[i].c_str());
    }
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::GetArraysNames(std::vector<const char*>& arraysNames)
{
  this->UpdateSeriesNames();
  arraysNames.clear();
  std::set<std::string>::iterator iter = this->ArrayNames.begin();
  while(iter != this->ArrayNames.end())
    {
    arraysNames.push_back(iter->c_str());

    // Next name
    iter++;
    }
}
//----------------------------------------------------------------------------
void vtkChartRepresentation::AddCompositeDataSetIndex(unsigned int v)
{
#ifdef DEBUGME
  cout<<"Add "<<v<<" to selection"<<endl;
#endif
  this->CompositeIndices.insert(v);
  this->MarkModified();
}

void vtkChartRepresentation::ResetCompositeDataSetIndices()
{
#ifdef DEBUGME
  cout<<"Cleared selection"<<endl;
#endif
  this->CompositeIndices.clear();
  this->MarkModified();
}

void vtkChartRepresentation::UpdateSeriesNames()
{
  this->SeriesNames.clear();
  this->ArrayNames.clear();
  std::vector<vtkTable*> tables;
  std::vector<std::string> blockNames;
  std::set<std::string> uniqueNames;
  this->GetLocalOutput(tables, &blockNames);
  for(size_t i=0; i<tables.size(); i++)
    {
    vtkTable* table = tables[i];
    for(vtkIdType j = 0; j< table->GetNumberOfColumns(); j++)
      {
      const char* name = table->GetColumnName(j);
      std::ostringstream fullName;

      // Append block name only if blocks
      if(tables.size() > 1)
        {
        fullName << blockNames[i] << "/";
        }

      fullName << name;
      if(uniqueNames.find(fullName.str())==uniqueNames.end())
        {
        uniqueNames.insert(fullName.str());
        this->SeriesNames.push_back(fullName.str());
        }

      // Keep track of any array name
      this->ArrayNames.insert(name);
      }
    }
}

//----------------------------------------------------------------------------
unsigned int vtkChartRepresentation::Initialize(
  unsigned int minId, unsigned int maxId)
{
  minId = this->SelectionRepresentation->Initialize(minId, maxId);
  return  this->Superclass::Initialize(minId, maxId);
}

//----------------------------------------------------------------------------
void vtkChartRepresentation::FillTableList(vtkMultiBlockDataSet* tree, std::vector<vtkTable*>& tables, std::vector<std::string> *blockNames, const char* currentPath)
{
  if(tree == NULL)
    {
    return;
    }

  // Let's traverse that level
  unsigned int nbChildren = tree->GetNumberOfBlocks();
  for(unsigned int i=0; i < nbChildren; ++i)
    {
    vtkTable* table = vtkTable::SafeDownCast(tree->GetBlock(i));
    vtkMultiBlockDataSet* subTree = vtkMultiBlockDataSet::SafeDownCast(tree->GetBlock(i));
    std::ostringstream blockName;
    if(tree->HasMetaData(i) && tree->GetMetaData(i)->Has(vtkCompositeDataSet::NAME()))
      {
      blockName << currentPath << tree->GetMetaData(i)->Get(vtkCompositeDataSet::NAME());
      }
    else if(table != NULL)
      {
      blockName << currentPath << "DataSet " << i;
      }
    else
      {
      blockName << currentPath << "Block " << i;
      }

    // register the table if found
    if(table != NULL)
      {
      tables.push_back(table);
      if(blockNames != NULL)
        {
        blockNames->push_back(blockName.str());
        }
      }

    // Go deeper
    if(subTree != NULL)
      {
      blockName << "/";
      this->FillTableList(subTree, tables, blockNames, blockName.str().c_str());
      }
    }
}
