/*=========================================================================

   Program: ParaView
   Module:    pqExodusIIPanel.cxx

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

#include "pqExodusIIPanel.h"

// Qt includes
#include <QAction>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QTimer>
#include <QTreeWidget>
#include <QVariant>
#include <QVector>

// VTK includes

// ParaView Server Manager includes
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"

// ParaView includes
#include "pqPropertyManager.h"
#include "pqProxy.h"
#include "pqSMAdaptor.h"
#include "pqTreeWidgetCheckHelper.h"
#include "pqTreeWidgetItemObject.h"
#include "ui_pqExodusIIPanel.h"
#include "vtkSMDoubleVectorProperty.h"

class pqExodusIIPanel::pqUI : public QObject, public Ui::ExodusIIPanel 
{
public:
  pqUI(pqExodusIIPanel* p) : QObject(p)
  {
  }
};

pqExodusIIPanel::pqExodusIIPanel(pqProxy* object_proxy, QWidget* p) :
  pqNamedObjectPanel(object_proxy, p)
{
  this->UI = new pqUI(this);
  this->UI->setupUi(this);
  
  this->DisplItem = 0;
  
  QObject::connect(this, SIGNAL(onaccept()),
                   this, SLOT(propertyChanged()));

  this->DataUpdateInProgress = false;
  
  this->linkServerManagerProperties();
}

pqExodusIIPanel::~pqExodusIIPanel()
{
}


void pqExodusIIPanel::addSelectionsToTreeWidget(const QString& prop, 
                                      QTreeWidget* tree,
                                      PixmapType pix)
{
  vtkSMProperty* SMProperty = this->proxy()->GetProperty(prop.toAscii().data());
  QList<QVariant> SMPropertyDomain;
  SMPropertyDomain = pqSMAdaptor::getSelectionPropertyDomain(SMProperty);
  int j;
  for(j=0; j<SMPropertyDomain.size(); j++)
    {
    QString varName = SMPropertyDomain[j].toString();
    this->addSelectionToTreeWidget(varName, varName, tree, pix, prop, j);
    }
}

void pqExodusIIPanel::addSelectionToTreeWidget(const QString& name,
                                               const QString& realName,
                                               QTreeWidget* tree,
                                               PixmapType pix,
                                               const QString& prop,
                                               int propIdx)
{
  static QPixmap pixmaps[] =
    {
    QPixmap(":/pqWidgets/Icons/pqPointData16.png"),
    QPixmap(":/pqWidgets/Icons/pqCellData16.png"),
    QPixmap(":/pqWidgets/Icons/pqSideSet16.png"),
    QPixmap(":/pqWidgets/Icons/pqNodeSet16.png")
    };

  vtkSMProperty* SMProperty = this->proxy()->GetProperty(prop.toAscii().data());

  if(!SMProperty || !tree)
    {
    return;
    }

  QList<QString> strs;
  strs.append(name);
  pqTreeWidgetItemObject* item;
  item = new pqTreeWidgetItemObject(tree, strs);
  item->setData(0, Qt::ToolTipRole, name);
  if(pix >= 0)
    {
    item->setData(0, Qt::DecorationRole, pixmaps[pix]);
    }
  item->setData(0, Qt::UserRole, QString("%1 %2").arg((int)pix).arg(realName));
  this->propertyManager()->registerLink(item, 
                      "checked", 
                      SIGNAL(checkedStateChanged(bool)),
                      this->proxy(), SMProperty, propIdx);
}

void pqExodusIIPanel::linkServerManagerProperties()
{

  // parent class hooks up some of our widgets in the ui
  pqNamedObjectPanel::linkServerManagerProperties();

  this->DisplItem = 0;

  // we hook up the node/element variables
  new pqTreeWidgetCheckHelper(this->UI->Variables, 0, this);
  
  // do block id, global element id
  this->addSelectionToTreeWidget("Object Ids", "ObjectId", this->UI->Variables,
                   PM_ELEM, "GenerateObjectIdCellArray");
  
  this->addSelectionToTreeWidget("Global Element Ids", "GlobalElementId", this->UI->Variables,
                   PM_ELEM, "GenerateGlobalElementIdArray");
  
  // do the cell variables
  this->addSelectionsToTreeWidget("ElementResultArrayStatus",
                                  this->UI->Variables, PM_ELEM);
  
  this->addSelectionToTreeWidget("Global Node Ids", "GlobalNodeId", this->UI->Variables,
                   PM_NODE, "GenerateGlobalNodeIdArray");

  
  int numBef = this->UI->Variables->topLevelItemCount();
  
  // do the node variables
  this->addSelectionsToTreeWidget("PointResultArrayStatus",
                                  this->UI->Variables, PM_NODE);
  
  int numAft = this->UI->Variables->topLevelItemCount();

  // find displacement variable
  for(int j=numBef; j<numAft; j++)
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
  QTreeWidget* SetsTree = this->UI->Sets;
  new pqTreeWidgetCheckHelper(SetsTree, 0, this);


  // blocks
  this->addSelectionsToTreeWidget("EdgeBlockArrayStatus",
                                  this->UI->Blocks, PM_NONE);
  this->addSelectionsToTreeWidget("FaceBlockArrayStatus",
                                  this->UI->Blocks, PM_NONE);
  this->addSelectionsToTreeWidget("ElementBlockArrayStatus",
                                  this->UI->Blocks, PM_NONE);
  
  // sets
  this->addSelectionsToTreeWidget("SideSetArrayStatus",
                                  this->UI->Sets, PM_SIDESET);
  this->addSelectionsToTreeWidget("NodeSetArrayStatus",
                                  this->UI->Sets, PM_NODESET);
  this->addSelectionsToTreeWidget("FaceSetArrayStatus",
                                  this->UI->Sets, PM_ELEM);
  this->addSelectionsToTreeWidget("EdgeSetArrayStatus",
                                  this->UI->Sets, PM_ELEM);
  this->addSelectionsToTreeWidget("ElementSetArrayStatus",
                                  this->UI->Sets, PM_ELEM);
  
  // set results
  this->addSelectionsToTreeWidget("SideSetResultArrayStatus",
                                  this->UI->SetResults, PM_SIDESET);
  this->addSelectionsToTreeWidget("NodeSetResultArrayStatus",
                                  this->UI->SetResults, PM_NODESET);
  this->addSelectionsToTreeWidget("FaceSetResultArrayStatus",
                                  this->UI->SetResults, PM_ELEM);
  this->addSelectionsToTreeWidget("EdgeSetResultArrayStatus",
                                  this->UI->SetResults, PM_ELEM);
  this->addSelectionsToTreeWidget("ElementSetResultArrayStatus",
                                  this->UI->SetResults, PM_ELEM);
  
  // maps
  this->addSelectionsToTreeWidget("NodeMapArrayStatus",
                                  this->UI->Maps, PM_NONE);
  this->addSelectionsToTreeWidget("EdgeMapArrayStatus",
                                  this->UI->Maps, PM_NONE);
  this->addSelectionsToTreeWidget("FaceMapArrayStatus",
                                  this->UI->Maps, PM_NONE);
  this->addSelectionsToTreeWidget("ElementMapArrayStatus",
                                  this->UI->Maps, PM_NONE);

  // update ranges to begin with
  this->updateDataRanges();

}
  
void pqExodusIIPanel::applyDisplacements(int state)
{
  if(state == Qt::Checked && this->DisplItem)
    {
    this->DisplItem->setCheckState(0, Qt::Checked);
    }
  this->UI->DisplacementMagnitude->setEnabled(state == Qt::Checked ? 
                                                  true : false);
}

void pqExodusIIPanel::displChanged(bool state)
{
  if(!state)
    {
    QCheckBox* ApplyDisp = this->UI->ApplyDisplacements;
    ApplyDisp->setCheckState(Qt::Unchecked);
    }
}

QString pqExodusIIPanel::formatDataFor(vtkPVArrayInformation* ai)
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

void pqExodusIIPanel::updateDataRanges()
{
  this->DataUpdateInProgress = false;

  // update data information about loaded arrays

  vtkSMSourceProxy* sp =
    vtkSMSourceProxy::SafeDownCast(this->proxy());
  vtkPVDataSetAttributesInformation* pdi = 0;
  vtkPVDataSetAttributesInformation* cdi = 0;
  if (sp->GetNumberOfParts() > 0)
    {
    vtkPVDataInformation* di = sp->GetDataInformation();
    pdi = di->GetPointDataInformation();
    cdi = di->GetCellDataInformation();
    }
  vtkPVArrayInformation* ai;
  
  QTreeWidget* VariablesTree = this->UI->Variables;
  QString dataString;

  int numItems = VariablesTree->topLevelItemCount();

  for(int i=0; i<numItems; i++)
    {
    QTreeWidgetItem* item = VariablesTree->topLevelItem(i);
    QString var = item->data(0, Qt::UserRole).toString();
    QStringList strs = var.split(" ");
    int which = strs[0].toInt();
    strs.removeFirst();
    var = strs.join(" ");

    ai = 0;
    if (which == PM_ELEM && cdi)
      {
      ai = cdi->GetArrayInformation(var.toAscii().data());
      }
    else if (which == PM_NODE && cdi)
      {
      ai = pdi->GetArrayInformation(var.toAscii().data());
      }
    
    dataString = this->formatDataFor(ai);
    item->setData(1, Qt::DisplayRole, dataString);
    item->setData(1, Qt::ToolTipRole, dataString);
    }
}


void pqExodusIIPanel::propertyChanged()
{
  if(this->DataUpdateInProgress == false)
    {
    this->DataUpdateInProgress = true;
    QTimer::singleShot(0, this, SLOT(updateDataRanges()));
    }
}

