/*=========================================================================

   Program: ParaView
   Module:    pqElementInspectorWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

=========================================================================*/
#include "pqElementInspectorWidget.h"
#include "ui_pqElementInspectorWidget.h"

#include "vtkProcessModule.h"
#include "vtkSMAbstractViewModuleProxy.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkUnstructuredGrid.h"

#include <QPointer>
#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqDataSetModel.h"
#include "pqElementInspectorView.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqSelectionManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerSelectionModel.h"
#include "pqSMAdaptor.h"

//////////////////////////////////////////////////////////////////////////////
// pqElementInspectorWidget::pqImplementation

struct pqElementInspectorWidget::pqImplementation : public Ui::ElementInspector
{
public:
  pqImplementation() :
    Data(vtkUnstructuredGrid::New()),
    SelectionManager(0)
  {
  }

  ~pqImplementation()
    {
    this->Data->Delete();
    }

  void clear()
    {
    this->Data->Initialize();
    }

  void setData(vtkUnstructuredGrid* data)
    {
    if (data)
      {
      this->Data->ShallowCopy(data);
      }
    else
      {
      this->Data->Initialize();
      }
    }

  vtkUnstructuredGrid* const Data;

  QPointer<pqSelectionManager> SelectionManager;

  /// Server we are currently connected to. Only 1 server at a time please.
  QPointer<pqServer> Server;

  /// The ViewModule for that current server.
  QPointer<pqElementInspectorView> ViewModule;

  /// Displayer for the current selection.
  vtkSmartPointer<vtkSMClientDeliveryRepresentationProxy> SelectionDisplayer;

  /// Currently selected source.
  QPointer<pqPipelineSource> CurrentSource;
};

/////////////////////////////////////////////////////////////////////////////////
// pqElementInspectorWidget

pqElementInspectorWidget::pqElementInspectorWidget(QWidget *p) :
  QWidget(p),
  Implementation(new pqImplementation())
{
  this->setObjectName("ElementInspectorWidget");

  this->Implementation->setupUi(this);

  QObject::connect(
    this->Implementation->DataTypeComboBox, 
    SIGNAL(currentIndexChanged(const QString&)), 
    this, SLOT(onCurrentTypeTextChanged(const QString&)));
  
  this->Implementation->TreeView->setRootIsDecorated(false);
  this->Implementation->TreeView->setAlternatingRowColors(true);
  this->Implementation->TreeView->setModel(
    new pqDataSetModel(this->Implementation->TreeView));

  this->Implementation->DataTypeComboBox->setCurrentIndex(
    this->Implementation->DataTypeComboBox->findText("Cell Data"));

  QObject::connect(pqApplicationCore::instance()->getSelectionModel(),
    SIGNAL(currentChanged(pqServerManagerModelItem*)),
    this, SLOT(onCurrentChanged(pqServerManagerModelItem*)));

  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(preSourceRemoved(pqPipelineSource*)),
    this, SLOT(onSourceRemoved(pqPipelineSource*)));
}

//-----------------------------------------------------------------------------
pqElementInspectorWidget::~pqElementInspectorWidget()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::showOnly(vtkSMClientDeliveryRepresentationProxy* display)
{
  if (!this->Implementation->ViewModule)
    {
    return;
    }

  emit this->beginNonUndoableChanges();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->Implementation->ViewModule->getProxy()->GetProperty("Representations"));
  for (unsigned int cc=0; cc < pp->GetNumberOfProxies(); cc++)
    {
    vtkSMProxy* disp = pp->GetProxy(cc);
    pqSMAdaptor::setElementProperty(
      disp->GetProperty("Visibility"),
      (disp == display)? 1 : 0);
    disp->UpdateVTKObjects();
    }

  emit this->endNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::setServer(pqServer* server)
{
  if (this->Implementation->Server)
    {
    QObject::disconnect(this->Implementation->Server, 0, this, 0);
    if (this->Implementation->ViewModule)
      {
      QObject::disconnect(this->Implementation->ViewModule, 0, this, 0);
      }
    }

  this->Implementation->Server = server;
  this->Implementation->ViewModule = 0;

  if (!server)
    {
    this->updateGUI();
    return;
    }

  // For now we'll simply create a new view module.
  // In future, we may want to locate an existsing one first.
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

  pqElementInspectorView* view = qobject_cast<pqElementInspectorView*>(
    builder->createView(pqElementInspectorView::eiViewType(), server));
  this->Implementation->ViewModule = view;
  QObject::connect(view, SIGNAL(endRender()), this, SLOT(updateGUI()),
    Qt::QueuedConnection); 

  // When the server disappear, we want to ensure that the
  // element inspector is clean.
  QObject::connect(server, SIGNAL(destroyed()),
    this, SLOT(cleanServer()));
}

