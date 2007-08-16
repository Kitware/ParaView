/*=========================================================================

   Program: ParaView
   Module:    pqExtractSelectionsPanel.cxx

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
#include "pqExtractSelectionsPanel.h"
#include "ui_pqExtractSelectionsPanel.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSelection.h"
#include "vtkSMExtractSelectionsProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <QtDebug>
#include <QTextStream>

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqPropertyManager.h"
#include "pqProxy.h"
#include "pqSelectionManager.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"

class pqExtractSelectionsPanel::pqInternal : public Ui::ExtractSelectionsPanel
{
public:

  vtkSmartPointer<vtkSMSourceProxy> SelectionSource;

};

//-----------------------------------------------------------------------------
pqExtractSelectionsPanel::pqExtractSelectionsPanel(pqProxy* _proxy, QWidget* _parent)
  : pqObjectPanel(_proxy, _parent)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  
  QObject::connect(this->Internal->pushButtonCopySelection, SIGNAL(clicked()),
    this, SLOT(copyActiveSelection()));

  this->linkServerManagerProperties();
  this->referenceProxy()->setModifiedState(pqProxy::UNMODIFIED);
}

//-----------------------------------------------------------------------------
pqExtractSelectionsPanel::~pqExtractSelectionsPanel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqExtractSelectionsPanel::linkServerManagerProperties()
{
  vtkSMProxyManager* pm = vtkSMProxy::GetProxyManager();
  this->Internal->SelectionSource.TakeReference(
    vtkSMSourceProxy::SafeDownCast(
    pm->NewProxy("sources", "SelectionSource")));

  pqSelectionManager* selMan = 
    (pqSelectionManager*)(pqApplicationCore::instance()
    ->manager("SelectionManager"));

  if (selMan)
    {
    QObject::connect(selMan, SIGNAL(selectionChanged(pqSelectionManager*)),
      this, SLOT(onActiveSelectionChanged()));
    }

}

//-----------------------------------------------------------------------------
void pqExtractSelectionsPanel::updateLabels()
{
  this->Internal->label->setText("Copied Selection");
  vtkSMSourceProxy* selectionSource = 
    this->Internal->SelectionSource.GetPointer();
  vtkSMProperty* ivp = selectionSource->GetProperty("ContentType");

  if(!ivp)
    {
    return;
    }
  int type = pqSMAdaptor::getElementProperty(ivp).toInt();
  
  QString text = QString("Type: ");
  QTextStream columnValues(&text, QIODevice::ReadWrite);
  if(type == vtkSelection::FRUSTUM)
    {  
    columnValues << "Frustum" << endl << endl << "Values:" << endl ;
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
  else if(type == vtkSelection::GLOBALIDS || type == vtkSelection::INDICES)
    {
    columnValues << "Surface" << endl << endl << endl;
    if(type == vtkSelection::INDICES)
      {
      columnValues << "Process ID" << "\t\t" << "Index" << endl;
      }
    else
      {
      columnValues << "Process ID" << "\t\t" << "Global ID" << endl;
      }
    vtkSMProperty* idvp = selectionSource->GetProperty("IDs");
    QList<QVariant> value = pqSMAdaptor::getMultipleElementProperty(idvp);

    for (int cc=0; cc <value.size(); cc++)
      {
      if((cc%2) == 0)
        {
        columnValues << endl;
        }
      columnValues << value[cc].toInt() << "\t\t";
      }
    }
  else
    {
    columnValues << "None" << endl;
    }
  //QString text = columnValues.string()->toAscii();
  this->Internal->textBrowser->setText(text);
  columnValues.flush();
}

//-----------------------------------------------------------------------------
void pqExtractSelectionsPanel::copyActiveSelection()
{
  pqSelectionManager* selMan = (pqSelectionManager*)(
    pqApplicationCore::instance()->manager("SelectionManager"));

  if (!selMan)
    {
    return;
    }

  pqOutputPort* port = selMan->getSelectedPort();
  if(!port || !port->getSource())
    {
    //this->setSelectionSource(0);
    return;
    }
  pqPipelineSource* input = port->getSource();

  vtkSMSourceProxy *inputsrc = 
    vtkSMSourceProxy::SafeDownCast(input->getProxy());

  if(!inputsrc)
    {
    //this->setSelectionSource(0);
    return;
    }
  this->setSelectionSource(inputsrc->GetSelectionInput(
    port->getPortNumber()));
}

//-----------------------------------------------------------------------------
void pqExtractSelectionsPanel::onActiveSelectionChanged()
{
  // The selection has changed, either a new selection was created
  // or an old one cleared.
  this->Internal->label->setText("Copied Selection (Active Selection Changed)");
}

//-----------------------------------------------------------------------------
void pqExtractSelectionsPanel::updateInformationAndDomains()
{
  this->Superclass::updateInformationAndDomains();
}

//-----------------------------------------------------------------------------
void pqExtractSelectionsPanel::setSelectionSource(vtkSMSourceProxy *selSource)
{
  if(selSource)
    {
    this->Internal->SelectionSource.GetPointer()->Copy(selSource, 
      0, vtkSMProxy::COPY_PROXY_PROPERTY_VALUES_BY_CLONING);
    this->updateLabels();
    this->referenceProxy()->setModifiedState(pqProxy::MODIFIED);
    }
}

//-----------------------------------------------------------------------------
void pqExtractSelectionsPanel::accept()
{
  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(
    this->referenceProxy());
  vtkSMExtractSelectionsProxy* smproxy = vtkSMExtractSelectionsProxy::SafeDownCast(
    filter->getProxy());

  if(smproxy)
    {
    smproxy->CopySelectionSource(this->Internal->SelectionSource.GetPointer());
    }

  this->referenceProxy()->setModifiedState(pqProxy::UNMODIFIED);
}
