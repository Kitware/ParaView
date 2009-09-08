/*=========================================================================

   Program:   ParaView
   Module:    pqPipelineModel.cxx

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

/// \file pqPipelineModel.cxx
/// \date 4/14/2006

#include "pqPipelineModel.h"

#include "pqApplicationCore.h"
#include "pqBarChartView.h"
#include "pqDataRepresentation.h"
#include "pqDisplayPolicy.h"
#include "pqLineChartView.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerModelItem.h"
#include "pqSpreadSheetView.h"
#include "pqView.h"

#include <QFont>
#include <QIcon>
#include <QList>
#include <QMap>
#include <QPixmap>
#include <QPointer>
#include <QString>
#include <QtDebug>

#include "vtkSMProxy.h"

class pqPipelineModelSource;


/// \class pqPipelineModelItem
class pqPipelineModelItem : public QObject
{
public:
  enum VisibleState
    {
    NotAllowed,
    Visible,
    NotVisible
    };

  enum IconType
    {
    SERVER,
    LINK,
    GEOMETRY,
    BARCHART,
    LINECHART,
    TABLE,
    INDETERMINATE,
    LAST
    };
public:
  pqPipelineModelItem(QObject *parent=0);
  virtual ~pqPipelineModelItem() {}

  virtual QString getName() const=0;
  virtual VisibleState getVisibleState() const=0;
  virtual bool isSelectable() const {return true;}
  virtual void setSelectable(bool selectable)=0;
  virtual bool isModified() const=0;
  virtual pqPipelineModelItem *getParent() const=0;
  virtual pqServerManagerModelItem *getObject() const=0;
  virtual int getChildCount() const=0;
  virtual int getChildIndex(pqPipelineModelItem *item) const=0;
  virtual pqPipelineModelItem *getChild(int row) const=0;
  virtual void removeChild(pqPipelineModelItem *item)=0;

  pqPipelineModel::ItemType getType() const {return this->Type;}

  /// Returns the type of icon to use for this item.
  virtual IconType getIconType() const=0;
protected:
  void setType(pqPipelineModel::ItemType type) {this->Type = type;}

private:
  pqPipelineModel::ItemType Type;
};


/// \class pqPipelineModelServer
class pqPipelineModelServer : public pqPipelineModelItem
{
public:
  pqPipelineModelServer(pqServer *server=0, QObject *parent=0);
  virtual ~pqPipelineModelServer();

  virtual QString getName() const;
  virtual pqPipelineModelItem::VisibleState getVisibleState() const;
  virtual bool isSelectable() const {return this->Selectable;}
  virtual void setSelectable(bool selectable) {this->Selectable = selectable;}
  virtual bool isModified() const {return false;}
  virtual pqPipelineModelItem *getParent() const {return 0;}
  virtual pqServerManagerModelItem *getObject() const {return this->Server;}
  virtual int getChildCount() const {return this->Sources.size();}
  virtual int getChildIndex(pqPipelineModelItem *item) const;
  virtual pqPipelineModelItem *getChild(int row) const;
  virtual void removeChild(pqPipelineModelItem *item);

  pqServer *getServer() const {return this->Server;}
  void setServer(pqServer *server) {this->Server = server;}

  QList<pqPipelineModelSource *> &getSources() {return this->Sources;}

  /// Returns the type of icon to use for this item.
  virtual IconType getIconType() const
    { return SERVER; }
private:
  pqServer *Server;
  QList<pqPipelineModelSource *> Sources;
  bool Selectable;
};


/// \class pqPipelineModelObject
class pqPipelineModelObject : public pqPipelineModelItem
{
public:
  pqPipelineModelObject(pqPipelineModelServer *server=0, QObject *parent=0);
  virtual ~pqPipelineModelObject() {}

  pqPipelineModelServer *getServer() const {return this->Server;}
  void setServer(pqPipelineModelServer *server) {this->Server = server;}

private:
  pqPipelineModelServer *Server;
};


/// \class pqPipelineModelOutput
class pqPipelineModelOutput : public pqPipelineModelObject
{
public:
  pqPipelineModelOutput(pqPipelineModelServer *server=0, QObject *parent=0);
  ~pqPipelineModelOutput();

  virtual pqPipelineModelItem::VisibleState getVisibleState() const;
  virtual bool isSelectable() const {return this->Selectable;}
  virtual void setSelectable(bool selectable) {this->Selectable = selectable;}
  virtual int getChildCount() const {return this->Outputs.size();}
  virtual int getChildIndex(pqPipelineModelItem *item) const;
  virtual pqPipelineModelItem *getChild(int row) const;
  virtual void removeChild(pqPipelineModelItem *item);

  void setVisibleState(pqPipelineModelItem::VisibleState eye);

  QList<pqPipelineModelObject *> &getOutputs() {return this->Outputs;}

  static pqPipelineModelItem::VisibleState computeVisibleState(
      pqOutputPort *port, pqView *view);

  /// Returns the type of icon to use for this item.
  virtual IconType getIconType() const
    { return INDETERMINATE; }
private:
  QList<pqPipelineModelObject *> Outputs;
  pqPipelineModelItem::VisibleState Eyeball;
  bool Selectable;
};


/// \class pqPipelineModelSource
class pqPipelineModelSource : public pqPipelineModelOutput
{
  typedef pqPipelineModelOutput Superclass;
public:
  pqPipelineModelSource(pqPipelineModelServer *server=0,
      pqPipelineSource *source=0, QObject *parent=0);
  virtual ~pqPipelineModelSource() {}

  virtual QString getName() const;
  virtual bool isModified() const;
  virtual pqPipelineModelItem *getParent() const {return this->getServer();}
  virtual pqServerManagerModelItem *getObject() const {return this->Source;}

  pqPipelineSource *getSource() const {return this->Source;}
  void setSource(pqPipelineSource *source) {this->Source = source;}

  void updateVisibleState(pqView *module);

  /// Returns the type of icon to use for this item.
  virtual IconType getIconType() const;

private:
  pqPipelineSource *Source;
};


/// \class pqPipelineModelFilter
class pqPipelineModelFilter : public pqPipelineModelSource
{
public:
  pqPipelineModelFilter(pqPipelineModelServer *server=0,
      pqPipelineSource *source=0,
      pqPipelineModel::ItemType type=pqPipelineModel::Filter,
      QObject *parent=0);
  virtual ~pqPipelineModelFilter() {}

  virtual pqPipelineModelItem *getParent() const;

  QList<pqPipelineModelOutput *> &getInputs() {return this->Inputs;}

private:
  QList<pqPipelineModelOutput *> Inputs;
};

/// \class pqPipelineModelOutputPort
/// \brief
///   The pqPipelineModelOutputPort class is used for sources that
///   have more than 1 output port.
class pqPipelineModelOutputPort : public pqPipelineModelOutput
{
public:
  pqPipelineModelOutputPort(pqPipelineModelServer *server=0,
      pqPipelineModelSource *source=0, int port=0, QObject *parent=0);
  virtual ~pqPipelineModelOutputPort() {}

  virtual QString getName() const;
  virtual bool isModified() const {return false;}
  virtual pqPipelineModelItem *getParent() const {return this->Source;}
  virtual pqServerManagerModelItem *getObject() const;

  void updateVisibleState(pqView *view);

  pqPipelineModelSource *getSource() const {return this->Source;}
  void setSource(pqPipelineModelSource *source) {this->Source = source;}

  int getPort() const {return this->Port;}
  void setPort(int port) {this->Port = port;}

  /// Returns the type of icon to use for this item.
  virtual IconType getIconType() const;
private:
  pqPipelineModelSource *Source;
  int Port;
};


/// \class pqPipelineModelLink
class pqPipelineModelLink : public pqPipelineModelObject
{
public:
  pqPipelineModelLink(pqPipelineModelServer *server=0, QObject *parent=0);
  virtual ~pqPipelineModelLink() {}

  virtual QString getName() const;
  virtual pqPipelineModelItem::VisibleState getVisibleState() const;
  virtual bool isSelectable() const;
  virtual void setSelectable(bool selectable);
  virtual bool isModified() const;
  virtual pqPipelineModelItem *getParent() const {return this->Source;}
  virtual pqServerManagerModelItem *getObject() const;
  virtual int getChildCount() const {return 0;}
  virtual int getChildIndex(pqPipelineModelItem *) const {return -1;}
  virtual pqPipelineModelItem *getChild(int) const {return 0;}
  virtual void removeChild(pqPipelineModelItem *) {}

  pqPipelineModelOutput *getSource() const {return this->Source;}
  void setSource(pqPipelineModelOutput *source) {this->Source = source;}

  pqPipelineModelFilter *getSink() const {return this->Sink;}
  void setSink(pqPipelineModelFilter *sink) {this->Sink = sink;}


  /// Returns the type of icon to use for this item.
  virtual IconType getIconType() const
    { return LINK; }

private:
  pqPipelineModelOutput *Source;
  pqPipelineModelFilter *Sink;
};


/// \class pqPipelineModelInternal
/// \brief
///   The pqPipelineModelInternal class hides implementation details
///   from the interface.
class pqPipelineModelInternal
{
public:
  enum PixmapIndex
    {
    EYEBALL = pqPipelineModelItem::LAST,
    EYEBALL_GRAY,
    LAST 
    };

public:
  pqPipelineModelInternal();
  ~pqPipelineModelInternal() {}

  QList<pqPipelineModelServer *> Servers;
  QMap<pqServerManagerModelItem *, QPointer<pqPipelineModelItem> > ItemMap;
  pqView *RenderModule;
  pqServer *CleanupServer;
  QFont Modified;
};


//-----------------------------------------------------------------------------
pqPipelineModelItem::pqPipelineModelItem(QObject *parentObject)
  : QObject(parentObject)
{
  this->Type = pqPipelineModel::Invalid;
}


//-----------------------------------------------------------------------------
pqPipelineModelServer::pqPipelineModelServer(pqServer *server,
    QObject *parentObject)
  : pqPipelineModelItem(parentObject), Sources()
{
  this->Server = server;
  this->Selectable = true;

  this->setType(pqPipelineModel::Server);
}

pqPipelineModelServer::~pqPipelineModelServer()
{
  // Delete any source items still in the list.
  QList<pqPipelineModelSource *>::Iterator iter = this->Sources.begin();
  for( ; iter != this->Sources.end(); ++iter)
    {
    delete *iter;
    }

  this->Sources.clear();
}

QString pqPipelineModelServer::getName() const
{
  if(this->Server)
    {
    return this->Server->getResource().toURI();
    }

  return QString();
}

pqPipelineModelItem::VisibleState pqPipelineModelServer::getVisibleState() const
{
  return pqPipelineModelItem::NotAllowed;
}

int pqPipelineModelServer::getChildIndex(pqPipelineModelItem *item) const
{
  pqPipelineModelSource *source = dynamic_cast<pqPipelineModelSource *>(item);
  if(source)
    {
    return this->Sources.indexOf(source);
    }

  return -1;
}

pqPipelineModelItem *pqPipelineModelServer::getChild(int row) const
{
  return this->Sources[row];
}

void pqPipelineModelServer::removeChild(pqPipelineModelItem *item)
{
  pqPipelineModelSource *source = dynamic_cast<pqPipelineModelSource *>(item);
  if(source)
    {
    this->Sources.removeAll(source);
    }
}


//-----------------------------------------------------------------------------
pqPipelineModelObject::pqPipelineModelObject(pqPipelineModelServer *server,
    QObject *parentObject)
  : pqPipelineModelItem(parentObject)
{
  this->Server = server;
}


//-----------------------------------------------------------------------------
pqPipelineModelOutput::pqPipelineModelOutput(pqPipelineModelServer *server,
    QObject *parentObject)
  : pqPipelineModelObject(server, parentObject), Outputs()
{
  this->Eyeball = pqPipelineModelItem::NotAllowed;
  this->Selectable = true;
}

pqPipelineModelOutput::~pqPipelineModelOutput()
{
  // Clean up the ouputs left in the list.
  QList<pqPipelineModelObject *>::Iterator iter = this->Outputs.begin();
  for( ; iter != this->Outputs.end(); ++iter)
    {
    delete *iter;
    }

  this->Outputs.clear();
}

pqPipelineModelItem::VisibleState
    pqPipelineModelOutput::getVisibleState() const
{
  return this->Eyeball;
}

int pqPipelineModelOutput::getChildIndex(pqPipelineModelItem *item) const
{
  pqPipelineModelObject *object =
      dynamic_cast<pqPipelineModelObject *>(item);
  if(object)
    {
    pqPipelineModelLink *link = 0;
    QList<pqPipelineModelObject *>::ConstIterator iter = this->Outputs.begin();
    for(int index = 0; iter != this->Outputs.end(); ++iter, ++index)
      {
      if(object == *iter)
        {
        return index;
        }

      // The output may be in a link object.
      link = dynamic_cast<pqPipelineModelLink *>(*iter);
      if(link && link->getSink() == object)
        {
        return index;
        }
      }
    }

  return -1;
}

pqPipelineModelItem *pqPipelineModelOutput::getChild(int row) const
{
  return this->Outputs[row];
}

void pqPipelineModelOutput::removeChild(pqPipelineModelItem *item)
{
  pqPipelineModelObject *object = dynamic_cast<pqPipelineModelObject *>(item);
  if(object)
    {
    this->Outputs.removeAll(object);
    }
}

void pqPipelineModelOutput::setVisibleState(
    pqPipelineModelItem::VisibleState eye)
{
  this->Eyeball = eye;
}

pqPipelineModelItem::VisibleState pqPipelineModelOutput::computeVisibleState(
    pqOutputPort *port, pqView *view)
{
  // If no view is present, it implies that a suitable type of view
  // will be created.
  pqPipelineModelItem::VisibleState eye = pqPipelineModelItem::NotVisible;
  pqApplicationCore* core = pqApplicationCore::instance();
  pqDisplayPolicy* policy = core->getDisplayPolicy();
  if (policy)
    {
    switch (policy->getVisibility(view, port))
      {
    case pqDisplayPolicy::Visible:
      eye = pqPipelineModelItem::Visible;
      break;

    case pqDisplayPolicy::Hidden:
      eye = pqPipelineModelItem::NotVisible;
      break;

    case pqDisplayPolicy::NotApplicable:
    default:
      eye = pqPipelineModelItem::NotAllowed;
      }
    }

  return eye;
}


//-----------------------------------------------------------------------------
pqPipelineModelSource::pqPipelineModelSource(pqPipelineModelServer *server,
    pqPipelineSource *source, QObject *parentObject)
  : pqPipelineModelOutput(server, parentObject)
{
  this->Source = source;

  this->setType(pqPipelineModel::Source);
}

QString pqPipelineModelSource::getName() const
{
  if(this->Source)
    {
    return this->Source->getSMName();
    }

  return QString();
}

bool pqPipelineModelSource::isModified() const
{
  if(this->Source)
    {
    return this->Source->modifiedState() != pqProxy::UNMODIFIED;
    }

  return false;
}

pqPipelineModelItem::IconType pqPipelineModelSource::getIconType() const
{
  if (this->Source->getNumberOfOutputPorts() > 1)
    {
    // the icon will be shown on each outport port.
    return INDETERMINATE;
    }

  // This has only 1 output port.
  pqApplicationCore* core = pqApplicationCore::instance();
  pqDisplayPolicy* policy = core->getDisplayPolicy();
  if (policy)
    {
    QString type = policy->getPreferredViewType(
      this->Source->getOutputPort(0), false);
    if (type == pqBarChartView::barChartViewType())
      {
      return BARCHART;
      }
    if (type == pqLineChartView::lineChartViewType())
      {
      return LINECHART;
      }
    if (type == pqSpreadSheetView::spreadsheetViewType())
      {
      return TABLE;
      }
    }
  
  return GEOMETRY;
}

void pqPipelineModelSource::updateVisibleState(pqView *view)
{
  if(this->Source->getNumberOfOutputPorts() > 1)
    {
    // If the source has more than one output port, update the visible
    // state of the output ports.
    this->setVisibleState(pqPipelineModelItem::NotAllowed);
    QList<pqPipelineModelObject *>::Iterator iter = this->getOutputs().begin();
    for( ; iter != this->getOutputs().end(); ++iter)
      {
      pqPipelineModelOutputPort *outputPort = 
          dynamic_cast<pqPipelineModelOutputPort *>(*iter);
      if(outputPort)
        {
        outputPort->updateVisibleState(view);
        }
      }
    }
  else
    {
    this->setVisibleState(pqPipelineModelOutput::computeVisibleState(
        this->Source->getOutputPort(0), view));
    }
}


//-----------------------------------------------------------------------------
pqPipelineModelFilter::pqPipelineModelFilter(pqPipelineModelServer *server,
    pqPipelineSource *source, pqPipelineModel::ItemType type,
    QObject *parentObject)
  : pqPipelineModelSource(server, source, parentObject), Inputs()
{
  if(type != pqPipelineModel::CustomFilter)
    {
    type = pqPipelineModel::Filter;
    }

  this->setType(type);
}

pqPipelineModelItem *pqPipelineModelFilter::getParent() const
{
  if(this->Inputs.size() == 1)
    {
    return this->Inputs[0];
    }
  else
    {
    return this->getServer();
    }
}


//-----------------------------------------------------------------------------
pqPipelineModelOutputPort::pqPipelineModelOutputPort(
    pqPipelineModelServer *server, pqPipelineModelSource *source, int port,
    QObject *parentObject)
  : pqPipelineModelOutput(server, parentObject)
{
  this->Source = source;
  this->Port = port;

  this->setType(pqPipelineModel::OutputPort);
}

QString pqPipelineModelOutputPort::getName() const
{
  pqOutputPort* opPort = qobject_cast<pqOutputPort*>(
    this->getObject());
  return opPort->getPortName();
}

pqServerManagerModelItem *pqPipelineModelOutputPort::getObject() const 
{
  // Return the ouput port this item represents.
  if(this->Source)
    {
    pqPipelineSource *source = this->Source->getSource();
    return source ? source->getOutputPort(this->Port) : 0;
    }

  return 0;
}

void pqPipelineModelOutputPort::updateVisibleState(pqView *view)
{
  if(this->Source)
    {
    pqPipelineSource *source = this->Source->getSource();
    this->setVisibleState(pqPipelineModelOutput::computeVisibleState(
        source ? source->getOutputPort(this->Port) : 0, view));
    }
}
pqPipelineModelItem::IconType pqPipelineModelOutputPort::getIconType() const
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqDisplayPolicy* policy = core->getDisplayPolicy();
  if (policy)
    {
    QString type = policy->getPreferredViewType(
      this->Source->getSource()->getOutputPort(this->Port), false);
    if (type == pqBarChartView::barChartViewType())
      {
      return BARCHART;
      }
    if (type == pqLineChartView::lineChartViewType())
      {
      return LINECHART;
      }
    if (type == pqSpreadSheetView::spreadsheetViewType())
      {
      return TABLE;
      }
    }
  
  return GEOMETRY;
}

//-----------------------------------------------------------------------------
pqPipelineModelLink::pqPipelineModelLink(pqPipelineModelServer *server,
    QObject *parentObject)
  : pqPipelineModelObject(server, parentObject)
{
  this->Source = 0;
  this->Sink = 0;

  this->setType(pqPipelineModel::Link);
}

QString pqPipelineModelLink::getName() const
{
  if(this->Sink)
    {
    return this->Sink->getName();
    }

  return QString();
}

pqPipelineModelItem::VisibleState pqPipelineModelLink::getVisibleState() const
{
  if(this->Sink)
    {
    return this->Sink->getVisibleState();
    }

  return pqPipelineModelItem::NotAllowed;
}

bool pqPipelineModelLink::isSelectable() const
{
  if(this->Sink)
    {
    return this->Sink->isSelectable();
    }

  return false;
}

void pqPipelineModelLink::setSelectable(bool)
{
}

bool pqPipelineModelLink::isModified() const
{
  if(this->Sink)
    {
    return this->Sink->isModified();
    }

  return false;
}

pqServerManagerModelItem *pqPipelineModelLink::getObject() const
{
  if(this->Sink)
    {
    return this->Sink->getObject();
    }

  return 0;
}


//-----------------------------------------------------------------------------
pqPipelineModelInternal::pqPipelineModelInternal()
  : Servers(), ItemMap(), Modified()
{
  this->RenderModule = 0;
  this->CleanupServer = 0;

  this->Modified.setBold(true);
}


//-----------------------------------------------------------------------------
pqPipelineModel::pqPipelineModel(QObject *parentObject)
  : QAbstractItemModel(parentObject)
{
  this->Internal = new pqPipelineModelInternal();
  this->PixmapList = 0;
  this->Editable = true;

  // Initialize the pixmap list.
  this->initializePixmaps();
}

pqPipelineModel::pqPipelineModel(const pqPipelineModel &other,
    QObject *parentObject)
  : QAbstractItemModel(parentObject)
{
  this->Internal = new pqPipelineModelInternal();
  this->PixmapList = 0;
  this->Editable = true;

  // Initialize the pixmap list.
  this->initializePixmaps();

  // Copy the other pipeline model. Start with the server objects.
  pqPipelineSource *sink = 0;
  pqPipelineModelItem *item = 0;
  pqPipelineModelSource *source = 0;
  pqPipelineModelOutput *output = 0;
  pqPipelineModelOutputPort *outputPort = 0;
  QList<pqPipelineModelServer *>::ConstIterator server =
      other.Internal->Servers.begin();
  for( ; server != other.Internal->Servers.end(); ++server)
    {
    // Add the server to the model.
    this->addServer((*server)->getServer());

    // Add all the server's objects to the model.
    item = other.getNextModelItem(*server, *server);
    while(item)
      {
      source = dynamic_cast<pqPipelineModelSource *>(item);
      if(source)
        {
        this->addSource(source->getSource());
        }

      item = other.getNextModelItem(item, *server);
      }

    // Set up all the connections.
    item = other.getNextModelItem(*server, *server);
    while(item)
      {
      output = dynamic_cast<pqPipelineModelOutput *>(item);
      if(output)
        {
        int port = 0;
        source = dynamic_cast<pqPipelineModelSource *>(output);
        outputPort = dynamic_cast<pqPipelineModelOutputPort *>(output);
        if(outputPort)
          {
          port = outputPort->getPort();
          source = outputPort->getSource();
          }

        for(int i = 0; i < output->getChildCount(); i++)
          {
          sink = dynamic_cast<pqPipelineSource *>(
              output->getChild(i)->getObject());
          if(sink)
            {
            this->addConnection(source->getSource(), sink, port); 
            }
          }
        }

      item = other.getNextModelItem(item, *server);
      }
    }
}

pqPipelineModel::pqPipelineModel(const pqServerManagerModel &other,
    QObject *parentObject)
  : QAbstractItemModel(parentObject)
{
  this->Internal = new pqPipelineModelInternal();
  this->PixmapList = 0;
  this->Editable = true;

  // Initialize the pixmap list.
  this->initializePixmaps();

  // Build a pipeline model from the current server manager model.
  QList<pqPipelineSource *> sources;
  QList<pqPipelineSource *>::Iterator source;
  QList<pqServer *> servers = other.findItems<pqServer*>();
  QList<pqServer *>::Iterator server = servers.begin();
  for( ; server != servers.end(); ++server)
    {
    // Add the server to the model.
    this->addServer(*server);

    // Add the sources for the server.
    sources = other.findItems<pqPipelineSource *>(*server);
    for(source = sources.begin(); source != sources.end(); ++source)
      {
      this->addSource(*source);
      }

    // Set up the pipeline connections.
    for(source = sources.begin(); source != sources.end(); ++source)
      {
      int numOutputPorts = (*source)->getNumberOfOutputPorts();
      for(int port = 0; port < numOutputPorts; port++)
        {
        int numConsumers = (*source)->getNumberOfConsumers(port);
        for(int i = 0; i < numConsumers; ++i)
          {
          this->addConnection(*source, (*source)->getConsumer(port, i), port);
          }
        }
      }
    }
}

pqPipelineModel::~pqPipelineModel()
{
  if(this->PixmapList)
    {
    delete [] this->PixmapList;
    }

  // Clean up the pipeline model items.
  this->Internal->ItemMap.clear();
  QList<pqPipelineModelServer *>::Iterator iter =
      this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    delete *iter;
    }

  this->Internal->Servers.clear();
  delete this->Internal;
}

int pqPipelineModel::rowCount(const QModelIndex &parentIndex) const
{
  if(parentIndex.isValid())
    {
    if(parentIndex.model() == this)
      {
      pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
          parentIndex.internalPointer());
      return item->getChildCount();
      }
    }
  else
    {
    return this->Internal->Servers.size();
    }

  return 0;
}

int pqPipelineModel::columnCount(const QModelIndex &) const
{
  return 2;
}

bool pqPipelineModel::hasChildren(const QModelIndex &parentIndex) const
{
  return this->rowCount(parentIndex) > 0;
}

QModelIndex pqPipelineModel::index(int row, int column,
    const QModelIndex &parentIndex) const
{
  if(row < 0 || column < 0 || column > this->columnCount())
    {
    return QModelIndex();
    }

  if(parentIndex.isValid())
    {
    if(parentIndex.model() == this)
      {
      pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
          parentIndex.internalPointer());
      if(row < item->getChildCount())
        {
        return this->createIndex(row, column, item->getChild(row));
        }
      }
    }
  else if(row < this->Internal->Servers.size())
    {
    return this->createIndex(row, column, this->Internal->Servers[row]);
    }

  return QModelIndex();
}

QModelIndex pqPipelineModel::parent(const QModelIndex &idx) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem*>(
        idx.internalPointer());
    pqPipelineModelItem *parentItem = item->getParent();
    if(parentItem)
      {
      return this->makeIndex(parentItem);
      }
    }

  return QModelIndex();
}

QVariant pqPipelineModel::data(const QModelIndex &idx, int role) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem*>(
        idx.internalPointer());
    if(item)
      {
      switch(role)
        {
        case Qt::DisplayRole:
          {
          if(idx.column() == 1)
            {
            pqPipelineModelItem::VisibleState visible = item->getVisibleState();
            if(visible == pqPipelineModelItem::Visible)
              {
              return QVariant(QIcon(
                  this->PixmapList[pqPipelineModelInternal::EYEBALL]));
              }
            else if(visible == pqPipelineModelItem::NotVisible)
              {
              return QVariant(QIcon(
                  this->PixmapList[pqPipelineModelInternal::EYEBALL_GRAY]));
              }
            }
          }
        case Qt::ToolTipRole:
        case Qt::EditRole:
          {
          if(idx.column() == 0)
            {
            return QVariant(item->getName());
            }

          break;
          }
        case Qt::DecorationRole:
          {
          if(idx.column() == 0 && this->PixmapList &&
              item->getType() != pqPipelineModel::Invalid)
            {
            return QVariant(this->PixmapList[item->getIconType()]);
            }

          break;
          }
        case Qt::FontRole:
          {
          if(idx.column() == 0 && item->isModified())
            {
            return qVariantFromValue<QFont>(this->Internal->Modified);
            }

          break;
          }
        }
      }
    }

  return QVariant();
}

Qt::ItemFlags pqPipelineModel::flags(const QModelIndex &idx) const
{
  Qt::ItemFlags indexFlags = Qt::ItemIsEnabled;
  if(idx.column() == 0)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem*>(
        idx.internalPointer());
    if(item->isSelectable())
      {
      indexFlags |= Qt::ItemIsSelectable;
      }

    if(this->Editable && item->getType() != pqPipelineModel::Server &&
        item->getType() != pqPipelineModel::OutputPort)
      {
      indexFlags |= Qt::ItemIsEditable;
      }
    }

  return indexFlags;
}

bool pqPipelineModel::setData(const QModelIndex &idx, const QVariant &value,
    int)
{
  if(value.toString().isEmpty())
    {
    return false;
    }

  pqPipelineSource *source = qobject_cast<pqPipelineSource *>(
      this->getItemFor(idx));
  if(source)
    {
    emit this->rename(idx, value.toString());
    }

  return true;
}

pqServerManagerModelItem *pqPipelineModel::getItemFor(
    const QModelIndex &idx) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item =reinterpret_cast<pqPipelineModelItem *>(
        idx.internalPointer());
    if(item)
      {
      return item->getObject();
      }
    }

  return 0;
}

QModelIndex pqPipelineModel::getIndexFor(pqServerManagerModelItem *item) const
{
  pqPipelineModelItem *modelItem = this->getModelItemFor(item);
  if(modelItem)
    {
    return this->makeIndex(modelItem);
    }

  return QModelIndex();
}

pqPipelineModel::ItemType pqPipelineModel::getTypeFor(
    const QModelIndex &idx) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
        idx.internalPointer());
    return item->getType();
    }

  return pqPipelineModel::Invalid;
}

QModelIndex pqPipelineModel::getNextIndex(const QModelIndex idx,
    const QModelIndex &root) const
{
  // If the index has children, return the first child.
  if(this->rowCount(idx) > 0)
    {
    return this->index(0, 0, idx);
    }

  // Search up the parent chain for an index with more children.
  QModelIndex current = idx;
  while(current.isValid() && current != root)
    {
    QModelIndex parentIndex = current.parent();
    if(current.row() < this->rowCount(parentIndex) - 1)
      {
      return this->index(current.row() + 1, 0, parentIndex);
      }

    current = parentIndex;
    }

  return QModelIndex();
}

bool pqPipelineModel::isSelectable(const QModelIndex &idx) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
        idx.internalPointer());
    return item->isSelectable();
    }

  return false;
}

void pqPipelineModel::setSelectable(const QModelIndex &idx, bool selectable)
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
        idx.internalPointer());
    item->setSelectable(selectable);
    }
}

void pqPipelineModel::setSubtreeSelectable(pqServerManagerModelItem *item,
    bool selectable)
{
  pqPipelineModelItem *root = this->getModelItemFor(item);
  pqPipelineModelItem *modelItem = root;
  while(modelItem)
    {
    modelItem->setSelectable(selectable);
    modelItem = this->getNextModelItem(modelItem, root);
    }
}

void pqPipelineModel::setModifiedFont(const QFont &font)
{
  this->Internal->Modified = font;
}

void pqPipelineModel::addServer(pqServer *server)
{
  if(!server)
    {
    qDebug() << "Unable to add null server to the pipeline model.";
    return;
    }

  // Make sure the server does not already have an item in the model.
  if(this->getModelItemFor(server))
    {
    qWarning() << "Server already added to the pipeline model.";
    return;
    }

  // Create a new pipeline model item for the server.
  pqPipelineModelServer *item = new pqPipelineModelServer(server);
  if(item)
    {
    // Add the server to the map and to the model.
    this->Internal->ItemMap.insert(server, item);

    int row = this->Internal->Servers.size();
    this->beginInsertRows(QModelIndex(), row, row);
    this->Internal->Servers.append(item);
    this->endInsertRows();
    }
}

void pqPipelineModel::startRemovingServer(pqServer *server)
{
  // Check if another server is being cleaned up.
  if(this->Internal->CleanupServer)
    {
    qDebug() << "Already cleaning up another server.";
    return;
    }

  // Keep a pointer to the server around until all the sources are
  // removed to avoid debug warnings.
  this->Internal->CleanupServer = server;

  // Remove the server from the model.
  this->removeServer(server);
}

void pqPipelineModel::removeServer(pqServer *server)
{
  if(!server)
    {
    qDebug() << "Null server not found in the pipeline model.";
    return;
    }

  // Make sure the server has an item in the model.
  pqPipelineModelServer *serverItem =
      dynamic_cast<pqPipelineModelServer *>(this->getModelItemFor(server));
  if(!serverItem)
    {
    if(this->Internal->CleanupServer == server)
      {
      this->Internal->CleanupServer = 0;
      }
    else
      {
      qWarning() << "Server not found in the pipeline model.";
      }

    return;
    }

  // Remove the server from the model.
  int row = this->Internal->Servers.indexOf(serverItem);
  this->beginRemoveRows(QModelIndex(), row, row);
  this->Internal->Servers.removeAll(serverItem);
  this->endRemoveRows();

  // Deleting the item for the server will also delete all the child
  // items in the subtree. Since the map uses QPointers, all the
  // references in the map will be null including the server. All the
  // null references can then be cleaned up.
  delete serverItem;
  this->cleanPipelineMap();
}

void pqPipelineModel::addSource(pqPipelineSource *source)
{
  if(!source)
    {
    qDebug() << "Unable to add null source to the pipeline model.";
    return;
    }

  // Make sure the source item is not in the model.
  if(this->getModelItemFor(source))
    {
    qWarning() << "Source already added to the pipeline model.";
    return;
    }

  // Find the source's parent model item.
  pqPipelineModelServer *server = dynamic_cast<pqPipelineModelServer *>(
      this->getModelItemFor(source->getServer()));
  if(!server)
    {
    qWarning() << "Source server not found in the pipeline model.";
    return;
    }

  // Create the appropriate source object. Determine the type using
  // the source's xml group name.
  pqPipelineModelSource *item = 0;
  vtkSMProxy *proxy = source->getProxy();
  if(proxy->IsA("vtkSMCompoundSourceProxy"))
    {
    item = new pqPipelineModelFilter(server, source,
        pqPipelineModel::CustomFilter);
    }
  else if(strcmp(proxy->GetXMLGroup(), "filters") == 0)
    {
    item = new pqPipelineModelFilter(server, source);
    }
  else if(strcmp(proxy->GetXMLGroup(), "sources") == 0)
    {
    item = new pqPipelineModelSource(server, source);
    }
  else
    {
    // Now we determine type using some heuristics.
    if(proxy->GetProperty("Input"))
      {
      item = new pqPipelineModelFilter(server, source);
      }
    else
      {
      item = new pqPipelineModelSource(server, source);
      }
    }

  if(item)
    {
    this->connect(source, 
        SIGNAL(visibilityChanged(pqPipelineSource*, pqDataRepresentation*)),
        this, SLOT(updateRepresentations(pqPipelineSource*)));

    // Add the source to the map.
    this->Internal->ItemMap.insert(source, item);

    // Add the source to the server list.
    QModelIndex parentIndex = this->makeIndex(server);
    int row = server->getChildCount();
    this->beginInsertRows(parentIndex, row, row);
    server->getSources().insert(row, item);
    this->endInsertRows();
    if(server->getChildCount() == 1)
      {
      emit this->firstChildAdded(parentIndex);
      }

    // If the source has more than one output port, create output port
    // items for each of the ports.
    int numOutputPorts = source->getNumberOfOutputPorts();
    if(numOutputPorts > 1)
      {
      pqPipelineModelOutputPort *outputPort = 0;
      parentIndex = this->makeIndex(item);
      this->beginInsertRows(parentIndex, 0, numOutputPorts - 1);
      for(int port = 0; port < numOutputPorts; port++)
        {
        outputPort = new pqPipelineModelOutputPort(server, item, port);

        // Add the output port to the map.
        this->Internal->ItemMap.insert(outputPort->getObject(), outputPort);

        // Add the output port to the source item.
        item->getOutputs().append(outputPort);
        }

      this->endInsertRows();
      if(server->getChildCount() == 1)
        {
        emit this->firstChildAdded(parentIndex);
        }
      }

    // Set up the source visibility state. The output ports' visibility
    // state will be updates as well.
    item->updateVisibleState(this->Internal->RenderModule);
    }
}

void pqPipelineModel::removeSource(pqPipelineSource *source)
{
  if(!source)
    {
    qDebug() << "Null source not found in the pipeline model.";
    return;
    }

  // Ignore source removal when cleaning up a server since it has
  // already been deleted.
  if(source->getServer() == this->Internal->CleanupServer)
    {
    return;
    }

  // Make sure the source has an item in the model.
  pqPipelineModelSource *sourceItem =
      dynamic_cast<pqPipelineModelSource *>(this->getModelItemFor(source));
  if(!sourceItem)
    {
    qDebug() << "Source not found in the pipeline model.";
    return;
    }

  this->disconnect(source, 0, this, 0);

  // The source should not have any outputs when it is deleted.
  int i = 0;
  pqPipelineModelOutputPort *outputPort = 0;
  QList<pqPipelineModelOutput *> outputs;
  if(source->getNumberOfOutputPorts() > 1)
    {
    for(i = 0; i < sourceItem->getChildCount(); i++)
      {
      outputPort = dynamic_cast<pqPipelineModelOutputPort *>(
          sourceItem->getChild(i));
      if(outputPort && outputPort->getChildCount() > 0)
        {
        outputs.append(outputPort);
        }
      }
    }
  else if(sourceItem->getChildCount() > 0)
    {
    outputs.append(sourceItem);
    }

  if(outputs.size() > 0)
    {
    qWarning() << "Source deleted with outputs attached.";
    }

  pqPipelineModelObject *output = 0;
  pqPipelineModelFilter *filter = 0;
  pqPipelineModelLink *link = 0;
  QList<pqPipelineModelOutput *>::Iterator iter = outputs.begin();
  for( ; iter != outputs.end(); ++iter)
    {
    // Clean up the outputs to maintain model integrity.
    for(i = (*iter)->getChildCount() - 1; i >= 0; i--)
      {
      output = (*iter)->getOutputs().at(i);
      filter = dynamic_cast<pqPipelineModelFilter *>(output);
      if(!filter)
        {
        link = dynamic_cast<pqPipelineModelLink *>(output);
        if(link)
          {
          filter = link->getSink();
          if(!filter)
            {
            qDebug() << "Empty link found in source output.";
            continue;
            }
          }
        }

      // Calling remove connection will modify the output list. Since
      // the loop counts from the end, this should not be a problem.
      this->removeConnection(*iter, filter);
      }
    }

  // If the filter has more than one input, the link items pointing
  // to it need to be removed.
  int row = 0;
  QModelIndex parentIndex;
  pqPipelineModelItem *parentItem = 0;
  filter = dynamic_cast<pqPipelineModelFilter *>(sourceItem);
  if(filter && filter->getInputs().size() > 1)
    {
    pqPipelineModelOutput *input = 0;
    for(i = filter->getInputs().size() - 1; i >= 0; i--)
      {
      input = filter->getInputs().at(i);
      row = input->getChildIndex(filter);
      link = dynamic_cast<pqPipelineModelLink *>(input->getChild(row));
      if(link)
        {
        parentIndex = this->makeIndex(input);
        this->beginRemoveRows(parentIndex, row, row);
        input->removeChild(link);
        this->endRemoveRows();
        delete link;
        }
      }
    }

  // Finally, remove the source from the model and map.
  parentItem = sourceItem->getParent();
  parentIndex = this->makeIndex(parentItem);
  row = parentItem->getChildIndex(sourceItem);
  this->beginRemoveRows(parentIndex, row, row);
  parentItem->removeChild(sourceItem);
  this->endRemoveRows();
  delete sourceItem;

  this->cleanPipelineMap();
}

void pqPipelineModel::addConnection(pqPipelineSource *source,
    pqPipelineSource *sink, int sourceOutputPort)
{
  pqPipelineModelOutput *sourceItem = 0;
  if(source->getNumberOfOutputPorts() > 1)
    {
    sourceItem = dynamic_cast<pqPipelineModelOutput *>(
        this->getModelItemFor(source->getOutputPort(sourceOutputPort)));
    }
  else
    {
    sourceItem = dynamic_cast<pqPipelineModelOutput *>(
        this->getModelItemFor(source));
    }

  if(!sourceItem)
    {
    qDebug() << "Connection source not found in the pipeline model.";
    return;
    }

  pqPipelineModelFilter *sinkItem =
      dynamic_cast<pqPipelineModelFilter *>(this->getModelItemFor(sink));
  if(!sinkItem)
    {
    qDebug() << "Connection sink not found in the pipeline model.";
    return;
    }

  this->addConnection(sourceItem, sinkItem);
}

void pqPipelineModel::removeConnection(pqPipelineSource *source,
    pqPipelineSource *sink, int sourceOutputPort)
{
  // Ignore disconnect when cleaning up a server since it has already
  // been removed.
  if(source->getServer() == this->Internal->CleanupServer)
    {
    return;
    }

  pqPipelineModelOutput *sourceItem = 0;
  if(source->getNumberOfOutputPorts() > 1)
    {
    sourceItem = dynamic_cast<pqPipelineModelOutput *>(
        this->getModelItemFor(source->getOutputPort(sourceOutputPort)));
    }
  else
    {
    sourceItem = dynamic_cast<pqPipelineModelOutput *>(
        this->getModelItemFor(source));
    }

  if(!sourceItem)
    {
    qDebug() << "Connection source not found in the pipeline model.";
    return;
    }

  pqPipelineModelFilter *sinkItem =
      dynamic_cast<pqPipelineModelFilter *>(this->getModelItemFor(sink));
  if(!sinkItem)
    {
    qDebug() << "Connection sink not found in the pipeline model.";
    return;
    }

  this->removeConnection(sourceItem, sinkItem);
}

void pqPipelineModel::updateItemName(pqServerManagerModelItem *item)
{
  pqPipelineModelItem *modelItem = this->getModelItemFor(item);
  if(modelItem)
    {
    // Update the name column.
    QModelIndex changed = this->makeIndex(modelItem, 0);
    emit this->dataChanged(changed, changed);

    // If the item is a fan in point, update the link items.
    this->updateInputLinks(dynamic_cast<pqPipelineModelFilter *>(modelItem));
    }
}

void pqPipelineModel::updateRepresentations(pqPipelineSource *source)
{
  pqPipelineModelSource *item = dynamic_cast<pqPipelineModelSource *>(
      this->getModelItemFor(source));
  if(item)
    {
    // Update the current window column.
    item->updateVisibleState(this->Internal->RenderModule);
    QModelIndex changed = this->makeIndex(item, 1);
    emit this->dataChanged(changed, changed);

    // TODO: Update the column with the display list.
    // If the item is a fan in point, update the link items.
    this->updateInputLinks(dynamic_cast<pqPipelineModelFilter *>(item), 1);
    this->updateOutputPorts(item, 1);
    }
}

void pqPipelineModel::setView(pqView *module)
{
  if(module == this->Internal->RenderModule)
    {
    return;
    }

  // If the render modules are from different servers or either one
  // of them is NULL, the whole column needs to be updated. Otherwise,
  // use the previous and current render module to look up the
  // affected sources.
  if(!this->Internal->RenderModule || !module ||
      (this->Internal->RenderModule && module &&
      (this->Internal->RenderModule->getServer() != module->getServer() ||
      this->Internal->RenderModule->getViewType() != module->getViewType())))
    {
    this->Internal->RenderModule = module;

    pqPipelineModelItem *item = 0;
    if(this->Internal->Servers.size() > 0)
      {
      item = this->Internal->Servers.first();
      }

    QModelIndex changed;
    pqPipelineModelSource *source = 0;
    while(item)
      {
      source = dynamic_cast<pqPipelineModelSource *>(item);
      if(source)
        {
        source->updateVisibleState(this->Internal->RenderModule);
        changed = this->makeIndex(source, 1);
        emit this->dataChanged(changed, changed);

        // If the item is a fan in point, update the link items.
        this->updateInputLinks(
            dynamic_cast<pqPipelineModelFilter *>(source), 1);
        this->updateOutputPorts(source, 1);
        }

      item = this->getNextModelItem(item);
      }
    }
  else
    {
    pqView *oldModule = this->Internal->RenderModule;
    this->Internal->RenderModule = module;
    if(oldModule)
      {
      this->updateDisplays(oldModule);
      }

    if(this->Internal->RenderModule)
      {
      this->updateDisplays(this->Internal->RenderModule);
      }
    }
}

void pqPipelineModel::addConnection(pqPipelineModelOutput *source,
    pqPipelineModelFilter *sink)
{
  pqPipelineModelServer *server = source->getServer();
  if(!server)
    {
    return;
    }

  int row = 0;
  QModelIndex parentIndex;
  if(sink->getInputs().size() == 0)
    {
    // The sink item needs to be moved from the server's source list
    // to the source's output list. Notify observers that the sink
    // will be temporarily removed.
    emit this->movingIndex(this->makeIndex(sink));
    parentIndex = this->makeIndex(server);
    row = server->getChildIndex(sink);
    this->beginRemoveRows(parentIndex, row, row);
    server->getSources().removeAll(sink);
    this->endRemoveRows();

    parentIndex = this->makeIndex(source);
    row = source->getChildCount();
    this->beginInsertRows(parentIndex, row, row);
    source->getOutputs().append(sink);
    sink->getInputs().append(source);
    this->endInsertRows();
    if(source->getChildCount() == 1)
      {
      emit this->firstChildAdded(parentIndex);
      }

    emit this->indexRestored(this->makeIndex(sink));
    }
  else
    {
    // The other cases both require a link object.
    pqPipelineModelLink *link = new pqPipelineModelLink(server);
    link->setSource(source);
    link->setSink(sink);
    if(sink->getInputs().size() == 1)
      {
      // If the sink has one input, it needs to be moved to the server's
      // source list. An additional link item needs to be added in place
      // of the sink item.
      pqPipelineModelLink *otherLink = new pqPipelineModelLink(server);
      pqPipelineModelOutput *otherSource = sink->getInputs().first();
      otherLink->setSource(otherSource);
      otherLink->setSink(sink);

      emit this->movingIndex(this->makeIndex(sink));
      parentIndex = this->makeIndex(otherSource);
      row = otherSource->getChildIndex(sink);
      this->beginRemoveRows(parentIndex, row, row);
      otherSource->getOutputs().removeAll(sink);
      this->endRemoveRows();

      int serverRow = server->getChildCount();
      this->beginInsertRows(this->makeIndex(server), serverRow, serverRow);
      sink->getInputs().append(source);
      server->getSources().append(sink);
      this->endInsertRows();

      this->beginInsertRows(parentIndex, row, row);
      otherSource->getOutputs().insert(row, otherLink);
      this->endInsertRows();
      if(otherSource->getChildCount() == 1)
        {
        emit this->firstChildAdded(parentIndex);
        }

      emit this->indexRestored(this->makeIndex(sink));
      }
    else
      {
      sink->getInputs().append(source);
      }

    // A link item needs to be added to the source's output list.
    parentIndex = this->makeIndex(source);
    row = source->getChildCount();
    this->beginInsertRows(parentIndex, row, row);
    source->getOutputs().append(link);
    this->endInsertRows();
    if(source->getChildCount() == 1)
      {
      emit this->firstChildAdded(parentIndex);
      }
    }
}

void pqPipelineModel::removeConnection(pqPipelineModelOutput *source,
    pqPipelineModelFilter *sink)
{
  pqPipelineModelServer *server = source->getServer();
  if(!server)
    {
    return;
    }

  int row = 0;
  QModelIndex parentIndex;
  if(sink->getInputs().size() == 1)
    {
    // The sink needs to be moved to the server's source list when
    // the last input is removed.
    emit this->movingIndex(this->makeIndex(sink));
    parentIndex = this->makeIndex(source);
    row = source->getChildIndex(sink);
    this->beginRemoveRows(parentIndex, row, row);
    sink->getInputs().removeAll(source);
    source->getOutputs().removeAll(sink);
    this->endRemoveRows();

    parentIndex = this->makeIndex(server);
    row = server->getChildCount();
    this->beginInsertRows(parentIndex, row, row);
    server->getSources().append(sink);
    this->endInsertRows();
    emit this->indexRestored(this->makeIndex(sink));
    }
  else
    {
    // If there are only two inputs, removing the connection will
    // cause the sink to be moved.
    if(sink->getInputs().size() == 2)
      {
      emit this->movingIndex(this->makeIndex(sink));
      }

    // The link item in the source's output needs to be removed.
    parentIndex = this->makeIndex(source);
    row = source->getChildIndex(sink);
    pqPipelineModelLink *link = dynamic_cast<pqPipelineModelLink *>(
        source->getChild(row));
    this->beginRemoveRows(parentIndex, row, row);
    source->getOutputs().removeAll(link);
    this->endRemoveRows();
    delete link;

    // Remove the source from the sink's input list.
    row = sink->getInputs().indexOf(source);
    sink->getInputs().removeAt(row);
    if(sink->getInputs().size() == 1)
      {
      // The sink item needs to be moved from the server's source list
      // to the other source's output list. The link item needs to be
      // removed as well.
      pqPipelineModelOutput *otherSource = sink->getInputs().at(0);
      row = otherSource->getChildIndex(sink);
      pqPipelineModelLink *otherLink = dynamic_cast<pqPipelineModelLink *>(
          otherSource->getChild(row));

      cout << __LINE__ << endl;
      parentIndex = this->makeIndex(otherSource);
      this->beginRemoveRows(parentIndex, row, row);
      otherSource->removeChild(otherLink);
      this->endRemoveRows();
      delete otherLink;
      cout << __LINE__ << endl;

      QModelIndex serverIndex = this->makeIndex(server);
      int serverRow = server->getChildIndex(sink);
      this->beginRemoveRows(serverIndex, serverRow, serverRow);
      server->getSources().removeAll(sink);
      this->endRemoveRows();
      cout << __LINE__ << endl;

      this->beginInsertRows(parentIndex, row, row);
      otherSource->getOutputs().insert(row, sink);
      this->endInsertRows();
      if(otherSource->getChildCount() == 1)
        {
        emit this->firstChildAdded(parentIndex);
        }

      emit this->indexRestored(this->makeIndex(sink));
      cout << __LINE__ << endl;
      }
    }
}

void pqPipelineModel::updateDisplays(pqView *module)
{
  QModelIndex changed;
  QList<pqRepresentation*> reprs = module->getRepresentations();

  foreach (pqRepresentation* repr, reprs)
    {
    pqDataRepresentation* dataRepr = qobject_cast<pqDataRepresentation*>(repr);
    if(dataRepr)
      {
      pqPipelineModelSource *source = dynamic_cast<pqPipelineModelSource *>(
        this->getModelItemFor(dataRepr->getInput()));
      if(source)
        {
        source->updateVisibleState(this->Internal->RenderModule);
        changed = this->makeIndex(source, 1);
        emit this->dataChanged(changed, changed);

        // If the item is a fan in point, update the link items.
        this->updateInputLinks(
            dynamic_cast<pqPipelineModelFilter *>(source), 1);
        this->updateOutputPorts(source, 1);
        }
      }
    }
}

void pqPipelineModel::updateInputLinks(pqPipelineModelFilter *sink, int column)
{
  if(sink && sink->getInputs().size() > 1)
    {
    QList<pqPipelineModelOutput *>::Iterator source =
        sink->getInputs().begin();
    for( ; source != sink->getInputs().end(); ++source)
      {
      pqPipelineModelLink *link = dynamic_cast<pqPipelineModelLink *>(
          (*source)->getChild((*source)->getChildIndex(sink)));
      if(link)
        {
        QModelIndex changed = this->makeIndex(link, column);
        emit this->dataChanged(changed, changed);
        }
      }
    }
}

void pqPipelineModel::updateOutputPorts(pqPipelineModelSource *source,
    int column)
{
  if(source && source->getSource()->getNumberOfOutputPorts() > 1)
    {
    QList<pqPipelineModelObject *>::Iterator output =
        source->getOutputs().begin();
    for( ; output != source->getOutputs().end(); ++output)
      {
      pqPipelineModelOutputPort* outputPort =
          dynamic_cast<pqPipelineModelOutputPort*>(*output);
      if(outputPort)
        {
        QModelIndex changed = this->makeIndex(outputPort, column);
        emit this->dataChanged(changed, changed);
        }
      }
    }
}

pqPipelineModelItem *pqPipelineModel::getModelItemFor(
    pqServerManagerModelItem *item) const
{
  QMap<pqServerManagerModelItem *, QPointer<pqPipelineModelItem> >::Iterator iter =
      this->Internal->ItemMap.find(item);
  if(iter != this->Internal->ItemMap.end())
    {
    return *iter;
    }

  pqOutputPort *output = qobject_cast<pqOutputPort *>(item);
  if(output && output->getPortNumber() == 0)
    {
    return this->getModelItemFor(output->getSource());
    }

  return 0;
}

QModelIndex pqPipelineModel::makeIndex(pqPipelineModelItem *item,
    int column) const
{
  int row = 0;
  pqPipelineModelServer *server = dynamic_cast<pqPipelineModelServer *>(item);
  if(server)
    {
    row = this->Internal->Servers.indexOf(server);
    return this->createIndex(row, column, item);
    }
  else
    {
    row = item->getParent()->getChildIndex(item);
    return this->createIndex(row, column, item);
    }
}

void pqPipelineModel::cleanPipelineMap()
{
  // Clean out all the items in the map with null pointers.
  QMap<pqServerManagerModelItem *, QPointer<pqPipelineModelItem> >::Iterator iter =
      this->Internal->ItemMap.begin();
  while(iter != this->Internal->ItemMap.end())
    {
    if(iter->isNull())
      {
      iter = this->Internal->ItemMap.erase(iter);
      }
    else
      {
      ++iter;
      }
    }
}

pqPipelineModelItem *pqPipelineModel::getNextModelItem(
    pqPipelineModelItem *item, pqPipelineModelItem *root) const
{
  if(item->getChildCount() > 0)
    {
    return item->getChild(0);
    }

  // Search up the ancestors for an item with multiple children.
  // The next item will be the next child.
  int row = 0;
  int count = 0;
  pqPipelineModelItem *itemParent = 0;
  pqPipelineModelServer *server = 0;
  while(item != root)
    {
    itemParent = item->getParent();
    if(itemParent)
      {
      count = itemParent->getChildCount();
      if(count > 1)
        {
        row = itemParent->getChildIndex(item) + 1;
        if(row < count)
          {
          return itemParent->getChild(row);
          }
        }

      item = itemParent;
      }
    else
      {
      server = dynamic_cast<pqPipelineModelServer *>(item);
      if(!server)
        {
        break;
        }

      count = this->Internal->Servers.size();
      row = this->Internal->Servers.indexOf(server) + 1;
      if(row < 0 || row >= count)
        {
        break;
        }
      else
        {
        return this->Internal->Servers[row];
        }
      }
    }

  return 0;
}

void pqPipelineModel::initializePixmaps()
{
  if(this->PixmapList == 0)
    {
    this->PixmapList = new QPixmap[pqPipelineModelInternal::LAST];
    
    this->PixmapList[pqPipelineModelItem::SERVER].load(
      ":/pqWidgets/Icons/pqServer16.png");
    this->PixmapList[pqPipelineModelItem::LINK].load(
      ":/pqWidgets/Icons/pqLinkBack16.png");
    this->PixmapList[pqPipelineModelItem::GEOMETRY].load(
      ":/pqWidgets/Icons/pq3DView16.png");
    this->PixmapList[pqPipelineModelItem::BARCHART].load(
        ":/pqWidgets/Icons/pqHistogram16.png");
    this->PixmapList[pqPipelineModelItem::LINECHART].load(
       ":/pqWidgets/Icons/pqLineChart16.png");
    this->PixmapList[pqPipelineModelItem::TABLE].load(
       ":/pqWidgets/Icons/pqSpreadsheet16.png");
    this->PixmapList[pqPipelineModelItem::INDETERMINATE].load(
      ":/pqWidgets/Icons/pq3DView16.png");
    this->PixmapList[pqPipelineModelInternal::EYEBALL].load(
      ":/pqWidgets/Icons/pqEyeball16.png");
    this->PixmapList[pqPipelineModelInternal::EYEBALL_GRAY].load(
      ":/pqWidgets/Icons/pqEyeballd16.png");
    }
}


