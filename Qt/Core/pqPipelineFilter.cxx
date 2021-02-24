/*=========================================================================

   Program: ParaView
   Module:    pqPipelineFilter.cxx

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

/// \file pqPipelineFilter.cxx
/// \date 4/17/2006

#include "pqPipelineFilter.h"

// ParaView Server Manager includes.
#include "vtkEventQtSlotConnect.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMInputProperty.h"
#include "vtkSMOrderedPropertyIterator.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSmartPointer.h"

// Qt includes.
#include <QList>
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QtDebug>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqServerManagerModel.h"

uint qHash(QPair<QPointer<pqPipelineSource>, int> arg)
{
  return qHash(static_cast<pqPipelineSource*>(arg.first)) + qHash(arg.second);
}

//-----------------------------------------------------------------------------
class pqPipelineFilter::pqInternal
{
public:
  typedef QList<QPointer<pqOutputPort> > InputList;
  typedef QMap<QString, InputList> InputMap;
  InputMap Inputs;

  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  pqInternal() { this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New(); }
};

//-----------------------------------------------------------------------------
pqPipelineFilter::pqPipelineFilter(
  QString name, vtkSMProxy* proxy, pqServer* server, QObject* p /*=nullptr*/)
  : pqPipelineSource(name, proxy, server, p)
{
  this->Internal = new pqInternal();
  QList<const char*> inputPortNames = pqPipelineFilter::getInputPorts(proxy);
  foreach (const char* pname, inputPortNames)
  {
    this->Internal->Inputs.insert(pname, pqInternal::InputList());

    vtkSMProperty* inputProp = proxy->GetProperty(pname);

    // Listen to proxy events to get input changes.
    this->Internal->VTKConnect->Connect(inputProp, vtkCommand::ModifiedEvent, this,
      SLOT(inputChanged(vtkObject*, unsigned long, void*)), const_cast<char*>(pname));
  }
}

static void pqPipelineFilterGetInputProperties(
  QList<const char*>& list, vtkSMProxy* proxy, bool skip_optional)
{
  vtkSMOrderedPropertyIterator* propIter = vtkSMOrderedPropertyIterator::New();
  propIter->SetProxy(proxy);
  for (propIter->Begin(); !propIter->IsAtEnd(); propIter->Next())
  {
    vtkSMInputProperty* inputProp = vtkSMInputProperty::SafeDownCast(propIter->GetProperty());
    if (inputProp)
    {
      vtkPVXMLElement* hints = inputProp->GetHints();
      if (hints && (hints->FindNestedElementByName("NoGUI") ||
                     hints->FindNestedElementByName("SelectionInput")))
      {
        // hints suggest that this input property is not to be considered by the
        // GUI.
        continue;
      }

      if (hints && skip_optional && hints->FindNestedElementByName("Optional"))
      {
        // skip optional inputs.
        continue;
      }

      bool has_plist_domain = false;

      // Ensure that this property does not have a proxy list domain. If it
      // does, we don't treat it as an input.
      vtkSmartPointer<vtkSMDomainIterator> domainIter;
      domainIter.TakeReference(inputProp->NewDomainIterator());
      for (domainIter->Begin(); !domainIter->IsAtEnd(); domainIter->Next())
      {
        if (domainIter->GetDomain()->IsA("vtkSMProxyListDomain"))
        {
          has_plist_domain = true;
          break;
        }
      }

      if (has_plist_domain)
      {
        continue;
      }

      const char* pname = propIter->GetKey();
      if (!list.contains(pname))
      {
        list.push_back(pname);
      }
    }
  }
  propIter->Delete();
}

//-----------------------------------------------------------------------------
QList<const char*> pqPipelineFilter::getInputPorts(vtkSMProxy* proxy)
{
  QList<const char*> list;
  pqPipelineFilterGetInputProperties(list, proxy, false);
  return list;
}

//-----------------------------------------------------------------------------
QList<const char*> pqPipelineFilter::getRequiredInputPorts(vtkSMProxy* proxy)
{
  QList<const char*> list;
  pqPipelineFilterGetInputProperties(list, proxy, true);
  return list;
}

