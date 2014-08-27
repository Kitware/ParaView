/*=========================================================================

  Program:   ParaView
  Module:    pqMultiBlockInspectorPanel.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqMultiBlockInspectorPanel.h"

#include "pqActiveObjects.h"
#include "pqDoubleRangeDialog.h"
#include "pqOutputPort.h"
#include "pqSelectionManager.h"
#include "pqTreeWidgetSelectionHelper.h"
#include "pqUndoStack.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkSMDoubleMapProperty.h"
#include "vtkSMDoubleMapPropertyIterator.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSelection.h"

#include <QColorDialog>
#include <QHeaderView>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

pqMultiBlockInspectorPanel::pqMultiBlockInspectorPanel(QWidget *parent_)
  : QWidget(parent_)
{
  // setup tree widget
  this->TreeWidget = new QTreeWidget(this);
  this->TreeWidget->setColumnCount(1);
  this->TreeWidget->header()->close();

  // create tree widget selection helper
  new pqTreeWidgetSelectionHelper(this->TreeWidget);

  // disconnect from selection helper's context menu so that
  // we can use our own menu with multi-block support
  this->TreeWidget->disconnect(
    SIGNAL(customContextMenuRequested(const QPoint&)), 0, 0);

  this->connect(this->TreeWidget, SIGNAL(itemSelectionChanged()),
                this, SLOT(currentTreeItemSelectionChanged()));

  this->VisibilityPropertyListener = vtkEventQtSlotConnect::New();

  QVBoxLayout *layout_ = new QVBoxLayout;
  layout_->addWidget(this->TreeWidget);
  setLayout(layout_);

  // listen to active object changes
  pqActiveObjects *activeObjects = &pqActiveObjects::instance();
  this->connect(activeObjects, SIGNAL(portChanged(pqOutputPort*)),
                this, SLOT(setOutputPort(pqOutputPort*)));
  this->connect(activeObjects, SIGNAL(representationChanged(pqRepresentation*)),
                this, SLOT(setRepresentation(pqRepresentation*)));

  // listen to selection changes
  pqSelectionManager *selectionManager =
    qobject_cast<pqSelectionManager*>(
      pqApplicationCore::instance()->manager("SelectionManager"));
  if(selectionManager)
    {
    this->connect(selectionManager, SIGNAL(selectionChanged(pqOutputPort*)),
                  this, SLOT(currentSelectionChanged(pqOutputPort*)));
    }

  // connect to right-click signals in the tree widget
  this->TreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  this->connect(this->TreeWidget, SIGNAL(customContextMenuRequested(QPoint)),
                this, SLOT(treeWidgetCustomContextMenuRequested(QPoint)));
  this->connect(this->TreeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
                this, SLOT(blockItemChanged(QTreeWidgetItem*, int)));

  //  UpdateUITimer helps use collapse updates to the UI whenever the SM
  //  properties change.
  this->UpdateUITimer.setSingleShot(true);
  this->UpdateUITimer.setInterval(0);
  this->connect(&this->UpdateUITimer, SIGNAL(timeout()),
                this, SLOT(updateTreeWidgetBlockVisibilities()));
}

pqMultiBlockInspectorPanel::~pqMultiBlockInspectorPanel()
{
  this->VisibilityPropertyListener->Delete();
}

void pqMultiBlockInspectorPanel::setOutputPort(pqOutputPort *port)
{
  if(this->OutputPort == port)
    {
    return;
    }

  if(this->OutputPort)
    {
    QObject::disconnect(this->OutputPort->getSource(),
                        SIGNAL(dataUpdated(pqPipelineSource*)),
                        this,
                        SLOT(updateInformation()));
    }

  this->OutputPort = port;

  if(this->OutputPort)
    {
    QObject::connect(this->OutputPort->getSource(),
                     SIGNAL(dataUpdated(pqPipelineSource*)),
                     this,
                     SLOT(updateInformation()));
    }

  this->updateInformation();
}

pqOutputPort* pqMultiBlockInspectorPanel::getOutputPort() const
{
  return this->OutputPort.data();
}

void pqMultiBlockInspectorPanel::setRepresentation(pqRepresentation *representation)
{
  if(this->Representation == representation)
    {
    return;
    }

  // disconnect from previous representation
  this->VisibilityPropertyListener->Disconnect();

  this->Representation = representation;

  if(this->Representation)
    {
    // update properties
    this->updateInformation();

    // listen to property changes
    vtkSMProxy *proxy = this->Representation->getProxy();
    vtkSMProperty *visibilityProperty = proxy->GetProperty("BlockVisibility");
    if(visibilityProperty)
      {
      this->VisibilityPropertyListener->Connect(visibilityProperty,
                                                vtkCommand::ModifiedEvent,
                                                &this->UpdateUITimer,
                                                SLOT(start()));
      }

    vtkSMProperty *colorProperty = proxy->GetProperty("BlockColors");
    if(colorProperty)
      {
      this->VisibilityPropertyListener->Connect(colorProperty,
                                                vtkCommand::ModifiedEvent,
                                                &this->UpdateUITimer,
                                                SLOT(start()));
      }
    }
  else
    {
    this->BlockVisibilites.clear();
    this->BlockColors.clear();
    this->TreeWidget->blockSignals(true);
    this->TreeWidget->clear();
    this->TreeWidget->blockSignals(false);
    }
}

void pqMultiBlockInspectorPanel::buildTree(vtkPVCompositeDataInformation *info,
                                           QTreeWidgetItem *parent_,
                                           unsigned int &flatIndex)
{
  for(unsigned int i = 0; i < info->GetNumberOfChildren(); i++)
    {
    vtkPVDataInformation *childInfo = info->GetDataInformation(i);
    const char *childName = info->GetName(i);

    QString text;
    if(childName && childName[0])
      {
      text = childName;
      }
    else
      {
      text = QString("Block #%1").arg(flatIndex);
      }

    QTreeWidgetItem *item = new QTreeWidgetItem(parent_, QStringList() << text);
    item->setData(0, Qt::UserRole, flatIndex);
    item->setData(0, Qt::CheckStateRole, Qt::Checked);
    item->setData(0, Qt::DecorationRole, makeBlockIcon(flatIndex));

    flatIndex++;

    if(childInfo)
      {
      vtkPVCompositeDataInformation *compositeChildInfo =
        childInfo->GetCompositeDataInformation();

      // recurse down through child blocks only if the child block
      // is composite and is not a multi-piece data set
      if(compositeChildInfo->GetDataIsComposite() &&
         !compositeChildInfo->GetDataIsMultiPiece())
        {
        this->buildTree(compositeChildInfo, item, flatIndex);
        }
      }
    else
      {
      item->setDisabled(true);
      }
    }
}

void pqMultiBlockInspectorPanel::updateInformation()
{
  // clear previous information
  this->TreeWidget->blockSignals(true);
  this->TreeWidget->clear();
  this->TreeWidget->blockSignals(false);

  if(!this->OutputPort)
    {
    return;
    }

  // update information
  pqPipelineSource *source = this->OutputPort->getSource();
  vtkPVDataInformation *info = this->OutputPort->getDataInformation();

  if(!source || !info)
    {
    return;
    }

  vtkPVCompositeDataInformation *compositeInfo =
    info->GetCompositeDataInformation();

  if(compositeInfo->GetDataIsComposite())
    {
    this->TreeWidget->blockSignals(true);

    unsigned int flat_index = 0;

    // create root item
    QString rootLabel = source->getSMName();
    QTreeWidgetItem *rootItem =
      new QTreeWidgetItem(this->TreeWidget->invisibleRootItem(),
                          QStringList() << rootLabel);
    rootItem->setData(0, Qt::UserRole, flat_index++);
    rootItem->setData(0, Qt::CheckStateRole, Qt::Checked);

    // build the rest of the tree
    this->buildTree(compositeInfo,
                    rootItem,
                    flat_index);

    // expand root item
    this->TreeWidget->expandItem(rootItem);

    // update visibilities
    this->updateTreeWidgetBlockVisibilities();

    this->TreeWidget->blockSignals(false);
    }
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::setBlockVisibility(unsigned int index, bool visible)
{
  QList<unsigned int> indices;
  indices.push_back(index);
  this->setBlockVisibility(indices, visible);
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::clearBlockVisibility(unsigned int index)
{
  QList<unsigned int> indices;
  indices.push_back(index);
  this->clearBlockVisibility(indices);
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::setBlockVisibility(
  const QList<unsigned int>& indices, bool visible)
{
  foreach(const unsigned int &index, indices)
    {
    this->BlockVisibilites[index] = visible;
    }

  this->updateBlockVisibilities();
  this->Representation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::clearBlockVisibility(
  const QList<unsigned int>& indices)
{
  foreach(const unsigned int &index, indices)
    {
    this->BlockVisibilites.remove(index);
    }

  this->updateBlockVisibilities();
  this->Representation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::setBlockColor(unsigned int index, const QColor &color)
{
  QList<unsigned int> indices;
  indices.push_back(index);
  this->setBlockColor(indices, color);
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::clearBlockColor(unsigned int index)
{
  QList<unsigned int> indices;
  indices.push_back(index);
  this->clearBlockColor(indices);
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::setBlockColor(
  const QList<unsigned int>& indices, const QColor &color)
{
  foreach(const unsigned int &index, indices)
    {
    this->BlockColors[index] = color;
    }

  this->updateBlockColors();
  this->Representation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::clearBlockColor(const QList<unsigned int>& indices)
{
  foreach(const unsigned int &index, indices)
    {
    this->BlockColors.remove(index);
    }
  this->updateBlockColors();
  this->Representation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::setBlockOpacity(unsigned int index, double opacity)
{
  QList<unsigned int> indices;
  indices.push_back(index);
  this->setBlockOpacity(indices, opacity);
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::setBlockOpacity(
  const QList<unsigned int>& indices, double opacity)
{
  foreach(const unsigned int &index, indices)
    {
    this->BlockOpacities[index] = opacity;
    }

  this->updateBlockOpacities();
  this->Representation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::clearBlockOpacity(unsigned int index)
{
  QList<unsigned int> indices;
  indices.push_back(index);
  this->clearBlockOpacity(indices);
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::clearBlockOpacity(
  const QList<unsigned int>& indices)
{
  foreach(const unsigned int &index, indices)
    {
    this->BlockOpacities.remove(index);
    }

  this->updateBlockOpacities();
  this->Representation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::promptAndSetBlockOpacity(unsigned int index)
{
  QList<unsigned int> list;
  list.append(index);
  this->promptAndSetBlockOpacity(list);
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::promptAndSetBlockOpacity(const QList<unsigned int> &indices)
{
  if(indices.isEmpty())
    {
    return;
    }

  double current_opacity = this->BlockOpacities.value(indices[0], 1.0);
  pqDoubleRangeDialog dialog("Opacity:", 0.0, 1.0, this);
  dialog.setValue(current_opacity);
  bool ok = dialog.exec();
  if(ok)
    {
    this->setBlockOpacity(indices, dialog.value());
    }
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::showOnlyBlock(unsigned int index)
{
  QList<unsigned int> indices;
  indices.push_back(index);
  this->showOnlyBlocks(indices);
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::showAllBlocks()
{
  this->BlockVisibilites.clear();
  this->BlockVisibilites[0] = true; // show root block
  this->updateBlockVisibilities();
  this->Representation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::showOnlyBlocks(
  const QList<unsigned int>& indices)
{
  this->BlockVisibilites.clear();
  this->BlockVisibilites[0] = false; // hide root block
  foreach (const unsigned int &index, indices)
    {
    this->BlockVisibilites[index] = true;
    }
  this->updateBlockVisibilities();
  this->Representation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::updateBlockVisibilities()
{
  std::vector<int> vector;

  for(QMap<unsigned int, bool>::const_iterator i = this->BlockVisibilites.begin();
      i != this->BlockVisibilites.end(); i++)
    {
    vector.push_back(static_cast<int>(i.key()));
    vector.push_back(static_cast<int>(i.value()));
    }

  // update vtk property
  vtkSMProxy *proxy = this->Representation->getProxy();
  vtkSMProperty *property_ = proxy->GetProperty("BlockVisibility");

  if(property_)
    {
    BEGIN_UNDO_SET("Change Block Visibilities");
    vtkSMIntVectorProperty *ivp =
      vtkSMIntVectorProperty::SafeDownCast(property_);
    if(!vector.empty())
      {
      // if property changes, ModifiedEvent will be fired and
      // this->UpdateUITimer will be started.
      ivp->SetElements(&vector[0], static_cast<unsigned int>(vector.size()));
      }
    proxy->UpdateVTKObjects();
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::updateBlockColors()
{
  // update vtk property
  vtkSMProxy *proxy = this->Representation->getProxy();
  vtkSMProperty *property_ = proxy->GetProperty("BlockColor");

  if(property_)
    {
    BEGIN_UNDO_SET("Change Block Colors");
    vtkSMDoubleMapProperty *dmp =
      vtkSMDoubleMapProperty::SafeDownCast(property_);

    // if property changes, ModifiedEvent will be fired and
    // this->UpdateUITimer will be started.
    dmp->ClearElements();

    QMap<unsigned int, QColor>::const_iterator iter;
    for(iter = this->BlockColors.begin();
        iter != this->BlockColors.end();
        iter++)
      {
      QColor qcolor = iter.value();
      double color[] = { qcolor.redF(), qcolor.greenF(), qcolor.blueF() };
      dmp->SetElements(iter.key(), color);
      }

    proxy->UpdateVTKObjects();
    END_UNDO_SET();
    }

  this->updateTreeWidgetBlockVisibilities();
}

void pqMultiBlockInspectorPanel::updateBlockOpacities()
{
  // update vtk property
  vtkSMProxy *proxy = this->Representation->getProxy();
  vtkSMProperty *property_ = proxy->GetProperty("BlockOpacity");

  if(property_)
    {
    BEGIN_UNDO_SET("Change Block Opacities");
    vtkSMDoubleMapProperty *dmp =
      vtkSMDoubleMapProperty::SafeDownCast(property_);

    // if property changes, ModifiedEvent will be fired and
    // this->UpdateUITimer will be started.
    dmp->ClearElements();

    QMap<unsigned int, double>::const_iterator iter;
    for(iter = this->BlockOpacities.begin();
        iter != this->BlockOpacities.end();
        iter++)
      {
      dmp->SetElement(iter.key(), iter.value());
      }

    proxy->UpdateVTKObjects();
    END_UNDO_SET();
    }

  this->updateTreeWidgetBlockVisibilities();
}

void pqMultiBlockInspectorPanel::updateTreeWidgetBlockVisibilities()
{
  if(!this->Representation)
    {
    return;
    }

  // update BlockVisibility map from vtk property
  vtkSMProxy *proxy = this->Representation->getProxy();
  vtkSMProperty *blockVisibilityProperty = proxy->GetProperty("BlockVisibility");
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(blockVisibilityProperty);
  this->BlockVisibilites.clear();

  if(ivp)
    {
    vtkIdType nbElems = static_cast<vtkIdType>(ivp->GetNumberOfElements());
    for(vtkIdType i = 0; i + 1 < nbElems; i += 2)
      {
      this->BlockVisibilites[ivp->GetElement(i)] = ivp->GetElement(i+1);
      }
    }

  // update BlockColor map from vtk property
  vtkSMProperty *blockColorProperty = proxy->GetProperty("BlockColor");
  vtkSMDoubleMapProperty *dmp = vtkSMDoubleMapProperty::SafeDownCast(blockColorProperty);
  this->BlockColors.clear();

  if(dmp)
    {
    vtkSMDoubleMapPropertyIterator *iter = dmp->NewIterator();
    for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
      QColor color;
      color.setRedF(iter->GetElementComponent(0));
      color.setGreenF(iter->GetElementComponent(1));
      color.setBlueF(iter->GetElementComponent(2));
      this->BlockColors[iter->GetKey()] = color;
      }
    iter->Delete();
    }

  // update BlockOpacity map from vtk property
  vtkSMProperty *blockOpacityProperty = proxy->GetProperty("BlockOpacity");
  dmp = vtkSMDoubleMapProperty::SafeDownCast(blockOpacityProperty);
  this->BlockOpacities.clear();

  if(dmp)
    {
    vtkSMDoubleMapPropertyIterator *iter = dmp->NewIterator();
    for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
      this->BlockOpacities[iter->GetKey()] = iter->GetElementComponent(0);
      }
    iter->Delete();
    }

  // update ui
  pqPipelineSource *source = this->OutputPort->getSource();
  vtkPVDataInformation *info = this->OutputPort->getDataInformation();

  if(!source || !info)
    {
    return;
    }

  vtkPVCompositeDataInformation *compositeInfo =
    info->GetCompositeDataInformation();

  if(compositeInfo->GetDataIsComposite())
    {
    this->TreeWidget->blockSignals(true);
    unsigned int flat_index = 0;
    bool root_visibility = this->BlockVisibilites.value(0, true);

    // get root item and set its visibility
    QTreeWidgetItem *rootItem =
      this->TreeWidget->invisibleRootItem()->child(0);
    rootItem->setData(0,
                      Qt::CheckStateRole,
                      root_visibility ? Qt::Checked : Qt::Unchecked);
    rootItem->setData(0, Qt::DecorationRole, makeBlockIcon(0));

    // recurse down the tree updating child visibilities
    this->updateTreeWidgetBlockVisibilities(compositeInfo,
                                            rootItem,
                                            flat_index,
                                            root_visibility);
    this->TreeWidget->blockSignals(false);
    }
}

void pqMultiBlockInspectorPanel::updateTreeWidgetBlockVisibilities(
  vtkPVCompositeDataInformation *info, QTreeWidgetItem *parent_,
  unsigned int &flatIndex, bool parentVisibility)
{
  for(unsigned int i = 0; i < info->GetNumberOfChildren(); i++)
    {
    QTreeWidgetItem *item = parent_->child(i);
    flatIndex++;

    bool visibility = parentVisibility;
    if(this->BlockVisibilites.contains(flatIndex))
      {
      visibility = this->BlockVisibilites[flatIndex];
      }

    item->setData(0, Qt::CheckStateRole, visibility ? Qt::Checked : Qt::Unchecked);
    item->setData(0, Qt::DecorationRole, makeBlockIcon(flatIndex));

    vtkPVDataInformation *childInfo = info->GetDataInformation(i);
    if(childInfo)
      {
      vtkPVCompositeDataInformation *compositeChildInfo =
        childInfo->GetCompositeDataInformation();

      // recurse down through child blocks only if the child block
      // is composite and is not a multi-piece data set
      if(compositeChildInfo->GetDataIsComposite() &&
         !compositeChildInfo->GetDataIsMultiPiece())
        {
        this->updateTreeWidgetBlockVisibilities(compositeChildInfo,
                                                item,
                                                flatIndex,
                                                visibility);
        }
      }
    }
}

void pqMultiBlockInspectorPanel::treeWidgetCustomContextMenuRequested(const QPoint &)
{
  // selected items
  QList<QTreeWidgetItem*> items = this->TreeWidget->selectedItems();
  if(items.isEmpty())
    {
    return;
    }

  int hiddenItemCount = 0;
  foreach(const QTreeWidgetItem *item, items)
    {
    if(item->data(0, Qt::CheckStateRole).toBool() == false)
      {
      hiddenItemCount++;
      }
    }
  int visibleItemCount = items.size() - hiddenItemCount;

  QMenu menu;

  QAction *hideAction = 0;
  if(visibleItemCount > 0)
    {
    QString label;
    if(visibleItemCount > 1)
      {
      label = QString("Hide %1 Blocks").arg(visibleItemCount);
      }
    else
      {
      label = "Hide Block";
      }
    hideAction = menu.addAction(label);
    }
  QAction *showAction = 0;
  if(hiddenItemCount > 0)
    {
    QString label;
    if(hiddenItemCount > 1)
      {
      label = QString("Show %1 Blocks").arg(hiddenItemCount);
      }
    else
      {
      label = "Show Block";
      }
    showAction = menu.addAction(label);
    }
  QAction *unsetVisibilityAction = menu.addAction("Unset Visibility");
  menu.addSeparator();
  QAction *setColorAction = menu.addAction("Set Color...");
  QAction *unsetColorAction = menu.addAction("Unset Color");
  menu.addSeparator();
  QAction *setOpacityAction = menu.addAction("Set Opacity...");
  QAction *unsetOpacityAction = menu.addAction("Unset Opacity");

  // show menu
  QAction *action = menu.exec(QCursor::pos());

  if(!action)
    {
    return;
    }
  else if(action == hideAction)
    {
    foreach(QTreeWidgetItem *item, items)
      {
      unsigned int flat_index =
        item->data(0, Qt::UserRole).value<unsigned int>();

      // first unset any child item visibilites
      this->unsetChildVisibilities(item);

      this->setBlockVisibility(flat_index, false);
      item->setData(
        0, Qt::CheckStateRole, Qt::Unchecked);
      }
    }
  else if(action == showAction)
    {
    foreach(QTreeWidgetItem *item, items)
      {
      unsigned int flat_index =
        item->data(0, Qt::UserRole).value<unsigned int>();

      // first unset any child item visibilites
      this->unsetChildVisibilities(item);

      this->setBlockVisibility(flat_index, true);
      item->setData(
        0, Qt::CheckStateRole, Qt::Checked);
      }
    }
  else if(action == unsetVisibilityAction)
    {
    foreach(QTreeWidgetItem *item, items)
      {
      unsigned int flat_index =
        item->data(0, Qt::UserRole).value<unsigned int>();

      this->clearBlockVisibility(flat_index);
      item->setData(
        0, Qt::CheckStateRole, item->parent()->data(0, Qt::CheckStateRole));
      }
    }
  else if(action == setColorAction)
    {
    QColor color = QColorDialog::getColor(QColor(), this, "Select Color",
      QColorDialog::DontUseNativeDialog);
    if(color.isValid())
      {
      foreach(QTreeWidgetItem *item, items)
        {
        unsigned int flat_index =
          item->data(0, Qt::UserRole).value<unsigned int>();
        this->setBlockColor(flat_index, color);
        }
      }
    }
  else if(action == unsetColorAction)
    {
    foreach(QTreeWidgetItem *item, items)
      {
      unsigned int flat_index =
        item->data(0, Qt::UserRole).value<unsigned int>();
      this->clearBlockColor(flat_index);
      }
    }
  else if(action == setOpacityAction)
    {
    QList<unsigned int> indices;
    foreach(QTreeWidgetItem *item, items)
      {
      indices.append(item->data(0, Qt::UserRole).value<unsigned int>());
      }

    this->promptAndSetBlockOpacity(indices);
    }
  else if(action == unsetOpacityAction)
    {
    foreach(QTreeWidgetItem *item, items)
      {
      unsigned int flat_index =
        item->data(0, Qt::UserRole).value<unsigned int>();

      this->clearBlockOpacity(flat_index);
      }
    }
}

void pqMultiBlockInspectorPanel::blockItemChanged(QTreeWidgetItem *item, int column)
{
  Q_UNUSED(column);

  unsigned int flat_index = item->data(0, Qt::UserRole).value<unsigned int>();
  bool visible = item->data(0, Qt::CheckStateRole).toBool();

  // first unset any child item visibilites
  this->unsetChildVisibilities(item);

  // set block visibility
  this->setBlockVisibility(flat_index, visible);
}

void pqMultiBlockInspectorPanel::unsetChildVisibilities(QTreeWidgetItem *parent_)
{
  for(int i = 0; i < parent_->childCount(); i++)
    {
    QTreeWidgetItem *child = parent_->child(i);
    unsigned int flatIndex = child->data(0, Qt::UserRole).value<unsigned int>();
    this->BlockVisibilites.remove(flatIndex);
    unsetChildVisibilities(child);
    }
}

void pqMultiBlockInspectorPanel::currentSelectionChanged(pqOutputPort *port)
{
  // find selected block ids
  std::vector<vtkIdType> block_ids;

  if(port)
    {
    vtkSMSourceProxy *activeSelection = port->getSelectionInput();
    if(activeSelection &&
       strcmp(activeSelection->GetXMLName(), "BlockSelectionSource") == 0)
      {
      vtkSMPropertyHelper blocksProp(activeSelection, "Blocks");
      block_ids.resize(blocksProp.GetNumberOfElements());
      if (block_ids.size() > 0)
        {
        blocksProp.Get(&block_ids[0], blocksProp.GetNumberOfElements());
        }
      }
    }

  // sort block ids so we can use binary_search
  std::sort(block_ids.begin(), block_ids.end());

  // update visibilities in the tree widget
  this->TreeWidget->blockSignals(true);

  foreach(QTreeWidgetItem *item,
          this->TreeWidget->findItems("", Qt::MatchContains | Qt::MatchRecursive))
    {
    vtkIdType flatIndex =
      item->data(0, Qt::UserRole).value<vtkIdType>();

    item->setSelected(
      std::binary_search(block_ids.begin(), block_ids.end(), flatIndex));
    }

  this->TreeWidget->blockSignals(false);
}

void pqMultiBlockInspectorPanel::currentTreeItemSelectionChanged()
{
  // create vector of selected block ids
  std::vector<vtkIdType> blockIds;
  foreach(const QTreeWidgetItem *item, this->TreeWidget->selectedItems())
    {
    unsigned int flatIndex =
      item->data(0, Qt::UserRole).value<unsigned int>();

    blockIds.push_back(flatIndex);
    }

  // create block selection source proxy
  vtkSMSessionProxyManager *proxyManager =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

  vtkSMProxy* selectionSource =
    proxyManager->NewProxy("sources", "BlockSelectionSource");

  // set selected blocks
  if (blockIds.size() > 0)
    {
    vtkSMPropertyHelper(selectionSource, "Blocks")
      .Set(&blockIds[0], static_cast<unsigned int>(blockIds.size()));
    }
  else
    {
    vtkSMPropertyHelper(selectionSource, "Blocks").SetNumberOfElements(0);
    }
  selectionSource->UpdateVTKObjects();

  vtkSMSourceProxy *selectionSourceProxy =
    vtkSMSourceProxy::SafeDownCast(selectionSource);

  // set the selection
  if(this->OutputPort)
    {
    this->OutputPort->setSelectionInput(selectionSourceProxy, 0);
    }

  // update the selection manager
  pqSelectionManager *selectionManager =
    qobject_cast<pqSelectionManager*>(
      pqApplicationCore::instance()->manager("SelectionManager"));
  if(selectionManager)
    {
    selectionManager->select(this->OutputPort);
    }

  // delete the selection source
  selectionSourceProxy->Delete();

  // update the views
  if(this->OutputPort)
    {
    this->OutputPort->renderAllViews();
    }
}

QString pqMultiBlockInspectorPanel::lookupBlockName(unsigned int flatIndex) const
{
  foreach(QTreeWidgetItem *item,
          this->TreeWidget->findItems("", Qt::MatchContains | Qt::MatchRecursive))
    {
    unsigned int itemFlatIndex =
      item->data(0, Qt::UserRole).value<unsigned int>();

    if(itemFlatIndex == flatIndex)
      {
      return item->text(0);
      }
    }

  return QString();
}

QIcon pqMultiBlockInspectorPanel::makeBlockIcon(unsigned int flatIndex) const
{
  BlockIcon options;
  options.HasColor = this->BlockColors.contains(flatIndex);
  options.HasOpacity = this->BlockOpacities.contains(flatIndex);
  options.Color = options.HasColor ? this->BlockColors[flatIndex] : QColor();
  options.Opacity = options.HasOpacity ? this->BlockOpacities[flatIndex] : 1.0;

  if(this->BlockIconCache.contains(options))
    {
    return this->BlockIconCache[options];
    }

  QPixmap pixmap(32, 16);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);

  // draw color circle
  if(this->BlockColors.contains(flatIndex))
    {
    QColor color = this->BlockColors[flatIndex];
    painter.setPen(Qt::black);
    painter.setBrush(color);
    painter.drawEllipse(0, 0, 14, 14);
    }
  else
    {
    static QIcon inheritedColorIcon(
      ":/pqWidgets/Icons/pqBlockInheritColor16.png"
    );

    painter.drawPixmap(0, 0, inheritedColorIcon.pixmap(16, 16));
    }

  // draw opacity circle
  if(this->BlockOpacities.contains(flatIndex))
    {
    double opacity = this->BlockOpacities[flatIndex];
    int angle = 5760 * opacity;
    painter.setBrush(Qt::lightGray);
    painter.drawPie(16, 0, 14, 14, 0, angle);
    painter.setBrush(Qt::transparent);
    painter.drawEllipse(16, 0, 14, 14);
    }
  else
    {
    static QIcon inheritedOpacityIcon(
      ":/pqWidgets/Icons/pqBlockInheritOpacity16.png"
    );

    painter.drawPixmap(16, 0, inheritedOpacityIcon.pixmap(16, 16));
    }

  painter.end();

  QIcon icon(pixmap);

  // store icon in the cache
  this->BlockIconCache[options] = icon;

  return icon;
}
