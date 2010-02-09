/*=========================================================================

   Program: ParaView
   Module:    pqActiveObjects.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqActiveObjects.h"

#include "pqActiveView.h"
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerSelectionModel.h"
#include "pqView.h"

#include <QDebug>

//-----------------------------------------------------------------------------
pqActiveObjects& pqActiveObjects::instance() 
{
  static pqActiveObjects activeObject;
  return activeObject;
}

//-----------------------------------------------------------------------------
pqActiveObjects::pqActiveObjects()
{
  this->CachedSource = 0;
  this->CachedView = 0;
  this->CachedPort = 0;
  this->CachedServer = 0;

  pqActiveView* actView = &pqActiveView::instance();
  QObject::connect(actView, SIGNAL(changed(pqView*)),
    this, SLOT(activeViewChanged(pqView*)));

  pqServerManagerSelectionModel *selection =
      pqApplicationCore::instance()->getSelectionModel();
  QObject::connect(selection, SIGNAL(currentChanged(pqServerManagerModelItem*)),
    this, SLOT(onSelectionChanged()));
  QObject::connect(selection,
    SIGNAL(selectionChanged(
        const pqServerManagerSelection&, const pqServerManagerSelection&)),
    this, SLOT(onSelectionChanged()));

  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(serverAdded(pqServer*)),
    this, SLOT(onServerChanged()));
  QObject::connect(smmodel, SIGNAL(serverRemoved(pqServer*)),
    this, SLOT(onServerChanged()));

  QObject::connect(this, SIGNAL(viewChanged(pqView*)),
    this, SLOT(updateRepresentation()));
  QObject::connect(this, SIGNAL(portChanged(pqOutputPort*)),
    this, SLOT(updateRepresentation()));
}

//-----------------------------------------------------------------------------
pqActiveObjects::~pqActiveObjects()
{

}

//-----------------------------------------------------------------------------
void pqActiveObjects::activeViewChanged(pqView* newView)
{
  if (newView)
    {
    QObject::connect(newView, SIGNAL(representationAdded(pqRepresentation*)),
      this, SLOT(updateRepresentation()));
    QObject::connect(newView, SIGNAL(representationRemoved(pqRepresentation*)),
      this, SLOT(updateRepresentation()));
    }
  if (this->CachedView != newView)
    {
    this->CachedView = newView;
    emit this->viewChanged(newView);
    }
}

//-----------------------------------------------------------------------------
void pqActiveObjects::onServerChanged()
{
  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  pqServer* server = smmodel->getNumberOfItems<pqServer*>() == 1?
    smmodel->getItemAtIndex<pqServer*>(0) : NULL;
  if (this->CachedServer != server)
    {
    this->CachedServer = server;
    emit this->serverChanged(server);
    }
}

//-----------------------------------------------------------------------------
void pqActiveObjects::onSelectionChanged()
{
  pqServerManagerModelItem *item = 0;
  pqServerManagerSelectionModel *selection =
    pqApplicationCore::instance()->getSelectionModel();
  const pqServerManagerSelection *selected = selection->selectedItems();
  if(selected->size() == 1)
    {
    item = selected->first();
    }
  else if(selected->size() > 1)
    {
    item = selection->currentItem();
    if(item && !selection->isSelected(item))
      {
      item = 0;
      }
    }
  pqOutputPort* opPort = qobject_cast<pqOutputPort*>(item);
  pqPipelineSource *source = opPort? opPort->getSource() : 
    qobject_cast<pqPipelineSource*>(item);
  if (source && !opPort && source->getNumberOfOutputPorts() > 0)
    {
    opPort = source->getOutputPort(0);
    }

  bool port_changed =  (this->CachedPort != opPort);
  bool source_changed = (this->CachedSource != source);

  if (port_changed && this->CachedPort)
    {
    QObject::disconnect(this->CachedPort, 0, this, 0);
    }

  this->CachedPort = opPort;
  this->CachedSource = source;

  if (port_changed)
    {
    if (opPort)
      {
      QObject::connect(opPort,
        SIGNAL(representationAdded(pqOutputPort*, pqDataRepresentation*)),
        this, SLOT(updateRepresentation()));
      }
    emit this->portChanged(opPort);
    }

  if (source_changed)
    {
    emit this->sourceChanged(source);
    }
}

//-----------------------------------------------------------------------------
void pqActiveObjects::setActiveView(pqView* view)
{
  pqActiveView::instance().setCurrent(view);
}

//-----------------------------------------------------------------------------
void pqActiveObjects::setActiveSource(pqPipelineSource* source)
{
  pqApplicationCore::instance()->getSelectionModel()->setCurrentItem(source,
    pqServerManagerSelectionModel::ClearAndSelect);
}

//-----------------------------------------------------------------------------
void pqActiveObjects::setActivePort(pqOutputPort* port)
{
  pqApplicationCore::instance()->getSelectionModel()->setCurrentItem(port,
    pqServerManagerSelectionModel::ClearAndSelect);
}

//-----------------------------------------------------------------------------
void pqActiveObjects::updateRepresentation()
{
  pqDataRepresentation* repr = 0;
  if (this->activePort())
    {
    repr = this->activePort()->getRepresentation(this->activeView());
    }
  if (this->CachedRepresentation != repr)
    {
    this->CachedRepresentation = repr;
    emit this->representationChanged(repr);
    emit this->representationChanged(static_cast<pqRepresentation*>(repr));
    }
}

//-----------------------------------------------------------------------------
void pqActiveObjects::setActiveServer(pqServer*)
{
  qDebug() << "pqActiveObjects::setActiveServer is not supported yet since "
    " ParaView only support 1 server connection at a time.";
}

