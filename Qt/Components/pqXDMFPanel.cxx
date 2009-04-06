/*=========================================================================

   Program: ParaView
   Module:    pqXDMFPanel.cxx

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

#include "pqXDMFPanel.h"

// Qt includes
#include <QTreeWidget>
#include <QVariant>
#include <QLabel>
#include <QComboBox>
#include <QTableWidget>

// VTK includes

// ParaView Server Manager includes
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMArraySelectionDomain.h"

// ParaView includes
#include "pqProxy.h"
#include "pqSMAdaptor.h"
#include "pqTreeWidgetSelectionHelper.h"
#include "pqTreeWidgetItemObject.h"
#include "ui_pqXDMFPanel.h"

class pqXDMFPanel::pqUI : public QObject, public Ui::XDMFPanel 
{
public:
  pqUI(pqXDMFPanel* p) : QObject(p)
  {
   // Make a clone of the XDMFReader proxy.
   // We'll use the clone to help us with the interdependent properties.
   // In other words, modifying properties outside of accept()/reset() is wrong.
   // We have to modify properties to get the information we need
   // and we'll do that with the clone.
    vtkSMProxyManager* pm = vtkSMProxy::GetProxyManager();
    XDMFHelper.TakeReference(pm->NewProxy("misc", "XdmfReaderHelper"));
    XDMFHelper->InitializeAndCopyFromProxy(p->proxy());
    this->XDMFHelper->UpdatePropertyInformation();
  }
  // our helper
  vtkSmartPointer<vtkSMProxy> XDMFHelper;
};

//----------------------------------------------------------------------------
pqXDMFPanel::pqXDMFPanel(pqProxy* object_proxy, QWidget* p) :
  pqNamedObjectPanel(object_proxy, p)
{
  this->UI = new pqUI(this);
  this->UI->setupUi(this);
  new pqTreeWidgetSelectionHelper(this->UI->Variables);
  this->linkServerManagerProperties();
}

//----------------------------------------------------------------------------
pqXDMFPanel::~pqXDMFPanel()
{
}

//----------------------------------------------------------------------------
void pqXDMFPanel::accept()
{
  // set the arrays
  vtkSMProperty* CellProperty = this->proxy()->GetProperty("CellArrayStatus");
  vtkSMProperty* PointProperty = this->proxy()->GetProperty("PointArrayStatus");

  QList< QList< QVariant > > cellProps, pointProps;
  for(int i=0; i<this->UI->Variables->topLevelItemCount(); i++)
    {
    QTreeWidgetItem* item = this->UI->Variables->topLevelItem(i);
    QList<QVariant> prop;
    prop.append(item->data(0, Qt::DisplayRole));
    prop.append(item->data(0, Qt::CheckStateRole) == Qt::Checked);
    item->data(0, Qt::UserRole) == 0 ?  
      cellProps.append(prop) : pointProps.append(prop);
    }

  pqSMAdaptor::setSelectionProperty(CellProperty, cellProps);
  pqSMAdaptor::setSelectionProperty(PointProperty, pointProps);
  
  QTableWidget* paramsContainer = this->UI->Parameters;
  QList<QVariant> params;
  params = pqSMAdaptor::getMultipleElementProperty(
    this->proxy()->GetProperty("ParametersInfo"));

  for(int j=0; j<paramsContainer->rowCount(); j++)
    {
    QSpinBox *widget = (QSpinBox*)paramsContainer->cellWidget(j,1);
    int value = widget->value();
    params[j*2+1] = QString("%1").arg(value);
    }
  
  pqSMAdaptor::setMultipleElementProperty(
    this->proxy()->GetProperty("ParameterIndex"), params);

  this->proxy()->UpdateVTKObjects();

  this->setGridProperty(this->proxy());

  pqNamedObjectPanel::accept();
}

//----------------------------------------------------------------------------
void pqXDMFPanel::reset()
{
  // ignore domain & grid for now because we can't change them after the first
  // accept

  // clear possible changes in helper
  this->UI->XDMFHelper->GetProperty("PointArrayStatus")->Copy(
    this->proxy()->GetProperty("PointArrayStatus"));
  this->UI->XDMFHelper->GetProperty("CellArrayStatus")->Copy(
    this->proxy()->GetProperty("CellArrayStatus"));
  this->UI->XDMFHelper->GetProperty("ParameterIndex")->Copy(
    this->proxy()->GetProperty("ParameterIndex"));
  this->UI->XDMFHelper->UpdateVTKObjects();

  this->populateArrayWidget();
  this->populateParameterWidget();
  
  pqNamedObjectPanel::reset();
}

//----------------------------------------------------------------------------
void pqXDMFPanel::linkServerManagerProperties()
{
  this->populateDomainWidget();
  this->populateGridWidget();
  
  this->populateParameterWidget();
  
  // parent class hooks up some of our widgets in the ui
  pqNamedObjectPanel::linkServerManagerProperties();
}

//----------------------------------------------------------------------------
void pqXDMFPanel::populateDomainWidget()
{
  //empty the selection widget on the UI (and don't call the changed slot)
  QComboBox* selectionWidget = this->UI->DomainNames;
  
  vtkSMProperty* GetNamesProperty = this->proxy()->GetProperty("GetDomainName");
  QList<QVariant> names;
  names = pqSMAdaptor::getMultipleElementProperty(GetNamesProperty);

  //add each xdmf-domain name to the widget and to the paraview-Domain
  foreach(QVariant v, names)
    {
    selectionWidget->addItem(v.toString());
    }
   
  // get the current value
  vtkSMProperty* SetNamesProperty = this->proxy()->GetProperty("SetDomainName");
  QVariant str = pqSMAdaptor::getEnumerationProperty(SetNamesProperty);
  
  if(str.toString().isEmpty())
    {
    // initialize our helper to whatever item is current
    pqSMAdaptor::setElementProperty(
      this->UI->XDMFHelper->GetProperty("SetDomainName"),
      selectionWidget->currentText());
    this->UI->XDMFHelper->UpdateVTKObjects();
    this->UI->XDMFHelper->UpdatePropertyInformation();
    }
  else
    {
    // set the combo box to the current
    selectionWidget->setCurrentIndex(selectionWidget->findText(str.toString()));
    }
    
  //watch for changes in the widget so that we can tell the proxy
  QObject::connect(selectionWidget, SIGNAL(currentIndexChanged(QString)), 
                   this, SLOT(setSelectedDomain(QString)));
}

//----------------------------------------------------------------------------
void pqXDMFPanel::populateGridWidget()
{
  //empty the selection widget on the UI (and don't call the changed slot)
  pqTreeWidget* gridWidget = this->UI->GridNames;
  QObject::disconnect(gridWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), 
                      this, SLOT(gridItemChanged(QTreeWidgetItem*, int)));
  gridWidget->clear();

  vtkSMProperty* GetNamesProperty = 
    this->UI->XDMFHelper->GetProperty("GetGridName");
  QList<QVariant> grids;
  grids = pqSMAdaptor::getMultipleElementProperty(GetNamesProperty);

  foreach(QVariant v, grids)
    {
    pqTreeWidgetItemObject *anitem;
    anitem = new pqTreeWidgetItemObject(gridWidget, QStringList(v.toString()));
    anitem->setChecked(1);
    }
  
  this->setGridProperty(this->UI->XDMFHelper);
  this->UI->XDMFHelper->UpdatePropertyInformation();

  //whenever grid changes, update the available arrays
  this->populateArrayWidget();
    
  //watch for changes in the widget so that we can tell the proxy
  QObject::connect(gridWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), 
                      this, SLOT(gridItemChanged(QTreeWidgetItem*, int)));
}


//----------------------------------------------------------------------------
void pqXDMFPanel::resetArrays()
{
  //Get the names and state of the available arrays from the server.
  //Copy them to the properties that the GUI shows that let the client control
  //the server.
  vtkSMStringVectorProperty* IProperty;
  vtkSMStringVectorProperty* OProperty;
  vtkSMArraySelectionDomain* ODomain;
  
  IProperty = vtkSMStringVectorProperty::SafeDownCast(
    this->UI->XDMFHelper->GetProperty("PointArrayInfo")
    );
  OProperty = vtkSMStringVectorProperty::SafeDownCast(
    this->UI->XDMFHelper->GetProperty("PointArrayStatus")
    );
  ODomain = vtkSMArraySelectionDomain::SafeDownCast(
    OProperty->GetDomain("array_list")
    );

  OProperty->Copy(IProperty);
  //ODomain->Update(IProperty);


  IProperty = vtkSMStringVectorProperty::SafeDownCast(
    this->UI->XDMFHelper->GetProperty("CellArrayInfo")
    );
  OProperty = vtkSMStringVectorProperty::SafeDownCast(
    this->UI->XDMFHelper->GetProperty("CellArrayStatus")
    );
  ODomain = vtkSMArraySelectionDomain::SafeDownCast(
    OProperty->GetDomain("array_list")
    );

  OProperty->Copy(IProperty);
  //ODomain->Update(IProperty);
}

//---------------------------------------------------------------------------
static void pqXDMFPanelAddArray(vtkSMProperty* prop, 
                                QTreeWidget* tree, int userData)
{
  QPixmap cellPixmap(":/pqWidgets/Icons/pqCellData16.png");
  QPixmap pointPixmap(":/pqWidgets/Icons/pqPointData16.png");

  QList< QList<QVariant> > Props;
  pqTreeWidgetItemObject* item;
  Props = pqSMAdaptor::getSelectionProperty(prop);
  foreach(QList<QVariant> v, Props)
    {
    QString var = v[0].toString();
    item = new pqTreeWidgetItemObject(tree, QStringList(var));
    item->setChecked(v[1].toBool());
    item->setData(0, Qt::ToolTipRole, var);
    item->setData(0, Qt::DecorationRole, 
      userData == 0 ? cellPixmap : pointPixmap);
    item->setData(0, Qt::UserRole, userData);
    }
}

//----------------------------------------------------------------------------
void pqXDMFPanel::populateArrayWidget()
{
  QObject::disconnect(this->UI->Variables,
                      SIGNAL(itemChanged(QTreeWidgetItem*, int)),
                      this, SLOT(setModified()));
 
  this->UI->XDMFHelper->UpdatePropertyInformation();
  this->resetArrays();

  this->UI->Variables->clear();

  pqXDMFPanelAddArray(this->UI->XDMFHelper->GetProperty("CellArrayStatus"),
    this->UI->Variables, 0);
  
  pqXDMFPanelAddArray(this->UI->XDMFHelper->GetProperty("PointArrayStatus"),
    this->UI->Variables, 1);
  
  QObject::connect(this->UI->Variables,
                   SIGNAL(itemChanged(QTreeWidgetItem*, int)),
                   this, SLOT(setModified()));
}

//----------------------------------------------------------------------------
void pqXDMFPanel::populateParameterWidget()
{
  QTableWidget* paramsContainer = this->UI->Parameters;
  
  //ask the reader for the list of available Xdmf parameters
  vtkSMProperty* GetParameterProperty = 
    this->UI->XDMFHelper->GetProperty("ParametersInfo");
  QList<QVariant> params;
  params = pqSMAdaptor::getMultipleElementProperty(GetParameterProperty);

  int numParameter = params.size();
  paramsContainer->setRowCount(numParameter/5);
  if (numParameter == 0)
    {
    QLabel* paramsLabel = this->UI->Parameters_label;
    paramsLabel->setText("No Parameters");
    paramsContainer->hide();
    return;
    }

  //add a control for each paramter to the panel
  QTableWidgetItem *item;
  item = new QTableWidgetItem("Min");
  paramsContainer->setHorizontalHeaderItem(0,item);
  item = new QTableWidgetItem("Value");
  paramsContainer->setHorizontalHeaderItem(1,item);
  item = new QTableWidgetItem("Max");
  paramsContainer->setHorizontalHeaderItem(2,item);
  for (int i = 0; i < numParameter; i+=5)
    {
    int row = i/5;
    QString name = params[i+0].toString();
    item = new QTableWidgetItem(name);
    item->setFlags(0);
    paramsContainer->setVerticalHeaderItem(row,item);

    QString firstIdxS = params[i+2].toString();
    item = new QTableWidgetItem(firstIdxS);
    item->setFlags(0);
    paramsContainer->setItem(row,0,item);
    int min = firstIdxS.toInt();

    QString strideS = params[i+3].toString();
    int stride = strideS.toInt();

    QString countS = params[i+4].toString();
    int count = countS.toInt();    
    int max = min + stride*count;
    item = new QTableWidgetItem(QString("%1").arg(max));
    item->setFlags(0);
    paramsContainer->setItem(row,2,item);

    QSpinBox *valBox = new QSpinBox(paramsContainer);
    valBox->setMinimum(min);
    valBox->setMaximum(max);
    valBox->setSingleStep(stride);
    QString currentValS = params[i+1].toString();
    int val = currentValS.toInt();
    valBox->setValue(val);
    paramsContainer->setCellWidget(row,1,valBox);    
    }
    
  QObject::connect(paramsContainer, SIGNAL(itemChanged(QTreeWidgetItem*)), 
                   this, SLOT(setModified()));

  paramsContainer->resizeColumnsToContents();
}


//-----------------------------------------------------------------------------
void pqXDMFPanel::setSelectedDomain(QString newDomain)
{
  //get access to the property that lets us pick the domain
  pqSMAdaptor::setElementProperty(
    this->UI->XDMFHelper->GetProperty("SetDomainName"), newDomain);
  this->UI->XDMFHelper->UpdateVTKObjects();
  this->UI->XDMFHelper->UpdatePropertyInformation();

  //when domain changes, update the available grids
  this->populateGridWidget();

  this->setModified();
}

//---------------------------------------------------------------------------
void pqXDMFPanel::gridItemChanged(QTreeWidgetItem* item, int)
{
  // if all items are off, turn this last one back on
  // we must always have at least one on
  int count = 0;
  for(int i=0; i<this->UI->GridNames->topLevelItemCount(); i++)
    {
    QTreeWidgetItem* it = this->UI->GridNames->topLevelItem(i);
    if(it->data(0, Qt::CheckStateRole) == Qt::Checked)
      {
      count++;
      }
    }
  if(count == 0)
    {
    item->setData(0, Qt::CheckStateRole, Qt::Checked);
    qWarning("At least one grid must be enabled.");
    return;
    }

  this->setGridProperty(this->UI->XDMFHelper);
  this->UI->XDMFHelper->UpdatePropertyInformation();
  
  //whenever grid changes, update the available arrays
  this->populateArrayWidget();

  this->setModified();
}
  
//---------------------------------------------------------------------------
void pqXDMFPanel::setGridProperty(vtkSMProxy* pxy)
{
  QList<QVariant> grids;
  for(int i=0; i<this->UI->GridNames->topLevelItemCount(); i++)
    {
    QTreeWidgetItem* item = this->UI->GridNames->topLevelItem(i);
    if(item->data(0, Qt::CheckStateRole) == Qt::Checked)
      {
      grids.append(item->text(0));
      }
    }
  vtkSMProperty* EnableProperty = pxy->GetProperty("EnableGrid");
  pqSMAdaptor::setMultipleElementProperty(EnableProperty, grids);
  pxy->UpdateVTKObjects();
}


