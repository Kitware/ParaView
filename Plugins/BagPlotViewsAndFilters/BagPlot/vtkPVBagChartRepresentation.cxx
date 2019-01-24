/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBagChartRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVBagChartRepresentation.h"

#include "vtkAppendPolyData.h"
#include "vtkBrush.h"
#include "vtkCellArray.h"
#include "vtkChartXY.h"
#include "vtkClientServerMoveData.h"
#include "vtkColor.h"
#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkContextView.h"
#include "vtkContourFilter.h"
#include "vtkDataObjectToTable.h"
#include "vtkDoubleArray.h"
#include "vtkExtractBlock.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkInformationVector.h"
#include "vtkLookupTable.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVContextView.h"
#include "vtkPVXYChartView.h"
#include "vtkPen.h"
#include "vtkPlotBag.h"
#include "vtkPlotHistogram2D.h"
#include "vtkPlotLine.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkReductionFilter.h"
#include "vtkTable.h"

#include <algorithm>
#include <sstream>
#include <string>

vtkStandardNewMacro(vtkPVBagChartRepresentation);
vtkCxxSetObjectMacro(vtkPVBagChartRepresentation, LookupTable, vtkScalarsToColors)

  //----------------------------------------------------------------------------
  vtkPVBagChartRepresentation::vtkPVBagChartRepresentation()
  : LineThickness(1)
  , LineStyle(0)
  , LookupTable(NULL)
  , Opacity(1.)
  , PointSize(5)
  , GridLineThickness(1)
  , GridLineStyle(0)
  , XAxisSeriesName(NULL)
  , YAxisSeriesName(NULL)
  , DensitySeriesName(NULL)
  , UseIndexForXAxis(true)
{
  this->BagColor[0] = 1.0;
  this->BagColor[1] = this->BagColor[2] = 0.0;
  this->SelectionColor[0] = this->SelectionColor[2] = 1.0;
  this->SelectionColor[1] = 0.0;
  this->LineColor[0] = this->LineColor[1] = this->LineColor[2] = 0.0;
  this->PointColor[0] = this->PointColor[1] = this->PointColor[2] = 0.0;
  this->PUserColor[0] = this->PUserColor[2] = 0.5;
  this->PUserColor[1] = 0.0;
  this->P50Color[0] = this->P50Color[2] = 0.2;
  this->P50Color[1] = 0.0;

  vtkNew<vtkColorTransferFunction> lut;
  lut->SetColorSpaceToDiverging();
  lut->AddRGBPoint(0, 59. / 255., 76. / 255., 192. / 255.);
  lut->AddRGBPoint(1, 221. / 255., 221. / 255., 221. / 255.);
  lut->AddRGBPoint(2, 180. / 255., 4. / 255., 38. / 255.);
  SetLookupTable(lut.Get());
}

//----------------------------------------------------------------------------
vtkPVBagChartRepresentation::~vtkPVBagChartRepresentation()
{
  SetLookupTable(NULL);
}

