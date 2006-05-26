/*=========================================================================

   Program:   ParaQ
   Module:    pqExodusPanel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqExodusPanel.h"

// Qt includes
#include <QTreeWidget>
#include <QVariant>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QAction>

// VTK includes

// ParaView includes
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkSMSourceProxy.h"

// ParaQ includes
#include "pqTreeWidgetItemObject.h"
#include "pqSMAdaptor.h"
#include "pqPropertyManager.h"

pqExodusPanel::pqExodusPanel(QWidget* p) :
  pqLoadedFormObjectPanel(":/pqWidgets/ExodusPanel.ui", p)
{
  this->DisplItem = 0;
}

pqExodusPanel::~pqExodusPanel()
{
}

//-----------------------------------------------------------------------------
void pqExodusPanel::setProxyInternal(pqSMProxy p)
{
  pqLoadedFormObjectPanel::setProxyInternal(p);
}

void pqExodusPanel::linkServerManagerProperties()
{
  // parent class hooks up some of our widgets in the ui
  pqLoadedFormObjectPanel::linkServerManagerProperties();
  
  this->DisplItem = 0;

  QPixmap cellPixmap(":/pqWidgets/pqCellData16.png");
  QPixmap pointPixmap(":/pqWidgets/pqPointData16.png");
  QPixmap sideSetPixmap(":/pqWidgets/pqSideSet16.png");
  QPixmap nodeSetPixmap(":/pqWidgets/pqNodeSet16.png");

  // set the range for the slider 
  // (should probably be moved down to pqNamedObjectPanel)
  QList<QVariant> range;
  range = pqSMAdaptor::getElementPropertyDomain(
       this->proxy()->GetProperty("TimeStep"));

  QSlider* timeSlider = this->findChild<QSlider*>("TimeStep");
  QSpinBox* timeSpin = this->findChild<QSpinBox*>("TimeStepSpin");

  if((range.size() == 2) && (range[1] != -1))
    {
    timeSlider->setEnabled(true);
    timeSpin->setEnabled(true);
    timeSlider->setRange(range[0].toInt(), 
                         range[1].toInt());
    timeSpin->setRange(range[0].toInt(), 
                       range[1].toInt());
    }
  else
    {
    timeSlider->setEnabled(false);
    timeSpin->setEnabled(false);
    }

  // we hook up the node/element variables
  QTreeWidget* VariablesTree = this->findChild<QTreeWidget*>("Variables");
  pqTreeWidgetItemObject* item;
  QList<QString> strs;
  QString varName;
  
  // do block id, global element id
  varName = "Block Ids";
  strs.append(varName);
  item = new pqTreeWidgetItemObject(VariablesTree, strs);
  item->setData(0, Qt::ToolTipRole, varName);
  item->setData(0, Qt::DecorationRole, cellPixmap);
  this->PropertyManager->registerLink(item, 
                      "checked", 
                      SIGNAL(checkedStateChanged(bool)),
                      this->proxy(), 
                      this->proxy()->GetProperty("GenerateBlockIdCellArray"));
  
  varName = "Global Element Ids";
  strs.clear();
  strs.append(varName);
  item = new pqTreeWidgetItemObject(VariablesTree, strs);
  item->setData(0, Qt::ToolTipRole, varName);
  item->setData(0, Qt::DecorationRole, cellPixmap);
  this->PropertyManager->registerLink(item, 
                    "checked", 
                    SIGNAL(checkedStateChanged(bool)),
                    this->proxy(), 
                    this->proxy()->GetProperty("GenerateGlobalElementIdArray"));
  
  // do the cell variables
  vtkSMProperty* CellProperty = this->proxy()->GetProperty("CellArrayStatus");
  QList<QVariant> CellDomain;
  CellDomain = pqSMAdaptor::getSelectionPropertyDomain(CellProperty);
  int j;
  for(j=0; j<CellDomain.size(); j++)
    {
    varName = CellDomain[j].toString();
    strs.clear();
    strs.append(varName);
    item = new pqTreeWidgetItemObject(VariablesTree, strs);
    item->setData(0, Qt::ToolTipRole, varName);
    item->setData(0, Qt::DecorationRole, cellPixmap);
    this->PropertyManager->registerLink(item, 
                                      "checked", 
                                      SIGNAL(checkedStateChanged(bool)),
                                      this->proxy(), CellProperty, j);
    }
  
  
  // do global node ids
  varName = "Global Node Ids";
  strs.clear();
  strs.append(varName);
  item = new pqTreeWidgetItemObject(VariablesTree, strs);
  item->setData(0, Qt::ToolTipRole, varName);
  item->setData(0, Qt::DecorationRole, pointPixmap);
  this->PropertyManager->registerLink(item, 
                    "checked", 
                    SIGNAL(checkedStateChanged(bool)),
                    this->proxy(), 
                    this->proxy()->GetProperty("GenerateGlobalNodeIdArray"));

  // do the node variables
  vtkSMProperty* NodeProperty = this->proxy()->GetProperty("PointArrayStatus");
  QList<QVariant> PointDomain;
  PointDomain = pqSMAdaptor::getSelectionPropertyDomain(NodeProperty);

  for(j=0; j<PointDomain.size(); j++)
    {
    varName = PointDomain[j].toString();
    strs.clear();
    strs.append(varName);
    item = new pqTreeWidgetItemObject(VariablesTree, strs);
    item->setData(0, Qt::ToolTipRole, varName);
    item->setData(0, Qt::DecorationRole, pointPixmap);
    this->PropertyManager->registerLink(item, 
                                      "checked", 
                                      SIGNAL(checkedStateChanged(bool)),
                                      this->proxy(), NodeProperty, j);

    if(PointDomain[j].toString() == "DISPL")
      {
      this->DisplItem = item;
      }
    }




  if(this->DisplItem)
    {
    QObject::connect(this->DisplItem, SIGNAL(checkedStateChanged(bool)),
                     this, SLOT(displChanged(bool)));

    // connect the apply displacements check box with the "DISPL" node variable
    QCheckBox* ApplyDisp = this->findChild<QCheckBox*>("ApplyDisplacements");
    QObject::connect(ApplyDisp, SIGNAL(stateChanged(int)),
                     this, SLOT(applyDisplacements(int)));
    this->applyDisplacements(Qt::Checked);
    ApplyDisp->setEnabled(true);
    }
  else
    {
    QCheckBox* ApplyDisp = this->findChild<QCheckBox*>("ApplyDisplacements");
    this->applyDisplacements(Qt::Unchecked);
    ApplyDisp->setEnabled(false);
    }

  // we hook up the sideset/nodeset 
  QTreeWidget* SetsTree = this->findChild<QTreeWidget*>("Sets");
  
  // do the sidesets
  vtkSMProperty* SideProperty = this->proxy()->GetProperty("SideSetArrayStatus");
  QList<QVariant> SideDomain;
  SideDomain = pqSMAdaptor::getSelectionPropertyDomain(SideProperty);
  for(j=0; j<SideDomain.size(); j++)
    {
    varName = SideDomain[j].toString();
    strs.clear();
    strs.append(varName);
    item = new pqTreeWidgetItemObject(SetsTree, strs);
    item->setData(0, Qt::ToolTipRole, varName);
    item->setData(0, Qt::DecorationRole, sideSetPixmap);
    this->PropertyManager->registerLink(item, 
                                      "checked", 
                                      SIGNAL(checkedStateChanged(bool)),
                                      this->proxy(), SideProperty, j);
    }
  
  // do the nodesets
  vtkSMProperty* NSProperty = this->proxy()->GetProperty("NodeSetArrayStatus");
  QList<QVariant> NSDomain;
  NSDomain = pqSMAdaptor::getSelectionPropertyDomain(NSProperty);
  for(j=0; j<NSDomain.size(); j++)
    {
    varName = NSDomain[j].toString();
    strs.clear();
    strs.append(varName);
    item = new pqTreeWidgetItemObject(SetsTree, strs);
    item->setData(0, Qt::ToolTipRole, varName);
    item->setData(0, Qt::DecorationRole, nodeSetPixmap);
    this->PropertyManager->registerLink(item, 
                                      "checked", 
                                      SIGNAL(checkedStateChanged(bool)),
                                      this->proxy(), NSProperty, j);
    }

  this->updateDataRanges();

  QTreeWidget* BlockTree = this->findChild<QTreeWidget*>("BlockArrayStatus");

  QAction* a;
  a = new QAction("All Blocks On", BlockTree);
  QObject::connect(a, SIGNAL(triggered(bool)), this, SLOT(blocksOn()));
  BlockTree->addAction(a);
  a = new QAction("All Blocks Off", BlockTree);
  QObject::connect(a, SIGNAL(triggered(bool)), this, SLOT(blocksOff()));
  BlockTree->addAction(a);
  BlockTree->setContextMenuPolicy(Qt::ActionsContextMenu);
  
  a = new QAction("All Sets On", SetsTree);
  QObject::connect(a, SIGNAL(triggered(bool)), this, SLOT(setsOn()));
  SetsTree->addAction(a);
  a = new QAction("All Sets Off", SetsTree);
  QObject::connect(a, SIGNAL(triggered(bool)), this, SLOT(setsOff()));
  SetsTree->addAction(a);
  SetsTree->setContextMenuPolicy(Qt::ActionsContextMenu);

  a = new QAction("All Variables On", VariablesTree);
  QObject::connect(a, SIGNAL(triggered(bool)), this, SLOT(variablesOn()));
  VariablesTree->addAction(a);
  a = new QAction("All Variables Off", VariablesTree);
  QObject::connect(a, SIGNAL(triggered(bool)), this, SLOT(variablesOff()));
  VariablesTree->addAction(a);
  VariablesTree->setContextMenuPolicy(Qt::ActionsContextMenu);

}

void pqExodusPanel::unlinkServerManagerProperties()
{
  // parent class un-hooks some of our widgets in the ui
  pqLoadedFormObjectPanel::unlinkServerManagerProperties();
  
  
  // we un-hook the node/element variables
  QTreeWidget* VariablesTree = this->findChild<QTreeWidget*>("Variables");
  pqTreeWidgetItemObject* item;
  
  // remove block ids
  item = static_cast<pqTreeWidgetItemObject*>(VariablesTree->topLevelItem(0));
  this->PropertyManager->unregisterLink(item, 
                      "checked", 
                      SIGNAL(checkedStateChanged(bool)),
                      this->proxy(), 
                      this->proxy()->GetProperty("GenerateBlockIdCellArray"));
  
  // remove global element id
  item = static_cast<pqTreeWidgetItemObject*>(VariablesTree->topLevelItem(1));
  this->PropertyManager->unregisterLink(item, 
                      "checked", 
                      SIGNAL(checkedStateChanged(bool)),
                      this->proxy(), 
                      this->proxy()->GetProperty("GenerateGlobalElementIdArray"));

  const int CellOffset = 2;
  
  // do the cell variables
  vtkSMProperty* CellProperty = this->proxy()->GetProperty("CellArrayStatus");
  QList<QVariant> CellDomain;
  CellDomain = pqSMAdaptor::getSelectionPropertyDomain(CellProperty);
  int j;
  for(j=0; j<CellDomain.size(); j++)
    {
    item = static_cast<pqTreeWidgetItemObject*>(
                  VariablesTree->topLevelItem(j+CellOffset));
    this->PropertyManager->unregisterLink(item, 
                                          "checked", 
                                          SIGNAL(checkedStateChanged(bool)),
                                          this->proxy(), CellProperty, j);
    }
  
  // remove global node id
  item =
    static_cast<pqTreeWidgetItemObject*>(
        VariablesTree->topLevelItem(CellOffset + CellDomain.size()));
  this->PropertyManager->unregisterLink(item, 
                      "checked", 
                      SIGNAL(checkedStateChanged(bool)),
                      this->proxy(), 
                      this->proxy()->GetProperty("GenerateGlobalNodeIdArray"));

  const int PointOffset = 1;
  // do the node variables
  vtkSMProperty* NodeProperty = this->proxy()->GetProperty("PointArrayStatus");
  QList<QVariant> PointDomain;
  PointDomain = pqSMAdaptor::getSelectionPropertyDomain(NodeProperty);

  for(j=0; j<PointDomain.size(); j++)
    {
    item = static_cast<pqTreeWidgetItemObject*>(
        VariablesTree->topLevelItem(j + CellOffset + PointOffset + CellDomain.size()));
    this->PropertyManager->unregisterLink(item, 
                                          "checked", 
                                          SIGNAL(checkedStateChanged(bool)),
                                          this->proxy(), NodeProperty, j);
    }

  // disconnect the apply displacements check box with the "DISPL" node variable
  if(this->DisplItem)
    {
    QObject::disconnect(this->DisplItem, SIGNAL(checkedStateChanged(bool)),
                     this, SLOT(displChanged(bool)));

    QCheckBox* ApplyDisp = this->findChild<QCheckBox*>("ApplyDisplacements");
    QObject::disconnect(ApplyDisp, SIGNAL(stateChanged(int)),
                        this, SLOT(applyDisplacements(int)));
    }

  this->DisplItem = 0;
  
  VariablesTree->clear();
  
  
  QTreeWidget* SetsTree = this->findChild<QTreeWidget*>("Sets");

  // do the sidesets
  vtkSMProperty* SideProperty = this->proxy()->GetProperty("SideSetArrayStatus");
  QList<QVariant> SideDomain;
  SideDomain = pqSMAdaptor::getSelectionPropertyDomain(SideProperty);
  for(j=0; j<SideDomain.size(); j++)
    {
    item = static_cast<pqTreeWidgetItemObject*>(
                  SetsTree->topLevelItem(j));
    this->PropertyManager->unregisterLink(item, 
                                          "checked", 
                                          SIGNAL(checkedStateChanged(bool)),
                                          this->proxy(), SideProperty, j);
    }
  
  // do the nodesets
  vtkSMProperty* NSProperty = this->proxy()->GetProperty("NodeSetArrayStatus");
  QList<QVariant> NSDomain;
  NSDomain = pqSMAdaptor::getSelectionPropertyDomain(NSProperty);

  for(j=0; j<NSDomain.size(); j++)
    {
    item = static_cast<pqTreeWidgetItemObject*>(
        SetsTree->topLevelItem(j + SideDomain.size()));
    this->PropertyManager->unregisterLink(item, 
                                          "checked", 
                                          SIGNAL(checkedStateChanged(bool)),
                                          this->proxy(), NSProperty, j);
    }

}

void pqExodusPanel::applyDisplacements(int state)
{
  if(state == Qt::Checked && this->DisplItem)
    {
    this->DisplItem->setCheckState(0, Qt::Checked);
    }
  QDoubleSpinBox* Mag;
  Mag = this->findChild<QDoubleSpinBox*>("DisplacementMagnitude");
  Mag->setEnabled(state == Qt::Checked ? true : false);
}

void pqExodusPanel::displChanged(bool state)
{
  if(!state)
    {
    QCheckBox* ApplyDisp = this->findChild<QCheckBox*>("ApplyDisplacements");
    ApplyDisp->setCheckState(Qt::Unchecked);
    }
}

void pqExodusPanel::postAccept()
{
  this->updateDataRanges();
  pqLoadedFormObjectPanel::postAccept();
}

static QString formatDataFor(vtkPVArrayInformation* ai)
{
  QString info;
  if(ai)
    {
    int numComponents = ai->GetNumberOfComponents();
    int dataType = ai->GetDataType();
    double range[2];
    for(int i=0; i<numComponents; i++)
      {
      ai->GetComponentRange(i, range);
      QString s;
      if(dataType != VTK_VOID && dataType != VTK_FLOAT && 
         dataType != VTK_DOUBLE)
        {
        // display as integers (capable of 64 bit ids)
        qlonglong min = qRound64(range[0]);
        qlonglong max = qRound64(range[1]);
        s = QString("%1 - %2").arg(min).arg(max);
        }
      else
        {
        // display as reals
        double min = range[0];
        double max = range[1];
        s = QString("%1 - %2").arg(min,0,'f',6).arg(max,0,'f',6);
        }
      if(i > 0)
        {
        info += ", ";
        }
      info += s;
      }
    }
  else
    {
    info = "Unavailable";
    }
  return info;
}

void pqExodusPanel::updateDataRanges()
{
  // update data information about loaded arrays

  vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(this->proxy());
  vtkPVDataInformation* di = sp->GetDataInformation();
  vtkPVDataSetAttributesInformation* pdi = di->GetPointDataInformation();
  vtkPVDataSetAttributesInformation* cdi = di->GetCellDataInformation();
  vtkPVArrayInformation* ai;
  
  QTreeWidget* VariablesTree = this->findChild<QTreeWidget*>("Variables");
  pqTreeWidgetItemObject* item;
  QString dataString;
  
  // block ids
  item = static_cast<pqTreeWidgetItemObject*>(VariablesTree->topLevelItem(0));
  ai = cdi->GetArrayInformation("BlockId");
  dataString = formatDataFor(ai);
  item->setData(1, Qt::DisplayRole, dataString);
  item->setData(1, Qt::ToolTipRole, dataString);

  
  // remove global element id
  item = static_cast<pqTreeWidgetItemObject*>(VariablesTree->topLevelItem(1));
  ai = cdi->GetArrayInformation("GlobalElementId");
  dataString = formatDataFor(ai);
  item->setData(1, Qt::DisplayRole, dataString);
  item->setData(1, Qt::ToolTipRole, dataString);

  const int CellOffset = 2;
  
  // do the cell variables
  vtkSMProperty* CellProperty = this->proxy()->GetProperty("CellArrayStatus");
  QList<QVariant> CellDomain;
  CellDomain = pqSMAdaptor::getSelectionPropertyDomain(CellProperty);
  int j;
  for(j=0; j<CellDomain.size(); j++)
    {
    item = static_cast<pqTreeWidgetItemObject*>(
                  VariablesTree->topLevelItem(j+CellOffset));
    ai = cdi->GetArrayInformation(CellDomain[j].toString().toAscii().data());
    dataString = formatDataFor(ai);
    item->setData(1, Qt::DisplayRole, dataString);
    item->setData(1, Qt::ToolTipRole, dataString);
    }
  
  // remove global node id
  item = static_cast<pqTreeWidgetItemObject*>(
        VariablesTree->topLevelItem(CellOffset + CellDomain.size()));
  ai = pdi->GetArrayInformation("GlobalNodeId");
  dataString = formatDataFor(ai);
  item->setData(1, Qt::DisplayRole, dataString);
  item->setData(1, Qt::ToolTipRole, dataString);

  const int PointOffset = 1;
  // do the node variables
  vtkSMProperty* NodeProperty = this->proxy()->GetProperty("PointArrayStatus");
  QList<QVariant> PointDomain;
  PointDomain = pqSMAdaptor::getSelectionPropertyDomain(NodeProperty);

  for(j=0; j<PointDomain.size(); j++)
    {
    item = static_cast<pqTreeWidgetItemObject*>(
        VariablesTree->topLevelItem(j + CellOffset + 
                                    PointOffset + CellDomain.size()));
    ai = pdi->GetArrayInformation(PointDomain[j].toString().toAscii().data());
    dataString = formatDataFor(ai);
    item->setData(1, Qt::DisplayRole, dataString);
    item->setData(1, Qt::ToolTipRole, dataString);
    }
}


void pqExodusPanel::blocksOn()
{
  this->blocksToggle(Qt::Checked);
}

void pqExodusPanel::blocksOff()
{
  this->blocksToggle(Qt::Unchecked);
}

void pqExodusPanel::blocksToggle(Qt::CheckState c)
{
  QTreeWidget* Tree = this->findChild<QTreeWidget*>("BlockArrayStatus");
  this->toggle(Tree, c);
}


void pqExodusPanel::variablesOn()
{
  this->variablesToggle(Qt::Checked);
}

void pqExodusPanel::variablesOff()
{
  this->variablesToggle(Qt::Unchecked);
}

void pqExodusPanel::variablesToggle(Qt::CheckState c)
{
  QTreeWidget* Tree = this->findChild<QTreeWidget*>("Variables");
  this->toggle(Tree, c);
}


void pqExodusPanel::setsOn()
{
  this->setsToggle(Qt::Checked);
}

void pqExodusPanel::setsOff()
{
  this->setsToggle(Qt::Unchecked);
}

void pqExodusPanel::setsToggle(Qt::CheckState c)
{
  QTreeWidget* Tree = this->findChild<QTreeWidget*>("Sets");
  this->toggle(Tree, c);
}

void pqExodusPanel::toggle(QTreeWidget* Tree, Qt::CheckState c)
{
  if(Tree)
    {
    QTreeWidgetItem* item;
    int i, end = Tree->topLevelItemCount();
    for(i=0; i<end; i++)
      {
      item = Tree->topLevelItem(i);
      item->setCheckState(0, c);
      }
    }
}