//-----------------------------------------------------------------------------
void pqPipelineFilter::initialize()
{
  this->Superclass::initialize();

  // Now update the input connections.
  foreach (QString portname, this->Internal->Inputs.keys())
  {
    this->inputChanged(portname);
  }
}

//-----------------------------------------------------------------------------
pqPipelineFilter::~pqPipelineFilter()
{
  pqInternal::InputMap::iterator mapIter;
  for (mapIter = this->Internal->Inputs.begin(); mapIter != this->Internal->Inputs.end(); ++mapIter)
  {
    foreach (pqOutputPort* opPort, mapIter.value())
    {
      if (opPort)
      {
        opPort->removeConsumer(this);
      }
    }
  }

  delete this->Internal;
}

//-----------------------------------------------------------------------------
int pqPipelineFilter::getNumberOfInputPorts() const
{
  return this->Internal->Inputs.size();
}

//-----------------------------------------------------------------------------
QString pqPipelineFilter::getInputPortName(int index) const
{
  if (index < 0 || index >= this->Internal->Inputs.size())
  {
    qCritical() << "Invalid input port index : " << index
                << ". Available number of input ports : " << this->Internal->Inputs.size();
    return QString();
  }

  return this->Internal->Inputs.keys()[index];
}

//-----------------------------------------------------------------------------
int pqPipelineFilter::getNumberOfInputs(const QString& portname) const
{
  pqInternal::InputMap::iterator iter = this->Internal->Inputs.find(portname);
  if (iter == this->Internal->Inputs.end())
  {
    qCritical() << "Invalid input port name: " << portname;
    return 0;
  }

  return iter.value().size();
}

//-----------------------------------------------------------------------------
QList<pqOutputPort*> pqPipelineFilter::getAllInputs() const
{
  QList<pqOutputPort*> list;

  foreach (const pqInternal::InputList& inputs, this->Internal->Inputs)
  {
    for (int cc = 0; cc < inputs.size(); cc++)
    {
      if (inputs[cc] && !list.contains(inputs[cc]))
      {
        list.push_back(inputs[cc]);
      }
    }
  }

  return list;
}

//-----------------------------------------------------------------------------
QList<pqOutputPort*> pqPipelineFilter::getInputs(const QString& portname) const
{
  QList<pqOutputPort*> list;

  pqInternal::InputMap::iterator iter = this->Internal->Inputs.find(portname);
  if (iter == this->Internal->Inputs.end())
  {
    qCritical() << "Invalid input port name: " << portname;
    return list;
  }

  foreach (pqOutputPort* port, iter.value())
  {
    if (port)
    {
      list.push_back(port);
    }
  }

  return list;
}

//-----------------------------------------------------------------------------
QMap<QString, QList<pqOutputPort*> > pqPipelineFilter::getNamedInputs() const
{
  QMap<QString, QList<pqOutputPort*> > map;

  pqInternal::InputMap::iterator iter = this->Internal->Inputs.begin();
  for (; iter != this->Internal->Inputs.end(); ++iter)
  {
    QList<pqOutputPort*>& list = map[iter.key()];

    foreach (pqOutputPort* port, iter.value())
    {
      if (port)
      {
        list.push_back(port);
      }
    }
  }
  return map;
}

