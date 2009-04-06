/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ClientTableView.h"

#include <vtkAbstractArray.h>
#include <vtkCommand.h>
#include <vtkConvertSelection.h>
#include <vtkDataObjectTypes.h>
#include <vtkDataRepresentation.h>
#include <vtkDataSetAttributes.h>
#include <vtkGraph.h>
#include <vtkIdTypeArray.h>
#include <vtkIntArray.h>
#include <vtkPVDataInformation.h>
#include <vtkQtTableView.h>
#include <vtkSelection.h>
#include <vtkSelectionLink.h>
#include <vtkSelectionNode.h>
#include <vtkSmartPointer.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMSelectionDeliveryRepresentationProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMViewProxy.h>
#include <vtkTable.h>
#include <vtkVariantArray.h>

#include <pqDataRepresentation.h>
#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqSelectionManager.h>
#include <pqServer.h>

#include <QPointer>
#include <QVBoxLayout>
#include <QWidget>

////////////////////////////////////////////////////////////////////////////////////
// ClientTableView::command

class ClientTableView::command : public vtkCommand
{
public:
  command(ClientTableView& view) : Target(view) { }
  virtual void Execute(vtkObject*, unsigned long, void*)
  {
    Target.selectionChanged();
  }
  ClientTableView& Target;
};

////////////////////////////////////////////////////////////////////////////////////
// ClientTableView::implementation

class ClientTableView::implementation
{
public:
  implementation()
  {
    this->Widget = new QWidget();
    this->View = vtkSmartPointer<vtkQtTableView>::New();
    QVBoxLayout *layout = new QVBoxLayout(this->Widget);
    layout->addWidget(this->View->GetWidget());
    layout->setContentsMargins(0,0,0,0);
    this->AttributeType = -1;
  }

  ~implementation()
  {
    this->View->RemoveAllRepresentations();
    if(this->Widget)
      delete this->Widget;
  }

  int AttributeType;
  vtkSmartPointer<vtkQtTableView> View;
  QPointer<QWidget> Widget;
};

////////////////////////////////////////////////////////////////////////////////////
// ClientTableView

ClientTableView::ClientTableView(
    const QString& viewmoduletype, 
    const QString& group, 
    const QString& name, 
    vtkSMViewProxy* viewmodule, 
    pqServer* server, 
    QObject* p) :
  pqSingleInputView(viewmoduletype, group, name, viewmodule, server, p),
  Implementation(new implementation()),
  Command(new command(*this))
{
  this->Implementation->View->AddObserver(
    vtkCommand::SelectionChangedEvent, this->Command);
  this->Implementation->View->SetSelectionType(vtkSelectionNode::PEDIGREEIDS);
}

ClientTableView::~ClientTableView()
{
  delete this->Implementation;
  this->Command->Delete();
}

QWidget* ClientTableView::getWidget()
{
  return this->Implementation->Widget;
}

void ClientTableView::selectionChanged()
{
  // Get the representaion's source
  pqDataRepresentation* pqRepr =
    qobject_cast<pqDataRepresentation*>(this->visibleRepresentation());
  pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
  vtkSMSourceProxy* repSource = vtkSMSourceProxy::SafeDownCast(
    opPort->getSource()->getProxy());

  // Fill the selection source with the selection from the view
  this->Implementation->View->GetRepresentation()->GetSelectionLink()->Update();
  vtkSelection* sel = this->Implementation->View->GetRepresentation()->
    GetSelectionLink()->GetOutput();
  vtkSMSourceProxy* selectionSource = pqSelectionManager::createSelectionSource(
    sel, repSource->GetConnectionID());

  // Set the selection on the representation's source
  repSource->SetSelectionInput(opPort->getPortNumber(),
    selectionSource, 0);
  selectionSource->Delete();
}

bool ClientTableView::canDisplay(pqOutputPort* output_port) const
{
  if(!output_port)
    return false;

  pqPipelineSource* const source = output_port->getSource();
  if(!source)
    return false;

  if(this->getServer()->GetConnectionID() != source->getServer()->GetConnectionID())
    return false;

  vtkSMSourceProxy* source_proxy =
    vtkSMSourceProxy::SafeDownCast(source->getProxy());
  if (!source_proxy ||
     source_proxy->GetOutputPortsCreated() == 0)
    {
    return false;
    }

  const char* name = output_port->getDataClassName();
  int type = vtkDataObjectTypes::GetTypeIdFromClassName(name);
  switch(type)
    {
    case VTK_DIRECTED_ACYCLIC_GRAPH:
    case VTK_DIRECTED_GRAPH:
    case VTK_GRAPH:
    case VTK_TREE:
    case VTK_UNDIRECTED_GRAPH:
    case VTK_TABLE:
      return true;
    }

  return false;
}

void ClientTableView::updateRepresentation(pqRepresentation* repr)
{
  vtkSMClientDeliveryRepresentationProxy* const proxy = repr ? 
    vtkSMClientDeliveryRepresentationProxy::SafeDownCast(repr->getProxy()) : NULL;
  proxy->Update(vtkSMViewProxy::SafeDownCast(this->getProxy()));  

  vtkDataObject *data = proxy ? proxy->GetOutput() : NULL;
  if (!data)
    {
    return;
    }

  // Add the representation to the view
  this->Implementation->View->SetRepresentationFromInputConnection(proxy->GetOutputPort());
}

void ClientTableView::showRepresentation(pqRepresentation* representation)
{
  this->updateRepresentation(representation);
}

void ClientTableView::hideRepresentation(pqRepresentation* representation)
{
  this->Implementation->View->RemoveAllRepresentations();
}

void ClientTableView::renderInternal()
{
  pqRepresentation* representation = this->visibleRepresentation();
  vtkSMSelectionDeliveryRepresentationProxy* const proxy = representation?
    vtkSMSelectionDeliveryRepresentationProxy::SafeDownCast(representation->getProxy()) : NULL;
  if(!proxy)
    {
    return;
    }

  proxy->Update();

  int attributeType = QString(vtkSMPropertyHelper(proxy, "AttributeType").GetAsString(3)).toInt();

  if (attributeType == vtkDataObject::FIELD_ASSOCIATION_EDGES)
    {
    this->Implementation->View->SetFieldType(vtkQtTableView::EDGE_DATA);
    }
  else if(attributeType == vtkDataObject::FIELD_ASSOCIATION_ROWS)
    {
    this->Implementation->View->SetFieldType(vtkQtTableView::ROW_DATA);
    }
  else
    {
    this->Implementation->View->SetFieldType(vtkQtTableView::VERTEX_DATA);
    }

  if(this->Implementation->View->GetRepresentation())
    {
    proxy->GetSelectionRepresentation()->Update();
    vtkSelection* sel = vtkSelection::SafeDownCast(
      proxy->GetSelectionRepresentation()->GetOutput());
    this->Implementation->View->GetRepresentation()->GetSelectionLink()->SetSelection(sel);
    }

  this->Implementation->View->Update();
}

