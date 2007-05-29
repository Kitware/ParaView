/*=========================================================================

   Program: ParaView
   Module:    pqExtractSelectionPanel.cxx

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
#include "pqExtractSelectionPanel.h"
#include "ui_pqExtractSelectionPanel.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSelection.h"
#include "vtkSMExtractSelectionProxy.h"
#include "vtkSMProxy.h"

#include <QtDebug>

#include "pqPipelineFilter.h"
#include "pqPropertyManager.h"
#include "pqProxy.h"
#include "pqServer.h"
#include "pqSignalAdaptorTreeWidget.h"
#include "pqTreeWidgetItemObject.h"

class pqExtractSelectionTreeItem : public pqTreeWidgetItemObject
{
public:
  pqExtractSelectionTreeItem(const QStringList& l) 
    : pqTreeWidgetItemObject(l)
  {
  }
  bool operator< ( const QTreeWidgetItem & other ) const  
  {
    int sortCol = treeWidget()->sortColumn();
    double myNumber = text(sortCol).toDouble();
    double otherNumber = other.text(sortCol).toDouble();
    return myNumber < otherNumber;
  }
};

class pqExtractSelectionPanel::pqInternal : public Ui::ExtractSelectionPanel
{
public:
  pqSignalAdaptorTreeWidget* GlobalIDsAdaptor;
  pqSignalAdaptorTreeWidget* IndicesAdaptor;
};

//-----------------------------------------------------------------------------
pqExtractSelectionPanel::pqExtractSelectionPanel(pqProxy* _proxy, QWidget* _parent)
  : pqObjectPanel(_proxy, _parent)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  
  this->Internal->GlobalIDs->sortItems(0, Qt::AscendingOrder);
  this->Internal->Indices->sortItems(0, Qt::AscendingOrder);

  this->Internal->GlobalIDsAdaptor =
    new pqSignalAdaptorTreeWidget(this->Internal->GlobalIDs, true);

  this->Internal->IndicesAdaptor=
    new pqSignalAdaptorTreeWidget(this->Internal->Indices, true);

  this->Internal->UseGlobalIDs->toggle();
  this->Internal->UseGlobalIDs->toggle();


  QObject::connect(this->Internal->Delete, SIGNAL(clicked()),
    this, SLOT(deleteSelected()));
  QObject::connect(this->Internal->DeleteAll, SIGNAL(clicked()),
    this, SLOT(deleteAll()));
  QObject::connect(this->Internal->NewValue, SIGNAL(clicked()),
    this, SLOT(newValue()));

  this->linkServerManagerProperties();
}

//-----------------------------------------------------------------------------
pqExtractSelectionPanel::~pqExtractSelectionPanel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqExtractSelectionPanel::linkServerManagerProperties()
{
  pqPropertyManager* pmanager = this->propertyManager();

  vtkSMProxy* smproxy = this->proxy();
  pmanager->registerLink(
    this->Internal->UseGlobalIDs, "checked", SIGNAL(toggled(bool)),
    smproxy, smproxy->GetProperty("UseGlobalIDs"));
  pmanager->registerLink(
    this->Internal->GlobalIDsAdaptor, "values", SIGNAL(valuesChanged()),
    smproxy, smproxy->GetProperty("GlobalIDs"));

  pmanager->registerLink(
    this->Internal->IndicesAdaptor, "values", SIGNAL(valuesChanged()),
    smproxy, smproxy->GetProperty("Indices"));
}

//-----------------------------------------------------------------------------
void pqExtractSelectionPanel::select()
{
  this->updateIDRanges();
  this->Superclass::select();
}

//-----------------------------------------------------------------------------
void pqExtractSelectionPanel::updateIDRanges()
{
  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(this->referenceProxy());
  if (!filter)
    {
    return;
    }

  // this filter can have at most 1 input.
  pqPipelineSource* input = (filter->getInputCount()>0)?
    filter->getInput(0):0;
  if (!input)
    {
    return;
    }
 
  vtkPVDataInformation* dataInfo = input->getDataInformation();
  if (!dataInfo)
    {
    return;
    }

  int numPartitions = filter->getServer()->getNumberOfPartitions();
  
  this->Internal->ProcessIDRange->setText(
    QString("Process ID Range: 0 - %1").arg(numPartitions-1));

  vtkPVDataSetAttributesInformation* dsainfo = 0;
  vtkTypeInt64 numIndices;
  if (filter->getProxy()->GetXMLName() == QString("ExtractCellSelection"))
    {
    numIndices = dataInfo->GetNumberOfCells();
    dsainfo = dataInfo->GetCellDataInformation();
    }
  else
    {
    numIndices = dataInfo->GetNumberOfPoints();
    dsainfo = dataInfo->GetPointDataInformation();
    }
  this->Internal->IndexRange->setText(
    QString("Index Range: 0 - %1").arg(numIndices-1));

  vtkPVArrayInformation* gidsInfo = dsainfo->GetAttributeInformation(
    vtkDataSetAttributes::GLOBALIDS);
  if (gidsInfo)
    {
    double* range =gidsInfo->GetComponentRange(0);
    vtkTypeInt64 gid_min = static_cast<vtkTypeInt64>(range[0]);
    vtkTypeInt64 gid_max = static_cast<vtkTypeInt64>(range[1]);

    this->Internal->GlobalIDRange->setText(
      QString("Global ID Range: %1 - %2").arg(gid_min).arg(gid_max));
    }
  else
    {
    this->Internal->GlobalIDRange->setText("Global ID Range: <not available>");
    }
}

//-----------------------------------------------------------------------------
void pqExtractSelectionPanel::deleteSelected()
{
  QTreeWidget* activeTree = (this->Internal->UseGlobalIDs->isChecked()?  
    this->Internal->GlobalIDs: this->Internal->Indices);

  QList<QTreeWidgetItem*> items = activeTree->selectedItems(); 
  foreach (QTreeWidgetItem* item, items)
    {
    delete item;
    }
}

//-----------------------------------------------------------------------------
void pqExtractSelectionPanel::deleteAll()
{
  QTreeWidget* activeTree = (this->Internal->UseGlobalIDs->isChecked()?  
    this->Internal->GlobalIDs: this->Internal->Indices);
  activeTree->clear();
}

//-----------------------------------------------------------------------------
void pqExtractSelectionPanel::newValue()
{
  pqSignalAdaptorTreeWidget* adaptor = 
    (this->Internal->UseGlobalIDs->isChecked()?  
    this->Internal->GlobalIDsAdaptor: this->Internal->IndicesAdaptor);
  QTreeWidget* activeTree = (this->Internal->UseGlobalIDs->isChecked()?  
    this->Internal->GlobalIDs: this->Internal->Indices);

  QStringList value;
  // TODO: Use some good defaults.
  value.push_back(QString::number(0));
  if (!this->Internal->UseGlobalIDs->isChecked())
    {
    value.push_back(QString::number(0));
    }

  pqExtractSelectionTreeItem* item = new pqExtractSelectionTreeItem(value);
  adaptor->appendItem(item);

  // change the current item and make it editable.
  activeTree->setCurrentItem(item, 0);
  activeTree->editItem(item, 0);
}

//-----------------------------------------------------------------------------
void pqExtractSelectionPanel::updateInformationAndDomains()
{
  this->Superclass::updateInformationAndDomains();

  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(
    this->referenceProxy());
  
  pqPipelineSource* input = (filter->getInputCount() > 0)?
    filter->getInput(0) : 0;
  if (!input)
    {
    return;
    }

  vtkPVDataInformation* dataInfo = input->getDataInformation();
  vtkSMExtractSelectionProxy* smproxy = vtkSMExtractSelectionProxy::SafeDownCast(
    filter->getProxy());

  vtkPVDataSetAttributesInformation* dsainfo = 0;
  if (smproxy && smproxy->GetSelectionFieldType() == vtkSelection::CELL)
    {
    dsainfo = dataInfo->GetCellDataInformation();
    }
  else
    {
    dsainfo = dataInfo->GetPointDataInformation();
    }

  if (dsainfo->GetAttributeInformation(vtkDataSetAttributes::GLOBALIDS))
    {
    // We have global ids.
    this->Internal->UseGlobalIDs->setEnabled(true);
    }
  else
    {
    this->Internal->UseGlobalIDs->setCheckState(Qt::Unchecked);
    this->Internal->UseGlobalIDs->setEnabled(false);
    }
}
