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

#include "vtkSMProxy.h"
#include "vtkEventQtSlotConnect.h"

#include <QtDebug>

#include "pqPropertyLinks.h"
#include "pqProxy.h"
#include "pqSignalAdaptorTreeWidget.h"

class pqExtractSelectionPanel::pqInternal : public Ui::ExtractSelectionPanel
{
public:
  pqPropertyLinks Links;
  pqSignalAdaptorTreeWidget* GlobalIDsAdaptor;
  pqSignalAdaptorTreeWidget* IndicesAdaptor;
};

//-----------------------------------------------------------------------------
pqExtractSelectionPanel::pqExtractSelectionPanel(pqProxy* _proxy, QWidget* _parent)
  : pqObjectPanel(_proxy, _parent)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  this->Internal->Links.setUseUncheckedProperties(false);

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
  vtkSMProxy* smproxy = this->proxy();
  this->Internal->Links.addPropertyLink(
    this->Internal->UseGlobalIDs, "checked", SIGNAL(toggled(bool)),
    smproxy, smproxy->GetProperty("UseGlobalIDs"));

  this->Internal->Links.addPropertyLink(
    this->Internal->GlobalIDsAdaptor, "values", SIGNAL(valuesChanged()),
    smproxy, smproxy->GetProperty("GlobalIDs"));

  this->Internal->Links.addPropertyLink(
    this->Internal->IndicesAdaptor, "values", SIGNAL(valuesChanged()),
    smproxy, smproxy->GetProperty("Indices"));
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

  QList<QVariant> value;
  // TODO: Use some good defaults.
  value.push_back(0);
  if (!this->Internal->UseGlobalIDs->isChecked())
    {
    value.push_back(0);
    }

  QTreeWidgetItem* item = adaptor->appendValue(value);

  // change the current item and make it editable.
  activeTree->setCurrentItem(item, 0);
  activeTree->editItem(item, 0);
}