//-----------------------------------------------------------------------------
pqOutputPort* pqPipelineFilter::getInput(const QString& portname, int index) const
{
  pqInternal::InputMap::iterator iter = this->Internal->Inputs.find(portname);
  if (iter == this->Internal->Inputs.end())
  {
    qCritical() << "Invalid input port name: " << portname;
    return nullptr;
  }

  if (index < 0 || index >= iter.value().size())
  {
    qCritical() << "Invalid index: " << index;
    return nullptr;
  }

  return iter.value()[index];
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqPipelineFilter::getInput(int index) const
{
  pqOutputPort* op = this->getInput(this->getInputPortName(0), index);
  return (op ? op->getSource() : nullptr);
}

//-----------------------------------------------------------------------------
pqOutputPort* pqPipelineFilter::getAnyInput() const
{
  for (int i = 0; i < this->getNumberOfInputPorts(); i++)
  {
    QString portName = this->getInputPortName(i);
    pqInternal::InputMap::iterator iter = this->Internal->Inputs.find(portName);
    if (iter != this->Internal->Inputs.end() && iter.value().size() > 0)
    {
      return iter.value()[0];
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void pqPipelineFilter::inputChanged(vtkObject*, unsigned long, void* client_data)
{
  const char* pname = reinterpret_cast<const char*>(client_data);
  this->inputChanged(pname);
}

//-----------------------------------------------------------------------------
static void pqPipelineFilterBuildInputList(vtkSMInputProperty* ivp, QSet<pqOutputPort*>& set)
{
  pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();

  unsigned int max = ivp->GetNumberOfProxies();
  for (unsigned int cc = 0; cc < max; cc++)
  {
    vtkSMProxy* proxy = ivp->GetProxy(cc);
    if (!proxy)
    {
      continue;
    }
    pqPipelineSource* pqSrc = model->findItem<pqPipelineSource*>(proxy);
    if (!pqSrc)
    {
      qCritical() << "Some proxy is added as input but was not registered with"
                  << " Proxy Manager. This is not recommended.";
      continue;
    }
    set.insert(pqSrc->getOutputPort(ivp->GetOutputPortForConnection(cc)));
  }
}

//-----------------------------------------------------------------------------
void pqPipelineFilter::inputChanged(const QString& portname)
{
  pqInternal::InputMap::iterator iter = this->Internal->Inputs.find(portname);
  if (iter == this->Internal->Inputs.end())
  {
    qCritical() << "Invalid input port name: " << portname;
    return;
  }

  vtkSMInputProperty* ivp =
    vtkSMInputProperty::SafeDownCast(this->getProxy()->GetProperty(portname.toLocal8Bit().data()));
  if (!ivp)
  {
    qCritical() << "Failed to locate input property " << portname;
    return;
  }

  // We must determine what changed on the input property.
  // Remember that all proxy that are added to the input property are
  // must already have pqPipelineFilter associated with them. If not, the input proxy was not
  // registered with the SM and as far as we are concerned, does not even exist :).

  QSet<pqOutputPort*> oldInputs;
  foreach (pqOutputPort* obj, iter.value())
  {
    oldInputs.insert(obj);
  }

  QSet<pqOutputPort*> currentInputs;
  pqPipelineFilterBuildInputList(ivp, currentInputs);

  QSet<pqOutputPort*> removed = oldInputs - currentInputs;
  QSet<pqOutputPort*> added = currentInputs - oldInputs;

  // To preserve the order, we do this funny computation to sync the inputs list.
  foreach (pqOutputPort* obj, removed)
  {
    iter.value().removeAll(obj);
  }
  foreach (pqOutputPort* obj, added)
  {
    iter.value().push_back(obj);
  }

  // Now tell pqPipelineSource in removed list that we are no longer their
  // descendent.
  foreach (pqOutputPort* obj, removed)
  {
    obj->removeConsumer(this);
  }

  foreach (pqOutputPort* obj, added)
  {
    obj->addConsumer(this);
  }

  // The pqPipelineSource whose consumer changes raises the events when the
  // consumer is removed added, so we don't need to raise any events here to
  // let the world know that connections were broken/created.
  Q_EMIT this->producerChanged(portname);
}

//-----------------------------------------------------------------------------
int pqPipelineFilter::replaceInput() const
{
  vtkSMProxy* proxy = this->getProxy();
  if (!proxy)
  {
    return 1;
  }

  vtkPVXMLElement* hints = proxy->GetHints();
  if (!hints)
  {
    return 1;
  }
  for (unsigned int cc = 0; cc < hints->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* child = hints->GetNestedElement(cc);
    if (!child || !child->GetName() || strcmp(child->GetName(), "Visibility") != 0)
    {
      continue;
    }
    int replace_input = 1;
    if (!child->GetScalarAttribute("replace_input", &replace_input))
    {
      continue;
    }
    return replace_input;
  }
  return 1; // default value.
}