//----------------------------------------------------------------------------
void vtkPVBagChartRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkPVBagChartRepresentation::AddToView(vtkView* view)
{
  if (!this->Superclass::AddToView(view))
  {
    return false;
  }

  if (this->GetChart())
  {
    this->GetChart()->SetVisible(this->GetVisibility());
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkPVBagChartRepresentation::RemoveFromView(vtkView* view)
{
  if (this->GetChart())
  {
    this->GetChart()->GetPlot(0)->SetInputData(0);
    this->GetChart()->SetVisible(false);
  }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkPVBagChartRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);

  vtkChartXY* chart = this->GetChart();
  if (chart && !visible)
  {
    // Refer to vtkChartRepresentation::PrepareForRendering() documentation to
    // know why this is cannot be done in PrepareForRendering();
    chart->SetVisible(false);
  }

  this->Modified();
}

//----------------------------------------------------------------------------
vtkChartXY* vtkPVBagChartRepresentation::GetChart()
{
  if (this->ContextView)
  {
    return vtkChartXY::SafeDownCast(this->ContextView->GetContextItem());
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPVBagChartRepresentation::PrepareForRendering()
{
  this->Superclass::PrepareForRendering();

  vtkChartXY* chart = this->GetChart();
  vtkPlotBag* bagPlot = 0;
  vtkPlotHistogram2D* gridPlot = 0;
  vtkPlotLine* p50LinePlot = 0;
  vtkPlotLine* pUserLinePlot = 0;
  for (int i = 0;
       i < chart->GetNumberOfPlots() && (!bagPlot || !gridPlot || !p50LinePlot || !pUserLinePlot);
       i++)
  {
    // Search for the first bag plot of the chart. For now
    // the chart only manage one bag plot
    if (vtkPlotBag::SafeDownCast(chart->GetPlot(i)))
    {
      bagPlot = vtkPlotBag::SafeDownCast(chart->GetPlot(i));
    }
    if (vtkPlotHistogram2D::SafeDownCast(chart->GetPlot(i)))
    {
      gridPlot = vtkPlotHistogram2D::SafeDownCast(chart->GetPlot(i));
    }
    if (vtkPlotLine::SafeDownCast(chart->GetPlot(i)))
    {
      if (p50LinePlot == NULL)
      {
        p50LinePlot = vtkPlotLine::SafeDownCast(chart->GetPlot(i));
      }
      else
      {
        pUserLinePlot = vtkPlotLine::SafeDownCast(chart->GetPlot(i));
      }
    }
  }
  if (!gridPlot)
  {
    gridPlot = vtkPlotHistogram2D::New();
    chart->AddPlot(gridPlot);
    gridPlot->Delete();
  }

  if (!p50LinePlot)
  {
    p50LinePlot = vtkPlotLine::SafeDownCast(chart->AddPlot(vtkChart::LINE));
    p50LinePlot->PolyLineOff();
    p50LinePlot->SelectableOff();
  }
  if (!pUserLinePlot)
  {
    pUserLinePlot = vtkPlotLine::SafeDownCast(chart->AddPlot(vtkChart::LINE));
    pUserLinePlot->PolyLineOff();
    pUserLinePlot->SelectableOff();
  }

  vtkDoubleArray* thresholdArray = this->LocalThreshold
    ? vtkDoubleArray::SafeDownCast(this->LocalThreshold->GetColumnByName("TValues"))
    : NULL;

  std::ostringstream labelp50;
  labelp50 << "P" << (thresholdArray ? thresholdArray->GetValue(0) : 50);
  p50LinePlot->SetLabel(labelp50.str().c_str());
  std::ostringstream labelpUser;
  labelpUser << "P" << (thresholdArray ? thresholdArray->GetValue(2) : 99);
  pUserLinePlot->SetLabel(labelpUser.str().c_str());
  if (!bagPlot)
  {
    // Create and add a bag plot to the chart
    bagPlot = vtkPlotBag::SafeDownCast(chart->AddPlot(vtkChart::BAG));
  }

  vtkTable* plotInput = this->GetLocalOutput();
  bool hasBagPlot = plotInput && plotInput->GetNumberOfRows() > 0;
  chart->SetVisible((hasBagPlot || this->LocalGrid != NULL) && this->GetVisibility());
  bagPlot->SetVisible(hasBagPlot);
  bagPlot->SetBagVisible(false);

  // Set bag (polygons) brush properties
  bagPlot->GetBrush()->SetColorF(this->BagColor);
  bagPlot->GetBrush()->SetOpacityF(this->Opacity);

  // Set point pen properties
  bagPlot->GetPen()->SetWidth(this->PointSize);
  bagPlot->GetPen()->SetLineType(this->LineStyle);
  bagPlot->GetPen()->SetColorF(this->PointColor);
  bagPlot->GetPen()->SetOpacityF(1.0);

  // Set selection point pen properties
  bagPlot->GetSelectionPen()->SetColorF(this->SelectionColor);

  if (hasBagPlot)
  {
    // We only consider the first vtkTable.
    if (this->YAxisSeriesName && this->DensitySeriesName)
    {
      bagPlot->SetUseIndexForXSeries(this->UseIndexForXAxis);
      if (this->UseIndexForXAxis)
      {
        bagPlot->SetInputData(plotInput, this->YAxisSeriesName, this->DensitySeriesName);
      }
      else
      {
        bagPlot->SetInputData(
          plotInput, this->XAxisSeriesName, this->YAxisSeriesName, this->DensitySeriesName);
      }
    }
  }

  gridPlot->SetInputData(this->LocalGrid);
  gridPlot->SetVisible(this->LocalGrid != NULL);
  p50LinePlot->SetVisible(this->LocalGrid != NULL);
  pUserLinePlot->SetVisible(this->LocalGrid != NULL);

  if (this->LocalGrid)
  {
    double range[2];
    this->LocalGrid->GetScalarRange(range);

    gridPlot->SetTransferFunction(GetLookupTable());
    if (gridPlot->GetTransferFunction())
    {
      vtkLookupTable* lut = vtkLookupTable::SafeDownCast(gridPlot->GetTransferFunction());
      vtkColorTransferFunction* tf =
        vtkColorTransferFunction::SafeDownCast(gridPlot->GetTransferFunction());
      if (lut)
      {
        lut->SetTableRange(range);
        lut->Build();
      }
      else if (tf)
      {
        double oldRange[2];
        tf->GetRange(oldRange);
        int numberOfNodes = tf->GetSize();
        double* newNodes = new double[6 * numberOfNodes];
        for (int i = 0; i < numberOfNodes; ++i)
        {
          double* newNode = &newNodes[6 * i];
          tf->GetNodeValue(i, newNode);
          newNode[0] =
            (range[1] - range[0]) * (newNode[0] - oldRange[0]) / (oldRange[1] - oldRange[0]) +
            range[0];
        }
        tf->RemoveAllPoints();
        for (int i = 0; i < numberOfNodes; ++i)
        {
          double* newNode = &newNodes[6 * i];
          tf->AddRGBPoint(newNode[0], newNode[1], newNode[2], newNode[3], newNode[4], newNode[5]);
        }
        delete[] newNodes;
      }
    }

    double p50 = thresholdArray ? thresholdArray->GetValue(1) : 0.;
    double pUser = thresholdArray ? thresholdArray->GetValue(3) : 0.;

    vtkNew<vtkContourFilter> medianContour;
    medianContour->SetInputData(this->LocalGrid);
    medianContour->SetNumberOfContours(1);
    medianContour->SetValue(0, p50);
    medianContour->Update();

    vtkNew<vtkTable> medianTable;
    SetPolyLineToTable(vtkPolyData::SafeDownCast(medianContour->GetOutput()), medianTable.Get());
    p50LinePlot->SetInputData(medianTable.Get(), "X", "Y");

    p50LinePlot->GetPen()->SetWidth(this->GridLineThickness);
    p50LinePlot->GetPen()->SetLineType(this->GridLineStyle);
    p50LinePlot->SetColor(this->P50Color[0], this->P50Color[1], this->P50Color[2]);
    p50LinePlot->SetOpacity(this->Opacity);

    vtkNew<vtkContourFilter> pUserContour;
    pUserContour->SetInputData(this->LocalGrid);
    pUserContour->SetNumberOfContours(1);
    pUserContour->SetValue(0, pUser);
    pUserContour->Update();

    vtkNew<vtkTable> pUserTable;
    SetPolyLineToTable(vtkPolyData::SafeDownCast(pUserContour->GetOutput()), pUserTable.Get());
    pUserLinePlot->SetInputData(pUserTable.Get(), "X", "Y");

    pUserLinePlot->GetPen()->SetWidth(this->GridLineThickness);
    pUserLinePlot->GetPen()->SetLineType(this->GridLineStyle);
    pUserLinePlot->SetColor(this->PUserColor[0], this->PUserColor[1], this->PUserColor[2]);
    pUserLinePlot->SetOpacity(this->Opacity);

    p50LinePlot->SetVisible(medianTable->GetNumberOfRows() >= 2);
    pUserLinePlot->SetVisible(pUserTable->GetNumberOfRows() >= 2);
  }

  // Initialize the view title with the explained variance
  vtkPVXYChartView* view = vtkPVXYChartView::SafeDownCast(this->GetView());
  if (view && thresholdArray)
  {
    std::stringstream title;
    title << "Explained variance: " << static_cast<int>(thresholdArray->GetValue(4)) << "%";
    view->SetTitle(title.str().c_str());
  }
}

//----------------------------------------------------------------------------
void vtkPVBagChartRepresentation::SetPolyLineToTable(vtkPolyData* polyline, vtkTable* table)
{
  vtkNew<vtkDoubleArray> x;
  x->SetName("X");
  vtkNew<vtkDoubleArray> y;
  y->SetName("Y");

  vtkCellArray* lines = polyline->GetLines();
  lines->InitTraversal();
  for (vtkIdType nbpts, *pts; lines->GetNextCell(nbpts, pts);)
  {
    for (vtkIdType i = 0; i < nbpts; ++i)
    {
      double* point = polyline->GetPoint(pts[i]);
      x->InsertNextValue(point[0]);
      y->InsertNextValue(point[1]);
    }
  }
  table->AddColumn(x.Get());
  table->AddColumn(y.Get());
}

//----------------------------------------------------------------------------
int vtkPVBagChartRepresentation::RequestData(
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
        int i = 0;
        for (std::set<unsigned int>::const_iterator iter = this->CompositeIndices.begin();
             iter != this->CompositeIndices.end(); ++iter, ++i)
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

      // data must be a multiblock dataset, no matter what.
      assert(data->IsA("vtkMultiBlockDataSet"));

      // now deliver data to the rendering sides:
      // first, reduce it to root node.
      vtkNew<vtkReductionFilter> reductionFilter;
      // For now we do not support parallel servers
      // vtkNew<vtkPVMergeTablesMultiBlock> algo;
      // reductionFilter->SetPostGatherHelper(algo.GetPointer());
      reductionFilter->SetController(pm->GetGlobalController());
      reductionFilter->SetInputData(data);
      reductionFilter->Update();

      data = reductionFilter->GetOutputDataObject(0);

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

  this->LocalGrid = NULL;
  this->LocalThreshold = NULL;

  if (this->LocalOutput)
  {
    this->LocalGrid = vtkImageData::SafeDownCast(this->LocalOutput->GetBlock(2));
    this->LocalThreshold = vtkTable::SafeDownCast(this->LocalOutput->GetBlock(3));
  }

  return vtkPVDataRepresentation::RequestData(request, inputVector, outputVector);
}
