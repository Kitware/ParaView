/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqPassArraysPanel.h"

#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqSMAdaptor.h"
#include "pqTreeWidget.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPixmap>
#include <QTimer>
#include <QTreeWidgetItem>

namespace
{
  enum ArrayType
  {
    FIELD = 0,
    CELL,
    POINT
  };
}

//-----------------------------------------------------------------------------
pqPassArraysPanel::pqPassArraysPanel(pqProxy* pxy, QWidget* p)
  : pqObjectPanel(pxy, p)
{
  QHBoxLayout* hbox = new QHBoxLayout(this);
  this->SelectorWidget = new pqTreeWidget;

  std::string pixmapDir;
  for(int i=0; i<3; i++)
    {
    if(i == POINT)
      {
      pixmapDir = ":/pqWidgets/Icons/pqPointData16.png";
      }
    else if(i == CELL)
      {
      pixmapDir = ":/pqWidgets/Icons/pqCellData16.png";
      }
    else if(i == FIELD)
      {
      pixmapDir = ":/pqWidgets/Icons/pqGlobalData16.png";
      }
    this->TypePixmaps[i] = new QPixmap(pixmapDir.c_str());
    }

  hbox->addWidget(this->SelectorWidget);
  hbox->setMargin(0);
  hbox->setSpacing(4);

  this->ArrayListInitialized = false;
  // wait a bit before updating the array list so that the info can catch up.
  QTimer::singleShot(10, this, SLOT(updateArrayList() ) );
}

//-----------------------------------------------------------------------------
pqPassArraysPanel::~pqPassArraysPanel()
{
  for(int i=0; i<3; i++)
    {
    delete this->TypePixmaps[i];
    }
}

//-----------------------------------------------------------------------------
/// accept the changes made to the properties
/// changes will be propogated down to the server manager
void pqPassArraysPanel::accept()
{
  this->pqObjectPanel::accept();

  if(!this->proxy())
    {
    return;
    }

  vtkSMProxy* passArraysProxy = this->proxy();
  vtkSMStringVectorProperty* pointArraysProperty =
    vtkSMStringVectorProperty::SafeDownCast(
      passArraysProxy->GetProperty("AddPointDataArray"));
  vtkSMStringVectorProperty* cellArraysProperty =
    vtkSMStringVectorProperty::SafeDownCast(
      passArraysProxy->GetProperty("AddCellDataArray"));
  vtkSMStringVectorProperty* fieldArraysProperty =
    vtkSMStringVectorProperty::SafeDownCast(
      passArraysProxy->GetProperty("AddFieldDataArray"));

  int pointCounter = 0, cellCounter = 0, fieldCounter = 0;
  pointArraysProperty->SetNumberOfElements(0);
  cellArraysProperty->SetNumberOfElements(0);
  fieldArraysProperty->SetNumberOfElements(0);

  int end = this->SelectorWidget->topLevelItemCount();
  for(int i=0; i<end; i++)
    {
    QTreeWidgetItem* item = this->SelectorWidget->topLevelItem(i);
    if(item->checkState(0) == Qt::Checked)
      {
      if(this->ArrayTypes[i] == POINT)
        {
        pqSMAdaptor::setMultipleElementProperty(pointArraysProperty, pointCounter,
                                                item->text(0).toAscii());
        pointCounter++;
        }
      else if(this->ArrayTypes[i] == CELL)
        {
        pqSMAdaptor::setMultipleElementProperty(cellArraysProperty, cellCounter,
                                                item->text(0).toAscii());
        cellCounter++;
        }
      else if(this->ArrayTypes[i] == FIELD)
        {
        pqSMAdaptor::setMultipleElementProperty(fieldArraysProperty, fieldCounter,
                                                item->text(0).toAscii());
        fieldCounter++;
        }
      }
    }

  passArraysProxy->UpdateVTKObjects();
  this->referenceProxy()->setModifiedState(pqProxy::UNMODIFIED);
  this->pqObjectPanel::accept();
}

