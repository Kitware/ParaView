/*=========================================================================

   Program: ParaView
   Module:    pqSignalAdaptorCompositeTreeWidget.cxx

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
#include "vtkSMOutputPort.h"
#include "vtkSMSourceProxy.h"

// Qt Includes.
#include <QPointer>
#include <QTreeWidget>
#include <QtDebug>

// ParaView Includes.
#include "pqTreeWidgetItem.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
class pqSignalAdaptorCompositeTreeWidget::pqInternal
{
public:
  QPointer<QTreeWidget> TreeWidget;
  vtkSmartPointer<vtkSMIntVectorProperty> Property;
  vtkSmartPointer<vtkSMOutputPort> OutputPort;
  vtkSmartPointer<vtkSMCompositeTreeDomain> Domain;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnectSelection;

  QList<pqTreeWidgetItem*> Items;
  int DomainMode;
};

//-----------------------------------------------------------------------------
class pqSignalAdaptorCompositeTreeWidget::pqCallbackAdaptor :
  public pqTreeWidgetItem::pqCallbackHandler
{
  int ChangeDepth;// used to avoid repeated valuesChanged() signal firing when
                  // check states are being changed as a consequence of toggle
                  // the check state for a non-leaf node.
  bool BlockCallbacks;
  bool PrevBlockSignals;
  pqSignalAdaptorCompositeTreeWidget* Adaptor;
public:
  pqCallbackAdaptor(pqSignalAdaptorCompositeTreeWidget* adaptor)
    {
    this->Adaptor = adaptor;
    this->BlockCallbacks = false;
    this->ChangeDepth = 0;
    }
  bool blockCallbacks(bool val)
    {
    bool old = this->BlockCallbacks;
    this->BlockCallbacks = val;
    return old;
    }
  virtual void checkStateChanged(pqTreeWidgetItem* item, int column)
    {
    // When a non-leaf node is toggled, Qt changes the check state of all child
    // nodes. When that's happening we don't want to repeatedly fire
    // valuesChanged() signal. Hence we use this ChangeDepth magic.
    this->ChangeDepth--;
    if (this->ChangeDepth == 0)
      {
      this->Adaptor->blockSignals(this->PrevBlockSignals);
      }
    if (this->BlockCallbacks == false)
      {
      this->Adaptor->updateCheckState(item, column);
      }
    }

  virtual void checkStateAboutToChange(
    pqTreeWidgetItem* /*item*/, int /*column*/)
    {
    // When a non-leaf node is toggled, Qt changes the check state of all child
    // nodes. When that's happening we don't want to repeatedly fire
    // valuesChanged() signal. Hence we use this ChangeDepth magic.
    this->ChangeDepth++;
    if (this->ChangeDepth == 1)
      {
      this->PrevBlockSignals = this->Adaptor->blockSignals(true);
      }
    }

  virtual bool acceptChange(pqTreeWidgetItem* item,
    const QVariant& curValue, const QVariant& newValue, int column, int role)
    {
    if (this->BlockCallbacks == false)
      {
      // * In SINGLE_ITEM, it's an error to not have any item checked. Hence if
      // the user is un-checking an item, we ensure that atleast one other item
      // is checked.
      if (this->Adaptor->CheckMode == pqSignalAdaptorCompositeTreeWidget::SINGLE_ITEM &&
        role == Qt::CheckStateRole && curValue.toInt() == Qt::Checked &&
        newValue.toInt() == Qt::Unchecked && 
        (item->flags() & Qt::ItemIsTristate) == 0)
        {
        // ensure that at least one item is always checked.
        foreach (pqTreeWidgetItem* curitem, this->Adaptor->Internal->Items)
          {
          if (item != curitem && curitem->checkState(column) == Qt::Checked)
            {
            return true;
            }
          }
        return false;
        }
      }
    return true;
    }
};

