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
#include "vtkSMProxy.h"
#include "vtkPVDataInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkSMProperty.h"
#include "vtkSMIntVectorProperty.h"

#include <QMenu>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>

pqMultiBlockInspectorPanel::pqMultiBlockInspectorPanel(QWidget *parent)
  : QWidget(parent)
{
  // setup tree widget
  this->TreeWidget = new QTreeWidget(this);
  this->TreeWidget->setColumnCount(1);
  this->TreeWidget->header()->close();

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(this->TreeWidget);
  setLayout(layout);

  // listen to active object changes
  pqActiveObjects *activeObjects = &pqActiveObjects::instance();
  this->connect(activeObjects, SIGNAL(portChanged(pqOutputPort*)),
                this, SLOT(setOutputPort(pqOutputPort*)));
  this->connect(activeObjects, SIGNAL(representationChanged(pqRepresentation*)),
                this, SLOT(setRepresentation(pqRepresentation*)));

  // connect to right-click signals in the tree widget
  this->TreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  this->connect(this->TreeWidget, SIGNAL(customContextMenuRequested(QPoint)),
                this, SLOT(treeWidgetCustomContextMenuRequested(QPoint)));
  this->connect(this->TreeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
                this, SLOT(blockItemChanged(QTreeWidgetItem*, int)));
}

pqMultiBlockInspectorPanel::~pqMultiBlockInspectorPanel()
{
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

  this->Representation = representation;
}

void pqMultiBlockInspectorPanel::buildTree(vtkPVCompositeDataInformation *info,
                                           QTreeWidgetItem *parent,
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

    QTreeWidgetItem *item = new QTreeWidgetItem(parent, QStringList() << text);
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
  this->TreeWidget->clear();

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
    unsigned int flat_index = 1;
    this->buildTree(compositeInfo,
                    this->TreeWidget->invisibleRootItem(),
                    flat_index);
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
  vtkSMProperty *property = proxy->GetProperty("BlockVisibility");

  if(property)
    {
    vtkSMIntVectorProperty *ivp =
      vtkSMIntVectorProperty::SafeDownCast(property);

    ivp->SetNumberOfElements(vector.size());
    ivp->SetElements(&vector[0]);

    proxy->UpdateVTKObjects();
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
    this->updateBlockVisibilities(compositeInfo,
                                  this->TreeWidget->invisibleRootItem(),
                                  flat_index,
                                  true);
    this->TreeWidget->blockSignals(false);
    }

}

void pqMultiBlockInspectorPanel::updateBlockVisibilities(
  vtkPVCompositeDataInformation *info, QTreeWidgetItem *parent,
  unsigned int &flatIndex, bool parentVisibility)
{
  for(unsigned int i = 0; i < info->GetNumberOfChildren(); i++)
    {
    QTreeWidgetItem *item = parent->child(i);
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
        this->updateBlockVisibilities(compositeChildInfo,
                                      item,
                                      flatIndex,
                                      visibility);
        }
      }
    }
}

void pqMultiBlockInspectorPanel::treeWidgetCustomContextMenuRequested(const QPoint &)
{
  QTreeWidgetItem *item = this->TreeWidget->currentItem();
  if(!item)
    {
    return;
    }

  bool visible = item->data(0, Qt::CheckStateRole).toBool();

  QMenu menu;
  menu.addAction(visible ? "Hide" : "Show");
  menu.addAction("Unset Visibility");
  connect(&menu, SIGNAL(triggered(QAction*)),
          this, SLOT(toggleBlockVisibility(QAction*)));
  menu.exec(QCursor::pos());
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

void pqMultiBlockInspectorPanel::unsetChildVisibilities(QTreeWidgetItem *parent)
{
  for(int i = 0; i < parent->childCount(); i++)
    {
    QTreeWidgetItem *child = parent->child(i);
    unsigned int flatIndex = child->data(0, Qt::UserRole).value<unsigned int>();
    this->BlockVisibilites.remove(flatIndex);
    unsetChildVisibilities(child);
    }
}
