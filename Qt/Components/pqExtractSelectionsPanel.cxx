/*=========================================================================

   Program: ParaView
   Module:    pqExtractSelectionsPanel.cxx

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
#include "pqExtractSelectionsPanel.h"
#include "ui_pqExtractSelectionsPanel.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkSelection.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"

#include <QtDebug>
#include <QTextStream>
#include <QTimer>

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProxy.h"
#include "pqSelectionManager.h"
#include "pqSMAdaptor.h"

class pqExtractSelectionsPanel::pqInternal : public Ui::ExtractSelectionsPanel
{
public:
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  vtkSmartPointer<vtkSMProxy> SelectionSource;
  vtkSmartPointer<vtkSMProxy> UnacceptedSelectionSource;
  bool SourceChanged;
};

//-----------------------------------------------------------------------------
pqExtractSelectionsPanel::pqExtractSelectionsPanel(pqProxy* _proxy, QWidget* _parent)
  : pqObjectPanel(_proxy, _parent)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  this->Internal->SourceChanged = false;
  this->Internal->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Internal->VTKConnect->Connect(
    _proxy->getProxy()->GetProperty("Selection"), vtkCommand::ModifiedEvent,
    this, SLOT(selectionInputChanged()));

  QObject::connect(this->Internal->pushButtonCopySelection, SIGNAL(clicked()),
    this, SLOT(copyActiveSelection()));

  pqSelectionManager* selMan = qobject_cast<pqSelectionManager*>(
    pqApplicationCore::instance()->manager("SelectionManager"));

  if (selMan)
    {
    QObject::connect(selMan, SIGNAL(selectionChanged(pqOutputPort*)),
      this, SLOT(onActiveSelectionChanged()));
    }

  QTimer::singleShot(10, this, SLOT(selectionInputChanged()));
}

//-----------------------------------------------------------------------------
pqExtractSelectionsPanel::~pqExtractSelectionsPanel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqExtractSelectionsPanel::selectionInputChanged()
{
  vtkSMProxy* filterProxy = this->referenceProxy()->getProxy();
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    filterProxy->GetProperty("Selection"));

  vtkSMProxy* newSelSource = 0;
  if (pp->GetNumberOfProxies() == 1)
    {
    newSelSource = pp->GetProxy(0);
    }

  if (newSelSource == this->Internal->SelectionSource.GetPointer())
    {
    return;
    }

  if (this->Internal->SelectionSource)
    {
    this->Internal->VTKConnect->Disconnect(
      this->Internal->SelectionSource, vtkCommand::PropertyModifiedEvent);
    }
  this->Internal->SelectionSource = newSelSource;
  if (newSelSource)
    {
    this->Internal->VTKConnect->Connect(
      this->Internal->SelectionSource, vtkCommand::PropertyModifiedEvent,
      this, SLOT(updateLabels()));
    }

  QTimer::singleShot(10, this, SLOT(updateLabels()));
}

//-----------------------------------------------------------------------------
void pqExtractSelectionsPanel::updateLabels()
{
  vtkSMProxy* selectionSource = this->Internal->UnacceptedSelectionSource?
    this->Internal->UnacceptedSelectionSource : this->Internal->SelectionSource;

  if (!selectionSource)
    {
    this->Internal->label->setText("No selection");
    return;
    }

  this->Internal->label->setText("Copied Selection");

  int fieldType = pqSMAdaptor::getElementProperty(
    selectionSource->GetProperty("FieldType")).toInt();
 
  const char* xmlname = selectionSource->GetXMLName();
  QString text = QString("Type: ");
  QTextStream columnValues(&text, QIODevice::ReadWrite);
  if(strcmp(xmlname, "FrustumSelectionSource") == 0)
    {  
    columnValues << "Frustum Selection" << endl << endl << "Values:" << endl ;
    vtkSMProperty* dvp = selectionSource->GetProperty("Frustum");
    QList<QVariant> value = pqSMAdaptor::getMultipleElementProperty(dvp);
    for (int cc=0; cc <value.size(); cc++)
      {
      if((cc%4) == 0)
        {
        columnValues << endl;
        }
      columnValues << value[cc].toDouble() << "\t";
      }
    }
  else if (strcmp(xmlname, "GlobalIDSelectionSource") == 0)
    {
    columnValues << "Global ID Selection" << endl << endl << endl;
    QList<QVariant> value = pqSMAdaptor::getMultipleElementProperty(
      selectionSource->GetProperty("IDs"));
    columnValues << "Global ID" << endl;
    foreach (QVariant val, value)
      {
      columnValues << val.toString() << endl;
      }
    }
  else if(strcmp(xmlname, "IDSelectionSource") == 0)
    {
    columnValues << QString("%1 ID Selection").arg(
      fieldType == 0? "Cell" : "Point") << endl << endl << endl;
    columnValues << "Process ID" << "\t\t" << "Index" << endl;
    vtkSMProperty* idvp = selectionSource->GetProperty("IDs");
    QList<QVariant> value = pqSMAdaptor::getMultipleElementProperty(idvp);
    for (int cc=0; cc <value.size(); cc++)
      {
      if((cc%2) == 0)
        {
        columnValues << endl;
        }
      columnValues << value[cc].toString() << "\t\t";
      }
    }
  else if(strcmp(xmlname, "CompositeDataIDSelectionSource") == 0)
    {
    columnValues << QString("%1 ID Selection").arg(
      fieldType == 0? "Cell" : "Point") << endl << endl << endl;
    columnValues  << "Composite ID" 
      << "\t" << "Process ID" << "\t\t" << "Index" << endl;
    vtkSMProperty* idvp = selectionSource->GetProperty("IDs");
    QList<QVariant> value = pqSMAdaptor::getMultipleElementProperty(idvp);
    for (int cc=0; cc <value.size(); cc++)
      {
      if((cc%3) == 0)
        {
        columnValues << endl;
        }
      columnValues << value[cc].toString() << "\t\t";
      }
    }
  else if(strcmp(xmlname, "HierarchicalDataIDSelectionSource") == 0)
    {
    columnValues << QString("%1 ID Selection").arg(
      fieldType == 0? "Cell" : "Point") << endl << endl << endl;
    columnValues  << "Level" << "\t\t" << "Dataset" << "\t\t" 
      << "Index" << endl;
    vtkSMProperty* idvp = selectionSource->GetProperty("IDs");
    QList<QVariant> value = pqSMAdaptor::getMultipleElementProperty(idvp);
    for (int cc=0; cc <value.size(); cc++)
      {
      if((cc%3) == 0)
        {
        columnValues << endl;
        }
      columnValues << value[cc].toString() << "\t\t";
      }
    }
  else if (strcmp(xmlname, "LocationSelectionSource") == 0)
    {
    columnValues << "Location-based Selection" << endl << endl << endl;
    columnValues << "Probe Locations" << endl;
    vtkSMProperty* idvp = selectionSource->GetProperty("Locations");
    QList<QVariant> value = pqSMAdaptor::getMultipleElementProperty(idvp);
    for (int cc=0; cc <value.size(); cc++)
      {
      if((cc%3) == 0)
        {
        columnValues << endl;
        }
      columnValues << value[cc].toString() << "\t\t";
      }
    }
  else if (strcmp(xmlname, "BlockSelectionSource") == 0)
    {
    columnValues << "Block Selection" << endl << endl << endl;
    columnValues << "Blocks" << endl;
    vtkSMProperty* prop = selectionSource->GetProperty("Blocks");
    QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(prop);
    foreach (const QVariant& value, values)
      {
      columnValues << value.toString() << endl;
      }
    }
  else
    {
    columnValues << "None" << endl;
    }

  this->Internal->textBrowser->setText(text);
  columnValues.flush();
}

//-----------------------------------------------------------------------------
// This will update the UnacceptedSelectionSource proxy with a clone of the
// active selection proxy. The filter proxy is not modified yet.
void pqExtractSelectionsPanel::copyActiveSelection()
{
  pqSelectionManager* selMan = (pqSelectionManager*)(
    pqApplicationCore::instance()->manager("SelectionManager"));

  if (!selMan)
    {
    qDebug() << "No selection manager was detected. "
      "Don't know where to get the active selection from.";
    return;
    }

  pqOutputPort* port = selMan->getSelectedPort();
  if (!port)
    {
    return;
    }

  vtkSMProxy* activeSelection = port->getSelectionInput();
  if (!activeSelection)
    {
    return;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (!this->Internal->UnacceptedSelectionSource ||
     strcmp(this->Internal->UnacceptedSelectionSource->GetXMLName(),
       activeSelection->GetXMLName()) != 0)
    {
    vtkSMProxy* newSource = pxm->NewProxy(activeSelection->GetXMLGroup(),
      activeSelection->GetXMLName());
    newSource->SetConnectionID(activeSelection->GetConnectionID());
    this->Internal->UnacceptedSelectionSource = newSource;
    newSource->Delete();
    }

  this->Internal->UnacceptedSelectionSource->Copy(activeSelection, 
    0, vtkSMProxy::COPY_PROXY_PROPERTY_VALUES_BY_CLONING);
  this->updateLabels();

  this->setModified();
}

//-----------------------------------------------------------------------------
void pqExtractSelectionsPanel::onActiveSelectionChanged()
{
  // The selection has changed, either a new selection was created
  // or an old one cleared.
  this->Internal->label->setText("Copied Selection (Active Selection Changed)");
}

//-----------------------------------------------------------------------------
void pqExtractSelectionsPanel::reset()
{
  this->Internal->UnacceptedSelectionSource = 0;
  this->updateLabels();
  this->Superclass::reset();
}

//-----------------------------------------------------------------------------
void pqExtractSelectionsPanel::accept()
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMProxy* filterProxy = this->referenceProxy()->getProxy();
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    filterProxy->GetProperty("Selection"));

  // remove current selection input.
  if (pp->GetNumberOfProxies() != 0)
    {
    vtkSMProxy* curSelInput = pp->GetProxy(0);
    // if curSelInput is registered under the "selection_sources" group, then we
    // should unregister it.
    const char* name = pxm->GetProxyName("selection_sources", curSelInput);
    if (name)
      {
      pxm->UnRegisterProxy("selection_sources", name, curSelInput);
      }
    }

  pp->RemoveAllProxies();
  if (this->Internal->UnacceptedSelectionSource)
    {
    pxm->RegisterProxy("selection_sources", 
      this->Internal->UnacceptedSelectionSource->GetSelfIDAsString(),
      this->Internal->UnacceptedSelectionSource);
    this->Internal->UnacceptedSelectionSource->UpdateVTKObjects();
    pp->AddProxy(this->Internal->UnacceptedSelectionSource); 
    }

  filterProxy->UpdateVTKObjects();
  this->Superclass::accept();
}