// This TreeItem specialization needs some explanation.
// Default Qt behavior for tristate items:
//   - If all immediate children are checked or partially checked
//     then the item becomes fully checked.
// This is not appropriate for this widget. A parent item should never be fully
// checked unless the user explicitly checked it, since otherwise, the behavior
// of the filter is to pass the entire subtree through. 
// This class fixes that issue.
class pqCompositeTreeWidgetItem : public pqTreeWidgetItem
{
  typedef pqTreeWidgetItem Superclass;
  int triStateCheckState;
  bool inSetData;
public:
  pqCompositeTreeWidgetItem(QTreeWidget* tree, QStringList _values):
    Superclass(tree, _values), triStateCheckState(-1),
    inSetData(false)

  {
  }

  pqCompositeTreeWidgetItem(QTreeWidgetItem* item, QStringList _values):
    Superclass(item, _values), triStateCheckState(-1),
    inSetData(false)
  {
  }

  virtual QVariant data(int column, int role) const
    {
    if (role == Qt::CheckStateRole && 
      this->triStateCheckState != -1 &&
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

  virtual void setData(int column, int role, const QVariant &in_value)
    {
    this->inSetData = true;
    this->triStateCheckState = -1;

    // Superclass will also mark all children checked or unchecked.
    this->Superclass::setData(column, role, in_value);

    if (role == Qt::CheckStateRole && column==0)
      {
      QVariant value = this->data(column, role);

      if (this->flags() & Qt::ItemIsTristate)
        {
        this->triStateCheckState = value.toInt();
        }

      // tell my parent it is partially checked (at best).
      pqCompositeTreeWidgetItem* itemParent = 
        dynamic_cast<pqCompositeTreeWidgetItem*>(
          static_cast<QTreeWidgetItem*>(this)->parent());
      while (itemParent && !itemParent->inSetData)
        {
        itemParent->triStateCheckState = Qt::PartiallyChecked;
        itemParent = 
          static_cast<pqCompositeTreeWidgetItem*>(
            static_cast<QTreeWidgetItem*>(itemParent)->parent());
        }
      }
    this->inSetData = false;
    }


};

//-----------------------------------------------------------------------------
void pqSignalAdaptorCompositeTreeWidget::constructor(
  QTreeWidget* tree, bool autoUpdateVisibility)
{
  this->Internal = new pqInternal();
  this->Internal->TreeWidget = tree;
  this->Internal->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Internal->VTKConnectSelection = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->AutoUpdateWidgetVisibility = autoUpdateVisibility;
  
  this->Internal->DomainMode = vtkSMCompositeTreeDomain::ALL; 
  this->CheckMode = SINGLE_ITEM;
  this->IndexMode = INDEX_MODE_FLAT;

  this->ShowFlatIndex = false;
  this->ShowDatasetsInMultiPiece = false;
  this->ShowSelectedElementCounts = false;

  this->CallbackAdaptor = new pqCallbackAdaptor(this);
}

//-----------------------------------------------------------------------------
pqSignalAdaptorCompositeTreeWidget::pqSignalAdaptorCompositeTreeWidget(
  QTreeWidget* tree, vtkSMIntVectorProperty* smproperty, 
  bool autoUpdateVisibility/*=false*/,
  bool showSelectedElementCounts/*false*/):
  Superclass(tree)
{
  this->constructor(tree, autoUpdateVisibility);
  this->ShowSelectedElementCounts = showSelectedElementCounts;
  this->Internal->Property = smproperty;
  if (!smproperty)
    {
    qCritical() << "Property cannot be NULL.";
    return;
    }

  // * Determine CheckMode.
  this->CheckMode = smproperty->GetRepeatCommand()? MULTIPLE_ITEMS : SINGLE_ITEM;

  // * Determine IndexMode.
  this->IndexMode = INDEX_MODE_FLAT;
  if (smproperty->GetNumberOfElementsPerCommand() == 2)
    {
    this->IndexMode = INDEX_MODE_LEVEL_INDEX; // (level, index) pairs.
    }

  // IndexMode defaults may be overridden by some hints.
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

  /// * Locate the Domain.
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

  // * Initialize the widget using the current value.
  bool prev = this->blockSignals(true);
  QList<QVariant> curValues = pqSMAdaptor::getMultipleElementProperty(smproperty);
  this->setValues(curValues);
  this->blockSignals(prev);
}

//-----------------------------------------------------------------------------
pqSignalAdaptorCompositeTreeWidget::pqSignalAdaptorCompositeTreeWidget(
  QTreeWidget* tree, 
  vtkSMOutputPort* port,
  int domainMode,
  IndexModes indexMode,
  bool selectMultiple,
  bool autoUpdateVisibility,
  bool showSelectedElementCounts) : Superclass(tree)
{
  this->constructor(tree, autoUpdateVisibility);

  if (!port)
    {
    qCritical() << "Output port cannot be NULL.";
    return;
    }

  this->ShowFlatIndex = true;
  this->ShowDatasetsInMultiPiece = true;
  this->ShowSelectedElementCounts = showSelectedElementCounts;
  this->CheckMode = selectMultiple? MULTIPLE_ITEMS : SINGLE_ITEM;
  this->IndexMode = indexMode;
  this->Internal->DomainMode = domainMode;
  this->Internal->OutputPort = port;
  this->Internal->VTKConnect->Connect(
    port, vtkCommand::UpdateDataEvent,
    this, SLOT(portInformationChanged()));
  this->portInformationChanged();
}

//-----------------------------------------------------------------------------
pqSignalAdaptorCompositeTreeWidget::~pqSignalAdaptorCompositeTreeWidget()
{
  delete this->Internal;
  delete this->CallbackAdaptor;
  this->CallbackAdaptor = 0;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorCompositeTreeWidget::select(unsigned int flat_index)
{
  QList<QTreeWidgetItem*> selItems = this->Internal->TreeWidget->selectedItems();
  foreach (QTreeWidgetItem* item, selItems)
    {
    item->setSelected(false);
    }
  QList<pqTreeWidgetItem*> treeitems = this->Internal->Items; 
  foreach (pqTreeWidgetItem* item, treeitems)
    {
    QVariant metadata = item->data(0, FLAT_INDEX);
    if (metadata.isValid() && metadata.toUInt() == flat_index)
      {
      item->setSelected(true);
      break;
      }
    }
}

//-----------------------------------------------------------------------------
unsigned int pqSignalAdaptorCompositeTreeWidget::getCurrentFlatIndex(bool* valid)
{
  if (valid)
    {
    *valid = false;
    }

  QList<QTreeWidgetItem*> selItems = this->Internal->TreeWidget->selectedItems();
  if (selItems.size() > 0)
    {
    if (valid)
      {
      *valid = true;
      }
    return this->flatIndex(selItems[0]);
    }
  return 0;
}

//-----------------------------------------------------------------------------
unsigned int pqSignalAdaptorCompositeTreeWidget::flatIndex(
  const QTreeWidgetItem* item) const
{
  return item->data(0, FLAT_INDEX).toUInt();
}

//-----------------------------------------------------------------------------
unsigned int pqSignalAdaptorCompositeTreeWidget::hierarchicalLevel(
  const QTreeWidgetItem* item) const
{
  QVariant val = item->data(0, AMR_LEVEL_NUMBER).toUInt();
  return val.toUInt();
}

//-----------------------------------------------------------------------------
unsigned int pqSignalAdaptorCompositeTreeWidget::hierarchicalBlockIndex(
  const QTreeWidgetItem* item) const
{
  QVariant val = item->data(0, AMR_BLOCK_INDEX).toUInt();
  return val.toUInt();
}

//-----------------------------------------------------------------------------
QString pqSignalAdaptorCompositeTreeWidget::blockName(
  const QTreeWidgetItem* item) const
{
  QString block_name = item->data(0, BLOCK_NAME).toString();
  return block_name;
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSignalAdaptorCompositeTreeWidget::values() const
{
  QList<QVariant> reply;

  QList<pqTreeWidgetItem*> treeitems = this->Internal->Items; 
  foreach (pqTreeWidgetItem* item, treeitems)
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
      //cout << metadata.toInt()  << ": " << item->checkState(0) << endl;
      if (metadata.isValid() && item->checkState(0)== Qt::Checked)
        {
        // metadata has the flat index for the node.
        reply.push_back(metadata);
        // cout << metadata.toInt()  << endl;
        }
      }
    else if (this->IndexMode == INDEX_MODE_LEVEL_INDEX)
      {
      QVariant metadata0 = item->data(0, AMR_LEVEL_NUMBER);
      QVariant metadata1 = item->data(0, AMR_BLOCK_INDEX);
      if (metadata0.isValid() && metadata1.isValid() && item->checkState(0) == Qt::Checked)
        {
        reply.push_back(metadata0);
        reply.push_back(metadata1);
        // cout << metadata0.toInt() << ", " << metadata1.toInt() << endl;
        }
      }
    else if (this->IndexMode == INDEX_MODE_LEVEL)
      {
      QVariant metadata0 = item->data(0, AMR_LEVEL_NUMBER);
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
inline bool pqItemIsCheckable(QTreeWidgetItem* item)
{
  return ((item->flags() & Qt::ItemIsUserCheckable) == Qt::ItemIsUserCheckable);
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorCompositeTreeWidget::setValues(const QList<QVariant>& new_values)
{
  bool prev = this->blockSignals(true);
  QList<pqTreeWidgetItem*> treeitems = this->Internal->Items; 
  bool changed = false;

  if (this->IndexMode == INDEX_MODE_FLAT)
    {
    foreach (pqTreeWidgetItem* item, treeitems)
      {
      QVariant metadata = item->data(0, FLAT_INDEX);
      Qt::CheckState cstate = 
        (metadata.isValid() && new_values.contains(metadata))? 
        Qt::Checked : Qt::Unchecked;
      if (::pqItemIsCheckable(item) && item->checkState(0) != cstate)
        {
        item->setCheckState(0, cstate);
        changed = true;
        }
      }
    }
  else if (this->IndexMode == INDEX_MODE_LEVEL)
    {
    foreach (pqTreeWidgetItem* item, treeitems)
      {
      QVariant metadata = item->data(0, AMR_LEVEL_NUMBER);
      Qt::CheckState cstate = 
        (metadata.isValid() && new_values.contains(metadata))? 
        Qt::Checked : Qt::Unchecked;
      if (pqItemIsCheckable(item) && item->checkState(0) != cstate)
        {
        item->setCheckState(0, cstate);
        changed = true;
        }
      }
    }
  else if (this->IndexMode == INDEX_MODE_LEVEL_INDEX)
    {
    QSet<QPair<unsigned int, unsigned int> > pairs;
    for (int cc=0; cc < new_values.size(); cc+=2)
      {
      unsigned int level = new_values[cc].toUInt();
      unsigned int index = new_values[cc+1].toUInt();
      pairs.insert(QPair<unsigned int, unsigned int>(level, index));
      }

    foreach (pqTreeWidgetItem* item, treeitems)
      {
      QVariant metadata0 = item->data(0, AMR_LEVEL_NUMBER);
      QVariant metadata1 = item->data(0, AMR_BLOCK_INDEX);
      Qt::CheckState cstate = (metadata0.isValid() && metadata1.isValid() &&
        pairs.contains(QPair<unsigned int, unsigned int>(metadata0.toUInt(), metadata1.toUInt())))?
        Qt::Checked : Qt::Unchecked;
      if (pqItemIsCheckable(item) && item->checkState(0) != cstate)
        {
        item->setCheckState(0, cstate);
        changed = true;
        }
      }
    }
  this->blockSignals(prev);
  if (changed)
    {
    emit this->valuesChanged();
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorCompositeTreeWidget::domainChanged()
{
  bool prev = this->blockSignals(true);
  QList<QVariant> widgetvalues = this->values();
  this->Internal->Items.clear();
  this->Internal->TreeWidget->clear();

  this->Internal->DomainMode = this->Internal->Domain->GetMode();
  vtkPVDataInformation* dInfo = this->Internal->Domain->GetInformation();

  this->FlatIndex = 0;
  this->LevelNo = 0;
  
  pqTreeWidgetItem* root = new pqCompositeTreeWidgetItem(
    this->Internal->TreeWidget, QStringList("Root"));
  root->setCallbackHandler(this->CallbackAdaptor);
  root->setData(0, ORIGINAL_LABEL, "Root");
  root->setData(0, BLOCK_NAME, QString());
  root->setToolTip(0, root->text(0));
  this->buildTree(root, dInfo);
  this->updateItemFlags();
  this->updateSelectionCounts();
  
  // now update check state.
  this->setValues(widgetvalues);
  this->blockSignals(prev);

  if (this->AutoUpdateWidgetVisibility)
    {
    this->Internal->TreeWidget->setVisible(
      dInfo?
      dInfo->GetCompositeDataInformation()->GetDataIsComposite()==1 : 0);
    }

  if (this->ShowSelectedElementCounts)
    {
    this->setupSelectionUpdatedCallback(
      this->Internal->Domain->GetSource(),
      this->Internal->Domain->GetSourcePort());
    }
  else
    {
    this->setupSelectionUpdatedCallback(NULL, 0);
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorCompositeTreeWidget::portInformationChanged()
{
  bool prev = this->blockSignals(true);
  QList<QVariant> widgetvalues = this->values();
  this->Internal->Items.clear();
  this->Internal->TreeWidget->clear();

  vtkPVDataInformation* dInfo = 
    this->Internal->OutputPort->GetDataInformation();

  this->FlatIndex = 0;
  this->LevelNo = 0;

  pqTreeWidgetItem* root = new pqCompositeTreeWidgetItem(
    this->Internal->TreeWidget, QStringList("Root"));
  root->setCallbackHandler(this->CallbackAdaptor);
  root->setData(0, ORIGINAL_LABEL, "Root");
  root->setData(0, BLOCK_NAME, QString());
  root->setToolTip(0, root->text(0));
  this->buildTree(root, dInfo);
  this->updateItemFlags();
  this->updateSelectionCounts();

  // now update check state.
  this->setValues(widgetvalues);
  this->blockSignals(prev);

  if (this->AutoUpdateWidgetVisibility)
    {
    this->Internal->TreeWidget->setVisible(
      dInfo->GetCompositeDataInformation()->GetDataIsComposite()==1);
    }

  this->setupSelectionUpdatedCallback(NULL, 0);
}

//-----------------------------------------------------------------------------
// For all elements in the tree, this method determines if the item is checkable
// (given the current mode).
void pqSignalAdaptorCompositeTreeWidget::updateItemFlags()
{
  if (this->Internal->DomainMode == vtkSMCompositeTreeDomain::NONE)
    {
    // no item is checkable.
    return;
    }

  foreach (pqTreeWidgetItem* item, this->Internal->Items)
    {
    QVariant vNodeType = item->data(0, NODE_TYPE);
    if (!vNodeType.isValid() || !vNodeType.canConvert<int>())
      {
      continue;
      }

    int nodeType = vNodeType.toInt();
    if (nodeType == LEAF)
      {
      // leaves are always checkable.
      item->setFlags(item->flags()|Qt::ItemIsUserCheckable);
      item->setCheckState(0, Qt::Unchecked);
      }
    else if (nodeType == NON_LEAF)
      {
      // If domainMode == LEAVES and CheckMode == SINGLE_ITEM, then non-leaf are
      // not checkable.
      if (this->Internal->DomainMode != vtkSMCompositeTreeDomain::LEAVES 
        || this->CheckMode != SINGLE_ITEM)
        {
        item->setFlags(item->flags()|Qt::ItemIsUserCheckable|Qt::ItemIsTristate);
        item->setCheckState(0, Qt::Unchecked);
        }
      }
    }
}

//-----------------------------------------------------------------------------
/// Called when an item check state is toggled. This is used only when
/// this->CheckMode == SINGLE_ITEM. We uncheck all other items.
void pqSignalAdaptorCompositeTreeWidget::updateCheckState(pqTreeWidgetItem* item,
  int column)
{
  this->CallbackAdaptor->blockCallbacks(true);
  bool checked = item->checkState(column) == Qt::Checked;
  if (item && checked && this->CheckMode == SINGLE_ITEM)
    {
    foreach (pqTreeWidgetItem* curitem, this->Internal->Items)
      {
      if (curitem != item && (curitem->flags() & Qt::ItemIsUserCheckable) && 
        (curitem->checkState(0) != Qt::Unchecked) &&
        (curitem->flags() & Qt::ItemIsTristate) == 0)
        {
        curitem->setCheckState(0, Qt::Unchecked);
        }
      }
    }
  this->CallbackAdaptor->blockCallbacks(false);
  emit this->valuesChanged();
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorCompositeTreeWidget::buildTree(pqTreeWidgetItem* item,
  vtkPVDataInformation* info)
{
  this->Internal->Items.push_back(item);
  item->setData(0, FLAT_INDEX, this->FlatIndex);
  item->setData(0, NODE_TYPE, LEAF);
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

  if (cinfo->GetDataIsMultiPiece())
    {
    if (this->ShowDatasetsInMultiPiece || 
      (this->IndexMode == INDEX_MODE_LEVEL_INDEX && 
       this->Internal->DomainMode != vtkSMCompositeTreeDomain::NON_LEAVES))
      {
      // multi-piece is treated as a leaf node, unless the IndexMode is
      // INDEX_MODE_LEVEL_INDEX, in which case the pieces in the multi-piece are
      // further expanded.
      item->setData(0, NODE_TYPE, NON_LEAF);
      // user should be able to select individual pieces.
      for (unsigned int cc=0; cc < cinfo->GetNumberOfChildren(); cc++)
        {
        // We don't fetch the names for each piece in a vtkMultiPieceDataSet
        // hence we just make up a name,
        QString childLabel = QString("DataSet %1").arg(cc);
        if (this->ShowFlatIndex)
          {
          childLabel = QString("DataSet (%1)").arg(this->FlatIndex);
          }

        pqTreeWidgetItem* child = new pqCompositeTreeWidgetItem(item,
          QStringList(childLabel));
        child->setCallbackHandler(this->CallbackAdaptor);
        child->setToolTip(0, child->text(0));
        child->setData(0, ORIGINAL_LABEL, childLabel);
        child->setData(0, BLOCK_NAME, QString());
        this->buildTree(child, NULL);
        child->setData(0, AMR_BLOCK_INDEX, cc);
        child->setData(0, AMR_LEVEL_NUMBER, this->LevelNo);
        }
      }
    else
      {
      this->FlatIndex += cinfo->GetNumberOfChildren();
      }
    item->setExpanded(false); // multipieces are not expanded by default.
    return;
    }

  // A composite dataset (non-multipiece) is always a non-leaf node.
  item->setExpanded(true);
  item->setData(0, NODE_TYPE, NON_LEAF);

  this->LevelNo = 0;
  bool is_hierarchical = 
    (strcmp(info->GetCompositeDataClassName(), "vtkHierarchicalBoxDataSet") == 0);

  for (unsigned int cc=0; cc < cinfo->GetNumberOfChildren(); cc++)
    {
    vtkPVDataInformation* childInfo = cinfo->GetDataInformation(cc);
    QString childLabel = QString("DataSet %1").arg(cc);
    QString block_name;

    bool is_leaf = true;
    if (childInfo && childInfo->GetCompositeDataInformation()->GetDataIsComposite())
      {
      childLabel = is_hierarchical? 
        QString("Level %1").arg(cc) : QString("Block %1").arg(cc);
      is_leaf = false;
      }
    if (const char* cname = cinfo->GetName(cc))
      {
      if (cname[0])
        {
        childLabel = cname;
        block_name = cname;
        }
      }

    if (this->ShowFlatIndex)
      {
      childLabel = QString("%1 (%2)").arg(childLabel).arg(this->FlatIndex);
      }

    if (this->Internal->DomainMode != vtkSMCompositeTreeDomain::NON_LEAVES || !is_leaf)
      {
      pqTreeWidgetItem* child = new pqCompositeTreeWidgetItem(item,
        QStringList(childLabel));
      child->setCallbackHandler(this->CallbackAdaptor);
      child->setData(0, ORIGINAL_LABEL, childLabel);
      child->setData(0, BLOCK_NAME, block_name);
      child->setToolTip(0, child->text(0));
      this->buildTree(child, cinfo->GetDataInformation(cc));
      child->setData(0, AMR_LEVEL_NUMBER, this->LevelNo);
      this->LevelNo++;
      }
    else
      {
      // don't add leaf nodes, when non-leaves alone are selectable.
      this->FlatIndex++;
      }
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorCompositeTreeWidget::updateSelectionCounts()
{
  if (!this->ShowSelectedElementCounts)
    {
    // Nothing to do.
    return;
    }

  if (!this->Internal->Domain)
    {
    return;
    }
  
  // Iterate over the selection data information and then update the labels.
  vtkSMSourceProxy* sourceProxy = this->Internal->Domain->GetSource();
  if (!sourceProxy ||
    !sourceProxy->GetSelectionOutput(this->Internal->Domain->GetSourcePort()))
    {
    return;
    }

  vtkPVDataInformation* info = sourceProxy->GetSelectionOutput(
    this->Internal->Domain->GetSourcePort())->GetDataInformation();

  foreach (pqTreeWidgetItem* item, this->Internal->Items)
    {
    if (item->data(0, NODE_TYPE).toInt() != LEAF)
      {
      continue;
      }

    unsigned int flat_index = item->data(0, FLAT_INDEX).toUInt();
    vtkPVDataInformation* subInfo = info->GetDataInformationForCompositeIndex(
      static_cast<int>(flat_index));
    if (subInfo)
      {
      item->setText(0, QString("%1 (%2, %3)").
        arg(item->data(0, ORIGINAL_LABEL).toString()).arg(subInfo->GetNumberOfPoints()).
        arg(subInfo->GetNumberOfCells()));
      item->setToolTip(0, item->text(0));
      }
    else
      {
      item->setText(0, QString("%1").
        arg(item->data(0, ORIGINAL_LABEL).toString()));
      item->setToolTip(0, item->text(0));
      }
    }
}

//-----------------------------------------------------------------------------
/// Set up the callback to know when the selection changes, so that we can
/// update the selected cells/points counts.
void pqSignalAdaptorCompositeTreeWidget::setupSelectionUpdatedCallback(
  vtkSMSourceProxy* source, unsigned int port)
{
  this->Internal->VTKConnectSelection->Disconnect();
  if (source)
    {
    vtkSMSourceProxy* selProxy = source->GetSelectionOutput(port);
    if (selProxy)
      {
      this->Internal->VTKConnectSelection->Connect(
        selProxy, vtkCommand::UpdateDataEvent,
        this, SLOT(updateSelectionCounts()));
      }
    }
}
