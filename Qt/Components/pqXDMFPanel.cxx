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

// we include this for static plugins
#define QT_STATICPLUGIN
#include <QtPlugin>

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

//----------------------------------------------------------------------------
QString pqXDMFPanelInterface::name() const
{
  return "XDMFReader";
}

//----------------------------------------------------------------------------
pqObjectPanel* pqXDMFPanelInterface::createPanel(pqProxy* proxy, QWidget* p)
{
  return new pqXDMFPanel(proxy, p);
}

//----------------------------------------------------------------------------
bool pqXDMFPanelInterface::canCreatePanel(pqProxy* proxy) const
{
  return (QString("sources") == proxy->getProxy()->GetXMLGroup()
    && QString("XdmfReader") == proxy->getProxy()->GetXMLName());
}

Q_EXPORT_PLUGIN(pqXDMFPanelInterface)

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
  this->NeedsResetGrid = 0;
      
  this->linkServerManagerProperties();
}

//----------------------------------------------------------------------------
pqXDMFPanel::~pqXDMFPanel()
{
  this->ArrayList.clear();
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
      this->proxy()->getProxy()->GetProperty("SetDomainName"));
  vtkSMStringListDomain* canChooseFrom = 
    vtkSMStringListDomain::SafeDownCast(
      SetNameProperty->GetDomain("AvailableDomains"));
  //empty it
  canChooseFrom->RemoveAllStrings();

  //ask the reader for the list of available Xdmf-"domain" names
  this->proxy()->getProxy()->UpdatePropertyInformation();
  vtkSMStringVectorProperty* GetNamesProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->getProxy()->GetProperty("GetDomainName"));

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
      this->proxy()->getProxy()->GetProperty("RemoveAllGrids"));
  RemoveProperty->Modified();
  this->proxy()->getProxy()->UpdateVTKObjects();

  //empty the selection widget on the UI (and don't call the changed slot)
  QListWidget* selectionWidget = this->UI->GridNames;
  QObject::disconnect(selectionWidget, SIGNAL(itemSelectionChanged()), 
                      this, SLOT(SetSelectedGrids()));
  selectionWidget->clear();

  //get access to the paraview Domain that restricts what names we can choose
  vtkSMStringVectorProperty* SetNameProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->getProxy()->GetProperty("SetGridName"));
  vtkSMStringListDomain* canChooseFrom = 
    vtkSMStringListDomain::SafeDownCast(
      SetNameProperty->GetDomain("AvailableGrids"));
  //empty it
  canChooseFrom->RemoveAllStrings();

  //get access to the property that turns on/off each grid
  vtkSMStringVectorProperty* EnableProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->getProxy()->GetProperty("EnableGrid"));

  //ask the reader for the list of available Xdmf grid names
  this->proxy()->getProxy()->UpdatePropertyInformation();
  vtkSMStringVectorProperty* GetNamesProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->getProxy()->GetProperty("GetGridName"));

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
    this->proxy()->getProxy()->UpdateVTKObjects();
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
        this->proxy()->getProxy()->GetProperty("CellArrayStatus");
      }
    else
      {
      registeredProperty =
        this->proxy()->getProxy()->GetProperty("PointArrayStatus");
      }
    this->propertyManager()->unregisterLink(item, 
                                            "checked", 
                                            SIGNAL(checkedStateChanged(bool)),
                                            this->proxy()->getProxy(), 
                                            registeredProperty, 
                                            mem.location);
    }

  //tell the servermanager to read what arrays the server has now
  this->proxy()->getProxy()->UpdatePropertyInformation();

  //now copy the names and enabled status for the arrays
  //from the input property to the output property
  vtkSMStringVectorProperty* IProperty;
  vtkSMStringVectorProperty* OProperty;
  vtkSMArraySelectionDomain* ODomain;
  
  IProperty = vtkSMStringVectorProperty::SafeDownCast(
    this->proxy()->getProxy()->GetProperty("PointArrayInfo")
    );
  OProperty = vtkSMStringVectorProperty::SafeDownCast(
    this->proxy()->getProxy()->GetProperty("PointArrayStatus")
    );
  ODomain = vtkSMArraySelectionDomain::SafeDownCast(
    OProperty->GetDomain("array_list")
    );

  OProperty->Copy(IProperty);
  ODomain->Update(IProperty);


  IProperty = vtkSMStringVectorProperty::SafeDownCast(
    this->proxy()->getProxy()->GetProperty("CellArrayInfo")
    );
  OProperty = vtkSMStringVectorProperty::SafeDownCast(
    this->proxy()->getProxy()->GetProperty("CellArrayStatus")
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

  this->proxy()->getProxy()->UpdatePropertyInformation();

  // do the cell variables
  // uses the input domain and output status to create widgets for each cell
  // array
  vtkSMProperty* CellProperty = 
    this->proxy()->getProxy()->GetProperty("CellArrayStatus");
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
                                      this->proxy()->getProxy(), CellProperty, j);
    }

  // do the node variables
  vtkSMProperty* NodeProperty = this->proxy()->getProxy()->GetProperty("PointArrayStatus");
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
                                      this->proxy()->getProxy(), NodeProperty, j);

    }
}

