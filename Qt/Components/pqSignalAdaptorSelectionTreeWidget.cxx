/*=========================================================================

   Program: ParaView
   Module:    pqSignalAdaptorSelectionTreeWidget.cxx

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
#include "pqSignalAdaptorSelectionTreeWidget.h"

#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSmartPointer.h"
#include "vtkSMStringListDomain.h"

#include <QTreeWidget>
#include <QPointer>

#include "pqTreeWidgetItemObject.h"

//-----------------------------------------------------------------------------
class pqSignalAdaptorSelectionTreeWidget::pqInternal
{
public:
  QPointer<QTreeWidget> TreeWidget;
  vtkSmartPointer<vtkSMStringListDomain> Domain;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
};

//-----------------------------------------------------------------------------
pqSignalAdaptorSelectionTreeWidget::pqSignalAdaptorSelectionTreeWidget(
  vtkSMStringListDomain* domain, QTreeWidget* treeWidget) :QObject(treeWidget)
{
  this->Internal = new pqInternal();
  this->Internal->Domain = domain;
  this->Internal->TreeWidget = treeWidget;
  this->Internal->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Internal->VTKConnect->Connect(
    domain, vtkCommand::DomainModifiedEvent,
    this, SLOT(domainChanged()), 0, 0, Qt::QueuedConnection);
  this->domainChanged();
}


//-----------------------------------------------------------------------------
pqSignalAdaptorSelectionTreeWidget::~pqSignalAdaptorSelectionTreeWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSignalAdaptorSelectionTreeWidget::values() const
{
  QList<QVariant> reply;

  int max = this->Internal->TreeWidget->topLevelItemCount();
  for (int cc=0; cc < max; cc++)
    {
    QTreeWidgetItem* item = this->Internal->TreeWidget->topLevelItem(cc);
    if (item && !item->isHidden() && (item->checkState(0) == Qt::Checked))
      {
      reply.push_back(item->text(0));
      }
    }

  return reply;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorSelectionTreeWidget::setValues(
  const QList<QVariant>& new_values)
{
  int max = this->Internal->TreeWidget->topLevelItemCount();
  for (int cc=0; cc < max; cc++)
    {
    QTreeWidgetItem* item = this->Internal->TreeWidget->topLevelItem(cc);
    if (new_values.contains(item->text(0)))
      {
      item->setCheckState(0, Qt::Checked);
      }
    else
      {
      item->setCheckState(0, Qt::Unchecked);
      }
    }
  emit this->valuesChanged();
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorSelectionTreeWidget::domainChanged()
{
  bool changed = false;
  QList<QString> domainValues;
  vtkSMStringListDomain* domain = this->Internal->Domain;
  int numEntries = domain->GetNumberOfStrings();
  for(int i=0; i<numEntries; i++)
    {
    domainValues.append(domain->GetString(i));
    }

  // Now update the tree widget. We hide any elements no longer in the domain.
  int max = this->Internal->TreeWidget->topLevelItemCount();
  for (int cc=0; cc < max; cc++)
    {
    QTreeWidgetItem* item = this->Internal->TreeWidget->topLevelItem(cc);
    if (domainValues.contains(item->text(0)))
      {
      if (item->isHidden())
        {
        changed = true;
        }
      item->setHidden(false);
      domainValues.removeAll(item->text(0));
      }
    else
      {
      if (!item->isHidden())
        {
        changed = true;
        }
      item->setHidden(true);
      }
    }

  foreach (QString newValue, domainValues)
    {
    QList<QString> itemValue;
    itemValue << newValue;
    pqTreeWidgetItemObject* item = new pqTreeWidgetItemObject(
      this->Internal->TreeWidget, itemValue);
    item->setCheckState(0, Qt::Unchecked);
    QObject::connect(item, SIGNAL(checkedStateChanged(bool)),
      this, SIGNAL(valuesChanged()), Qt::QueuedConnection);
    changed = true;
    }
  if (changed)
    {
    emit this->valuesChanged();
    }
}
