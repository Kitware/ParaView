/*=========================================================================

   Program: ParaView
   Module:    pqPipelineSource.cxx

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

/// \file pqPipelineSource.cxx
/// \date 4/17/2006

#include "pqPipelineSource.h"

// ParaView Server Manager.
#include "vtkClientServerStream.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"

// Qt
#include <QList>
#include <QMap>
#include <QtDebug>

// ParaView
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqTimeKeeper.h"
#include "pqUndoStack.h"
#include "pqXMLUtil.h"

//-----------------------------------------------------------------------------
class pqPipelineSourceInternal
{
public:
  vtkSmartPointer<vtkSMProxy> Proxy;

  QString Name;
  QList<pqOutputPort*> OutputPorts;

  QList<vtkSmartPointer<vtkSMPropertyLink> > Links;
  QList<vtkSmartPointer<vtkSMProxy> > ProxyListDomainProxies;

  pqPipelineSourceInternal(QString name, vtkSMProxy* proxy)
  {
    this->Name = name;
    this->Proxy = proxy;
  }
};

//-----------------------------------------------------------------------------
pqPipelineSource::pqPipelineSource(
  const QString& name, vtkSMProxy* proxy, pqServer* server, QObject* _parent /*=NULL*/)
  : pqProxy("sources", name, proxy, server, _parent)
{
  this->Internal = new pqPipelineSourceInternal(name, proxy);
  vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(this->getProxy());
  if (source)
  {
    // Obtain information about number of output ports.
    // Number of output ports is valid even if the output port proxies haven't
    // been created (so long as CreateVTKObjects() has been called on the
    // proxy).
    source->UpdateVTKObjects(); // causes CreateVTKObjects() to be called.
    int numports = source->GetNumberOfOutputPorts();

    for (int cc = 0; cc < numports; cc++)
    {
      pqOutputPort* op = new pqOutputPort(this, cc);

      // Relay all signals fired by the output ports
      QObject::connect(op, SIGNAL(connectionAdded(pqOutputPort*, pqPipelineSource*)), this,
        SLOT(portConnectionAdded(pqOutputPort*, pqPipelineSource*)));
      QObject::connect(op, SIGNAL(preConnectionAdded(pqOutputPort*, pqPipelineSource*)), this,
        SLOT(prePortConnectionAdded(pqOutputPort*, pqPipelineSource*)));
      QObject::connect(op, SIGNAL(connectionRemoved(pqOutputPort*, pqPipelineSource*)), this,
        SLOT(portConnectionRemoved(pqOutputPort*, pqPipelineSource*)));
      QObject::connect(op, SIGNAL(preConnectionRemoved(pqOutputPort*, pqPipelineSource*)), this,
        SLOT(prePortConnectionRemoved(pqOutputPort*, pqPipelineSource*)));

      QObject::connect(op, SIGNAL(representationAdded(pqOutputPort*, pqDataRepresentation*)), this,
        SLOT(portRepresentationAdded(pqOutputPort*, pqDataRepresentation*)));
      QObject::connect(op, SIGNAL(representationRemoved(pqOutputPort*, pqDataRepresentation*)),
        this, SLOT(portRepresentationRemoved(pqOutputPort*, pqDataRepresentation*)));
      QObject::connect(op, SIGNAL(visibilityChanged(pqOutputPort*, pqDataRepresentation*)), this,
        SLOT(portVisibilityChanged(pqOutputPort*, pqDataRepresentation*)));

      this->Internal->OutputPorts.push_back(op);
    }
    this->getConnector()->Connect(source, vtkCommand::UpdateDataEvent, this, SLOT(dataUpdated()));
    this->getConnector()->Connect(source, vtkCommand::SelectionChangedEvent, this,
      SLOT(onSelectionChanged(vtkObject*, unsigned long, void*, void*, vtkCommand*)));
  }
}

