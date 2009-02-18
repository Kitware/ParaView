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

// ParaView Includes.
#include "pqBarChartRepresentation.h"
#include "pqChartViewPropertyHandler.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"

// Qt Includes.
#include <QDebug>

//-----------------------------------------------------------------------------
class pqBarChartView::pqInternal
{
public:
  pqInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->BarChartView = vtkSmartPointer<vtkQtBarChartView>::New();
    this->ChartProperties = 0;
    }

  ~pqInternal()
    {

    }

  vtkSmartPointer<vtkEventQtSlotConnect>               VTKConnect;
  vtkSmartPointer<vtkQtBarChartView>                   BarChartView;
  QMap<pqRepresentation*,
      vtkSmartPointer<vtkQtChartTableRepresentation> > RepresentationMap;
  pqChartViewPropertyHandler*                          ChartProperties;
  QList<pqBarChartRepresentation*>                     RepsToBeAdded;
};

//-----------------------------------------------------------------------------
pqBarChartView::pqBarChartView(const QString& group,
                               const QString& name, 
                               vtkSMViewProxy* viewModule,
                               pqServer* server, 
                               QObject* parent/*=NULL*/):
  pqView(barChartViewType(), group, name, viewModule, server, parent)
{
  this->Internal = new pqInternal();
  QObject::connect(this, SIGNAL(representationAdded(pqRepresentation*)),
    this, SLOT(onAddRepresentation(pqRepresentation*)));
  QObject::connect(this, SIGNAL(representationRemoved(pqRepresentation*)),
    this, SLOT(onRemoveRepresentation(pqRepresentation*)));
  QObject::connect(
    this, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)),
    this, SLOT(updateRepresentationVisibility(pqRepresentation*, bool)));

  QObject::connect(
    this, SIGNAL(endRender()),
    this, SLOT(renderInternal()));

  // Set up the paraview style interactor.
  vtkQtChartArea* area = this->getVtkBarChartView()->GetChartArea();
  vtkQtChartMouseSelection* selector =
    vtkQtChartInteractorSetup::createSplitZoom(area);
  this->getVtkBarChartView()->AddChartSelectionHandlers(selector);

  // Set up the view undo/redo.
  vtkQtChartContentsSpace *contents = area->getContentsSpace();
  this->connect(contents, SIGNAL(historyPreviousAvailabilityChanged(bool)),
    this, SIGNAL(canUndoChanged(bool)));
  this->connect(contents, SIGNAL(historyNextAvailabilityChanged(bool)),
    this, SIGNAL(canRedoChanged(bool)));

  // Set up the basic chart properties handler.
  this->Internal->ChartProperties = new pqChartViewPropertyHandler(
    this->getVtkBarChartView(), viewModule, this);
  this->Internal->ChartProperties->connectProperties(
    this->Internal->VTKConnect);

  // Listen for bar chart property changes.
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("BarHelpFormat"), vtkCommand::ModifiedEvent,
      this, SLOT(updateHelpFormat()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("BarOutlineStyle"), vtkCommand::ModifiedEvent,
      this, SLOT(updateOutlineStyle()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("BarGroupFraction"), vtkCommand::ModifiedEvent,
      this, SLOT(updateGroupFraction()));
  this->Internal->VTKConnect->Connect(
      viewModule->GetProperty("BarWidthFraction"), vtkCommand::ModifiedEvent,
      this, SLOT(updateWidthFraction()));

  // Add the current Representations to the chart.
  QList<pqRepresentation*> currentRepresentations = this->getRepresentations();
  foreach(pqRepresentation* rep, currentRepresentations)
    {
    this->onAddRepresentation(rep);
    }

  // Set default color scheme to blues
  this->getVtkBarChartView()->SetColorSchemeToBlues();
}

//-----------------------------------------------------------------------------
pqBarChartView::~pqBarChartView()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QWidget* pqBarChartView::getWidget()
{
  return this->getVtkBarChartView()->GetChartWidget();
}

//-----------------------------------------------------------------------------
vtkQtBarChartView* pqBarChartView::getVtkBarChartView() const
{
  return this->Internal->BarChartView;
}

//-----------------------------------------------------------------------------
void pqBarChartView::setDefaultPropertyValues()
{
  pqView::setDefaultPropertyValues();

  // Load defaults for the properties that need them.
  this->Internal->ChartProperties->setDefaultPropertyValues();

  pqSMAdaptor::setElementProperty(
    this->getProxy()->GetProperty("BarHelpFormat"), QVariant("%s: %1, %2"));
}

//-----------------------------------------------------------------------------
void pqBarChartView::addPendingRepresentations()
{
  // For each representation in the list of representations to be added
  foreach(pqBarChartRepresentation* rep, this->Internal->RepsToBeAdded)
    {
    // Get the table data
    vtkTable* table = rep->getClientSideData();

    // Make sure table data is valid
    if (!table)
      {
      qWarning() << "Cannot add representation because represtation's table "
                    "data is null.";
      continue;
      }

    // Add the table to the view and get the returned representation
    vtkQtChartTableRepresentation* barChartRep =
      vtkQtChartTableRepresentation::SafeDownCast(
      this->getVtkBarChartView()->AddRepresentationFromInput(table));

    // Store the representation in the map for future lookup
    this->Internal->RepresentationMap.insert(rep, barChartRep);
    }

  // Clear the list
  this->Internal->RepsToBeAdded.clear();
}