//-----------------------------------------------------------------------------
/// reset the changes made
/// editor will query properties from the server manager
void pqPassArraysPanel::reset()
{
  vtkSMProxy* passArraysProxy = this->proxy();
  this->allOff(); // sm only stores the ones that have been turned on

  vtkSMStringVectorProperty* pointArraysProperty =
    vtkSMStringVectorProperty::SafeDownCast(
      passArraysProxy->GetProperty("AddPointDataArray"));
  QList<QVariant> pointArrays =
    pqSMAdaptor::getMultipleElementProperty(pointArraysProperty);
  for(QList<QVariant>::iterator it=pointArrays.begin();it!= pointArrays.end();it++)
    {
    int end = this->SelectorWidget->topLevelItemCount();
    for(int i=0; i<end; i++)
      {
      if(this->ArrayTypes[i] == POINT)
        {
        QTreeWidgetItem* item = this->SelectorWidget->topLevelItem(i);
        if( it->toString() == item->text(0) )
          {
          item->setCheckState(0, Qt::Checked);
          }
        }
      }
    }

  vtkSMStringVectorProperty* cellArraysProperty =
    vtkSMStringVectorProperty::SafeDownCast(
      passArraysProxy->GetProperty("AddCellDataArray"));
  QList<QVariant> cellArrays =
    pqSMAdaptor::getMultipleElementProperty(cellArraysProperty);
  for(QList<QVariant>::iterator it=cellArrays.begin();it!= cellArrays.end();it++)
    {
    int end = this->SelectorWidget->topLevelItemCount();
    for(int i=0; i<end; i++)
      {
      if(this->ArrayTypes[i] == CELL)
        {
        QTreeWidgetItem* item = this->SelectorWidget->topLevelItem(i);
        if( it->toString() == item->text(0) )
          {
          item->setCheckState(0, Qt::Checked);
          }
        }
      }
    }

  vtkSMStringVectorProperty* fieldArraysProperty =
    vtkSMStringVectorProperty::SafeDownCast(
      passArraysProxy->GetProperty("AddFieldDataArray"));
  QList<QVariant> fieldArrays =
    pqSMAdaptor::getMultipleElementProperty(fieldArraysProperty);
  for(QList<QVariant>::iterator it=fieldArrays.begin();it!= fieldArrays.end();it++)
    {
    int end = this->SelectorWidget->topLevelItemCount();
    for(int i=0; i<end; i++)
      {
      if(this->ArrayTypes[i] == FIELD)
        {
        QTreeWidgetItem* item = this->SelectorWidget->topLevelItem(i);
        if( it->toString() == item->text(0) )
          {
          item->setCheckState(0, Qt::Checked);
          }
        }
      }
    }
  this->SelectorWidget->reset();
  this->pqObjectPanel::reset();
  this->referenceProxy()->setModifiedState(pqProxy::UNMODIFIED);
}

//-----------------------------------------------------------------------------
void pqPassArraysPanel::allOn()
{
  QTreeWidgetItem* item;
  int end = this->SelectorWidget->topLevelItemCount();
  for(int i=0; i<end; i++)
    {
    item = this->SelectorWidget->topLevelItem(i);
    item->setCheckState(0, Qt::Checked);
    }
}

//-----------------------------------------------------------------------------
void pqPassArraysPanel::allOff()
{
  QTreeWidgetItem* item;
  int end = this->SelectorWidget->topLevelItemCount();
  for(int i=0; i<end; i++)
    {
    item = this->SelectorWidget->topLevelItem(i);
    item->setCheckState(0, Qt::Unchecked);
    }
}

//-----------------------------------------------------------------------------
void pqPassArraysPanel::doToggle(int column)
{
  if(column == 0)
    {
    bool convert = false;
    int cs = this->SelectorWidget->headerItem()->data(
      0, Qt::CheckStateRole).toInt(&convert);
    if(convert)
      {
      if(cs == Qt::Checked)
        {
        this->allOff();
        }
      else
        {
        // both unchecked and partial checked go here
        this->allOn();
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqPassArraysPanel::updateArrayList()
{
  this->SelectorWidget->reset();
  this->SelectorWidget->setHeaderLabel("Arrays");
  this->SelectorWidget->setColumnCount(1);
  this->SelectorWidget->header()->setClickable(true);
  QObject::connect(this->SelectorWidget->header(), SIGNAL(sectionClicked(int)),
                   this, SLOT(doToggle(int)), Qt::QueuedConnection);

  this->SelectorWidget->setRootIsDecorated(0);

  pqPipelineFilter* f = qobject_cast<pqPipelineFilter*>(this->referenceProxy());
  if(!f)
    {
    cerr << "pqPassArraysPanel: Problem creating panel.\n";
    return;
    }
  int index = 0;
  for(int i=0;i<3;i++)
    {
    vtkPVDataSetAttributesInformation* fdi = NULL;
    if(i == POINT)
      {
      fdi = f->getInput(f->getInputPortName(0), 0)->getDataInformation()
        ->GetPointDataInformation();
      }
    else if(i == CELL)
      {
      fdi = f->getInput(f->getInputPortName(0), 0)->getDataInformation()
        ->GetCellDataInformation();
      }
    else if(i == FIELD)
      {
      fdi = f->getInput(f->getInputPortName(0), 0)->getDataInformation()
        ->GetFieldDataInformation();
      }
    if(!fdi)
      {
      continue;
      }
    for(int array=0; array<fdi->GetNumberOfArrays(); array++)
      {
      vtkPVArrayInformation* arrayInfo = fdi->GetArrayInformation(array);
      QString name = arrayInfo->GetName();

      QTreeWidgetItem* item = new QTreeWidgetItem;
      item->setText(0, name);
      // mark the type of array (point, cell, field)
      this->ArrayTypes.insert(this->ArrayTypes.begin(),i);

      QFlags<Qt::ItemFlag> flg=Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
      item->setFlags(flg);
      item->setCheckState(0,Qt::Checked);
      item->setExpanded(true);

      item->setData(0, Qt::DecorationRole, *(this->TypePixmaps[i]));
      this->SelectorWidget->insertTopLevelItem(index, item);
      }
    }
  this->reset();

  // connect the itemChanged() signal to the setModified() slot only after
  // the array list was populated for the first time. otherwise this will
  // cause the apply/reset buttons to be activated incorrectly
  if(!this->ArrayListInitialized)
    {
    QObject::connect(this->SelectorWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
                     this, SLOT(setModified()));
    this->ArrayListInitialized = true;
    }
}