//-----------------------------------------------------------------------------
pqPipelineSource::~pqPipelineSource()
{
  foreach (pqOutputPort* opport, this->Internal->OutputPorts)
  {
    delete opport;
  }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMSourceProxy* pqPipelineSource::getSourceProxy()
{
  return vtkSMSourceProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
void pqPipelineSource::updatePipeline()
{
  pqTimeKeeper* timekeeper = this->getServer()->getTimeKeeper();
  double time = timekeeper->getTime();
  vtkSMSourceProxy::SafeDownCast(this->getProxy())->UpdatePipeline(time);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::dataUpdated()
{
  emit this->dataUpdated(this);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::onSelectionChanged(
  vtkObject*, unsigned long, void* vtkNotUsed(client_data), void* call_data, vtkCommand*)
{
  unsigned int portIndex = *reinterpret_cast<unsigned int*>(call_data);
  if (this->getSourceProxy())
  {
    auto port = this->getOutputPort(static_cast<int>(portIndex));
    if (port)
    {
      emit selectionChanged(port);
    }
  }
}

//-----------------------------------------------------------------------------
void pqPipelineSource::prePortConnectionAdded(pqOutputPort* op, pqPipelineSource* cons)
{
  emit this->preConnectionAdded(this, cons, op->getPortNumber());
}

//-----------------------------------------------------------------------------
void pqPipelineSource::portConnectionAdded(pqOutputPort* op, pqPipelineSource* cons)
{
  emit this->connectionAdded(this, cons, op->getPortNumber());
}

//-----------------------------------------------------------------------------
void pqPipelineSource::prePortConnectionRemoved(pqOutputPort* op, pqPipelineSource* cons)
{
  emit this->preConnectionRemoved(this, cons, op->getPortNumber());
}
//-----------------------------------------------------------------------------
void pqPipelineSource::portConnectionRemoved(pqOutputPort* op, pqPipelineSource* cons)
{
  emit this->connectionRemoved(this, cons, op->getPortNumber());
}

//-----------------------------------------------------------------------------
void pqPipelineSource::portRepresentationAdded(pqOutputPort* op, pqDataRepresentation* cons)
{
  emit this->representationAdded(this, cons, op->getPortNumber());
}

//-----------------------------------------------------------------------------
void pqPipelineSource::portRepresentationRemoved(pqOutputPort* op, pqDataRepresentation* cons)
{
  emit this->representationRemoved(this, cons, op->getPortNumber());
}

//-----------------------------------------------------------------------------
void pqPipelineSource::portVisibilityChanged(
  pqOutputPort* vtkNotUsed(op), pqDataRepresentation* cons)
{
  emit this->visibilityChanged(this, cons);
}

//-----------------------------------------------------------------------------
int pqPipelineSource::getNumberOfOutputPorts() const
{
  return this->Internal->OutputPorts.size();
}

//-----------------------------------------------------------------------------
QList<pqOutputPort*> pqPipelineSource::getOutputPorts() const
{
  QList<pqOutputPort*> ports;
  foreach (pqOutputPort* port, this->Internal->OutputPorts)
  {
    ports << port;
  }
  return ports;
}

//-----------------------------------------------------------------------------
pqOutputPort* pqPipelineSource::getOutputPort(int outputport) const
{
  if (outputport < 0 || outputport >= this->Internal->OutputPorts.size())
  {
    qCritical() << "Invalid output port : pqPipelineSource::getOutputPort(" << outputport
                << "). Available number of output ports: " << this->Internal->OutputPorts.size();
    return NULL;
  }
  return this->Internal->OutputPorts[outputport];
}

//-----------------------------------------------------------------------------
pqOutputPort* pqPipelineSource::getOutputPort(const QString& name) const
{
  vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(this->getProxy());
  unsigned int index = source->GetOutputPortIndex(name.toLocal8Bit().data());
  if (index != VTK_UNSIGNED_INT_MAX)
  {
    return this->getOutputPort(static_cast<int>(index));
  }

  return 0;
}

//-----------------------------------------------------------------------------
int pqPipelineSource::getNumberOfConsumers(int outputport) const
{
  if (outputport < 0 || outputport >= this->Internal->OutputPorts.size())
  {
    return 0;
  }

  return this->Internal->OutputPorts[outputport]->getNumberOfConsumers();
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqPipelineSource::getConsumer(int outputport, int index) const
{
  if (outputport < 0 || outputport >= this->Internal->OutputPorts.size())
  {
    qCritical() << "Invalid output port : pqPipelineSource::getConsumer(" << outputport << ", "
                << index
                << "). Available number of output ports: " << this->Internal->OutputPorts.size();
    return NULL;
  }

  return this->Internal->OutputPorts[outputport]->getConsumer(index);
}

//-----------------------------------------------------------------------------
QList<pqPipelineSource*> pqPipelineSource::getAllConsumers() const
{
  QList<pqPipelineSource*> consumers;
  foreach (pqOutputPort* port, this->Internal->OutputPorts)
  {
    QList<pqPipelineSource*> portConsumers = port->getConsumers();
    for (int cc = 0; cc < portConsumers.size(); cc++)
    {
      if (!consumers.contains(portConsumers[cc]))
      {
        consumers.push_back(portConsumers[cc]);
      }
    }
  }
  return consumers;
}

//-----------------------------------------------------------------------------
void pqPipelineSource::onRepresentationVisibilityChanged()
{
  emit this->visibilityChanged(this, qobject_cast<pqDataRepresentation*>(this->sender()));
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqPipelineSource::getRepresentation(int outputport, pqView* view) const
{
  if (outputport < 0 || outputport >= this->Internal->OutputPorts.size())
  {
    qCritical() << "Invalid output port : pqPipelineSource::getRepresentation(" << outputport
                << ", view)"
                << ". Available number of output ports: " << this->Internal->OutputPorts.size();
    return 0;
  }
  return this->Internal->OutputPorts[outputport]->getRepresentation(view);
}

//-----------------------------------------------------------------------------
QList<pqDataRepresentation*> pqPipelineSource::getRepresentations(
  int outputport, pqView* view) const
{
  if (outputport < 0 || outputport >= this->Internal->OutputPorts.size())
  {
    qCritical() << "Invalid output port : pqPipelineSource::getRepresentations(" << outputport
                << ", view). Available number of output ports: "
                << this->Internal->OutputPorts.size();
    return QList<pqDataRepresentation*>();
  }

  return this->Internal->OutputPorts[outputport]->getRepresentations(view);
}

//-----------------------------------------------------------------------------
QList<pqView*> pqPipelineSource::getViews() const
{
  QSet<pqView*> views;

  foreach (pqOutputPort* opPort, this->Internal->OutputPorts)
  {
    views.unite(QSet<pqView*>::fromList(opPort->getViews()));
  }

  return QList<pqView*>::fromSet(views);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::renderAllViews(bool force /*=false*/)
{
  foreach (pqOutputPort* opPort, this->Internal->OutputPorts)
  {
    opPort->renderAllViews(force);
  }
}
