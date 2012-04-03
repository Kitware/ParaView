/*=========================================================================

   Program: ParaView
   Module:    pqExodusIIPanel.cxx

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

=========================================================================*/
#include "pqExodusIIPanel.h"
#include "ui_pqExodusIIPanel.h"

// Qt includes
#include <QAction>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QtDebug>
#include <QVariant>
#include <QVector>

// VTK includes
#include "QFilterTreeProxyModel.h"

// ParaView Server Manager includes
#include "vtkEventQtSlotConnect.h"
#include "vtkGraph.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVCompositeDataInformationIterator.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVSILInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"

// ParaView includes
#include "pqApplicationCore.h"
#include "pqExodusIIVariableSelectionWidget.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqPropertyManager.h"
#include "pqProxy.h"
#include "pqProxySILModel.h"
#include "pqSelectionManager.h"
#include "pqServer.h"
#include "pqSILModel.h"
#include "pqSMAdaptor.h"
#include "pqTimeKeeper.h"
#include "pqTreeViewSelectionHelper.h"
#include "pqTreeWidgetItemObject.h"
#include "pqTreeWidgetSelectionHelper.h"

#include <vector>
#include <algorithm>

//-----------------------------------------------------------------------------
class pqExodusIIPanel::pqUI : public QObject, public Ui::ExodusIIPanel 
{
public:
  /// Specialization of pqSILModel is add support for icons on the 
  /// "Blocks".
  class pqExodusIISILModel : public pqSILModel
  {
public:
  typedef pqSILModel Superclass;
  pqExodusIISILModel(QObject* p = 0) : Superclass(p) {}
  QVariant data(const QModelIndex &idx, int role) const
    {
    if (role == Qt::DecorationRole && idx.isValid())
      {
      vtkIdType vertexId = static_cast<vtkIdType>(idx.internalId());
      // If this vertex is a block, then show the ELEM_BLOCK icon.
      if (this->SIL->GetOutDegree(vertexId) == 0)
        {
        return QVariant(QPixmap(":/pqWidgets/Icons/pqCellCenterData16.png"));
        }
      }
    return this->Superclass::data(idx, role);
    }
  };

public:
  pqUI(pqExodusIIPanel* p) : QObject(p) 
   {
   this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
   this->SILUpdateStamp = -1;
   }

  pqExodusIISILModel SILModel;
  QVector<double> TimestepValues;
  QMap<QTreeWidgetItem*, QString> TreeItemToPropMap;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  int SILUpdateStamp;
};

