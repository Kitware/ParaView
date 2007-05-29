/*=========================================================================

   Program: ParaView
   Module:    pqExtractLocationsPanel.cxx

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
#include "pqExtractLocationsPanel.h"
#include "ui_pqExtractLocationsPanel.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSelection.h"
#include "vtkSMExtractLocationsProxy.h"
#include "vtkSMProxy.h"

#include <QtDebug>

#include "pqPipelineFilter.h"
#include "pqPropertyManager.h"
#include "pqProxy.h"
#include "pqServer.h"
#include "pqSignalAdaptorTreeWidget.h"
#include "pqTreeWidgetItemObject.h"

class pqExtractLocationsPanel::pqInternal : public Ui::ExtractLocationsPanel
{
public:
  pqSignalAdaptorTreeWidget* LocationsAdaptor;
};

//-----------------------------------------------------------------------------
pqExtractLocationsPanel::pqExtractLocationsPanel(pqProxy* _proxy, QWidget* _parent)
  : pqObjectPanel(_proxy, _parent)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);

  this->Internal->LocationsAdaptor=
    new pqSignalAdaptorTreeWidget(this->Internal->Locations, true);

  QObject::connect(this->Internal->Delete, SIGNAL(clicked()),
    this, SLOT(deleteSelected()));
  QObject::connect(this->Internal->DeleteAll, SIGNAL(clicked()),
    this, SLOT(deleteAll()));
  QObject::connect(this->Internal->NewValue, SIGNAL(clicked()),
    this, SLOT(newValue()));

  this->linkServerManagerProperties();
}

//-----------------------------------------------------------------------------
pqExtractLocationsPanel::~pqExtractLocationsPanel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqExtractLocationsPanel::linkServerManagerProperties()
{
  pqPropertyManager* pmanager = this->propertyManager();

  vtkSMProxy* smproxy = this->proxy();
  pmanager->registerLink(
    this->Internal->LocationsAdaptor, "values", SIGNAL(valuesChanged()),
    smproxy, smproxy->GetProperty("Locations"));
}

//-----------------------------------------------------------------------------
void pqExtractLocationsPanel::select()
{
  this->Superclass::select();
}

//-----------------------------------------------------------------------------
void pqExtractLocationsPanel::deleteSelected()
{
  QTreeWidget* activeTree = this->Internal->Locations;

  QList<QTreeWidgetItem*> items = activeTree->selectedItems(); 
  foreach (QTreeWidgetItem* item, items)
    {
    delete item;
    }
}

//-----------------------------------------------------------------------------
void pqExtractLocationsPanel::deleteAll()
{
  QTreeWidget* activeTree = this->Internal->Locations;
  activeTree->clear();
}

//-----------------------------------------------------------------------------
void pqExtractLocationsPanel::newValue()
{
  pqSignalAdaptorTreeWidget* adaptor = this->Internal->LocationsAdaptor;
  QTreeWidget* activeTree = this->Internal->Locations;

  QList<QVariant> value;
  // TODO: Use some good defaults.
  value.push_back(0);
  value.push_back(0);
  value.push_back(0);

  QTreeWidgetItem* item = adaptor->appendValue(value);

  // change the current item and make it editable.
  activeTree->setCurrentItem(item, 0);
  activeTree->editItem(item, 0);
}

