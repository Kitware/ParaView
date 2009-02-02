/*=========================================================================

   Program: ParaView
   Module:    pqBarChartView.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqBarChartView.h"

// Server Manager Includes.
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSMChartRepresentationProxy.h"
#include "vtkPVDataInformation.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkTable.h"
#include "vtkSmartPointer.h"
#include "vtkQtBarChartView.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisModel.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartWidget.h"
#include "vtkQtChartSeriesModelCollection.h"
#include "vtkQtChartTableRepresentation.h"

// Qt Includes.
#include <QPushButton>

// ParaView Includes.
#include "pqDataRepresentation.h"
#include "pqBarChartRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
class pqBarChartView::pqInternal
{
public:
  pqInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->BarChartView = vtkSmartPointer<vtkQtBarChartView>::New();
    }

  ~pqInternal()
    {

    }

  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  vtkSmartPointer<vtkQtBarChartView> BarChartView;
  QMap<pqRepresentation*,
      vtkSmartPointer<vtkQtChartTableRepresentation> > RepresentationMap;

};

//-----------------------------------------------------------------------------
pqBarChartView::pqBarChartView(
 const QString& group, const QString& name, 
    vtkSMViewProxy* viewModule, pqServer* server, 
    QObject* _parent/*=NULL*/):
   pqView(barChartViewType(), group, name, viewModule, server, _parent)
{
  this->Internal = new pqInternal();
  QObject::connect(this, SIGNAL(representationAdded(pqRepresentation*)),
    this, SLOT(onAddRepresentation(pqRepresentation*)));
  QObject::connect(this, SIGNAL(representationRemoved(pqRepresentation*)),
    this, SLOT(onRemoveRepresentation(pqRepresentation*)));
  QObject::connect(
    this, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)),
    this, SLOT(updateRepresentationVisibility(pqRepresentation*, bool)));

  // Set up the paraview style interactor.
  vtkQtChartArea* area = this->Internal->BarChartView->GetChartArea();
  vtkQtChartMouseSelection* selector =
    vtkQtChartInteractorSetup::createSplitZoom(area);
  this->Internal->BarChartView->AddChartSelectionHandlers(selector);

  // Set up the view undo/redo.
  vtkQtChartContentsSpace *contents = area->getContentsSpace();
  this->connect(contents, SIGNAL(historyPreviousAvailabilityChanged(bool)),
    this, SIGNAL(canUndoChanged(bool)));
  this->connect(contents, SIGNAL(historyNextAvailabilityChanged(bool)),
    this, SIGNAL(canRedoChanged(bool)));

  // Listen for title property changes.
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("ChartTitle"), vtkCommand::ModifiedEvent,
      this, SLOT(updateTitle()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("ChartTitleFont"), vtkCommand::ModifiedEvent,
      this, SLOT(updateTitleFont()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("ChartTitleColor"), vtkCommand::ModifiedEvent,
      this, SLOT(updateTitleColor()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("ChartTitleAlignment"),
      vtkCommand::ModifiedEvent, this, SLOT(updateTitleAlignment()));

  // Listen for axis title property changes.
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("AxisTitle"), vtkCommand::ModifiedEvent,
      this, SLOT(updateAxisTitle()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("AxisTitleFont"), vtkCommand::ModifiedEvent,
      this, SLOT(updateAxisTitleFont()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("AxisTitleColor"), vtkCommand::ModifiedEvent,
      this, SLOT(updateAxisTitleColor()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("AxisTitleAlignment"),
      vtkCommand::ModifiedEvent, this, SLOT(updateAxisTitleAlignment()));

  // Listen for legend property changes.
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("ShowLegend"), vtkCommand::ModifiedEvent,
      this, SLOT(updateLegendVisibility()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("LegendLocation"), vtkCommand::ModifiedEvent,
      this, SLOT(updateLegendLocation()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("LegendFlow"), vtkCommand::ModifiedEvent,
      this, SLOT(updateLegendFlow()));

  // Listen for axis drawing property changes.
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("ShowAxis"), vtkCommand::ModifiedEvent,
      this, SLOT(updateAxisVisibility()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("AxisColor"), vtkCommand::ModifiedEvent,
      this, SLOT(updateAxisColor()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("ShowAxisGrid"), vtkCommand::ModifiedEvent,
      this, SLOT(updateGridVisibility()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("AxisGridType"), vtkCommand::ModifiedEvent,
      this, SLOT(updateGridColorType()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("AxisGridColor"), vtkCommand::ModifiedEvent,
      this, SLOT(updateGridColor()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("ShowAxisLabels"), vtkCommand::ModifiedEvent,
      this, SLOT(updateAxisLabelVisibility()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("AxisLabelFont"), vtkCommand::ModifiedEvent,
      this, SLOT(updateAxisLabelFont()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("AxisLabelColor"), vtkCommand::ModifiedEvent,
      this, SLOT(updateAxisLabelColor()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("AxisLabelPrecision"), vtkCommand::ModifiedEvent,
      this, SLOT(updateAxisLabelPrecision()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("AxisLabelNotation"), vtkCommand::ModifiedEvent,
      this, SLOT(updateAxisLabelNotation()));

  // Listen for axis layout property changes.
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("AxisScale"), vtkCommand::ModifiedEvent,
      this, SLOT(updateAxisScale()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("AxisBehavior"), vtkCommand::ModifiedEvent,
      this, SLOT(updateAxisBehavior()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("AxisMinimum"), vtkCommand::ModifiedEvent,
      this, SLOT(updateAxisRange()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("AxisMaximum"), vtkCommand::ModifiedEvent,
      this, SLOT(updateAxisRange()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("LeftAxisLabels"), vtkCommand::ModifiedEvent,
      this, SLOT(updateLeftAxisLabels()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("BottomAxisLabels"), vtkCommand::ModifiedEvent,
      this, SLOT(updateBottomAxisLabels()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("RightAxisLabels"), vtkCommand::ModifiedEvent,
      this, SLOT(updateRightAxisLabels()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("TopAxisLabels"), vtkCommand::ModifiedEvent,
      this, SLOT(updateTopAxisLabels()));

  // Listen for bar chart property changes.
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("HelpFormat"), vtkCommand::ModifiedEvent,
      this, SLOT(updateHelpFormat()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("OutlineStyle"), vtkCommand::ModifiedEvent,
      this, SLOT(updateOutlineStyle()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("GroupFraction"), vtkCommand::ModifiedEvent,
      this, SLOT(updateGroupFraction()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("WidthFraction"), vtkCommand::ModifiedEvent,
      this, SLOT(updateWidthFraction()));

  // Add the current Representations to the chart.
  QList<pqRepresentation*> currentRepresentations = this->getRepresentations();
  foreach(pqRepresentation* rep, currentRepresentations)
    {
    this->onAddRepresentation(rep);
    }
}

//-----------------------------------------------------------------------------
pqBarChartView::~pqBarChartView()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QWidget* pqBarChartView::getWidget()
{
  return this->Internal->BarChartView->GetChartWidget();
}

//-----------------------------------------------------------------------------
void pqBarChartView::setDefaultPropertyValues()
{
  pqView::setDefaultPropertyValues();

  // Load defaults for the properties that need them.
  int i = 0;
  QList<QVariant> values;
  for(i = 0; i < 4; i++)
    {
    values.append(QVariant((double)0.0));
    values.append(QVariant((double)0.0));
    values.append(QVariant((double)0.0));
    }

  vtkSMProxy *proxy = this->getProxy();
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisLabelColor"), values);
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisTitleColor"), values);
  values.clear();
  for(i = 0; i < 4; i++)
    {
    if(i < 2)
      {
      values.append(QVariant((double)0.0));
      values.append(QVariant((double)0.0));
      values.append(QVariant((double)0.0));
      }
    else
      {
      // Use a different color for the right and top axis.
      values.append(QVariant((double)0.0));
      values.append(QVariant((double)0.0));
      values.append(QVariant((double)0.5));
      }
    }

  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisColor"), values);
  values.clear();
  for(i = 0; i < 4; i++)
    {
    QColor grid = Qt::lightGray;
    values.append(QVariant((double)grid.redF()));
    values.append(QVariant((double)grid.greenF()));
    values.append(QVariant((double)grid.blueF()));
    }

  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisGridColor"), values);
  QFont chartFont = this->Internal->BarChartView->GetChartWidget()->font();
  values.clear();
  values.append(chartFont.family());
  values.append(QVariant(chartFont.pointSize()));
  values.append(QVariant(chartFont.bold() ? 1 : 0));
  values.append(QVariant(chartFont.italic() ? 1 : 0));
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("ChartTitleFont"), values);
  for(i = 0; i < 3; i++)
    {
    values.append(chartFont.family());
    values.append(QVariant(chartFont.pointSize()));
    values.append(QVariant(chartFont.bold() ? 1 : 0));
    values.append(QVariant(chartFont.italic() ? 1 : 0));
    }

  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisLabelFont"), values);
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisTitleFont"), values);

  pqSMAdaptor::setElementProperty(proxy->GetProperty("HelpFormat"),
      QVariant("%s: %1, %2"));
}

//-----------------------------------------------------------------------------
void pqBarChartView::render()
{
  this->Internal->BarChartView->Render();
}

//-----------------------------------------------------------------------------
void pqBarChartView::onAddRepresentation(pqRepresentation* repr)
{
  if (this->Internal->RepresentationMap.contains(repr))
    {
    return;
    }

  pqBarChartRepresentation* chartRep =
    qobject_cast<pqBarChartRepresentation*>(repr);

  if(!chartRep)
    {
    return;
    }

  // For some reason the vtkTable is null at this point, this
  // is my attempt to update the proxy so it won't be null,
  // but even after this the table still has no data :(
  vtkSMChartRepresentationProxy::
    SafeDownCast(chartRep->getProxy())->Update(
    vtkSMViewProxy::SafeDownCast(this->getProxy()));

  vtkTable* table = chartRep->getClientSideData();
  if (!table)
    {
    return;
    }

  // Add the representation to the view
  vtkQtChartTableRepresentation* barChartRep =
    vtkQtChartTableRepresentation::SafeDownCast(
    this->Internal->BarChartView->AddRepresentationFromInput(table));

  // Store the created representation in the map
  this->Internal->RepresentationMap.insert(repr, barChartRep);

  // Update the chart view
  this->Internal->BarChartView->Update();
}


//-----------------------------------------------------------------------------
void pqBarChartView::onRemoveRepresentation(pqRepresentation* repr)
{
  if (!this->Internal->RepresentationMap.contains(repr))
    {
    return;
    }

  vtkQtChartTableRepresentation* barChartRep =
    this->Internal->RepresentationMap.value(repr);

  this->Internal->BarChartView->RemoveRepresentation(barChartRep);
  this->Internal->BarChartView->Update();

  // Remove representation from representation map
  this->Internal->RepresentationMap.remove(repr);
}

//-----------------------------------------------------------------------------
void pqBarChartView::updateRepresentationVisibility(
  pqRepresentation* repr, bool visible)
{
  if (!this->Internal->RepresentationMap.contains(repr))
    {
    return;
    }

  vtkQtChartTableRepresentation* barChartRep =
    this->Internal->RepresentationMap.value(repr);

  if (visible)
    {
    this->Internal->BarChartView->AddRepresentation(barChartRep);
    }
  else
    {
    this->Internal->BarChartView->RemoveRepresentation(barChartRep);
    }

  this->Internal->BarChartView->Update();
}

//-----------------------------------------------------------------------------
void pqBarChartView::undo()
{
  vtkQtChartArea* area = this->Internal->BarChartView->GetChartArea();
  area->getContentsSpace()->historyPrevious();
}

//-----------------------------------------------------------------------------
void pqBarChartView::redo()
{
  vtkQtChartArea* area = this->Internal->BarChartView->GetChartArea();
  area->getContentsSpace()->historyNext();
}

//-----------------------------------------------------------------------------
bool pqBarChartView::canUndo() const
{
  vtkQtChartArea* area = this->Internal->BarChartView->GetChartArea();
  return area->getContentsSpace()->isHistoryPreviousAvailable();
}

//-----------------------------------------------------------------------------
bool pqBarChartView::canRedo() const
{
  vtkQtChartArea* area = this->Internal->BarChartView->GetChartArea();
  return area->getContentsSpace()->isHistoryNextAvailable();
}

//-----------------------------------------------------------------------------
bool pqBarChartView::canDisplay(pqOutputPort* opPort) const
{
  pqPipelineSource* source = opPort? opPort->getSource() :0;
  vtkSMSourceProxy* sourceProxy = source ? 
    vtkSMSourceProxy::SafeDownCast(source->getProxy()) : 0;
  if(!opPort || !source ||
     opPort->getServer()->GetConnectionID() !=
     this->getServer()->GetConnectionID() || !sourceProxy ||
     sourceProxy->GetOutputPortsCreated()==0)
    {
    return false;
    }

  vtkPVDataInformation* dataInfo = opPort->getDataInformation(true);
  return (dataInfo && dataInfo->DataSetTypeIsA("vtkDataObject"));
}

void pqBarChartView::updateTitle()
{
  this->Internal->BarChartView->SetTitle(pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("ChartTitle")).toString().toAscii().data());
}

void pqBarChartView::updateTitleFont()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      this->getProxy()->GetProperty("ChartTitleFont"));
  if(values.size() == 4)
    {
    this->Internal->BarChartView->SetTitleFont(
      values[0].toString().toAscii().data(), values[1].toInt(),
      values[2].toInt() != 0, values[3].toInt() != 0);
    }
}

void pqBarChartView::updateTitleColor()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      this->getProxy()->GetProperty("ChartTitleColor"));
  if(values.size() == 3)
    {
    this->Internal->BarChartView->SetTitleColor(values[0].toDouble(),
      values[1].toDouble(), values[2].toDouble());
    }
}

void pqBarChartView::updateTitleAlignment()
{
  this->Internal->BarChartView->SetTitleAlignment(
    pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("ChartTitleAlignment")).toInt());
}

void pqBarChartView::updateAxisTitle()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("AxisTitle"));
  for(int i = 0; i < 4 && i < values.size(); ++i)
    {
    this->Internal->BarChartView->SetAxisTitle(i,
      values[i].toString().toAscii().data());
    }
}

void pqBarChartView::updateAxisTitleFont()
{
  int i, j;
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("AxisTitleFont"));
  for(i = 0, j = 0; i < 4 && j + 3 < values.size(); i++, j += 4)
    {
    this->Internal->BarChartView->SetAxisTitleFont(i,
      values[j].toString().toAscii().data(), values[j + 1].toInt(),
      values[j + 2].toInt() != 0, values[j + 3].toInt() != 0);
    }
}

void pqBarChartView::updateAxisTitleColor()
{
  int i, j;
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("AxisTitleColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    this->Internal->BarChartView->SetAxisTitleColor(i, values[j].toDouble(),
      values[j + 1].toDouble(), values[j + 2].toDouble());
    }
}

void pqBarChartView::updateAxisTitleAlignment()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("AxisTitleAlignment"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->Internal->BarChartView->SetAxisTitleAlignment(i, values[i].toInt());
    }
}

void pqBarChartView::updateLegendVisibility()
{
  this->Internal->BarChartView->SetLegendVisibility(
    pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("ShowLegend")).toInt() != 0);
}

void pqBarChartView::updateLegendLocation()
{
  this->Internal->BarChartView->SetLegendLocation(
    pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("LegendLocation")).toInt());
}

void pqBarChartView::updateLegendFlow()
{
  this->Internal->BarChartView->SetLegendFlow(
    pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("LegendFlow")).toInt());
}

void pqBarChartView::updateAxisVisibility()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("ShowAxis"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->Internal->BarChartView->SetAxisVisibility(i, values[i].toInt() != 0);
    }
}

void pqBarChartView::updateAxisColor()
{
  int i, j;
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("AxisColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    this->Internal->BarChartView->SetAxisColor(i, values[j].toDouble(),
      values[j + 1].toDouble(), values[j + 2].toDouble());
    }
}

void pqBarChartView::updateGridVisibility()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("ShowAxisGrid"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->Internal->BarChartView->SetGridVisibility(i, values[i].toInt() != 0);
    }
}

void pqBarChartView::updateGridColorType()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("AxisGridType"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->Internal->BarChartView->SetGridColorType(i, values[i].toInt());
    }
}

void pqBarChartView::updateGridColor()
{
  int i, j;
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("AxisGridColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    this->Internal->BarChartView->SetGridColor(i, values[j].toDouble(),
      values[j + 1].toDouble(), values[j + 2].toDouble());
    }
}

