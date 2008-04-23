/*=========================================================================

   Program: ParaView
   Module:    pqExtractDataSetsPanel.cxx

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

#include "pqExtractDataSetsPanel.h"

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
#include "vtkSMProxyProperty.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkSMIntVectorProperty.h"

// ParaView includes
#include "pqPropertyManager.h"
#include "pqProxy.h"
#include "pqSMAdaptor.h"
#include "pqTreeWidgetItemObject.h"
#include "vtkSMDoubleVectorProperty.h"

#include <assert.h>

//----------------------------------------------------------------------------
/*
class pqExtractDataSetsPanel::pqUI : public QObject, public Ui::ExtractDataSetsPanel 
{
public:
  pqUI(pqExtractDataSetsPanel* p) : QObject(p)
  {
  }
};
*/
//----------------------------------------------------------------------------
typedef vtkstd::pair<int,int>                         GroupIndex;
typedef vtkstd::pair<Qt::CheckState,QTreeWidgetItem*> StateItem;
typedef vtkstd::map< GroupIndex, StateItem>           DataSetsMap;

class pqExtractDataSetsPanelInternals
{
public:
  pqExtractDataSetsPanelInternals() {};
  // store a list of <group, index> selected items
  DataSetsMap SelectedData;
  // make a copy of original state so we can put hings back when reset clicked
  DataSetsMap SelectedDataCopy;
};
//----------------------------------------------------------------------------
pqExtractDataSetsPanel::pqExtractDataSetsPanel(pqProxy* object_proxy, QWidget* p) :
  pqObjectPanel(object_proxy, p)
{
  this->UI = new Ui::ExtractDataSetsPanel();
  this->UI->setupUi(this);
  this->UpdateInProgress  = false;
  this->Internals = new pqExtractDataSetsPanelInternals();

  if(vtkSMProxyProperty* const input_property =
    vtkSMProxyProperty::SafeDownCast(
      this->proxy()->GetProperty("Input")))
    {
    if(vtkSMSourceProxy* const input_proxy = vtkSMSourceProxy::SafeDownCast(
        input_property->GetProxy(0)))
      {
      vtkPVCompositeDataInformation* cdi =
        input_proxy->GetDataInformation()->GetCompositeDataInformation();
      unsigned int numGroups = cdi->GetNumberOfChildren();
      QList<QString> strings;
      int firstTime = 1;
      pqTreeWidgetItemObject *groupitem;
      for (unsigned int G=0; G<numGroups; G++)
        {
        // If there are more than one group, add a label showing
        // the group number before listing the blocks for that
        // group. Store the index of this item to be used later.
        QString groupStr = QString("Group %1:").arg(G);
        strings.clear();
        strings.append(groupStr);
        groupitem = new pqTreeWidgetItemObject(this->UI->DataSetsList, strings);
        groupitem->setData(0, Qt::ToolTipRole, groupStr);
        groupitem->setData(0, Qt::UserRole, -1);
        groupitem->setChecked(0);

        /*
        // loop over datasets
        unsigned int numDataSets = cdi->GetNumberOfDataSets(G);
        for (unsigned int i=0; i<numDataSets; i++)
          {
          vtkPVDataInformation* dataInfo = cdi->GetDataInformation(G, i);
          QString dataStr;
          if (dataInfo)
            {
            dataStr = QString("  %1").arg(dataInfo->GetName());
            }
          else
            {
            dataStr = QString("  block %1").arg(i);
            }
          strings.clear();
          strings.append(dataStr);
          pqTreeWidgetItemObject *dataitem = new pqTreeWidgetItemObject(groupitem, strings);
          dataitem->setData(0, Qt::ToolTipRole, dataStr);
          dataitem->setData(0, Qt::UserRole, G);  //group number
          if (firstTime)
            {
            // By default select all first group
            dataitem->setChecked(1);
            this->Internals->SelectedData.insert( 
              vtkstd::pair< GroupIndex, StateItem>
              (GroupIndex(G,i), StateItem(Qt::Checked,dataitem)));
            }
          else
            {
            dataitem->setChecked(0);
            this->Internals->SelectedData.insert( 
              vtkstd::pair< GroupIndex, StateItem>
              (GroupIndex(G,i), StateItem(Qt::Unchecked,dataitem)));
            }
          }
          */
        assert("FIXME" && 0);
        if (firstTime)
          {
          // By default select first one
          groupitem->setChecked(1);
          firstTime = 0;
          }
        groupitem->setExpanded(1);
        }
      }
    }

  QObject::connect(this->UI->DataSetsList,
    SIGNAL(itemChanged(QTreeWidgetItem*, int)),
    this, SLOT(datasetsItemChanged(QTreeWidgetItem*)));
}
//----------------------------------------------------------------------------
pqExtractDataSetsPanel::~pqExtractDataSetsPanel()
{
  delete this->Internals;
}
//----------------------------------------------------------------------------
void pqExtractDataSetsPanel::accept()
{
  //
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->proxy()->GetProperty("SelectedDataSets"));
  if (!ivp)
    {
    return;
    }
  ivp->SetNumberOfElements(0);

  unsigned int idx=0;
  DataSetsMap::iterator iter;
  for (iter=this->Internals->SelectedData.begin(); 
    iter!=this->Internals->SelectedData.end(); ++iter) 
    {
    if (iter->second.first) // ie selected this {group,index}
      {
      int group = iter->first.first;
      int index = iter->first.second;
      ivp->SetElement(idx++, group);
      ivp->SetElement(idx++, index);
      }
    } 
  this->proxy()->UpdateVTKObjects();
  this->Internals->SelectedDataCopy = this->Internals->SelectedData;
  pqObjectPanel::accept();
}
//----------------------------------------------------------------------------
void pqExtractDataSetsPanel::reset()
{
  this->Internals->SelectedData = this->Internals->SelectedDataCopy;
  this->updateGUI();
}
//----------------------------------------------------------------------------
void pqExtractDataSetsPanel::updateMapState(QTreeWidgetItem* item)
{
  int group = item->data(0, Qt::UserRole).toInt();
  if (group==-1) // this is not a child node, so just ignore it
  {
    return;
  }
  int index = item->parent()->indexOfChild(item);
  DataSetsMap::iterator iter = this->Internals->SelectedData.find( 
    GroupIndex(group, index)
  );
  iter->second.first = item->checkState(0);
}
//----------------------------------------------------------------------------
void pqExtractDataSetsPanel::datasetsItemChanged(QTreeWidgetItem* item)
{
  if (this->UpdateInProgress)
    {
    return;
    }
  this->UpdateInProgress = true;
  this->updateMapState(item);
  // if a parent is changed, change all children, 
  for (int i=0; i<item->childCount(); i++) 
    {
    QTreeWidgetItem *child = item->child(i);
    child->setCheckState(0,item->checkState(0));
    this->updateMapState(child);
    }

  // if a child is changed, see if parent needs to be changed
  QTreeWidgetItem *p = item->parent();
  if (p && p->childCount()>0)
    {
    bool allsame = true;
    Qt::CheckState laststate = p->child(0)->checkState(0);
    for (int i=1; i<p->childCount(); i++) 
      {
      if (p->child(i)->checkState(0)!=laststate)
        {
        allsame = false;
        }
      }
    if (allsame) 
      {
      p->setCheckState(0, p->child(0)->checkState(0));
      }
    else 
      {
        p->setCheckState(0, Qt::PartiallyChecked);
      }
    } 
  this->setModified();
  this->UpdateInProgress = false;
}
//----------------------------------------------------------------------------
void pqExtractDataSetsPanel::updateGUI()
{
  DataSetsMap::iterator iter;
  for (iter=this->Internals->SelectedData.begin(); 
    iter!=this->Internals->SelectedData.end(); ++iter) 
    {
    QTreeWidgetItem *item = iter->second.second;
    item->setCheckState(0,iter->second.first);
    }
}
//----------------------------------------------------------------------------
