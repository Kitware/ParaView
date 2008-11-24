/*=========================================================================

   Program: ParaView
   Module:    ToolbarActions.cxx

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

#include "ToolbarActions.h"

#include <pqActiveView.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqDisplayPolicy.h>
#include <pqObjectBuilder.h>
#include <pqOutputPort.h>
#include <pqPendingDisplayManager.h>
#include <pqPluginManager.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <pqServerManagerModelItem.h>
#include <pqServerManagerSelectionModel.h>
#include <pqServerResource.h>
#include <pqServerResources.h>
#include <pqView.h>

#include <vtkPVDataInformation.h>
#include <vtkPVXMLParser.h>
#include <vtkSmartPointer.h>
#include <vtkSMInputProperty.h>
#include <vtkSMProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSourceProxy.h>

#include <QAction>
#include <QFile>
#include <QIcon>
#include <QtDebug>

ToolbarActions::ToolbarActions(QObject* p)
  : QActionGroup(p)
{
}

ToolbarActions::~ToolbarActions()
{
}

//-----------------------------------------------------------------------------
pqPipelineSource *ToolbarActions::getActiveSource() const
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

  if (item && qobject_cast<pqPipelineSource*>(item))
    {
    return static_cast<pqPipelineSource*>(item);
    }
  else if (item && qobject_cast<pqOutputPort*>(item))
    {
    pqOutputPort* port = static_cast<pqOutputPort*>(item);
    return port->getSource();
    }

  return 0;
}

//-----------------------------------------------------------------------------
void ToolbarActions::createSource()
{
  QAction* action = qobject_cast<QAction*>(this->sender());
  if (!action)
    {
    return;
    }

  QString type = action->data().toString();

  this->createSource(type);
}

//-----------------------------------------------------------------------------
void ToolbarActions::createSource(const QString &type)
{
  pqServer* server= pqApplicationCore::instance()->
    getServerManagerModel()->getItemAtIndex<pqServer*>(0);
  if (!server)
    {
    qDebug() << "No server present cannot convert source.";
    return;
    }

  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();  

  // Create source of the given type
  builder->createSource("sources", type, server);
}

//-----------------------------------------------------------------------------
void ToolbarActions::createFilter()
{
  QAction* action = qobject_cast<QAction*>(this->sender());
  if (!action)
    {
    return;
    }

  QString type = action->data().toString();

  this->createFilter(type);
}

//-----------------------------------------------------------------------------
void ToolbarActions::createFilter(const QString &type)
{
  pqServer* server= pqApplicationCore::instance()->
    getServerManagerModel()->getItemAtIndex<pqServer*>(0);
  if (!server)
    {
    qDebug() << "No server present cannot convert source.";
    return;
    }

  // Get the currently selected source
  pqPipelineSource *src = this->getActiveSource();
  if(!src)
    {
    return;
    }

  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();  
  builder->createFilter("filters", type, src);
}

//-----------------------------------------------------------------------------
void ToolbarActions::createAndExecuteFilter()
{
  QAction* action = qobject_cast<QAction*>(this->sender());
  if (!action)
    {
    return;
    }

  QString type = action->data().toString();

  this->createAndExecuteFilter(type);
}

//-----------------------------------------------------------------------------
void ToolbarActions::createAndExecuteFilter(const QString &type)
{
  pqServer* server= pqApplicationCore::instance()->
    getServerManagerModel()->getItemAtIndex<pqServer*>(0);
  if (!server)
    {
    qDebug() << "No server present cannot convert source.";
    return;
    }

  // Get the currently selected source
  pqPipelineSource *src = this->getActiveSource();
  if(!src)
    {
    return;
    }

  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();  

  // Create filter of the given type
  pqPendingDisplayManager *pdm = qobject_cast<pqPendingDisplayManager*>(pqApplicationCore::instance()->manager("PENDING_DISPLAY_MANAGER"));
  pdm->setAddSourceIgnored(true);
  pqPipelineSource *filter = builder->createFilter("filters", type, src);
  filter->getProxy()->UpdateVTKObjects();
  filter->setModifiedState(pqProxy::UNMODIFIED);

  pqOutputPort* opPort = filter->getOutputPort(0);
  QString preferredViewType = pqApplicationCore::instance()->getDisplayPolicy()->getPreferredViewType(opPort,0);
  if(preferredViewType.isNull())
    {
    return;
    }

  // Add it to the view
  pqView *view = builder->createView(preferredViewType, server);
  builder->createDataRepresentation(filter->getOutputPort(0),view);
  pdm->setAddSourceIgnored(false);
}

//-----------------------------------------------------------------------------
void ToolbarActions::createView()
{
  QAction* action = qobject_cast<QAction*>(this->sender());
  if(!action)
    {
    return;
    }

  QString type = action->data().toString();

  this->createView(type);
}


//-----------------------------------------------------------------------------
void ToolbarActions::createView(const QString &type)
{
  pqServer* server= pqApplicationCore::instance()->
    getServerManagerModel()->getItemAtIndex<pqServer*>(0);
  if (!server)
    {
    qDebug() << "No server present cannot convert view.";
    return;
    }

  pqObjectBuilder* builder = 
    pqApplicationCore::instance()-> getObjectBuilder();

  // Try to create a view of the given type
  pqView *view = builder->createView(type, server);

  if(!view)
    {
    return;
    }
  
  // Get the currently selected source
  pqPipelineSource *src = this->getActiveSource();
  if(!src)
    {
    return;
    }

  pqOutputPort* opPort = src->getOutputPort(0);

  // Add a representation of the source to the view
  builder->createDataRepresentation(opPort,view);
  view->render();
}

//-----------------------------------------------------------------------------
void ToolbarActions::createSelectionFilter()
{
  QAction* action = qobject_cast<QAction*>(this->sender());
  if (!action)
    {
    return;
    }

  QString type = action->data().toString();

  this->createSelectionFilter(type);
}

//-----------------------------------------------------------------------------
void ToolbarActions::createSelectionFilter(const QString &type)
{
  pqServer* server= pqApplicationCore::instance()->
    getServerManagerModel()->getItemAtIndex<pqServer*>(0);
  if (!server)
    {
    qDebug() << "No server present cannot convert source.";
    return;
    }

  pqView *view = pqActiveView::instance().current();
  if(!view)
    {
    return;
    }

  // Find the view's first visible representation
  QList<pqRepresentation*> reps = view->getRepresentations();
  pqDataRepresentation* pqRepr = NULL;
  for(int i=0; i<reps.size(); ++i)
    {
    if(reps[i]->isVisible())
      {
      pqRepr = qobject_cast<pqDataRepresentation*>(reps[i]);
      break;
      }
    } 

  // No visible rep
  if(!pqRepr)
    return;

  pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
  vtkSMSourceProxy* srcProxy = vtkSMSourceProxy::SafeDownCast(
    opPort->getSource()->getProxy());

  vtkSMSourceProxy *selProxy = vtkSMSourceProxy::SafeDownCast(srcProxy->GetSelectionInput(0));

  vtkSMSourceProxy* filterProxy = vtkSMSourceProxy::SafeDownCast(
        vtkSMProxyManager::GetProxyManager()->NewProxy("filters", type.toAscii().data()));
  filterProxy->SetConnectionID(server->GetConnectionID());
  vtkSMInputProperty *selInput = vtkSMInputProperty::SafeDownCast(filterProxy->GetProperty("Input"));
  selInput->AddProxy(selProxy);
  vtkSMInputProperty *graphInput = vtkSMInputProperty::SafeDownCast(filterProxy->GetProperty("Graph"));
  graphInput->AddProxy(srcProxy);
  filterProxy->UpdatePipeline();

  srcProxy->SetSelectionInput(opPort->getPortNumber(), filterProxy, 0);

  selInput->RemoveAllProxies();
  graphInput->RemoveAllProxies();
  filterProxy->Delete();
}

//-----------------------------------------------------------------------------
void ToolbarActions::loadState()
{
  QAction* action = qobject_cast<QAction*>(this->sender());
  if(!action)
    {
    return;
    }

  QString xmlfilename = action->data().toString();

  pqServer* server= pqApplicationCore::instance()->
    getServerManagerModel()->getItemAtIndex<pqServer*>(0);
  if (!server)
    {
    qDebug() << "No server present, cannot load state.";
    return;
    }

  QFile xml(xmlfilename);
  if (!xml.open(QIODevice::ReadOnly))
    {
    qDebug() << "Failed to load " << xmlfilename;
    return;
    }

  QByteArray dat = xml.readAll();

  // Read in the xml file to restore.
  vtkSmartPointer<vtkPVXMLParser> xmlParser = 
    vtkSmartPointer<vtkPVXMLParser>::New();

  if(!xmlParser->Parse(dat.data()))
    {
    qDebug() << "Failed to parse " << xmlfilename;
    xml.close();
    return;
    }

  // Get the root element from the parser.
  vtkPVXMLElement *root = xmlParser->GetRootElement();
  pqApplicationCore::instance()->loadState(root, server);

}