void pqBarChartView::updateAxisLabelVisibility()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("ShowAxisLabels"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->Internal->BarChartView->SetAxisLabelVisibility(i,
      values[i].toInt() != 0);
    }
}

void pqBarChartView::updateAxisLabelFont()
{
  int i, j;
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("AxisLabelFont"));
  for(i = 0, j = 0; i < 4 && j + 3 < values.size(); i++, j += 4)
    {
    this->Internal->BarChartView->SetAxisLabelFont(i,
      values[j].toString().toAscii().data(), values[j + 1].toInt(),
      values[j + 2].toInt() != 0, values[j + 3].toInt() != 0);
    }
}

void pqBarChartView::updateAxisLabelColor()
{
  int i, j;
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("AxisLabelColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    this->Internal->BarChartView->SetAxisLabelColor(i, values[j].toDouble(),
      values[j + 1].toDouble(), values[j + 2].toDouble());
    }
}

void pqBarChartView::updateAxisLabelPrecision()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("AxisLabelPrecision"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->Internal->BarChartView->SetAxisLabelPrecision(i, values[i].toInt());
    }
}

void pqBarChartView::updateAxisLabelNotation()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("AxisLabelNotation"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->Internal->BarChartView->SetAxisLabelNotation(i, values[i].toInt());
    }
}

