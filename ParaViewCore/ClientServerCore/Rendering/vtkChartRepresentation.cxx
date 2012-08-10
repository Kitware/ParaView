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
#include "vtkSelectionDeliveryFilter.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkTable.h"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkChartRepresentation);
vtkCxxSetObjectMacro(vtkChartRepresentation, Options, vtkChartNamedOptions);
//----------------------------------------------------------------------------
vtkChartRepresentation::vtkChartRepresentation()
{
  this->SetNumberOfInputPorts(2);

  this->AnnLink = vtkAnnotationLink::New();
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

  this->SelectionDeliveryFilter = vtkSelectionDeliveryFilter::New();
  
  this->EnableServerSideRendering = false;
}

//----------------------------------------------------------------------------
vtkChartRepresentation::~vtkChartRepresentation()
{
  this->SetOptions(0);
  this->AnnLink->Delete();

  this->Preprocessor->Delete();
  this->CacheKeeper->Delete();
  this->ReductionFilter->Delete();
  this->DeliveryFilter->Delete();

  this->SelectionDeliveryFilter->Delete();
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
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
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
  if (vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_RENDER_SERVER)
    {
    return this->Superclass::RequestData(request, inputVector, outputVector);
    }

  this->ReductionFilter->Modified();
  this->DeliveryFilter->Modified();
  this->SelectionDeliveryFilter->Modified();

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
    this->Preprocessor->SetInputConnection(
      this->GetInternalOutputPort(0));
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

  // Now deliver the selection.
  vtkSmartPointer<vtkSelection> sel;
  if (inputVector[1]->GetNumberOfInformationObjects()==1)
    {
    this->SelectionDeliveryFilter->SetInputConnection(
      this->GetInternalOutputPort(1, 0));
    this->SelectionDeliveryFilter->Update();
    if (this->EnableServerSideRendering)
      {
      if (numProcs > 1)
        {
        vtkMultiProcessController* controller = pm->GetGlobalController();
        if (myId == 0)
          {
          sel = vtkSelection::SafeDownCast(
            this->GetInternalOutputPort(1, 0)->GetProducer()->
            GetOutputDataObject(0));
          vtksys_ios::ostringstream res;
          vtkSelectionSerializer::PrintXML(res, vtkIndent(), 1, sel);

          // Send the size of the string.
          int size = static_cast<int>(res.str().size());
          controller->Broadcast(&size, 1, 0);

          // Send the XML string.
          controller->Broadcast(
            const_cast<char*>(res.str().c_str()), size, 0);
          }
        else
          {
          int size = 0;
          controller->Broadcast(&size, 1, 0);
          char* xml = new char[size+1];

          // Get the string itself.
          controller->Broadcast(xml, size, 0);
          xml[size] = 0;

          // Parse the XML.
          sel = vtkSmartPointer<vtkSelection>::New();
          vtkSelectionSerializer::Parse(xml, sel);
          delete[] xml;
          }
        }
      }
    else
      {
      sel = vtkSelection::SafeDownCast(
        this->SelectionDeliveryFilter->GetOutputDataObject(0));
      }
    }
  else
    {
    this->SelectionDeliveryFilter->RemoveAllInputs();
    this->SelectionDeliveryFilter->Update();
    sel = vtkSelection::SafeDownCast(
      this->SelectionDeliveryFilter->GetOutputDataObject(0));
    }

  if (this->ContextView)
    {
    if(vtkChart *chart = vtkChart::SafeDownCast(this->ContextView->GetContextItem()))
      {
      this->AnnLink->SetCurrentSelection(sel);
      chart->SetAnnotationLink(this->AnnLink);
      }
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
