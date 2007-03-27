/*=========================================================================

   Program: ParaView
   Module:    pqXDMFPanel.cxx

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

#include "pqXDMFPanel.h"

// Qt includes
#include <QTreeWidget>
#include <QVariant>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QAction>
#include <QTimer>
#include <QLabel>
#include <QComboBox>
#include <QListWidget>
#include <QTableWidget>
#include <QSignalMapper>

// VTK includes

// ParaView Server Manager includes
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMArraySelectionDomain.h"

// ParaView includes
#include "pqPropertyManager.h"
#include "pqProxy.h"
#include "pqSMAdaptor.h"
#include "pqTreeWidgetCheckHelper.h"
#include "pqTreeWidgetItemObject.h"
#include "ui_pqXDMFPanel.h"

// the purpose of this helper class is to remember what array selection
// widgets are linked to what server properties so that we can control the call
// backs when we need to rebuild
class pqXDMFPanelArrayRecord
{
public:
  pqXDMFPanelArrayRecord(pqTreeWidgetItemObject *_widget, 
                         int _CorP, 
                         int _location) :
    widget(_widget), CorP(_CorP), location(_location) 
    {
    };

  pqTreeWidgetItemObject *widget;
  int CorP; //cell or point
  int location; //index in property
};

class pqXDMFPanel::pqUI : public QObject, public Ui::XDMFPanel 
{
public:
  pqUI(pqXDMFPanel* p) : QObject(p) {}
};

//----------------------------------------------------------------------------
pqXDMFPanel::pqXDMFPanel(pqProxy* object_proxy, QWidget* p) :
  pqNamedObjectPanel(object_proxy, p)
{
  this->UI = new pqUI(this);
  this->UI->setupUi(this);
  this->LastGridDeselected = NULL;
  this->NeedsResetGrid = false;
  this->FirstAcceptHappened = false;
  this->linkServerManagerProperties();
}

//----------------------------------------------------------------------------
pqXDMFPanel::~pqXDMFPanel()
{
  this->ArrayList.clear();
}

//----------------------------------------------------------------------------
void pqXDMFPanel::accept()
{
  pqNamedObjectPanel::accept();
  if (!this->FirstAcceptHappened)
    {
    //when the change later on the number of output ports of the server side
    //object changes. If we can make paraview OK with that, then we should
    //take this out and let the user change it dynamically.
    QComboBox* domainWidget = this->UI->DomainNames;
    domainWidget->setEnabled(false);
    QListWidget* gridsWidget = this->UI->GridNames;
    gridsWidget->setEnabled(false);
    }
  this->FirstAcceptHappened = true;
}
//----------------------------------------------------------------------------
void pqXDMFPanel::linkServerManagerProperties()
{
  // parent class hooks up some of our widgets in the ui
  pqNamedObjectPanel::linkServerManagerProperties();

  this->PopulateDomainWidget();

  this->PopulateGridWidget();

  this->PopulateParameterWidget();
}

//----------------------------------------------------------------------------
void pqXDMFPanel::PopulateDomainWidget()
{
  //empty the selection widget on the UI (and don't call the changed slot)
  QComboBox* selectionWidget = this->UI->DomainNames;
  QObject::disconnect(selectionWidget, SIGNAL(currentIndexChanged(QString)), 
                   this, SLOT(SetSelectedDomain(QString)));
  selectionWidget->clear();

  //get access to the paraview-"Domain" that restricts what names we can choose
  vtkSMStringVectorProperty* SetNameProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->GetProperty("SetDomainName"));
  vtkSMStringListDomain* canChooseFrom = 
    vtkSMStringListDomain::SafeDownCast(
      SetNameProperty->GetDomain("AvailableDomains"));
  //empty it
  canChooseFrom->RemoveAllStrings();

  //ask the reader for the list of available Xdmf-"domain" names
  this->proxy()->UpdatePropertyInformation();
  vtkSMStringVectorProperty* GetNamesProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->GetProperty("GetDomainName"));

  //add each xdmf-domain name to the widget and to the paraview-Domain
  int numNames = GetNamesProperty->GetNumberOfElements();
  for (int i = 0; i < numNames; i++)
    {
    const char* name = GetNamesProperty->GetElement(i);
    selectionWidget->addItem(name);
    canChooseFrom->AddString(name);
    }

  //watch for changes in the widget so that we can tell the proxy
  QObject::connect(selectionWidget, SIGNAL(currentIndexChanged(QString)), 
                   this, SLOT(SetSelectedDomain(QString)));

}

//----------------------------------------------------------------------------
void pqXDMFPanel::PopulateGridWidget()
{
  //force the XdmfReader that ignore what it thinks it knows 
  //about the available grids, because it is wrong when domain changes
  vtkSMProperty* RemoveProperty = 
    vtkSMProperty::SafeDownCast(
      this->proxy()->GetProperty("RemoveAllGrids"));
  RemoveProperty->Modified();
  this->proxy()->UpdateVTKObjects();

  //empty the selection widget on the UI (and don't call the changed slot)
  QListWidget* selectionWidget = this->UI->GridNames;
  QObject::disconnect(selectionWidget, SIGNAL(itemSelectionChanged()), 
                      this, SLOT(SetSelectedGrids()));
  selectionWidget->clear();

  //get access to the paraview Domain that restricts what names we can choose
  vtkSMStringVectorProperty* SetNameProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->GetProperty("SetGridName"));
  vtkSMStringListDomain* canChooseFrom = 
    vtkSMStringListDomain::SafeDownCast(
      SetNameProperty->GetDomain("AvailableGrids"));
  //empty it
  canChooseFrom->RemoveAllStrings();

  //get access to the property that turns on/off each grid
  vtkSMStringVectorProperty* EnableProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->GetProperty("EnableGrid"));

  //ask the reader for the list of available Xdmf grid names
  this->proxy()->UpdatePropertyInformation();
  vtkSMStringVectorProperty* GetNamesProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->GetProperty("GetGridName"));

  //add each name to the widget and the Domain, and enable each one
  int numNames = GetNamesProperty->GetNumberOfElements();
  for (int i = 0; i < numNames; i++)
    {
    const char* name = GetNamesProperty->GetElement(i);
    QListWidgetItem *anitem = new QListWidgetItem(name, selectionWidget);
    selectionWidget->setItemSelected(anitem, true); //enable in gui

    canChooseFrom->AddString(name);

    EnableProperty->SetElement(0, name); //enable on proxy
    EnableProperty->Modified();
    this->proxy()->UpdateVTKObjects();
    }

  //whenever grid changes, update the available arrays
  this->ResetArrays();
  this->PopulateArrayWidget();

  //remeber what is pressed so we can prevent deselection of all grids
  QObject::connect(selectionWidget, 
                   SIGNAL(itemClicked(QListWidgetItem *)),
                   this, SLOT(RecordLastSelectedGrid(QListWidgetItem *)));

  //watch for changes in the widget so that we can tell the proxy
  QObject::connect(selectionWidget, SIGNAL(itemSelectionChanged()), 
                   this, SLOT(SetSelectedGrids()));

}


//----------------------------------------------------------------------------
void pqXDMFPanel::ResetArrays()
{
  //First, unregister all of the call backs so that we can set the property
  //values in peace.
  pqTreeWidgetItemObject* item;
  vtkSMProperty* registeredProperty;
  QList<pqXDMFPanelArrayRecord>::iterator it;
  for (it = this->ArrayList.begin(); it != this->ArrayList.end(); it++)
    {
    pqXDMFPanelArrayRecord mem = this->ArrayList.takeFirst();
    item = mem.widget;
    if (mem.CorP == 0)
      {
      registeredProperty =
        this->proxy()->GetProperty("CellArrayStatus");
      }
    else
      {
      registeredProperty =
        this->proxy()->GetProperty("PointArrayStatus");
      }
    this->propertyManager()->unregisterLink(item, 
                                            "checked", 
                                            SIGNAL(checkedStateChanged(bool)),
                                            this->proxy(), 
                                            registeredProperty, 
                                            mem.location);
    }

  //tell the servermanager to read what arrays the server has now
  this->proxy()->UpdatePropertyInformation();

  //now copy the names and enabled status for the arrays
  //from the input property to the output property
  vtkSMStringVectorProperty* IProperty;
  vtkSMStringVectorProperty* OProperty;
  vtkSMArraySelectionDomain* ODomain;
  
  IProperty = vtkSMStringVectorProperty::SafeDownCast(
    this->proxy()->GetProperty("PointArrayInfo")
    );
  OProperty = vtkSMStringVectorProperty::SafeDownCast(
    this->proxy()->GetProperty("PointArrayStatus")
    );
  ODomain = vtkSMArraySelectionDomain::SafeDownCast(
    OProperty->GetDomain("array_list")
    );

  OProperty->Copy(IProperty);
  ODomain->Update(IProperty);


  IProperty = vtkSMStringVectorProperty::SafeDownCast(
    this->proxy()->GetProperty("CellArrayInfo")
    );
  OProperty = vtkSMStringVectorProperty::SafeDownCast(
    this->proxy()->GetProperty("CellArrayStatus")
    );
  ODomain = vtkSMArraySelectionDomain::SafeDownCast(
    OProperty->GetDomain("array_list")
    );

  OProperty->Copy(IProperty);
  ODomain->Update(IProperty);

  //clean out the UI to be safe
  QTreeWidget* VariablesTree = this->UI->Variables;
  VariablesTree->clear();

}

//----------------------------------------------------------------------------
void pqXDMFPanel::PopulateArrayWidget()
{
  QPixmap cellPixmap(":/pqWidgets/Icons/pqCellData16.png");
  QPixmap pointPixmap(":/pqWidgets/Icons/pqPointData16.png");

  QTreeWidget* VariablesTree = this->UI->Variables;
  VariablesTree->clear();

  new pqTreeWidgetCheckHelper(VariablesTree, 0, this);
  pqTreeWidgetItemObject* item;
  QList<QString> strs;
  QString varName;

  this->proxy()->UpdatePropertyInformation();

  // do the cell variables
  // uses the input domain and output status to create widgets for each cell
  // array
  vtkSMProperty* CellProperty = 
    this->proxy()->GetProperty("CellArrayStatus");
  QList<QVariant> CellDomain;
  CellDomain = pqSMAdaptor::getSelectionPropertyDomain(CellProperty);
  int j;
  for(j=0; j<CellDomain.size(); j++)
    {
    varName = CellDomain[j].toString();
    strs.clear();
    strs.append(varName);
    item = new pqTreeWidgetItemObject(VariablesTree, strs);
    this->ArrayList.append(pqXDMFPanelArrayRecord(item,0,j));
    item->setData(0, Qt::ToolTipRole, varName);
    item->setData(0, Qt::DecorationRole, cellPixmap);
    this->propertyManager()->registerLink(item, 
                                      "checked", 
                                      SIGNAL(checkedStateChanged(bool)),
                                      this->proxy(), CellProperty, j);
    }

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
    this->ArrayList.append(pqXDMFPanelArrayRecord(item,1,j));
    item->setData(0, Qt::ToolTipRole, varName);
    item->setData(0, Qt::DecorationRole, pointPixmap);
    this->propertyManager()->registerLink(item, 
                                      "checked", 
                                      SIGNAL(checkedStateChanged(bool)),
                                      this->proxy(), NodeProperty, j);
    }
}

//----------------------------------------------------------------------------
void pqXDMFPanel::PopulateParameterWidget()
{
  QTableWidget* paramsContainer = this->UI->Parameters;

  //ask the reader for the list of available Xdmf parameters
  this->proxy()->UpdatePropertyInformation();
  vtkSMStringVectorProperty* GetParameterProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->GetProperty("ParametersInfo"));

  int numParameter = GetParameterProperty->GetNumberOfElements();
  paramsContainer->setRowCount(numParameter/5);
  if (numParameter == 0)
    {
    QLabel* paramsLabel = this->UI->Parameters_label;
    paramsLabel->setText("No Parameters");
    paramsContainer->hide();
    return;
    }

  //add a control for each paramter to the panel
  QSignalMapper* sm = new QSignalMapper(paramsContainer);
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
    const char* name = GetParameterProperty->GetElement(i+0);
    item = new QTableWidgetItem(name);
    item->setFlags(0);
    paramsContainer->setVerticalHeaderItem(row,item);

    const char* firstIdxS = GetParameterProperty->GetElement(i+2);
    item = new QTableWidgetItem(firstIdxS);
    item->setFlags(0);
    paramsContainer->setItem(row,0,item);
    int min = atoi(firstIdxS);

    const char *strideS = GetParameterProperty->GetElement(i+3);
    int stride = atoi(strideS);

    const char *countS = GetParameterProperty->GetElement(i+4);
    int count = atoi(countS);    
    int max = min + stride*count;
    char maxS[20];
    sprintf(maxS, "%d", max);
    item = new QTableWidgetItem(maxS);
    item->setFlags(0);
    paramsContainer->setItem(row,2,item);

    QSpinBox *valBox = new QSpinBox(paramsContainer);
    valBox->setMinimum(min);
    valBox->setMaximum(max);
    valBox->setSingleStep(stride);
    const char* currentValS = GetParameterProperty->GetElement(i+1);
    int val = atoi(currentValS);
    valBox->setValue(val);
    paramsContainer->setCellWidget(row,1,valBox);    

    QObject::connect(valBox, SIGNAL(valueChanged(int)), 
                     sm, SLOT(map()));
    sm->setMapping(valBox, i);
    }

  QObject::connect(sm, SIGNAL(mapped(int)),
                   this, SLOT(SetCellValue(int)));

  paramsContainer->resizeColumnsToContents();
}


//-----------------------------------------------------------------------------
void pqXDMFPanel::SetSelectedDomain(QString newDomain)
{
  //get access to the property that lets us pick the domain
  vtkSMStringVectorProperty* SetNameProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->GetProperty("SetDomainName"));
  SetNameProperty->SetElement(0, newDomain.toAscii());
  this->proxy()->UpdateVTKObjects();

  //when domain changes, update the available grids
  this->PopulateGridWidget();

  //turn on apply/reset button
  this->setModified();
}

//-----------------------------------------------------------------------------
void pqXDMFPanel::RecordLastSelectedGrid(QListWidgetItem *grid)
{
  //this little dance makes sure that when the last grid is unselected
  //we can select it to make sure that are least one grid is selected
  this->LastGridDeselected = grid;
  if (this->NeedsResetGrid)
    {
    this->UI->GridNames->setItemSelected(grid, true);    
    this->NeedsResetGrid = false;
    }
}

//---------------------------------------------------------------------------
void pqXDMFPanel::SetSelectedGrids()
{
  //go through the selections from the gui and turn them on in the server
  QList<QListWidgetItem *> selections = this->UI->GridNames->selectedItems();
  if (selections.size() == 0)
    {
    if (this->LastGridDeselected != NULL)
      {
      this->NeedsResetGrid = true;
      }
    qWarning("At least one grid must be enabled.");
    return;
    }

  //turn off all grids so we can enable only those selected in the gui
  vtkSMProperty* DisableProperty = 
    vtkSMProperty::SafeDownCast(
      this->proxy()->GetProperty("DisableAllGrids"));
  DisableProperty->Modified();
  this->proxy()->UpdateVTKObjects();
  
  //get access to the property that lets us turn on grids
  vtkSMStringVectorProperty* EnableProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->GetProperty("EnableGrid"));

  for (int i = 0; i < selections.size(); i++)
    {
    const char *namestr = ((selections.at(i))->text()).toAscii();
    EnableProperty->SetElement(0, namestr);
    EnableProperty->Modified();
    this->proxy()->UpdateVTKObjects();
    }
  
  //whenever grid changes, update the available arrays
  this->ResetArrays();
  this->PopulateArrayWidget();

  //turn on apply/reset button
  this->setModified();
}
  
//---------------------------------------------------------------------------
void pqXDMFPanel::SetCellValue(int row)
{
  QTableWidget* paramsContainer = this->UI->Parameters;
  QSpinBox *widget = (QSpinBox*)paramsContainer->cellWidget(row,1);
  int value = widget->value();

  vtkSMStringVectorProperty* SetParameterProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->GetProperty("ParameterIndex"));

  vtkSMStringVectorProperty* GetParameterProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->GetProperty("ParametersInfo"));

  char valS[20];
  sprintf(valS, "%d", value);
  int numRows = ((int)GetParameterProperty->GetNumberOfElements())/5;
  for (int i = 0; i < numRows; i++)
    {
    const char *name = GetParameterProperty->GetElement(i*5+0);
    SetParameterProperty->SetElement(i*2+0, name);
    if (i == row)
      {
      SetParameterProperty->SetElement(i*2+1, valS);
      }
    else
      {
      const char *oval = GetParameterProperty->GetElement(i*5+1);
      SetParameterProperty->SetElement(i*2+1, oval);
      }

    }
  this->proxy()->UpdateVTKObjects();
  this->setModified();
}
