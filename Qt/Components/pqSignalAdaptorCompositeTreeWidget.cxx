/*=========================================================================

   Program: ParaView
   Module:    pqSignalAdaptorCompositeTreeWidget.cxx

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

========================================================================*/
#include "pqSignalAdaptorCompositeTreeWidget.h"

// Server Manager Includes.
#include "vtkEventQtSlotConnect.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMIntVectorProperty.h"

// Qt Includes.
#include <QPointer>
#include <QTreeWidget>

// ParaView Includes.
#include "pqTreeWidgetItemObject.h"

// This TreeItem specialization needs some explaination.
// Default Qt behaviour for tristate items:
//   - If all immediate children are checked or partially checked
//     then the item becomes fully checked.
// This is not approriate for this widget. A parent item should never be fully
// checked unless the user explicitly checked it, since otherwise, the behaviour
// of the filter is to pass the entire subtree through. 
// This class fixes that issue.
class pqCompositeTreeWidgetItem : public pqTreeWidgetItemObject
{
  typedef pqTreeWidgetItemObject Superclass;
  int triStateCheckState;
public:
  pqCompositeTreeWidgetItem(QTreeWidget* tree, QStringList _values):
    Superclass(tree, _values), triStateCheckState(Qt::Unchecked)
  {
  }

  pqCompositeTreeWidgetItem(QTreeWidgetItem* item, QStringList _values):
    Superclass(item, _values), triStateCheckState(Qt::Unchecked)
  {
  }

  virtual QVariant data(int column, int role) const
    {
    if (role == Qt::CheckStateRole && 
      this->childCount() > 0 && (this->flags() & Qt::ItemIsTristate))
      {
      // superclass implementation of this method only checks the immediate
      // children.
      QVariant vSuggestedValue = this->Superclass::data(column, role);
      int suggestedValue = vSuggestedValue.toInt();
      if (this->triStateCheckState == Qt::PartiallyChecked)
        {
        if (suggestedValue == Qt::Checked || suggestedValue == Qt::PartiallyChecked)
          {
          return Qt::PartiallyChecked;
          }
        // suggestedValue == Qt:Unchecked
        return Qt::Unchecked;
        }
      return this->triStateCheckState;
      }
    return this->Superclass::data(column, role);

    }

  virtual void setData(int column, int role, const QVariant &value)
    {
    // Superclass will also mark all children checked or unchecked.
    this->Superclass::setData(column, role, value);

    if (role == Qt::CheckStateRole && column==0)
      {
      if (this->flags() & Qt::ItemIsTristate)
        {
        this->triStateCheckState = value.toInt();
        }

      // tell my parent it is partially checked (at best).
      pqCompositeTreeWidgetItem* itemParent = 
        dynamic_cast<pqCompositeTreeWidgetItem*>(
          static_cast<QTreeWidgetItem*>(this)->parent());
      while (itemParent)
        {
        itemParent->triStateCheckState = Qt::PartiallyChecked;
        itemParent = 
          static_cast<pqCompositeTreeWidgetItem*>(
            static_cast<QTreeWidgetItem*>(itemParent)->parent());
        }
      }
    }


};
class pqSignalAdaptorCompositeTreeWidget::pqInternal
{
public:
  QPointer<QTreeWidget> TreeWidget;
  vtkSmartPointer<vtkSMIntVectorProperty> Property;
  vtkSmartPointer<vtkSMCompositeTreeDomain> Domain;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  QList<pqTreeWidgetItemObject*> Items;
  int DomainMode;

};

//-----------------------------------------------------------------------------
pqSignalAdaptorCompositeTreeWidget::pqSignalAdaptorCompositeTreeWidget(
  QTreeWidget* tree, vtkSMIntVectorProperty* smproperty):
  Superclass(tree)
{
  this->Internal = new pqInternal();
  this->Internal->TreeWidget = tree;
  this->Internal->Property = smproperty;
  this->Internal->DomainMode = vtkSMCompositeTreeDomain::ALL; 
  this->Internal->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->IndexMode = INDEX_MODE_FLAT;
  if (smproperty->GetNumberOfElementsPerCommand() == 2)
    {
    this->IndexMode = INDEX_MODE_LEVEL_INDEX; // (level, index) pairs.
    }

  // If hints are provided, we use those to determine the IndexMode for this
  // property.
  vtkPVXMLElement* hints = smproperty->GetHints();
  if (hints)
    {
    vtkPVXMLElement* useFlatIndex = hints->FindNestedElementByName("UseFlatIndex");
    if (useFlatIndex && useFlatIndex->GetAttribute("value") &&
      strcmp(useFlatIndex->GetAttribute("value"),"0") == 0 && 
      this->IndexMode == INDEX_MODE_FLAT)
      {
      this->IndexMode = INDEX_MODE_LEVEL;
      }
    }

  // get domain
  vtkSMDomainIterator* iter = smproperty->NewDomainIterator();
  iter->Begin();
  while (!iter->IsAtEnd() && !this->Internal->Domain)
    {
    vtkSMDomain* domain = iter->GetDomain();
    this->Internal->Domain = 
      vtkSMCompositeTreeDomain::SafeDownCast(domain);
    iter->Next();
    }
  iter->Delete();

  if (this->Internal->Domain)
    {
    this->Internal->VTKConnect->Connect(
      this->Internal->Domain, vtkCommand::DomainModifiedEvent,
      this, SLOT(domainChanged()));
    this->domainChanged();
    }
}

