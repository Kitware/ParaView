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
#include "vtkSmartPointer.h"
#include "vtkSMGenericViewDisplayProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkUnstructuredGrid.h"

#include <QPointer>

#include "pqApplicationCore.h"
#include "pqDataSetModel.h"
#include "pqPipelineSource.h"
#include "pqSelectionManager.h"
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

    // clean the current.
    this->ClientSideDisplayer = 0;
    }

  void setData(vtkUnstructuredGrid* data)
    {
    if (data)
      {
      this->Data->DeepCopy(data);
      }
    else
      {
      this->Data->Initialize();
      }
    }

  /// Creates a new displayer for the current source, if any.
  void createDisplayer()
    {
    this->ClientSideDisplayer = 0;
    if (!this->CurrentSource)
      {
      return;
      }

    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    vtkSMProxy* input = this->CurrentSource->getProxy();
    vtkSMGenericViewDisplayProxy* csDisplayer = 
      vtkSMGenericViewDisplayProxy::SafeDownCast(
        pxm->NewProxy("displays", "GenericViewDisplay"));
    csDisplayer->SetConnectionID(input->GetConnectionID());
    csDisplayer->SetServers(vtkProcessModule::DATA_SERVER);
    pqSMAdaptor::setProxyProperty(csDisplayer->GetProperty("Input"),
      input);
    pqSMAdaptor::setEnumerationProperty(
      csDisplayer->GetProperty("ReductionType"), "UNSTRUCTURED_APPEND");
    csDisplayer->UpdateVTKObjects();
    csDisplayer->Update();
    this->ClientSideDisplayer = csDisplayer;
    csDisplayer->Delete();
    }

  vtkUnstructuredGrid* const Data;

  /// Source whose output is currently being "inspected"
  /// (either the entire output or a selection from it).
  QPointer<pqPipelineSource> CurrentSource;

  /// Displayer used to obtain the data to the client
  /// for "inspecting". 
  vtkSmartPointer<vtkSMGenericViewDisplayProxy> ClientSideDisplayer;

  /// When set, the element inspector only shows the user selection
  /// for the CurrentSource, if any.
  bool SelectionOnly;

  QPointer<pqSelectionManager> SelectionManager;
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
}

//-----------------------------------------------------------------------------
pqElementInspectorWidget::~pqElementInspectorWidget()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::clear()
{
  QString label =  "Create a selection to view here";
  if (this->Implementation->CurrentSource)
    {
    label = QString("%1 (nothing selected)").arg(
      this->Implementation->CurrentSource->getSMName());
    }
  this->Implementation->SourceLabel->setText(label);
  this->Implementation->DataTypeComboBox->setEnabled(false);
  this->Implementation->clear();
  this->onElementsChanged(); 
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::setElements(vtkUnstructuredGrid* ug)
{
  this->Implementation->setData(ug);
  this->onElementsChanged();  
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::onSourceChanged()
{
  pqDataSetModel* model = qobject_cast<pqDataSetModel*>(
    this->Implementation->TreeView->model());

  model->setDataSet(this->Implementation->Data);
  if (!this->Implementation->CurrentSource)
    {
    // Nothing to show.
    this->clear();
    return;
    }

  this->Implementation->DataTypeComboBox->setEnabled(true);

  // Check if there is an active user selection for this source.
  if (this->Implementation->SelectionManager)
    {
    this->Implementation->ClientSideDisplayer =
      this->Implementation->SelectionManager->getClientSideDisplayer(
        this->Implementation->CurrentSource);
    }

  if (this->Implementation->ClientSideDisplayer) 
    {
    this->Implementation->SourceLabel->setText(
      QString("%1 (Selection)").arg(
        this->Implementation->CurrentSource->getSMName()));
    }
  else if (!this->Implementation->SelectionOnly)
    {
    // Failed to locate active selection, fetch the entire data from the filter
    // and show it.
    this->Implementation->clear();
    this->Implementation->createDisplayer();
    this->Implementation->SourceLabel->setText(
      QString("%1 (Output)").arg(
        this->Implementation->CurrentSource->getSMName()));
    }
  else
    {
    this->clear();
    return;
    }

  if (this->Implementation->ClientSideDisplayer)
    {
    if (this->Implementation->ClientSideDisplayer->UpdateRequired())
      {
      this->Implementation->ClientSideDisplayer->Update();
      }

    this->setElements(
      vtkUnstructuredGrid::SafeDownCast(
        this->Implementation->ClientSideDisplayer->GetOutput()));
    }
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
  this->Implementation->clear();
  this->Implementation->CurrentSource = source;
  this->Implementation->SelectionOnly = true;

  if (source &&
    (source->getProxy()->GetXMLName() == QString("ExtractCellSelection") ||
     source->getProxy()->GetXMLName() == QString("ExtractPointSelection")))
    {
    this->Implementation->SelectionOnly = false;
    }

  this->onSourceChanged();
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
      this, SLOT(onSelectionChanged()), Qt::QueuedConnection);
    }
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::onSelectionChanged()
{
  // We'll simply refresh the view, if the current source has any
  // selection, we'll show it.
  this->onSourceChanged();
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