//-----------------------------------------------------------------------------
// Shows the data from the current visible display in the panel, if any.
// This should rarely be called directly, instead call render on the 
// view module since that ensures that the displayes are updated as well.
void pqElementInspectorWidget::updateGUI()
{
  vtkSMClientDeliveryRepresentationProxy* visibleDisplay = 0;
  if (this->Implementation->ViewModule)
    {
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      this->Implementation->ViewModule->getProxy()->GetProperty("Representations"));

    for (unsigned int cc=0; cc < pp->GetNumberOfProxies(); cc++)
      {
      vtkSMClientDeliveryRepresentationProxy* cdisplay = 
        vtkSMClientDeliveryRepresentationProxy::SafeDownCast(pp->GetProxy(cc));

      if (cdisplay && cdisplay->GetVisibility())
        {
        visibleDisplay = cdisplay;
        break;
        }
      }
    }

  if (visibleDisplay)
    {
    pqDataSetModel* model = qobject_cast<pqDataSetModel*>(
      this->Implementation->TreeView->model());

    // Now update the labels etc.
    if (visibleDisplay == this->Implementation->SelectionDisplayer)
      {
      // We are showing the selection, create the label accordingly.
      pqOutputPort* port = 
        this->Implementation->SelectionManager->getSelectedPort();
      pqPipelineSource* input = port? port->getSource() : 0;
      this->Implementation->SourceLabel->setText(
        QString("%1 (Selection)").arg(input->getSMName()));
      model->setSubstitutePointCellIdNames(true);
      }
    else
      {
      pqDataRepresentation* cdisplay = 
        pqApplicationCore::instance()->getServerManagerModel()->
        findItem<pqDataRepresentation*>(visibleDisplay);

      pqPipelineSource* input = cdisplay->getInput();
      this->Implementation->SourceLabel->setText(
        QString("%1 (Output)").arg(input->getSMName()));
      model->setSubstitutePointCellIdNames(false);
      }
    this->Implementation->DataTypeComboBox->setEnabled(true);
    this->setDataObject(vtkUnstructuredGrid::SafeDownCast(
        visibleDisplay->GetOutput()));
    }
  else
    {
    this->setDataObject(0);
    this->Implementation->SourceLabel->setText(
      "Create a selection to view here");
    // TODO: If selected source can only be displayed if selected, 
    // show such a message.
    this->Implementation->DataTypeComboBox->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::cleanServer()
{
  QString label =  "Create a selection to view here";
  this->Implementation->SourceLabel->setText(label);
  this->Implementation->DataTypeComboBox->setEnabled(false);
  
  this->Implementation->ViewModule = 0;
  this->Implementation->Server = 0;
  this->Implementation->SelectionDisplayer = 0;
  this->Implementation->clear();
  this->onElementsChanged(); 
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::setDataObject(vtkUnstructuredGrid* ug)
{
  this->Implementation->setData(ug);
  this->onElementsChanged();  
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::onElementsChanged()
{
  pqDataSetModel* model = qobject_cast<pqDataSetModel*>(
    this->Implementation->TreeView->model());
  model->setDataSet(this->Implementation->Data);

  this->Implementation->TreeView->reset();
  this->Implementation->TreeView->update();
  emit elementsChanged(this->Implementation->Data);
}


//-----------------------------------------------------------------------------
void pqElementInspectorWidget::inspect(pqPipelineSource* source)
{
  if (!this->Implementation->ViewModule)
    {
    return;
    }

  this->Implementation->CurrentSource = source;

  emit this->beginNonUndoableChanges();
  if (source)
    {
    // Does this source have any display in our view?
    // If not, we create one.
    pqDataRepresentation* srcDisplay = 
      source->getRepresentation(this->Implementation->ViewModule);
    if (!srcDisplay)
      {
      // create a new display for this source.
      // This will create a new display only if the source's
      // output can be displayed by the element inspector.
      srcDisplay = pqApplicationCore::instance()->getObjectBuilder()->
        createDataRepresentation(source->getOutputPort(0), this->Implementation->ViewModule);
      if (srcDisplay)
        {
        pqSMAdaptor::setEnumerationProperty(
          srcDisplay->getProxy()->GetProperty("ReductionType"),
          "UNSTRUCTURED_APPEND");
        pqSMAdaptor::setElementProperty(
          srcDisplay->getProxy()->GetProperty("GenerateProcessIds"), 1);
        srcDisplay->getProxy()->UpdateVTKObjects();
        }
      }

    // If there is an active selection for this source,
    // we always give it a preference.
    pqOutputPort* port = this->Implementation->SelectionManager->getSelectedPort();
    if ( (port? port->getSource() : 0) == source)
      {
      this->showOnly(this->Implementation->SelectionDisplayer);
      }
    else
      {
      vtkSMClientDeliveryRepresentationProxy* displayProxy = (srcDisplay?  
         vtkSMClientDeliveryRepresentationProxy::SafeDownCast(srcDisplay->getProxy()):0);
      this->showOnly(displayProxy);
      }
    }

  emit this->endNonUndoableChanges();
  this->Implementation->ViewModule->render();
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::setSelectionManager(pqSelectionManager* selMan)
{
  if (this->Implementation->SelectionManager)
    {
    QObject::disconnect(this->Implementation->SelectionManager, 0, this, 0);
    }

  this->Implementation->SelectionManager = selMan;

  if (selMan)
    {
    QObject::connect(selMan, SIGNAL(selectionChanged(pqSelectionManager*)),
      this, SLOT(onSelectionChanged()));
    }
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::onSelectionChanged()
{
  // The selection has changed, either a new selection was created
  // or an old one cleared.

  // * Remove the old selection displayer, if any.
  if (this->Implementation->SelectionDisplayer)
    {
    emit this->beginNonUndoableChanges();
    pqSMAdaptor::setElementProperty(
      this->Implementation->SelectionDisplayer->GetProperty("Visibility"), 0);
    pqSMAdaptor::removeProxyProperty(
      this->Implementation->ViewModule->getProxy()->GetProperty("Representations"),
      this->Implementation->SelectionDisplayer);
    this->Implementation->SelectionDisplayer->UpdateVTKObjects();
    this->Implementation->ViewModule->getProxy()->UpdateVTKObjects();
    this->Implementation->SelectionDisplayer = 0;
    emit this->endNonUndoableChanges();
    }
  /* FIXME: For now, we won't show active selection in the Element Inspector.
   * We'll fix that once we have spread sheet view */
  /*
  pqPipelineSource* selectedSource = 
    this->Implementation->SelectionManager->getSelectedSource();
  if (selectedSource)
    {
    emit this->beginNonUndoableChanges();
    
    this->Implementation->SelectionDisplayer = 
      this->Implementation->SelectionManager->getClientSideDisplayer(
        selectedSource);
    pqSMAdaptor::setElementProperty(
      this->Implementation->SelectionDisplayer->GetProperty("Visibility"), 0);
    this->Implementation->SelectionDisplayer->UpdateVTKObjects();
    pqSMAdaptor::addProxyProperty(
      this->Implementation->ViewModule->getProxy()->GetProperty("Representations"),
      this->Implementation->SelectionDisplayer);
    this->Implementation->ViewModule->getProxy()->UpdateVTKObjects();
    emit this->endNonUndoableChanges();

    this->inspect(selectedSource);
    }
  else
  */
    {
    this->inspect(this->Implementation->CurrentSource);
    }
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::onSourceRemoved(pqPipelineSource* source)
{
  // Cleanup displays for the source.
  pqDataRepresentation* srcDisplay = 
    source->getRepresentation(this->Implementation->ViewModule);
  if (srcDisplay)
    {
    emit this->beginNonUndoableChanges();
    bool was_visible = srcDisplay->isVisible();
    pqApplicationCore::instance()->getObjectBuilder()->destroy(srcDisplay);
    emit this->endNonUndoableChanges();
    if (was_visible)
      {
      this->Implementation->ViewModule->render();
      }
    }
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::onCurrentChanged(pqServerManagerModelItem* current)
{
  // We "inspect" the source only if it has been "updated".
  // This handles the case where the source hasn't been accepted even once.
  
  pqPipelineSource* source = qobject_cast<pqPipelineSource*>(current);
  vtkSMSourceProxy* sproxy = source? 
    vtkSMSourceProxy::SafeDownCast(source->getProxy()) : 0;
  if (sproxy && sproxy->GetNumberOfParts() > 0)
    {
    this->inspect(source);
    }
  else
    {
    this->inspect(0);
    }
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::refresh()
{
  this->onCurrentChanged(
    pqApplicationCore::instance()->getSelectionModel()->currentItem());
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::onCurrentTypeTextChanged(const QString& text)
{
  if (text == "Point Data")
    {
    qobject_cast<pqDataSetModel*>(
      this->Implementation->TreeView->model())->setFieldDataType(
      pqDataSetModel::POINT_DATA_FIELD);
    }
  else if (text == "Cell Data")
    {
    qobject_cast<pqDataSetModel*>(
      this->Implementation->TreeView->model())->setFieldDataType(
      pqDataSetModel::CELL_DATA_FIELD);
    }
  else if (text == "Field Data")
    {
    qobject_cast<pqDataSetModel*>(
      this->Implementation->TreeView->model())->setFieldDataType(
      pqDataSetModel::DATA_OBJECT_FIELD);
    }
}
