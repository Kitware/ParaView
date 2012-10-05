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
#include "vtkContextView.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPlot.h"
#include "vtkProcessModule.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVContextView.h"
#include "vtkPVMergeTables.h"
#include "vtkReductionFilter.h"
#include "vtkScatterPlotMatrix.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkChartRepresentation);
vtkCxxSetObjectMacro(vtkChartRepresentation, Options, vtkChartNamedOptions);
//----------------------------------------------------------------------------
vtkChartRepresentation::vtkChartRepresentation()
{
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
  vtkPVMergeTables* post_gather_algo = vtkPVMergeTables::New();
  this->ReductionFilter->SetPostGatherHelper(post_gather_algo);
  this->ReductionFilter->SetController(vtkMultiProcessController::GetGlobalController());

  this->CacheKeeper->SetInputConnection(this->Preprocessor->GetOutputPort());
  this->ReductionFilter->SetInputConnection(this->CacheKeeper->GetOutputPort());

  post_gather_algo->FastDelete();
  this->DeliveryFilter = vtkClientServerMoveData::New();
  this->DeliveryFilter->SetOutputDataType(VTK_TABLE);

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
vtkTable* vtkChartRepresentation::GetLocalOutput()
{
  if (this->LocalOutput)
    {
    return this->LocalOutput;
    }

  return vtkTable::SafeDownCast(this->DeliveryFilter->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int vtkChartRepresentation::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  cout << "vtkChartRepresentation::RequestData" << endl;
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
    this->Preprocessor->SetInputData(vtkDataObject::GetData(inputVector[0], 0));
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
        this->LocalOutput = vtkSmartPointer<vtkTable>::New();
        }
      if (myId == 0)
        {
        // clone the output of reduction filter on all satellites.
        this->LocalOutput->ShallowCopy(
          this->ReductionFilter->GetOutputDataObject(0));
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
  if (this->Options)
    {
    this->Options->SetTable(this->GetLocalOutput());
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
  vtkTable *table = this->GetLocalOutput();
  if (table)
    {
    return table->GetNumberOfColumns();
    }

  return 0;
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
  vtkTable *table = this->GetLocalOutput();
  if (table)
    {
    return table->GetColumnName(col);
    }

  return NULL;
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
  this->Preprocessor->SetCompositeDataSetIndex(v);
  this->MarkModified();
}

//----------------------------------------------------------------------------
unsigned int vtkChartRepresentation::Initialize(
  unsigned int minId, unsigned int maxId)
{
  minId = this->SelectionRepresentation->Initialize(minId, maxId);
  return  this->Superclass::Initialize(minId, maxId);
}