void pqBarChartView::updateAxisScale()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("AxisScale"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->Internal->BarChartView->SetAxisScale(i, values[i].toInt());
    }
}

void pqBarChartView::updateAxisBehavior()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("AxisBehavior"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->Internal->BarChartView->SetAxisBehavior(i, values[i].toInt());
    }
}

void pqBarChartView::updateAxisRange()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("AxisMinimum"));
  QList<QVariant> maxValues = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("AxisMaximum"));
  for(int i = 0; i < 4 && i < values.size() && i < maxValues.size(); i++)
    {
    this->Internal->BarChartView->SetAxisRange(i, values[i].toDouble(),
      maxValues[i].toDouble());
    }
}

void pqBarChartView::updateLeftAxisLabels()
{
  vtkQtChartArea* area = this->Internal->BarChartView->GetChartArea();
  if(area->getAxisLayer()->getAxisBehavior(vtkQtChartAxis::Left) ==
    vtkQtChartAxisLayer::FixedInterval)
    {
    QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      this->getProxy()->GetProperty("LeftAxisLabels"));
    vtkQtChartAxis* axis = this->Internal->BarChartView->GetAxis(0);
    vtkQtChartAxisModel* model = axis->getModel();
    model->startModifyingData();
    model->removeAllLabels();
    QList<QVariant>::Iterator iter = values.begin();
    for( ; iter != values.end(); ++iter)
      {
      model->addLabel(*iter);
      }

    model->finishModifyingData();
    }
}

