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
#include "pqOutputPort.h"
#include "pqUndoStack.h"
#include "vtkSMProxy.h"
#include "vtkPVDataInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkSMProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkEventQtSlotConnect.h"
#include "pqSelectionManager.h"
#include "vtkSelection.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"

#include <QMenu>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>

pqMultiBlockInspectorPanel::pqMultiBlockInspectorPanel(QWidget *parent_)
  : QWidget(parent_)
{
  // setup tree widget
  this->TreeWidget = new QTreeWidget(this);
  this->TreeWidget->setColumnCount(1);
  this->TreeWidget->header()->close();

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
    vtkSMProxy *proxy = this->Representation->getProxy();
    vtkSMProperty *property_ = proxy->GetProperty("BlockVisibility");

    if(property_)
      {
      this->VisibilityPropertyListener->Connect(property_,
                                                vtkCommand::ModifiedEvent,
                                                this,
                                                SLOT(updateTreeWidgetBlockVisibilities()));
      }
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
    item->setData(0, Qt::UserRole, flatIndex++);
    item->setData(0, Qt::CheckStateRole, Qt::Checked);

    if(childInfo)
      {
      vtkPVCompositeDataInformation *compositeChildInfo =
        childInfo->GetCompositeDataInformation();

      if(compositeChildInfo->GetDataIsComposite())
        {
        this->buildTree(compositeChildInfo, item, flatIndex);
        }
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

void pqMultiBlockInspectorPanel::setBlockVisibility(unsigned int index, bool visible)
{
  this->BlockVisibilites[index] = visible;
  this->updateBlockVisibilities();
  this->Representation->renderViewEventually();
}

void pqMultiBlockInspectorPanel::clearBlockVisibility(unsigned int index)
{
  this->BlockVisibilites.remove(index);
  this->updateBlockVisibilities();
  this->Representation->renderViewEventually();
}

void pqMultiBlockInspectorPanel::showOnlyBlock(unsigned int index)
{
  this->BlockVisibilites.clear();
  this->BlockVisibilites[0] = false; // hide root block
  this->BlockVisibilites[index] = true;
  this->updateBlockVisibilities();
  this->Representation->renderViewEventually();
}

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
    BEGIN_UNDO_SET("Change Block Properties");
    vtkSMIntVectorProperty *ivp =
      vtkSMIntVectorProperty::SafeDownCast(property_);

    ivp->SetNumberOfElements(static_cast<unsigned int>(vector.size()));
    ivp->SetElements(&vector[0]);

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
  vtkSMProperty *property_ = proxy->GetProperty("BlockVisibility");
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(property_);
  this->BlockVisibilites.clear();

  if(ivp)
    {
    for(vtkIdType i = 0; i + 1 < ivp->GetNumberOfElements(); i += 2)
      {
      this->BlockVisibilites[ivp->GetElement(i)] = ivp->GetElement(i+1);
      }
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

    vtkPVDataInformation *childInfo = info->GetDataInformation(i);
    if(childInfo)
      {
      vtkPVCompositeDataInformation *compositeChildInfo =
        childInfo->GetCompositeDataInformation();

      if(compositeChildInfo->GetDataIsComposite())
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
  //QTreeWidgetItem *item = this->TreeWidget->currentItem();
  //if(!item)
  //  {
  //  return;
  //  }

  //bool visible = item->data(0, Qt::CheckStateRole).toBool();

  //QMenu menu;
  //menu.addAction(visible ? "Hide" : "Show");
  //menu.addAction("Unset Visibility");
  //connect(&menu, SIGNAL(triggered(QAction*)),
  //        this, SLOT(toggleBlockVisibility(QAction*)));
  //menu.exec(QCursor::pos());
}

void pqMultiBlockInspectorPanel::toggleBlockVisibility(QAction *action)
{
  QTreeWidgetItem *item = this->TreeWidget->currentItem();
  if(!item)
    {
    return;
    }

  unsigned int flat_index = item->data(0, Qt::UserRole).value<unsigned int>();

  QString text = action->text();
  if(text == "Hide" || text == "Show")
    {
    bool visible = item->data(0, Qt::CheckStateRole).toBool();

    // first unset any child item visibilites
    this->unsetChildVisibilities(item);

    this->setBlockVisibility(flat_index, !visible);
    item->setData(0, Qt::CheckStateRole, visible ? Qt::Unchecked : Qt::Checked);
    }
  else if(text == "Unset Visibility")
    {
    this->clearBlockVisibility(flat_index);
    item->setData(0, Qt::CheckStateRole, item->parent()->data(0, Qt::CheckStateRole));
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
      blocksProp.Get(&block_ids[0], blocksProp.GetNumberOfElements());
      }
    }

  // sort block ids so we can use binary_search
  std::sort(block_ids.begin(), block_ids.end());

  // update visibilities in the tree widget
  this->TreeWidget->blockSignals(true);

  foreach(QTreeWidgetItem *item,
          this->TreeWidget->findItems("", Qt::MatchContains | Qt::MatchRecursive))
    {
    unsigned int flatIndex =
      item->data(0, Qt::UserRole).value<unsigned int>();

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
  vtkSMPropertyHelper(selectionSource, "Blocks").Set(&blockIds[0], blockIds.size());
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
