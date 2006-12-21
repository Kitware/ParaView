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

#include "pqDataSetModel.h"
#include "pqElementInspectorWidget.h"
#include "pqSelectionManager.h"
#include "pqPipelineSource.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"

#include "ui_pqElementInspectorWidget.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QPointer>

#include <vtkAppendFilter.h>
#include <vtkUnstructuredGrid.h>
#include <vtkSmartPointer.h>

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

  void clearData()
  {
    this->Data->Initialize();
  }

  void addData(vtkUnstructuredGrid* data)
  {
    vtkAppendFilter* const append_filter = vtkAppendFilter::New();
    append_filter->AddInput(this->Data);
    append_filter->AddInput(data);
    
    vtkUnstructuredGrid* const output = append_filter->GetOutput();
    output->Update();
    
    this->Data->DeepCopy(output);

    append_filter->Delete();
  }
  
  void setData(vtkUnstructuredGrid* data)
  {
    this->Data->DeepCopy(data);
  }

  vtkUnstructuredGrid* const Data;
  QList<QPointer<pqPipelineSource> > Sources;
  pqSelectionManager* SelectionManager;
};

/////////////////////////////////////////////////////////////////////////////////
// pqElementInspectorWidget

pqElementInspectorWidget::pqElementInspectorWidget(QWidget *p) :
  QWidget(p),
  Implementation(new pqImplementation())
{
  this->setObjectName("ElementInspectorWidget");

  this->Implementation->setupUi(this);
  QObject::connect(this->Implementation->ClearButton, 
    SIGNAL(clicked()), SLOT(clear()));
  QObject::connect(
    this->Implementation->SourceComboBox, SIGNAL(currentIndexChanged(int)), 
    this, SLOT(onCurrentSourceIndexChanged(int)));
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
}

//-----------------------------------------------------------------------------
pqElementInspectorWidget::~pqElementInspectorWidget()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::clear()
{
  this->Implementation->clearData();
  this->onElementsChanged(); 
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::addElements(vtkUnstructuredGrid* ug)
{
  this->Implementation->addData(ug);
  this->onElementsChanged();
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::setElements(vtkUnstructuredGrid* ug)
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
void pqElementInspectorWidget::onSelectionChanged(pqSelectionManager* selMan)
{
  this->Implementation->SelectionManager = selMan;

  int currentIndex = this->Implementation->SourceComboBox->currentIndex();
  QString currentDataAttributesType = 
    this->Implementation->DataTypeComboBox->currentText();

  pqPipelineSource* currentSource = 
    (currentIndex >= 0 && this->Implementation->Sources.size() > currentIndex)?
    this->Implementation->Sources[currentIndex] : NULL;
  currentIndex =0;

  this->Implementation->Sources.clear();
  this->Implementation->SourceComboBox->blockSignals(true);
  this->Implementation->SourceComboBox->clear();

  QList<pqPipelineSource*> proxies;
  QList<vtkDataObject*> dataObjects;

  selMan->getSelectedObjects(proxies, dataObjects);

  int index = 0;
  foreach(pqPipelineSource* source, proxies)
    {
    this->Implementation->SourceComboBox->addItem(
      source->getProxyName(), index);
    this->Implementation->Sources.push_back(source);
    if (currentSource && currentSource->getProxy() && 
      source->getProxy() == currentSource->getProxy())
      {
      currentIndex = index;
      }
    index++;
    }
  this->Implementation->SourceComboBox->blockSignals(false);

  // set the current index. If previously selected source is present in the 
  // new selection, we select that one, else the first source is selected.
  this->Implementation->SourceComboBox->setCurrentIndex(currentIndex);
  this->onCurrentSourceIndexChanged(currentIndex);
}

//-----------------------------------------------------------------------------
void pqElementInspectorWidget::onCurrentSourceIndexChanged(int index)
{
  if (index >= 0 && index < this->Implementation->Sources.size())
    {
    vtkSMProxy* proxy = 0;
    vtkDataObject* dataObject = 0;
    this->Implementation->SelectionManager->getSelectedObject(
      static_cast<unsigned int>(index), proxy, dataObject);
    this->setElements(vtkUnstructuredGrid::SafeDownCast(dataObject));
    }
  else
    {
    this->clear();
    }
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