//-----------------------------------------------------------------------------
pqSignalAdaptorCompositeTreeWidget::~pqSignalAdaptorCompositeTreeWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSignalAdaptorCompositeTreeWidget::values() const
{
  QList<QVariant> reply;

  QList<pqTreeWidgetItemObject*> treeitems = this->Internal->Items; 
  foreach (pqTreeWidgetItemObject* item, treeitems)
    {
    QVariant nodeType = item->data(0, NODE_TYPE);
    if (!nodeType.isValid())
      {
      continue;
      }

    if ((this->Internal->DomainMode == vtkSMCompositeTreeDomain::LEAVES &&
       nodeType.toInt() != LEAF) ||
      (this->Internal->DomainMode == vtkSMCompositeTreeDomain::NON_LEAVES &&
       nodeType.toInt() != NON_LEAF))
      {
      // Skip nodes the filter is not interested in.
      continue;
      }

    if (this->IndexMode == INDEX_MODE_FLAT)
      {
      QVariant metadata = item->data(0, FLAT_INDEX);
      if (metadata.isValid() && item->checkState(0)== Qt::Checked)
        {
        // metadata has the flat index for the node.
        reply.push_back(metadata);
        // cout << metadata.toInt()  << endl;
        }
      }
    else if (this->IndexMode == INDEX_MODE_LEVEL_INDEX)
      {
      QVariant metadata0 = item->data(0, LEVEL_NUMBER);
      QVariant metadata1 = item->data(0, DATASET_INDEX);
      if (metadata0.isValid() && metadata1.isValid() && item->checkState(0) == Qt::Checked)
        {
        reply.push_back(metadata0);
        reply.push_back(metadata1);
        // cout << metadata0.toInt() << ", " << metadata1.toInt() << endl;
        }
      }
    else if (this->IndexMode == INDEX_MODE_LEVEL)
      {
      QVariant metadata0 = item->data(0, LEVEL_NUMBER);
      if (metadata0.isValid() && item->checkState(0) == Qt::Checked)
        {
        reply.push_back(metadata0);
        // cout << metadata0.toInt() << endl;
        }
      }
    }

  return reply;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorCompositeTreeWidget::setValues(const QList<QVariant>& new_values)
{
  bool prev = this->Internal->TreeWidget->blockSignals(true);
  QList<pqTreeWidgetItemObject*> treeitems = this->Internal->Items; 
  bool changed = false;

  if (this->IndexMode == INDEX_MODE_FLAT)
    {
    foreach (pqTreeWidgetItemObject* item, treeitems)
      {
      QVariant metadata = item->data(0, FLAT_INDEX);
      Qt::CheckState cstate = 
        (metadata.isValid() && new_values.contains(metadata))? 
        Qt::Checked : Qt::Unchecked;
      if (item->checkState(0) != cstate)
        {
        item->setCheckState(0, cstate);
        changed = true;
        }
      }
    }
  else if (this->IndexMode == INDEX_MODE_LEVEL)
    {
    foreach (pqTreeWidgetItemObject* item, treeitems)
      {
      QVariant metadata = item->data(0, LEVEL_NUMBER);
      Qt::CheckState cstate = 
        (metadata.isValid() && new_values.contains(metadata))? 
        Qt::Checked : Qt::Unchecked;
      if (item->checkState(0) != cstate)
        {
        item->setCheckState(0, cstate);
        changed = true;
        }
      }
    }
  else if (this->IndexMode == INDEX_MODE_LEVEL_INDEX &&
    this->Internal->Property->GetNumberOfElementsPerCommand() == 2)
    {
    QSet<QPair<unsigned int, unsigned int> > pairs;
    for (int cc=0; cc < new_values.size(); cc+=2)
      {
      unsigned int level = new_values[cc].toUInt();
      unsigned int index = new_values[cc+1].toUInt();
      pairs.insert(QPair<unsigned int, unsigned int>(level, index));
      }

    foreach (pqTreeWidgetItemObject* item, treeitems)
      {
      QVariant metadata0 = item->data(0, LEVEL_NUMBER);
      QVariant metadata1 = item->data(0, DATASET_INDEX);
      Qt::CheckState cstate = (metadata0.isValid() && metadata1.isValid() &&
        pairs.contains(QPair<unsigned int, unsigned int>(metadata0.toUInt(), metadata1.toUInt())))?
        Qt::Checked : Qt::Unchecked;
      if (item->checkState(0) != cstate)
        {
        item->setCheckState(0, cstate);
        changed = true;
        }
      }
    }
  this->Internal->TreeWidget->blockSignals(prev);
  if (changed)
    {
    emit this->valuesChanged();
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorCompositeTreeWidget::domainChanged()
{
  bool prev = this->Internal->TreeWidget->blockSignals(true);
  QList<QVariant> widgetvalues = this->values();
  this->Internal->Items.clear();
  this->Internal->TreeWidget->clear();

  this->Internal->DomainMode = this->Internal->Domain->GetMode();
  vtkPVDataInformation* dInfo = this->Internal->Domain->GetInformation();

  this->FlatIndex = 0;
  this->LevelNo = 0;
  
  pqTreeWidgetItemObject* root = new pqCompositeTreeWidgetItem(
    this->Internal->TreeWidget, QStringList("Root"));
  this->buildTree(root, dInfo);
  
  // now update check state.
  this->setValues(widgetvalues);
  this->Internal->TreeWidget->blockSignals(prev);
  this->Internal->TreeWidget->expandAll();
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorCompositeTreeWidget::buildTree(pqTreeWidgetItemObject* item,
  vtkPVDataInformation* info)
{
  this->Internal->Items.push_back(item);
  item->setData(0, FLAT_INDEX, this->FlatIndex);
  item->setData(0, NODE_TYPE, LEAF);
  item->setCheckState(0, Qt::Unchecked);
  item->setFlags(item->flags()|Qt::ItemIsUserCheckable);
  QObject::connect(item, SIGNAL(checkedStateChanged(bool)),
    this, SIGNAL(valuesChanged()), Qt::QueuedConnection);
  this->FlatIndex++;
  if (!info)
    {
    return;
    }

  vtkPVCompositeDataInformation* cinfo = info->GetCompositeDataInformation();
  if (!cinfo->GetDataIsComposite())
    {
    return;
    }

  item->setData(0, NODE_TYPE, NON_LEAF);
  // only non-leaf nodes can have 3 states.
  item->setFlags(item->flags()|Qt::ItemIsTristate);

  if (cinfo->GetDataIsMultiPiece())
    {
    this->FlatIndex += cinfo->GetNumberOfChildren();
    if (this->IndexMode == INDEX_MODE_LEVEL_INDEX && 
      this->Internal->DomainMode != vtkSMCompositeTreeDomain::NON_LEAVES)
      {
      // user should be able to select individual pieces.
      for (unsigned int cc=0; cc < cinfo->GetNumberOfChildren(); cc++)
        {
        QString childLabel = QString("DataSet %1").arg(cc);
        pqTreeWidgetItemObject* child = new pqCompositeTreeWidgetItem(item,
          QStringList(childLabel));
        this->buildTree(child, NULL);
        child->setData(0, DATASET_INDEX, cc);
        child->setData(0, LEVEL_NUMBER, this->LevelNo);
        }
      }
    return;
    }

  this->LevelNo = 0;
  for (unsigned int cc=0; cc < cinfo->GetNumberOfChildren(); cc++)
    {
    vtkPVDataInformation* childInfo = cinfo->GetDataInformation(cc);
    QString childLabel = QString("DataSet %1").arg(cc);
    bool is_leaf = true;
    if (childInfo && childInfo->GetCompositeDataInformation()->GetDataIsComposite())
      {
      childLabel = QString("Node %1").arg(cc);
      is_leaf = false;
      }

    if (this->Internal->DomainMode != vtkSMCompositeTreeDomain::NON_LEAVES || !is_leaf)
      {
      pqTreeWidgetItemObject* child = new pqCompositeTreeWidgetItem(item,
        QStringList(childLabel));
      this->buildTree(child, cinfo->GetDataInformation(cc));
      child->setData(0, LEVEL_NUMBER, this->LevelNo);
      this->LevelNo++;
      }
    else
      {
      // don't add leaf nodes, when non-leaves alone are selectable.
      this->FlatIndex++;
      }
    }
}