void pqBarChartView::updateBottomAxisLabels()
{
  vtkQtChartArea* area = this->Internal->BarChartView->GetChartArea();
  if(area->getAxisLayer()->getAxisBehavior(vtkQtChartAxis::Bottom) ==
    vtkQtChartAxisLayer::FixedInterval)
    {
    QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      this->getProxy()->GetProperty("BottomAxisLabels"));
    vtkQtChartAxis* axis = this->Internal->BarChartView->GetAxis(1);
    vtkQtChartAxisModel* model = axis->getModel();
    model->startModifyingData();
    model->removeAllLabels();
    QList<QVariant>::Iterator iter = values.begin();
    for( ; iter != values.end(); ++iter)
      {
      model->addLabel(*iter);
      }

    model->finishModifyingData();
    }
}

void pqBarChartView::updateRightAxisLabels()
{
  vtkQtChartArea* area = this->Internal->BarChartView->GetChartArea();
  if(area->getAxisLayer()->getAxisBehavior(vtkQtChartAxis::Right) ==
    vtkQtChartAxisLayer::FixedInterval)
    {
    QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      this->getProxy()->GetProperty("RightAxisLabels"));
    vtkQtChartAxis* axis = this->Internal->BarChartView->GetAxis(2);
    vtkQtChartAxisModel* model = axis->getModel();
    model->startModifyingData();
    model->removeAllLabels();
    QList<QVariant>::Iterator iter = values.begin();
    for( ; iter != values.end(); ++iter)
      {
      model->addLabel(*iter);
      }

    model->finishModifyingData();
    }
}

