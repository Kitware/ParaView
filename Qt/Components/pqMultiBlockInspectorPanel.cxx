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

#if !defined(VTK_LEGACY_REMOVE)
#include "pqActiveObjects.h"
#include "pqDoubleRangeDialog.h"
#include "pqOutputPort.h"
#include "pqSelectionManager.h"
#include "pqTreeWidgetSelectionHelper.h"
#include "pqUndoStack.h"

#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPiecewiseFunction.h"
#include "vtkSMDoubleMapProperty.h"
#include "vtkSMDoubleMapPropertyIterator.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTransferFunctionManager.h"
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

#define FLAT_INDEX_ROLE Qt::UserRole
#define LEAF_INDEX_ROLE (Qt::UserRole + 1)

namespace
{
enum ValueType
{
  SYSTEM_VALUE,
  USER_VALUE
};

enum Column
{
  NAME_COLUMN,
  COLOR_COLUMN,
  OPACITY_COLUMN,
  COLUMN_COUNT
};

const int ICON_SIZE = 16;
const int BETWEEN_SIZE = 3;
void drawColorIcon(QPainter& painter, QColor& color, ValueType valueType = SYSTEM_VALUE)
{
  painter.setPen(QPen(Qt::NoPen));
  if (valueType == USER_VALUE)
  {
    painter.setBrush(Qt::black);
    painter.drawRect(0, 0, ICON_SIZE, ICON_SIZE);
  }
  painter.setBrush(color);
  painter.drawEllipse(
    BETWEEN_SIZE, BETWEEN_SIZE, ICON_SIZE - 2 * BETWEEN_SIZE, ICON_SIZE - 2 * BETWEEN_SIZE);
}

void drawNullIcon(QPainter& painter)
{
  painter.setPen(Qt::black);
  painter.setBrush(Qt::white);
  painter.drawEllipse(
    BETWEEN_SIZE, BETWEEN_SIZE, ICON_SIZE - 2 * BETWEEN_SIZE, ICON_SIZE - 2 * BETWEEN_SIZE);
  painter.drawLine(
    ICON_SIZE - BETWEEN_SIZE - 1, BETWEEN_SIZE, BETWEEN_SIZE, ICON_SIZE - BETWEEN_SIZE - 1);
}

void drawOpacityIcon(QPainter& painter, double opacity, ValueType valueType = SYSTEM_VALUE)
{
  if (valueType == USER_VALUE)
  {
    painter.setPen(Qt::black);
    painter.setBrush(Qt::black);
    painter.drawRect(0, 0, ICON_SIZE, ICON_SIZE);
  }
  painter.setPen(Qt::black);
  int angle = 5760 * opacity;
  painter.setBrush(Qt::white);
  painter.drawEllipse(
    BETWEEN_SIZE, BETWEEN_SIZE, ICON_SIZE - 2 * BETWEEN_SIZE, ICON_SIZE - 2 * BETWEEN_SIZE);
  painter.setBrush(Qt::lightGray);
  painter.drawPie(BETWEEN_SIZE, BETWEEN_SIZE, ICON_SIZE - 2 * BETWEEN_SIZE,
    ICON_SIZE - 2 * BETWEEN_SIZE, 0, angle);
}
};

pqMultiBlockInspectorPanel::pqMultiBlockInspectorPanel(QWidget* parent_)
  : QWidget(parent_)
  , ColorTransferProxy(NULL)
  , ColorTransferFunction(NULL)
  , OpacityTransferFunction(NULL)
  , BlockColorsDistinctValues(7)
  , CompositeWrap(0)
{
  VTK_LEGACY_REPLACED_BODY(pqMultiBlockInspectorPanel, "ParaView 5.4", pqMultiBlockInspectorWidget);

  // setup tree widget
  this->TreeWidget = new QTreeWidget(this);
  this->TreeWidget->setColumnCount(COLUMN_COUNT);
  this->TreeWidget->setExpandsOnDoubleClick(false);
  this->TreeWidget->header()->close();

  // create tree widget selection helper
  new pqTreeWidgetSelectionHelper(this->TreeWidget);

  // disconnect from selection helper's context menu so that
  // we can use our own menu with multi-block support
  this->TreeWidget->disconnect(SIGNAL(customContextMenuRequested(const QPoint&)), 0, 0);

  this->connect(
    this->TreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));

  this->PropertyListener = vtkEventQtSlotConnect::New();

  QVBoxLayout* layout_ = new QVBoxLayout;
  layout_->addWidget(this->TreeWidget);
  setLayout(layout_);

  // listen to active object changes
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  this->connect(
    activeObjects, SIGNAL(portChanged(pqOutputPort*)), this, SLOT(onPortChanged(pqOutputPort*)));
  this->connect(activeObjects, SIGNAL(representationChanged(pqRepresentation*)), this,
    SLOT(onRepresentationChanged(pqRepresentation*)));

  // listen to selection changes
  pqSelectionManager* selectionManager =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SelectionManager"));
  if (selectionManager)
  {
    this->connect(selectionManager, SIGNAL(selectionChanged(pqOutputPort*)), this,
      SLOT(onSelectionChanged(pqOutputPort*)));
  }

  // connect to right-click signals in the tree widget
  this->TreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  this->connect(this->TreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this,
    SLOT(onCustomContextMenuRequested(QPoint)));
  this->connect(this->TreeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this,
    SLOT(onItemChanged(QTreeWidgetItem*, int)));

  // setup double-click for colors and opacity
  this->connect(this->TreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this,
    SLOT(onItemDoubleClicked(QTreeWidgetItem*, int)));

  //  UpdateUITimer helps use collapse updates to the UI whenever the SM
  //  properties change.
  this->UpdateUITimer.setSingleShot(true);
  this->UpdateUITimer.setInterval(0);
  this->connect(&this->UpdateUITimer, SIGNAL(timeout()), this, SLOT(updateTree()));
}