//----------------------------------------------------------------------------
void pqXDMFPanel::PopulateParameterWidget()
{
  //ask the reader for the list of available Xdmf paramaters
  this->proxy()->getProxy()->UpdatePropertyInformation();
  vtkSMStringVectorProperty* GetNamesProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->getProxy()->GetProperty("ParametersInfo"));

  //add a control for each paramter to the panel
  int numNames = GetNamesProperty->GetNumberOfElements();
  //cerr << numNames << endl;
  for (int i = 0; i < numNames; i++)
    {
    //const char* name = GetNamesProperty->GetElement(i);
    //cerr << i << " = " << name << endl;
    }
}


//-----------------------------------------------------------------------------
void pqXDMFPanel::SetSelectedDomain(QString newDomain)
{
  //get access to the property that lets us pick the domain
  vtkSMStringVectorProperty* SetNameProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->getProxy()->GetProperty("SetDomainName"));
  SetNameProperty->SetElement(0, newDomain.toAscii());
  this->proxy()->getProxy()->UpdateVTKObjects();

  //when domain changes, update the available grids
  this->PopulateGridWidget();

  //turn on apply/reset button
  this->modified();
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
    this->NeedsResetGrid = 0;
    }
}

//-----------------------------------------------------------------------------
void pqXDMFPanel::SetSelectedGrids()
{
  //go through the selections from the gui and turn them on in the server
  QList<QListWidgetItem *> selections = this->UI->GridNames->selectedItems();
  if (selections.size() == 0)
    {
    if (this->LastGridDeselected != NULL)
      {
      this->NeedsResetGrid = 1;
      }
    qWarning("At least one grid must be enabled.");
    return;
    }

  //turn off all grids so we can enable only those selected in the gui
  vtkSMProperty* DisableProperty = 
    vtkSMProperty::SafeDownCast(
      this->proxy()->getProxy()->GetProperty("DisableAllGrids"));
  DisableProperty->Modified();
  this->proxy()->getProxy()->UpdateVTKObjects();
  
  //get access to the property that lets us turn on grids
  vtkSMStringVectorProperty* EnableProperty = 
    vtkSMStringVectorProperty::SafeDownCast(
      this->proxy()->getProxy()->GetProperty("EnableGrid"));

  for (int i = 0; i < selections.size(); i++)
    {
    const char *namestr = ((selections.at(i))->text()).toAscii();
    EnableProperty->SetElement(0, namestr);
    EnableProperty->Modified();
    this->proxy()->getProxy()->UpdateVTKObjects();
    }
  
  //whenever grid changes, update the available arrays
  this->ResetArrays();
  this->PopulateArrayWidget();

  //turn on apply/reset button
  this->modified();
}
  