void pqBarChartView::updateTopAxisLabels()
{
  vtkQtChartArea* area = this->Internal->BarChartView->GetChartArea();
  if(area->getAxisLayer()->getAxisBehavior(vtkQtChartAxis::Top) ==
    vtkQtChartAxisLayer::FixedInterval)
    {
    QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      this->getProxy()->GetProperty("TopAxisLabels"));
    vtkQtChartAxis* axis = this->Internal->BarChartView->GetAxis(3);
    vtkQtChartAxisModel* model = axis->getModel();
    model->startModifyingData();
    model->removeAllLabels();
    QList<QVariant>::Iterator iter = values.begin();
    for( ; iter != values.end(); ++iter)
      {
      model->addLabel(*iter);
      }

    model->finishModifyingData();
    }
}

void pqBarChartView::updateHelpFormat()
{
  this->Internal->BarChartView->SetHelpFormat(pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("HelpFormat")).toString().toAscii().data());
}

void pqBarChartView::updateOutlineStyle()
{
  this->Internal->BarChartView->SetOutlineStyle(
    pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("OutlineStyle")).toInt());
}

void pqBarChartView::updateGroupFraction()
{
  this->Internal->BarChartView->SetBarGroupFraction(
    (float)pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("GroupFraction")).toDouble());
}

void pqBarChartView::updateWidthFraction()
{
  this->Internal->BarChartView->SetBarWidthFraction(
    (float)pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("WidthFraction")).toDouble());
}