//-----------------------------------------------------------------------------
void pqBarChartView::renderInternal()
{
  // Add representations from the list of representations to be added
  this->addPendingRepresentations();

  // Update and render the chart view
  this->getVtkBarChartView()->Update();
  this->getVtkBarChartView()->Render();
}

//-----------------------------------------------------------------------------
void pqBarChartView::onAddRepresentation(pqRepresentation* repr)
{
  // Make sure it is a bar chart representation
  pqBarChartRepresentation* chartRep =
    qobject_cast<pqBarChartRepresentation*>(repr);
  if (!chartRep)
    {
    qWarning() << "Cannot add representation because given representation is "
                "not a pqBarChartRepresentation.";
    return;
    }

  // A pqBarChartRepresentation holds a vtkTable.  However, the vtkTable
  // might be null at this point because the data may not have been collected
  // from the server(s).  The vtkTable is not guaranteed to be valid until
  // the view proxy invokes the EndRender event.  So all we do here is add the
  // representation to a list to be added later.
  this->Internal->RepsToBeAdded.append(chartRep);
}


//-----------------------------------------------------------------------------
void pqBarChartView::onRemoveRepresentation(pqRepresentation* repr)
{
  // Remove from pending list in case the representation is there.
  // It could be in this list of the representation was added during
  // onAddRepresentation() and then removed before renderInternal() is called.
  this->Internal->RepsToBeAdded.removeAll(
    qobject_cast<pqBarChartRepresentation*>(repr));

  // The representation must be in our representation map
  if (!this->Internal->RepresentationMap.contains(repr))
    {
    return;
    }

  // Lookup the chart representation mapped to the given pqRepresentation
  vtkQtChartTableRepresentation* barChartRep =
    this->Internal->RepresentationMap.value(repr);

  // Remove the chart representation from the bar chart view
  this->getVtkBarChartView()->RemoveRepresentation(barChartRep);

  // Remove representation from representation map
  this->Internal->RepresentationMap.remove(repr);
}

//-----------------------------------------------------------------------------
void pqBarChartView::updateRepresentationVisibility(
  pqRepresentation* repr, bool visible)
{
  // The representation must be in our representation map
  if (!this->Internal->RepresentationMap.contains(repr))
    {
    return;
    }

  // Lookup the chart representation mapped to the given pqRepresentation.
  vtkQtChartTableRepresentation* barChartRep =
    this->Internal->RepresentationMap.value(repr);

  if (visible)
    {
    this->getVtkBarChartView()->AddRepresentation(barChartRep);
    }
  else
    {
    this->getVtkBarChartView()->RemoveRepresentation(barChartRep);
    }
}

//-----------------------------------------------------------------------------
void pqBarChartView::undo()
{
  vtkQtChartArea* area = this->getVtkBarChartView()->GetChartArea();
  area->getContentsSpace()->historyPrevious();
}

//-----------------------------------------------------------------------------
void pqBarChartView::redo()
{
  vtkQtChartArea* area = this->getVtkBarChartView()->GetChartArea();
  area->getContentsSpace()->historyNext();
}

//-----------------------------------------------------------------------------
bool pqBarChartView::canUndo() const
{
  vtkQtChartArea* area = this->getVtkBarChartView()->GetChartArea();
  return area->getContentsSpace()->isHistoryPreviousAvailable();
}

//-----------------------------------------------------------------------------
bool pqBarChartView::canRedo() const
{
  vtkQtChartArea* area = this->getVtkBarChartView()->GetChartArea();
  return area->getContentsSpace()->isHistoryNextAvailable();
}

//-----------------------------------------------------------------------------
void pqBarChartView::resetDisplay()
{
  vtkQtChartArea* area = this->getVtkBarChartView()->GetChartArea();
  area->getContentsSpace()->resetZoom();
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

//-----------------------------------------------------------------------------
void pqBarChartView::updateHelpFormat()
{
  this->getVtkBarChartView()->SetHelpFormat(pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("BarHelpFormat")).toString().toAscii().data());
}

//-----------------------------------------------------------------------------
void pqBarChartView::updateOutlineStyle()
{
  this->getVtkBarChartView()->SetOutlineStyle(
    pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("BarOutlineStyle")).toInt());
}

//-----------------------------------------------------------------------------
void pqBarChartView::updateGroupFraction()
{
  this->getVtkBarChartView()->SetBarGroupFraction(
    (float)pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("BarGroupFraction")).toDouble());
}

//-----------------------------------------------------------------------------
void pqBarChartView::updateWidthFraction()
{
  this->getVtkBarChartView()->SetBarWidthFraction(
    (float)pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("BarWidthFraction")).toDouble());
}