pqMultiBlockInspectorPanel::~pqMultiBlockInspectorPanel()
{
  this->PropertyListener->Delete();
}

void pqMultiBlockInspectorPanel::onPortChanged(pqOutputPort* port)
{
  if (this->OutputPort == port)
  {
    return;
  }

  if (this->OutputPort)
  {
    QObject::disconnect(this->OutputPort->getSource(), SIGNAL(dataUpdated(pqPipelineSource*)), this,
      SLOT(onDataUpdated()));
  }

  this->OutputPort = port;

  if (this->OutputPort)
  {
    QObject::connect(this->OutputPort->getSource(), SIGNAL(dataUpdated(pqPipelineSource*)), this,
      SLOT(onDataUpdated()));
  }

  this->onDataUpdated();
}

pqOutputPort* pqMultiBlockInspectorPanel::getOutputPort() const
{
  return this->OutputPort.data();
}

void pqMultiBlockInspectorPanel::onRepresentationChanged(pqRepresentation* representation)
{
  if (this->Representation == representation)
  {
    return;
  }

  // disconnect from previous representation
  this->PropertyListener->Disconnect();

  this->Representation = representation;

  if (this->Representation)
  {
    // update properties
    this->onDataUpdated();

    // listen to property changes
    vtkSMProxy* proxy = this->Representation->getProxy();
    vtkSMProperty* visibilityProperty = proxy->GetProperty("BlockVisibility");
    if (visibilityProperty)
    {
      this->PropertyListener->Connect(
        visibilityProperty, vtkCommand::ModifiedEvent, &this->UpdateUITimer, SLOT(start()));
    }

    vtkSMProperty* colorProperty = proxy->GetProperty("BlockColor");
    if (colorProperty)
    {
      this->PropertyListener->Connect(
        colorProperty, vtkCommand::ModifiedEvent, &this->UpdateUITimer, SLOT(start()));
    }
    vtkSMProperty* opacityProperty = proxy->GetProperty("BlockOpacity");
    if (opacityProperty)
    {
      this->PropertyListener->Connect(
        opacityProperty, vtkCommand::ModifiedEvent, &this->UpdateUITimer, SLOT(start()));
    }
    vtkSMProperty* colorArrayNameProperty = proxy->GetProperty("ColorArrayName");
    if (colorArrayNameProperty)
    {
      this->PropertyListener->Connect(
        colorArrayNameProperty, vtkCommand::ModifiedEvent, this, SLOT(onColorArrayNameModified()));
    }
    vtkSMProperty* compositeWrapDistinctValuesProperty =
      proxy->GetProperty("BlockColorsDistinctValues");
    if (compositeWrapDistinctValuesProperty)
    {
      this->PropertyListener->Connect(compositeWrapDistinctValuesProperty,
        vtkCommand::ModifiedEvent, &this->UpdateUITimer, SLOT(start()));
    }
    onColorArrayNameModified();
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

void pqMultiBlockInspectorPanel::buildTree(
  vtkPVCompositeDataInformation* info, QTreeWidgetItem* parent_, int& flatIndex, int& leafIndex)
{
  parent_->setTextAlignment(COLOR_COLUMN, Qt::AlignHCenter | Qt::AlignVCenter);
  parent_->setTextAlignment(OPACITY_COLUMN, Qt::AlignHCenter | Qt::AlignVCenter);
  for (unsigned int i = 0; i < info->GetNumberOfChildren(); i++)
  {
    vtkPVDataInformation* childInfo = info->GetDataInformation(i);
    const char* childName = info->GetName(i);

    QString text;
    if (childName && childName[0])
    {
      text = childName;
    }
    else
    {
      text = QString("Block %2").arg(leafIndex);
    }

    QTreeWidgetItem* item = new QTreeWidgetItem(parent_, QStringList() << text);
    item->setData(NAME_COLUMN, FLAT_INDEX_ROLE, flatIndex);
    item->setData(NAME_COLUMN, Qt::CheckStateRole, Qt::Checked);

    flatIndex++;

    if (childInfo)
    {
      vtkPVCompositeDataInformation* compositeChildInfo = childInfo->GetCompositeDataInformation();

      // recurse down through child blocks only if the child block
      // is composite and is not a multi-piece data set
      if (compositeChildInfo->GetDataIsComposite())
      {
        if (compositeChildInfo->GetDataIsMultiPiece())
        {
          flatIndex += compositeChildInfo->GetNumberOfChildren();
        }
        else
        {
          this->buildTree(compositeChildInfo, item, flatIndex, leafIndex);
        }
      }
      if (!compositeChildInfo->GetDataIsComposite() || compositeChildInfo->GetDataIsMultiPiece())
      {
        item->setData(NAME_COLUMN, LEAF_INDEX_ROLE, leafIndex);
        leafIndex++;
      }
    }
    else
    {
      item->setDisabled(true);
      leafIndex++;
    }
  }
}

void pqMultiBlockInspectorPanel::onDataUpdated()
{
  // clear previous information
  this->TreeWidget->blockSignals(true);
  this->TreeWidget->clear();
  this->TreeWidget->blockSignals(false);

  if (!this->OutputPort)
  {
    return;
  }

  // update information
  pqPipelineSource* source = this->OutputPort->getSource();
  vtkPVDataInformation* info = this->OutputPort->getDataInformation();

  if (!source || !info)
  {
    return;
  }

  vtkPVCompositeDataInformation* compositeInfo = info->GetCompositeDataInformation();

  if (compositeInfo->GetDataIsComposite())
  {
    this->TreeWidget->blockSignals(true);
    int flatIndex = 0;
    int leafIndex = 0;

    // create root item
    QString rootLabel = source->getSMName();
    QTreeWidgetItem* rootItem =
      new QTreeWidgetItem(this->TreeWidget->invisibleRootItem(), QStringList() << rootLabel);
    rootItem->setData(NAME_COLUMN, FLAT_INDEX_ROLE, flatIndex++);
    rootItem->setData(NAME_COLUMN, Qt::CheckStateRole, Qt::Checked);

    // build the rest of the tree
    this->buildTree(compositeInfo, rootItem, flatIndex, leafIndex);
    this->TreeWidget->expandAll();
    this->TreeWidget->resizeColumnToContents(NAME_COLUMN);

    // expand root item
    this->TreeWidget->expandItem(rootItem);

    // update visibilities
    this->updateTree();
    this->TreeWidget->resizeColumnToContents(COLOR_COLUMN);
    this->TreeWidget->resizeColumnToContents(OPACITY_COLUMN);

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
  foreach (const unsigned int& index, indices)
  {
    this->BlockVisibilites[index] = visible;
  }

  this->updateBlockVisibilities();
  this->Representation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::clearBlockVisibility(const QList<unsigned int>& indices)
{
  foreach (const unsigned int& index, indices)
  {
    this->BlockVisibilites.remove(index);
  }

  this->updateBlockVisibilities();
  this->Representation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::setBlockColor(unsigned int index, const QColor& color)
{
  QList<unsigned int> indices;
  indices.push_back(index);
  this->setBlockColor(indices, color);
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::setBlockColor(
  const QList<unsigned int>& indices, const QColor& color)
{
  foreach (const unsigned int& index, indices)
  {
    this->BlockColors[index] = color;
  }

  this->updateBlockColors();
  this->Representation->renderViewEventually();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::clearBlockColor(unsigned int index)
{
  QList<unsigned int> indices;
  indices.push_back(index);
  this->clearBlockColor(indices);
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorPanel::clearBlockColor(const QList<unsigned int>& indices)
{
  foreach (const unsigned int& index, indices)
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
void pqMultiBlockInspectorPanel::setBlockOpacity(const QList<unsigned int>& indices, double opacity)
{
  foreach (const unsigned int& index, indices)
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
void pqMultiBlockInspectorPanel::clearBlockOpacity(const QList<unsigned int>& indices)
{
  foreach (const unsigned int& index, indices)
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
void pqMultiBlockInspectorPanel::promptAndSetBlockOpacity(const QList<unsigned int>& indices)
{
  if (indices.isEmpty())
  {
    return;
  }

  double current_opacity = this->BlockOpacities.value(indices[0], 1.0);
  pqDoubleRangeDialog dialog("Opacity:", 0.0, 1.0, this);
  dialog.setValue(current_opacity);
  bool ok = dialog.exec();
  if (ok)
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
void pqMultiBlockInspectorPanel::showOnlyBlocks(const QList<unsigned int>& indices)
{
  this->BlockVisibilites.clear();
  this->BlockVisibilites[0] = false; // hide root block
  foreach (const unsigned int& index, indices)
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

  for (QMap<unsigned int, bool>::const_iterator i = this->BlockVisibilites.begin();
       i != this->BlockVisibilites.end(); i++)
  {
    vector.push_back(static_cast<int>(i.key()));
    vector.push_back(static_cast<int>(i.value()));
  }

  // update vtk property
  vtkSMProxy* proxy = this->Representation->getProxy();
  vtkSMProperty* property_ = proxy->GetProperty("BlockVisibility");

  if (property_)
  {
    BEGIN_UNDO_SET("Change Block Visibilities");
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(property_);
    if (!vector.empty())
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
  vtkSMProxy* proxy = this->Representation->getProxy();
  vtkSMProperty* property_ = proxy->GetProperty("BlockColor");

  if (property_)
  {
    BEGIN_UNDO_SET("Change Block Colors");
    vtkSMDoubleMapProperty* dmp = vtkSMDoubleMapProperty::SafeDownCast(property_);

    // if property changes, ModifiedEvent will be fired and
    // this->UpdateUITimer will be started.
    dmp->ClearElements();

    QMap<unsigned int, QColor>::const_iterator iter;
    for (iter = this->BlockColors.begin(); iter != this->BlockColors.end(); iter++)
    {
      QColor qcolor = iter.value();
      double color[] = { qcolor.redF(), qcolor.greenF(), qcolor.blueF() };
      dmp->SetElements(iter.key(), color);
    }

    proxy->UpdateVTKObjects();
    END_UNDO_SET();
  }

  this->updateTree();
}

void pqMultiBlockInspectorPanel::updateBlockOpacities()
{
  // update vtk property
  vtkSMProxy* proxy = this->Representation->getProxy();
  vtkSMProperty* property_ = proxy->GetProperty("BlockOpacity");

  if (property_)
  {
    BEGIN_UNDO_SET("Change Block Opacities");
    vtkSMDoubleMapProperty* dmp = vtkSMDoubleMapProperty::SafeDownCast(property_);

    // if property changes, ModifiedEvent will be fired and
    // this->UpdateUITimer will be started.
    dmp->ClearElements();

    QMap<unsigned int, double>::const_iterator iter;
    for (iter = this->BlockOpacities.begin(); iter != this->BlockOpacities.end(); iter++)
    {
      dmp->SetElement(iter.key(), iter.value());
    }

    proxy->UpdateVTKObjects();
    END_UNDO_SET();
  }

  this->updateTree();
}

void pqMultiBlockInspectorPanel::updateTree()
{
  if (!this->Representation || !this->OutputPort)
  {
    return;
  }

  // get the array name we color by
  vtkSMProxy* representationProxy = this->Representation->getProxy();
  vtkSMPropertyHelper colorArrayHelper(representationProxy, "ColorArrayName", true);
  const char* arrayName = colorArrayHelper.GetInputArrayNameToProcess();
  if (!arrayName)
  {
    return;
  }
  this->ColorTransferFunction = NULL;
  this->OpacityTransferFunction = NULL;
  // field name setup in vtkPVGeometryFilter to contain the current block index
  bool isComposite = !strcmp(arrayName, "vtkCompositeIndex");
  bool isCompositeWrap = !strcmp(arrayName, "vtkBlockColors");
  this->CompositeWrap = isCompositeWrap;
  if (isComposite || isCompositeWrap)
  {
    // get color/opacity transfer function
    vtkSMSessionProxyManager* activeSessionProxyManager =
      vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
    vtkNew<vtkSMTransferFunctionManager> transferFunctionManager;
    vtkSMProxy* colorTransferProxy =
      transferFunctionManager->GetColorTransferFunction(arrayName, activeSessionProxyManager);
    this->ColorTransferFunction = vtkDiscretizableColorTransferFunction::SafeDownCast(
      colorTransferProxy->GetClientSideObject());
    if (this->ColorTransferFunction->GetEnableOpacityMapping())
    {
      this->OpacityTransferFunction = this->ColorTransferFunction->GetScalarOpacityFunction();
    }
  }

  // update BlockVisibility map from vtk property
  vtkSMProperty* blockVisibilityProperty = representationProxy->GetProperty("BlockVisibility");
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(blockVisibilityProperty);
  this->BlockVisibilites.clear();

  if (ivp)
  {
    vtkIdType nbElems = static_cast<vtkIdType>(ivp->GetNumberOfElements());
    for (vtkIdType i = 0; i + 1 < nbElems; i += 2)
    {
      this->BlockVisibilites[ivp->GetElement(i)] = ivp->GetElement(i + 1);
    }
  }

  // update BlockColor map from vtk property
  vtkSMProperty* blockColorProperty = representationProxy->GetProperty("BlockColor");
  vtkSMDoubleMapProperty* dmp = vtkSMDoubleMapProperty::SafeDownCast(blockColorProperty);
  this->BlockColors.clear();

  // update BlockColorsDistinctValues
  vtkSMProperty* distinctValuesProperty =
    representationProxy->GetProperty("BlockColorsDistinctValues");
  ivp = vtkSMIntVectorProperty::SafeDownCast(distinctValuesProperty);
  this->BlockColorsDistinctValues = ivp ? ivp->GetElement(0) : 2;

  if (dmp)
  {
    vtkSMDoubleMapPropertyIterator* iter = dmp->NewIterator();
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
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
  vtkSMProperty* blockOpacityProperty = representationProxy->GetProperty("BlockOpacity");
  dmp = vtkSMDoubleMapProperty::SafeDownCast(blockOpacityProperty);
  this->BlockOpacities.clear();

  if (dmp)
  {
    vtkSMDoubleMapPropertyIterator* iter = dmp->NewIterator();
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
      this->BlockOpacities[iter->GetKey()] = iter->GetElementComponent(0);
    }
    iter->Delete();
  }

  // update ui
  pqPipelineSource* source = this->OutputPort->getSource();
  vtkPVDataInformation* info = this->OutputPort->getDataInformation();

  if (!source || !info)
  {
    return;
  }

  vtkPVCompositeDataInformation* compositeInfo = info->GetCompositeDataInformation();

  if (compositeInfo->GetDataIsComposite())
  {
    this->TreeWidget->blockSignals(true);
    bool root_visibility = this->BlockVisibilites.value(0, true);

    // get root item and set its visibility
    QTreeWidgetItem* rootItem = this->TreeWidget->invisibleRootItem()->child(0);
    rootItem->setData(
      NAME_COLUMN, Qt::CheckStateRole, root_visibility ? Qt::Checked : Qt::Unchecked);
    rootItem->setData(COLOR_COLUMN, Qt::DecorationRole, makeColorIcon(0, INTERNAL_NODE, -1));
    rootItem->setData(OPACITY_COLUMN, Qt::DecorationRole, makeOpacityIcon(0, INTERNAL_NODE, -1));

    // recurse down the tree updating child visibilities
    int flatIndex = 0;
    this->updateTree(compositeInfo, rootItem, flatIndex, root_visibility,
      this->BlockColors.contains(flatIndex) ? flatIndex : -1,
      this->BlockOpacities.contains(flatIndex) ? flatIndex : -1);
    this->TreeWidget->blockSignals(false);
  }
}

void pqMultiBlockInspectorPanel::onColorArrayNameModified()
{
  // get the array name we color by
  vtkSMProxy* representationProxy = this->Representation->getProxy();
  vtkSMPropertyHelper colorArrayHelper(representationProxy, "ColorArrayName", true);
  const char* arrayName = colorArrayHelper.GetInputArrayNameToProcess();
  if (!arrayName)
  {
    return;
  }

  // field name setup in vtkPVGeometryFilter to contain the current block index
  bool isComposite = !strcmp(arrayName, "vtkCompositeIndex");
  bool isCompositeWrap = !strcmp(arrayName, "vtkBlockColors");
  if (isComposite || isCompositeWrap)
  {
    // get color/opacity transfer function
    vtkSMSessionProxyManager* activeSessionProxyManager =
      vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
    vtkNew<vtkSMTransferFunctionManager> transferFunctionManager;
    vtkSMProxy* colorTransferProxy =
      transferFunctionManager->GetColorTransferFunction(arrayName, activeSessionProxyManager);
    if (this->ColorTransferProxy)
    {
      this->PropertyListener->Disconnect(colorTransferProxy);
    }
    this->PropertyListener->Connect(
      colorTransferProxy, vtkCommand::ModifiedEvent, &this->UpdateUITimer, SLOT(start()));
    this->ColorTransferProxy = colorTransferProxy;
  }
  updateTree();
}

void pqMultiBlockInspectorPanel::updateTree(vtkPVCompositeDataInformation* info,
  QTreeWidgetItem* parent_, int& flatIndex, bool parentVisibility, int inheritedColorIndex,
  int inheritedOpacityIndex)
{
  for (unsigned int i = 0; i < info->GetNumberOfChildren(); i++)
  {
    QTreeWidgetItem* item = parent_->child(i);
    flatIndex++;

    bool visibility = parentVisibility;
    if (this->BlockVisibilites.contains(flatIndex))
    {
      visibility = this->BlockVisibilites[flatIndex];
    }

    vtkPVDataInformation* childInfo = info->GetDataInformation(i);
    vtkPVCompositeDataInformation* compositeChildInfo = NULL;
    NodeType nodeType = LEAF_NODE;
    if (childInfo)
    {
      compositeChildInfo = childInfo->GetCompositeDataInformation();

      // recurse down through child blocks only if the child block
      // is composite and is not a multi-piece data set
      if (compositeChildInfo->GetDataIsComposite() && !compositeChildInfo->GetDataIsMultiPiece())
      {
        nodeType = INTERNAL_NODE;
      }
    }
    QVariant leafIndex = item->data(NAME_COLUMN, LEAF_INDEX_ROLE);
    item->setData(NAME_COLUMN, Qt::CheckStateRole, visibility ? Qt::Checked : Qt::Unchecked);
    item->setData(COLOR_COLUMN, Qt::DecorationRole,
      (item->isDisabled() ? makeNullIcon() : makeColorIcon(flatIndex, nodeType, inheritedColorIndex,
                                               leafIndex.isValid() ? leafIndex.toInt() : -1)));
    item->setData(OPACITY_COLUMN, Qt::DecorationRole,
      (item->isDisabled() ? makeNullIcon()
                          : makeOpacityIcon(flatIndex, nodeType, inheritedOpacityIndex)));

    if (nodeType == INTERNAL_NODE)
    {
      this->updateTree(compositeChildInfo, item, flatIndex, visibility,
        inheritedColorIndex != -1 ? inheritedColorIndex
                                  : (this->BlockColors.contains(flatIndex) ? flatIndex : -1),
        inheritedOpacityIndex != -1 ? inheritedOpacityIndex
                                    : (this->BlockOpacities.contains(flatIndex) ? flatIndex : -1));
    }
    if (compositeChildInfo && compositeChildInfo->GetDataIsMultiPiece())
    {
      flatIndex += compositeChildInfo->GetNumberOfChildren();
    }
  }
}

void pqMultiBlockInspectorPanel::onCustomContextMenuRequested(const QPoint&)
{
  // selected items
  QList<QTreeWidgetItem*> items = this->TreeWidget->selectedItems();
  if (items.isEmpty())
  {
    return;
  }

  bool hasOverriddenVisibilities = false;
  bool hasOverridenColor = false;
  bool hasOverriddenOpacity = false;
  int hiddenItemCount = 0;
  foreach (const QTreeWidgetItem* item, items)
  {
    if (item->data(NAME_COLUMN, Qt::CheckStateRole).toBool() == false)
    {
      hiddenItemCount++;
    }

    unsigned int flat_index = item->data(NAME_COLUMN, FLAT_INDEX_ROLE).value<unsigned int>();
    hasOverriddenVisibilities =
      hasOverriddenVisibilities || this->BlockVisibilites.contains(flat_index);
    hasOverridenColor = hasOverridenColor || this->BlockColors.contains(flat_index);
    hasOverriddenOpacity = hasOverriddenOpacity || this->BlockOpacities.contains(flat_index);
  }
  int visibleItemCount = items.size() - hiddenItemCount;

  QMenu menu;

  QAction* hideAction = 0;
  if (visibleItemCount > 0)
  {
    QString label = (visibleItemCount > 1) ? QString("Hide %1 Blocks").arg(visibleItemCount)
                                           : QString("Hide Block");
    hideAction = menu.addAction(label);
  }
  QAction* showAction = 0;
  if (hiddenItemCount > 0)
  {
    QString label = (hiddenItemCount > 1) ? QString("Show %1 Blocks").arg(hiddenItemCount)
                                          : QString("Show Block");
    showAction = menu.addAction(label);
  }

  QAction* unsetVisibilityAction = menu.addAction("Unset Visibility");
  unsetVisibilityAction->setEnabled(hasOverriddenVisibilities);

  menu.addSeparator();
  QAction* setColorAction = menu.addAction("Set Color...");
  QAction* unsetColorAction = menu.addAction("Unset Color");
  unsetColorAction->setEnabled(hasOverridenColor);

  menu.addSeparator();
  QAction* setOpacityAction = menu.addAction("Set Opacity...");
  QAction* unsetOpacityAction = menu.addAction("Unset Opacity");
  unsetOpacityAction->setEnabled(hasOverriddenOpacity);

  // show menu
  QAction* action = menu.exec(QCursor::pos());

  if (!action)
  {
    return;
  }
  else if (action == hideAction)
  {
    foreach (QTreeWidgetItem* item, items)
    {
      unsigned int flat_index = item->data(NAME_COLUMN, FLAT_INDEX_ROLE).value<unsigned int>();

      // first unset any child item visibilites
      this->unsetChildVisibilities(item);

      this->setBlockVisibility(flat_index, false);
      item->setData(NAME_COLUMN, Qt::CheckStateRole, Qt::Unchecked);
    }
  }
  else if (action == showAction)
  {
    foreach (QTreeWidgetItem* item, items)
    {
      unsigned int flat_index = item->data(NAME_COLUMN, FLAT_INDEX_ROLE).value<unsigned int>();

      // first unset any child item visibilites
      this->unsetChildVisibilities(item);

      this->setBlockVisibility(flat_index, true);
      item->setData(NAME_COLUMN, Qt::CheckStateRole, Qt::Checked);
    }
  }
  else if (action == unsetVisibilityAction)
  {
    foreach (QTreeWidgetItem* item, items)
    {
      unsigned int flat_index = item->data(NAME_COLUMN, FLAT_INDEX_ROLE).value<unsigned int>();

      this->clearBlockVisibility(flat_index);
      item->setData(NAME_COLUMN, Qt::CheckStateRole,
        item->parent() ? item->parent()->data(NAME_COLUMN, Qt::CheckStateRole) : Qt::Checked);
    }
  }
  else if (action == setColorAction)
  {
    QColor color =
      QColorDialog::getColor(QColor(), this, "Select Color", QColorDialog::DontUseNativeDialog);
    if (color.isValid())
    {
      foreach (QTreeWidgetItem* item, items)
      {
        unsigned int flat_index = item->data(NAME_COLUMN, FLAT_INDEX_ROLE).value<unsigned int>();
        this->setBlockColor(flat_index, color);
      }
    }
  }
  else if (action == unsetColorAction)
  {
    foreach (QTreeWidgetItem* item, items)
    {
      unsigned int flat_index = item->data(NAME_COLUMN, FLAT_INDEX_ROLE).value<unsigned int>();
      this->clearBlockColor(flat_index);
    }
  }
  else if (action == setOpacityAction)
  {
    QList<unsigned int> indices;
    foreach (QTreeWidgetItem* item, items)
    {
      indices.append(item->data(NAME_COLUMN, FLAT_INDEX_ROLE).value<unsigned int>());
    }
    this->promptAndSetBlockOpacity(indices);
  }
  else if (action == unsetOpacityAction)
  {
    foreach (QTreeWidgetItem* item, items)
    {
      unsigned int flat_index = item->data(NAME_COLUMN, FLAT_INDEX_ROLE).value<unsigned int>();

      this->clearBlockOpacity(flat_index);
    }
  }
}

void pqMultiBlockInspectorPanel::onItemDoubleClicked(QTreeWidgetItem* item, int column)
{
  unsigned int flatIndex = item->data(NAME_COLUMN, FLAT_INDEX_ROLE).value<unsigned int>();
  // if (column == NAME_COLUMN)
  //   {
  //   QModelIndex mi(flatIndex, column);
  //   this->TreeWidget->isExpanded(mi) ?
  //     this->TreeWidget->collapse(mi) : this->TreeWidget->expand(mi);
  //   }
  if (column == COLOR_COLUMN)
  {
    QColor color =
      QColorDialog::getColor(QColor(), this, "Select Color", QColorDialog::DontUseNativeDialog);
    if (color.isValid())
    {
      this->setBlockColor(flatIndex, color);
    }
  }
  else if (column == OPACITY_COLUMN)
  {
    this->promptAndSetBlockOpacity(flatIndex);
  }
}

void pqMultiBlockInspectorPanel::onItemChanged(QTreeWidgetItem* item, int column)
{
  Q_UNUSED(column);

  unsigned int flat_index = item->data(NAME_COLUMN, FLAT_INDEX_ROLE).value<unsigned int>();
  bool visible = item->data(NAME_COLUMN, Qt::CheckStateRole).toBool();

  // first unset any child item visibilites
  this->unsetChildVisibilities(item);

  // set block visibility
  this->setBlockVisibility(flat_index, visible);
}

void pqMultiBlockInspectorPanel::unsetChildVisibilities(QTreeWidgetItem* parent_)
{
  for (int i = 0; i < parent_->childCount(); i++)
  {
    QTreeWidgetItem* child = parent_->child(i);
    unsigned int flatIndex = child->data(NAME_COLUMN, FLAT_INDEX_ROLE).value<unsigned int>();
    this->BlockVisibilites.remove(flatIndex);
    unsetChildVisibilities(child);
  }
}

void pqMultiBlockInspectorPanel::onSelectionChanged(pqOutputPort* port)
{
  // find selected block ids
  std::vector<vtkIdType> block_ids;

  if (port)
  {
    vtkSMSourceProxy* activeSelection = port->getSelectionInput();
    if (activeSelection && strcmp(activeSelection->GetXMLName(), "BlockSelectionSource") == 0)
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

  foreach (
    QTreeWidgetItem* item, this->TreeWidget->findItems("", Qt::MatchContains | Qt::MatchRecursive))
  {
    vtkIdType flatIndex = item->data(NAME_COLUMN, FLAT_INDEX_ROLE).value<vtkIdType>();

    item->setSelected(std::binary_search(block_ids.begin(), block_ids.end(), flatIndex));
  }

  this->TreeWidget->blockSignals(false);
}

void pqMultiBlockInspectorPanel::onItemSelectionChanged()
{
  // create vector of selected block ids
  std::vector<vtkIdType> blockIds;
  foreach (const QTreeWidgetItem* item, this->TreeWidget->selectedItems())
  {
    unsigned int flatIndex = item->data(NAME_COLUMN, FLAT_INDEX_ROLE).value<unsigned int>();

    blockIds.push_back(flatIndex);
  }

  // create block selection source proxy
  vtkSMSessionProxyManager* proxyManager =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

  vtkSMProxy* selectionSource = proxyManager->NewProxy("sources", "BlockSelectionSource");

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

  vtkSMSourceProxy* selectionSourceProxy = vtkSMSourceProxy::SafeDownCast(selectionSource);

  // set the selection
  if (this->OutputPort)
  {
    this->OutputPort->setSelectionInput(selectionSourceProxy, 0);
  }

  // update the selection manager
  pqSelectionManager* selectionManager =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SelectionManager"));
  if (selectionManager)
  {
    selectionManager->select(this->OutputPort);
  }

  // delete the selection source
  selectionSourceProxy->Delete();

  // update the views
  if (this->OutputPort)
  {
    this->OutputPort->renderAllViews();
  }
}

QString pqMultiBlockInspectorPanel::lookupBlockName(unsigned int flatIndex) const
{
  foreach (
    QTreeWidgetItem* item, this->TreeWidget->findItems("", Qt::MatchContains | Qt::MatchRecursive))
  {
    unsigned int itemFlatIndex = item->data(NAME_COLUMN, FLAT_INDEX_ROLE).value<unsigned int>();

    if (itemFlatIndex == flatIndex)
    {
      return item->text(0);
    }
  }

  return QString();
}

QIcon pqMultiBlockInspectorPanel::makeNullIcon() const
{
  QPixmap pixmap(ICON_SIZE, ICON_SIZE);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  drawNullIcon(painter);
  painter.end();
  QIcon icon(pixmap);
  return icon;
}

QIcon pqMultiBlockInspectorPanel::makeColorIcon(
  int flatIndex, NodeType nodeType, int inheritedColorIndex, int leafIndex /*=-1*/) const
{
  QPixmap pixmap(ICON_SIZE, ICON_SIZE);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);

  int index = (inheritedColorIndex != -1) ? inheritedColorIndex : flatIndex;
  if (this->BlockColors.contains(index))
  {
    QColor color = this->BlockColors[index];
    drawColorIcon(painter, color, (index == flatIndex) ? USER_VALUE : SYSTEM_VALUE);
  }
  else if (this->ColorTransferFunction && nodeType == LEAF_NODE &&
    this->ColorTransferFunction->GetNumberOfAnnotatedValues() > 0)
  {
    unsigned char* rgb = this->ColorTransferFunction->MapValue(
      CompositeWrap ? (leafIndex % this->BlockColorsDistinctValues) : leafIndex);
    QColor color(rgb[0], rgb[1], rgb[2]);
    drawColorIcon(painter, color);
  }
  else
  {
    drawNullIcon(painter);
  }

  painter.end();
  QIcon icon(pixmap);
  return icon;
}

QIcon pqMultiBlockInspectorPanel::makeOpacityIcon(
  int flatIndex, NodeType nodeType, int inheritedOpacityIndex) const
{
  QPixmap pixmap(ICON_SIZE, ICON_SIZE);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);

  int index = (inheritedOpacityIndex != -1) ? inheritedOpacityIndex : flatIndex;
  if (this->BlockOpacities.contains(index))
  {
    double opacity = this->BlockOpacities[index];
    drawOpacityIcon(painter, opacity, (index == flatIndex) ? USER_VALUE : SYSTEM_VALUE);
  }
  else if (this->OpacityTransferFunction && nodeType == LEAF_NODE)
  {
    double opacity = this->OpacityTransferFunction->GetValue(
      CompositeWrap ? (flatIndex % this->BlockColorsDistinctValues) : flatIndex);
    drawOpacityIcon(painter, opacity);
  }
  else
  {
    drawNullIcon(painter);
  }

  painter.end();

  QIcon icon(pixmap);

  return icon;
}
#endif // !defined(VTK_LEGACY_REMOVE)