//-----------------------------------------------------------------------------
pqExodusIIPanel::pqExodusIIPanel(pqProxy* object_proxy, QWidget* p) :
  Superclass(object_proxy, p)
{
  this->UI = new pqUI(this);
  this->UI->setupUi(this);

  this->DisplItem = 0;
  
  this->UI->XMLFileName->setServer(this->referenceProxy()->getServer());

  this->UI->VTKConnect->Connect(
    this->referenceProxy()->getProxy(),
    vtkCommand::UpdateInformationEvent,
    this, SLOT(updateSIL()));

  // Blocks

  pqProxySILModel* proxyModel = new pqProxySILModel("Blocks", &this->UI->SILModel);
  proxyModel->setSourceModel(&this->UI->SILModel);

  // filterProxyModel performs sorting.
  QFilterTreeProxyModel* filterProxyModel = new QFilterTreeProxyModel();
  filterProxyModel->setSourceModel(proxyModel);

  this->UI->Blocks->setModel(filterProxyModel);
  this->UI->Blocks->header()->setClickable(true);
  this->UI->Blocks->header()->setSortIndicator(0, Qt::AscendingOrder);
  this->UI->Blocks->header()->setSortIndicatorShown(true);
  this->UI->Blocks->setSortingEnabled(true);
  QObject::connect(this->UI->Blocks->header(), SIGNAL(checkStateChanged()),
    proxyModel, SLOT(toggleRootCheckState()), Qt::QueuedConnection);

  // Assemblies

  proxyModel = new pqProxySILModel("Assemblies", &this->UI->SILModel);
  proxyModel->setSourceModel(&this->UI->SILModel);
  this->UI->Assemblies->setModel(proxyModel);
  this->UI->Assemblies->header()->setClickable(true);
  QObject::connect(this->UI->Assemblies->header(), SIGNAL(sectionClicked(int)),
    proxyModel, SLOT(toggleRootCheckState()), Qt::QueuedConnection);

  // Materials

  proxyModel = new pqProxySILModel("Materials", &this->UI->SILModel);
  proxyModel->setSourceModel(&this->UI->SILModel);

  filterProxyModel = new QFilterTreeProxyModel();
  filterProxyModel->setSourceModel(proxyModel);

  this->UI->Materials->setModel(filterProxyModel);
  this->UI->Materials->header()->setClickable(true);
  this->UI->Materials->header()->setSortIndicator(0, Qt::AscendingOrder);
  this->UI->Materials->header()->setSortIndicatorShown(true);
  this->UI->Materials->setSortingEnabled(true);
  QObject::connect(this->UI->Materials->header(), SIGNAL(checkStateChanged()),
    proxyModel, SLOT(toggleRootCheckState()), Qt::QueuedConnection);

  this->updateSIL();

  this->UI->Blocks->header()->setStretchLastSection(true);
  this->UI->Assemblies->header()->setStretchLastSection(true);
  this->UI->Materials->header()->setStretchLastSection(true);


  this->linkServerManagerProperties();

  QList<pqTreeWidget*> treeWidgets = this->findChildren<pqTreeWidget*>();
  foreach (pqTreeWidget* tree, treeWidgets)
    {
    new pqTreeWidgetSelectionHelper(tree);
    }

  QList<pqTreeView*> treeViews = this->findChildren<pqTreeView*>();
  foreach (pqTreeView* tree, treeViews)
    {
    new pqTreeViewSelectionHelper(tree);
    }

  pqSelectionManager* selMan = qobject_cast<pqSelectionManager*>(
    pqApplicationCore::instance()->manager("SelectionManager"));
  if (selMan)
    {
    QObject::connect(selMan, SIGNAL(selectionChanged(pqOutputPort*)),
      this, SLOT(onSelectionChanged(pqOutputPort*)));
    }
  QObject::connect(this->UI->checkSelected, SIGNAL(pressed()),
    this, SLOT(setSelectedBlocksCheckState()), Qt::QueuedConnection);
  QObject::connect(this->UI->uncheckSelected, SIGNAL(pressed()),
    this, SLOT(uncheckSelectedBlocks()), Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
pqExodusIIPanel::~pqExodusIIPanel()
{
}

//-----------------------------------------------------------------------------
void pqExodusIIPanel::updateSIL()
{
  vtkSMProxy* reader = this->referenceProxy()->getProxy();
  reader->UpdatePropertyInformation(reader->GetProperty("SILUpdateStamp"));

  int stamp = vtkSMPropertyHelper(reader, "SILUpdateStamp").GetAsInt();
  if (stamp != this->UI->SILUpdateStamp)
    {
    this->UI->SILUpdateStamp = stamp;
    vtkPVSILInformation* info = vtkPVSILInformation::New();
    reader->GatherInformation(info);
    this->UI->SILModel.update(info->GetSIL());

    this->UI->Blocks->expandAll();
    this->UI->Assemblies->expandAll();
    this->UI->Materials->expandAll();
    info->Delete();
    }
}

//-----------------------------------------------------------------------------
void pqExodusIIPanel::linkServerManagerProperties()
{
  vtkSMProxy* readerProxy = this->proxy();

  QFilterTreeProxyModel * filter =
    qobject_cast<QFilterTreeProxyModel *>(this->UI->Blocks->model());
  this->propertyManager()->registerLink(
    filter->sourceModel(), "values", SIGNAL(valuesChanged()),
    readerProxy,
    readerProxy->GetProperty("ElementBlocks"));

  // parent class hooks up some of our widgets in the ui
  this->Superclass::linkServerManagerProperties();

  this->DisplItem = 0;

  // we hook up the node/element variables
  QStringList variablesProperties;
  variablesProperties
    << "GenerateObjectIdCellArray"
    << "GenerateGlobalElementIdArray"
    << "ElementVariables"
    << "FaceVariables"
    << "EdgeVariables"

    // do the set results variables
    << "SideSetResultArrayStatus"
    << "NodeSetResultArrayStatus"
    << "FaceSetResultArrayStatus"
    << "EdgeSetResultArrayStatus"
    << "ElementSetResultArrayStatus"

    << "GenerateGlobalNodeIdArray"

    // do the node variables
    << "PointVariables"

    // do the global variables
    << "GlobalVariables";

  foreach (const QString& pname, variablesProperties)
    {
    this->propertyManager()->registerLink(
      this->UI->Variables, pname.toAscii().data(), SIGNAL(widgetModified()),
      readerProxy, readerProxy->GetProperty(pname.toAscii().data()));
    }

  // Find if there's any variable named "DIS*" in the PointVariables.
  // This chunk of code can be cleaned up too. But leaving it as it was for now.
  // We will have to think how can we auto-generate this panel in its entirety
  // to make things cleaner and easier.
  int numAft = this->UI->Variables->topLevelItemCount();

  // find displacement variable
  for(int j=0; j<numAft; j++)
    {
    QTreeWidgetItem* item = this->UI->Variables->topLevelItem(j);
    if(item->data(0, Qt::DisplayRole).toString().left(3).toUpper() == "DIS")
      {
      this->DisplItem = static_cast<pqTreeWidgetItemObject*>(item);
      }
    }

  if(this->DisplItem)
    {
    QObject::connect(this->DisplItem, SIGNAL(checkedStateChanged(bool)),
      this, SLOT(displChanged(bool)));

    // connect the apply displacements check box with the "DIS*" node variable
    QCheckBox* ApplyDisp = this->UI->ApplyDisplacements;
    QObject::connect(ApplyDisp, SIGNAL(stateChanged(int)),
      this, SLOT(applyDisplacements(int)));
    this->applyDisplacements(Qt::Checked);
    ApplyDisp->setEnabled(true);
    }
  else
    {
    // disable check 
    QCheckBox* ApplyDisp = this->UI->ApplyDisplacements;
    this->applyDisplacements(Qt::Unchecked);
    ApplyDisp->setEnabled(false);
    }

  // we hook up the sideset/nodeset 

  // edge/face blocks (the other block selection is done using SIL).
  this->propertyManager()->registerLink(
    this->UI->EdgeBlockArrays, "EdgeBlocks", SIGNAL(widgetModified()),
    readerProxy, readerProxy->GetProperty("EdgeBlocks"));
  this->propertyManager()->registerLink(
    this->UI->FaceBlockArrays, "FaceBlocks", SIGNAL(widgetModified()),
    readerProxy, readerProxy->GetProperty("FaceBlocks"));

  // sets
  this->propertyManager()->registerLink(
    this->UI->Sets, "SideSetArrayStatus", SIGNAL(widgetModified()),
    readerProxy, readerProxy->GetProperty("SideSetArrayStatus"));
  this->propertyManager()->registerLink(
    this->UI->Sets, "NodeSetArrayStatus", SIGNAL(widgetModified()),
    readerProxy, readerProxy->GetProperty("NodeSetArrayStatus"));
  this->propertyManager()->registerLink(
    this->UI->Sets, "FaceSetArrayStatus", SIGNAL(widgetModified()),
    readerProxy, readerProxy->GetProperty("FaceSetArrayStatus"));
  this->propertyManager()->registerLink(
    this->UI->Sets, "EdgeSetArrayStatus", SIGNAL(widgetModified()),
    readerProxy, readerProxy->GetProperty("EdgeSetArrayStatus"));
  this->propertyManager()->registerLink(
    this->UI->Sets, "ElementSetArrayStatus", SIGNAL(widgetModified()),
    readerProxy, readerProxy->GetProperty("ElementSetArrayStatus"));

  // maps
  this->propertyManager()->registerLink(
    this->UI->Maps, "NodeMapArrayStatus", SIGNAL(widgetModified()),
    readerProxy, readerProxy->GetProperty("NodeMapArrayStatus"));
  this->propertyManager()->registerLink(
    this->UI->Maps, "EdgeMapArrayStatus", SIGNAL(widgetModified()),
    readerProxy, readerProxy->GetProperty("EdgeMapArrayStatus"));
  this->propertyManager()->registerLink(
    this->UI->Maps, "FaceMapArrayStatus", SIGNAL(widgetModified()),
    readerProxy, readerProxy->GetProperty("FaceMapArrayStatus"));
  this->propertyManager()->registerLink(
    this->UI->Maps, "ElementMapArrayStatus", SIGNAL(widgetModified()),
    readerProxy, readerProxy->GetProperty("ElementMapArrayStatus"));

  // Get the timestep values.  Note that the TimestepValues property will change
  // if HasModeShapes is on.  However, we know that when this method is called
  // on initialization, it has the actual time steps in the data.  Store the
  // values now.
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
                      this->proxy()->GetProperty("TimestepValues"));
  this->UI->TimestepValues.resize(dvp->GetNumberOfElements());
  qCopy(dvp->GetElements(), dvp->GetElements()+dvp->GetNumberOfElements(),
        this->UI->TimestepValues.begin());

  // connect the mode shapes
  //this->propertyManager()->registerLink(this->UI->HasModeShapes,
  //                                      "checked",
  //                                      SIGNAL(toggled(bool)),
  //                                      this->proxy(),
  //                                      this->proxy()->
  //                                      GetProperty("HasModeShapes"));
  this->UI->ModeSelectSlider->setMinimum(1);
  this->UI->ModeSelectSlider->setMaximum(this->UI->TimestepValues.size());
  this->UI->ModeSelectSpinBox->setMinimum(1);
  this->UI->ModeSelectSpinBox->setMaximum(this->UI->TimestepValues.size());
  if (this->UI->TimestepValues.size() > 0)
    {
    this->UI->ModeLabel->setText(
                                QString("%1").arg(this->UI->TimestepValues[0]));
    }
  this->propertyManager()->registerLink(this->UI->ModeSelectSlider,
                                        "value",
                                        SIGNAL(valueChanged(int)),
                                        this->proxy(),
                                        this->proxy()
                                        ->GetProperty("ModeShape"));
  this->propertyManager()->registerLink(this->UI->ModeSelectSpinBox,
                                        "value",
                                        SIGNAL(valueChanged(int)),
                                        this->proxy(),
                                        this->proxy()->GetProperty("ModeShape"));
  QObject::connect(this->UI->HasModeShapes, SIGNAL(toggled(bool)),
                   this->UI->ModeShapeOptions, SLOT(setEnabled(bool)));
  QObject::connect(this->UI->ModeSelectSlider, SIGNAL(sliderMoved(int)),
                   this, SLOT(modeChanged(int)));
  QObject::connect(this->UI->ModeSelectSpinBox, SIGNAL(valueChanged(int)),
                   this, SLOT(modeChanged(int)));

  QObject::connect(this->UI->Refresh,
    SIGNAL(pressed()), this, SLOT(onRefresh()));

}
  
//-----------------------------------------------------------------------------
void pqExodusIIPanel::applyDisplacements(int state)
{
  if(state == Qt::Checked && this->DisplItem)
    {
    this->DisplItem->setCheckState(0, Qt::Checked);
    }
  this->UI->DisplacementMagnitude->setEnabled(state == Qt::Checked ? 
                                                  true : false);
}

//-----------------------------------------------------------------------------
void pqExodusIIPanel::displChanged(bool state)
{
  QCheckBox* ApplyDisp = this->UI->ApplyDisplacements;
  if (state)
    {
    // BUG #9843. When displ array is enabled, set the value of
    // ApplyDisplacements to the last accepted value.
    ApplyDisp->setCheckState(
      pqSMAdaptor::getElementProperty(
        this->proxy()->GetProperty("ApplyDisplacements")).toBool()?
      Qt::Checked : Qt::Unchecked);
    }
  else
    {
    ApplyDisp->setCheckState(Qt::Unchecked);
    }
}

//-----------------------------------------------------------------------------
void pqExodusIIPanel::modeChanged(int value)
{
  if ((value > 0) && (value <= this->UI->TimestepValues.size()))
    {
    this->UI->ModeLabel->setText(
      QString("%1").arg(this->UI->TimestepValues[value-1]));
    }
}

//-----------------------------------------------------------------------------
void pqExodusIIPanel::onRefresh()
{
  vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(this->proxy());
  vtkSMProperty *prop = sp->GetProperty("Refresh");

  // The "Refresh" property has no values, so force an update this way
  prop->SetImmediateUpdate(1);
  prop->Modified();

  // "Pull" the values
  sp->UpdatePropertyInformation(sp->GetProperty("TimeRange"));
  sp->UpdatePropertyInformation(sp->GetProperty("TimestepValues")); 
}

//-----------------------------------------------------------------------------
void pqExodusIIPanel::onSelectionChanged(pqOutputPort* port)
{
  this->UI->checkSelected->setEnabled(false);
  this->UI->uncheckSelected->setEnabled(false);

  if (!port || port->getSource()->getProxy() != this->proxy())
    {
    return;
    }

  vtkSMProxy* activeSelection = port->getSelectionInput();
  if (!activeSelection ||
    strcmp(activeSelection->GetXMLName(), "BlockSelectionSource") != 0)
    {
    return;
    }

  this->UI->checkSelected->setEnabled(true);
  this->UI->uncheckSelected->setEnabled(true);
}

//-----------------------------------------------------------------------------
void pqExodusIIPanel::setSelectedBlocksCheckState(bool check/*=true*/)
{
  pqSelectionManager* selMan = qobject_cast<pqSelectionManager*>(
    pqApplicationCore::instance()->manager("SelectionManager"));
  if (!selMan || !selMan->getSelectedPort())
    {
    return;
    }
  
  pqOutputPort* port = selMan->getSelectedPort();
  vtkSMProxy* activeSelection = port->getSelectionInput();
  vtkPVDataInformation* dataInfo = port->getDataInformation();

  vtkSMPropertyHelper blocksProp(activeSelection, "Blocks");
  std::vector<vtkIdType> block_ids;
  block_ids.resize(blocksProp.GetNumberOfElements());
  blocksProp.Get(&block_ids[0], blocksProp.GetNumberOfElements());
  std::sort(block_ids.begin(), block_ids.end());

  // if check is true then we are checking only the selected blocks,
  // if check is false, then we are un-checking the selected blocks, leaving
  // the selections for the other blocks as they are.
  if (check)
    {
    this->UI->SILModel.setData(
      this->UI->SILModel.makeIndex(0), Qt::Unchecked,
      Qt::CheckStateRole);
    }

  // block selection only has the block ids, now we need to convert the block
  // ids to names for the blocks (and sets) using the data information.
  vtkPVCompositeDataInformationIterator* iter =
    vtkPVCompositeDataInformationIterator::New();
  iter->SetDataInformation(dataInfo);
  unsigned int cur_index = 0;
  for (iter->InitTraversal();
    !iter->IsDoneWithTraversal() && cur_index < static_cast<unsigned int>(
      block_ids.size()); iter->GoToNextItem())
    {
    if (static_cast<vtkIdType>(
        iter->GetCurrentFlatIndex()) < block_ids[cur_index])
      {
      continue;
      }
    if (static_cast<vtkIdType>(
        iter->GetCurrentFlatIndex()) > block_ids[cur_index])
      {
      qDebug() << "Failed to locate block's name for block id: " <<
        block_ids[cur_index];
      cur_index++;
      continue;
      }

    vtkIdType vertexid =
      this->UI->SILModel.findVertex(iter->GetCurrentName());
    if (vertexid != -1)
      {
      this->UI->SILModel.setData(this->UI->SILModel.makeIndex(vertexid),
        check? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
      }
    else
      {
      // if vertexid==-1 from the SIL, then it's possible that this is a name of
      // one of the sets, since currently, sets are not part of the SIL. Until
      // the users ask for it, we will leave enabling/disabling the sets out.
      }
    cur_index++;
    }

  iter->Delete();
}
