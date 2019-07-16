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
#include "pqPipelineModel.h"

#include "pqBoxChartView.h"
#include "pqLiveInsituManager.h"
#include "pqLiveInsituVisualizationManager.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqPlotMatrixView.h"
#include "pqServer.h"
#include "pqServerConfiguration.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqSpreadSheetView.h"
#include "pqUndoStack.h"
#include "pqXYBarChartView.h"
#include "pqXYChartView.h"
#include "pqXYHistogramChartView.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkSMLiveInsituLinkProxy.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

#include <QApplication>
#include <QFont>
#include <QSignalMapper>
#include <QString>
#include <QStyle>
#include <QtDebug>

#include <cassert>

class ModifiedLiveInsituLink : public vtkCommand
{
public:
  ModifiedLiveInsituLink(pqServer* insituSession, pqPipelineModel* pipelineModel)
    : InsituServer(insituSession)
    , PipelineModel(pipelineModel)
  {
  }
  void Execute(vtkObject* vtkNotUsed(caller), unsigned long, void* vtkNotUsed(data)) override
  {
    this->PipelineModel->updateData(this->InsituServer);
  }

private:
  pqServer* InsituServer;
  pqPipelineModel* PipelineModel;
};

namespace PipelineModelIconType
{
static const QString SERVER = "SERVER";
static const QString SECURE_SERVER = "SECURE_SERVER";
static const QString LINK = "LINK";
static const QString GEOMETRY = "GEOMETRY";
static const QString BARCHART = pqXYBarChartView::XYBarChartViewType();
static const QString BOXCHART = pqBoxChartView::chartViewType();
static const QString HISTOGRAMCHART = pqXYHistogramChartView::XYHistogramChartViewType();
static const QString LINECHART = pqXYChartView::XYChartViewType();
static const QString PLOTMATRIX = pqPlotMatrixView::viewType();
static const QString TABLE = pqSpreadSheetView::spreadsheetViewType();
static const QString INDETERMINATE = "INDETERMINATE";
static const QString NONE = "None";
static const QString EYEBALL = "EYEBALL";
static const QString EYEBALL_GRAY = "EYEBALL_GRAY";
static const QString INSITU_EXTRACT = "INSITU_EXTRACT";
static const QString INSITU_EXTRACT_GRAY = "INSITU_EXTRACT_GRAY";
static const QString INSITU_SERVER_RUNNING = "INSITU_SERVER_RUNNING";
static const QString INSITU_SERVER_PAUSED = "INSITU_SERVER_PAUSED";
static const QString INSITU_BREAKPOINT = "INSITU_BREAKPOINT";
static const QString INSITU_WRITER_PARAMETERS = "INSITU_WRITER_PARAMETERS";
static const QString CINEMA_MARK = "CINEMA_MARK";
static const QString LAST = "LAST";
}

//-----------------------------------------------------------------------------
class pqPipelineModelDataItem : public QObject
{
  bool InConstructor;

public:
  static vtkNew<vtkSMParaViewPipelineControllerWithRendering> Controller;

public:
  pqPipelineModel* Model;
  pqPipelineModelDataItem* Parent;
  QList<pqPipelineModelDataItem*> Children;
  pqServerManagerModelItem* Object;
  pqPipelineModel::ItemType Type;
  QString VisibilityIcon;
  bool Selectable;

  // This is a terrible iVar, agreed. But it makes my life easier.
  // This is valid only for elements of Type==Proxy. These refer to the link
  // items present for this item, if any. This list is automatically kept
  // updated.
  QList<pqPipelineModelDataItem*> Links;

  pqPipelineModelDataItem(QObject* p, pqServerManagerModelItem* object,
    pqPipelineModel::ItemType itemType, pqPipelineModel* model)
    : QObject(p)
  {
    this->InConstructor = true;
    this->Selectable = true;
    this->Model = model;
    this->Parent = NULL;
    this->Object = object;
    this->Type = itemType;
    this->VisibilityIcon = PipelineModelIconType::LAST;
    if (itemType == pqPipelineModel::Link)
    {
      pqPipelineModelDataItem* proxyItem = model->getDataItem(object, NULL, pqPipelineModel::Proxy);
      assert(proxyItem != 0);
      proxyItem->Links.push_back(this);
    }
    if (this->Object)
    {
      this->updateVisibilityIcon(this->Model->view(), false);
    }
    this->InConstructor = false;
  }
  ~pqPipelineModelDataItem() override
  {
    if (this->Type == pqPipelineModel::Link && this->Model->Internal)
    {
      pqPipelineModelDataItem* proxyItem =
        this->Model->getDataItem(this->Object, NULL, pqPipelineModel::Proxy);
      if (proxyItem)
      {
        proxyItem->Links.removeAll(this);
      }
    }
  }

  pqPipelineModelDataItem& operator=(const pqPipelineModelDataItem& other)
  {
    this->Object = other.Object;
    this->Type = other.Type;
    this->VisibilityIcon = other.VisibilityIcon;
    foreach (pqPipelineModelDataItem* otherChild, other.Children)
    {
      pqPipelineModelDataItem* child =
        new pqPipelineModelDataItem(this, NULL, pqPipelineModel::Invalid, this->Model);
      this->addChild(child);
      *child = *otherChild;
    }
    return *this;
  }

  // no need to call this generally. Only needed when operator = is used.
  void updateLinks()
  {
    if (this->Type == pqPipelineModel::Link)
    {
      pqPipelineModelDataItem* proxyItem =
        this->Model->getDataItem(this->Object, NULL, pqPipelineModel::Proxy);
      assert(proxyItem != 0);
      proxyItem->Links.push_back(this);
    }
    foreach (pqPipelineModelDataItem* child, this->Children)
    {
      child->updateLinks();
    }
  }

  pqPipelineModel::ItemType getType() { return this->Type; }
  int getIndexInParent()
  {
    if (!this->Parent)
    {
      return 0;
    }
    return this->Parent->Children.indexOf(this);
  }

  QString getIconType() const
  {
    switch (this->Type)
    {
      case pqPipelineModel::Server:
      {
        pqServer* server = qobject_cast<pqServer*>(this->Object);
        if (server->getResource().configuration().isPortForwarding())
        {
          return PipelineModelIconType::SECURE_SERVER;
        }

        vtkSMLiveInsituLinkProxy* proxy = pqLiveInsituManager::linkProxy(server);
        return proxy ? ((vtkSMPropertyHelper(proxy, "SimulationPaused").GetAs<int>() == 1)
                           ? PipelineModelIconType::INSITU_SERVER_PAUSED
                           : PipelineModelIconType::INSITU_SERVER_RUNNING)
                     : PipelineModelIconType::SERVER;
      }

      case pqPipelineModel::Proxy:
      {
        pqPipelineSource* source = qobject_cast<pqPipelineSource*>(this->Object);
        if (source->getNumberOfOutputPorts() > 1)
        {
          // icon is shown on the output ports.
          return PipelineModelIconType::INDETERMINATE;
        }
        return this->getIconType(source->getOutputPort(0));
      }

      case pqPipelineModel::Port:
      {
        pqOutputPort* port = qobject_cast<pqOutputPort*>(this->Object);
        return this->getIconType(port);
      }

      case pqPipelineModel::Link:
        return PipelineModelIconType::LINK;

      case pqPipelineModel::Invalid:
        return PipelineModelIconType::INDETERMINATE;
    }
    return PipelineModelIconType::INDETERMINATE;
  }

  void addChild(pqPipelineModelDataItem* child)
  {
    if (child->Parent)
    {
      qCritical() << "child has parent.";
      return;
    }
    child->setParent(this);
    child->Parent = this;
    this->Children.push_back(child);
  }

  void removeChild(pqPipelineModelDataItem* child)
  {
    if (child->Parent != this)
    {
      qCritical() << "Cannot remove a non-child.";
      return;
    }
    child->setParent(NULL);
    child->Parent = NULL;
    this->Children.removeAll(child);
  }

  // returns true when the icon has changed.
  bool updateVisibilityIcon(pqView* view, bool traverse_subtree)
  {
    QString newIcon = PipelineModelIconType::LAST;
    switch (this->Type)
    {
      case pqPipelineModel::Proxy:
      case pqPipelineModel::Link:
      {
        pqPipelineSource* source = qobject_cast<pqPipelineSource*>(this->Object);
        if (source && source->getNumberOfOutputPorts() == 1)
        {
          newIcon = this->getVisibilityIcon(source->getOutputPort(0), view);
        }
      }
      break;

      case pqPipelineModel::Port:
      {
        pqOutputPort* port = qobject_cast<pqOutputPort*>(this->Object);
        newIcon = this->getVisibilityIcon(port, view);
      }
      break;

      case pqPipelineModel::Server:
      {
        pqServer* server = qobject_cast<pqServer*>(this->Object);
        newIcon = (pqLiveInsituManager::isInsituServer(server) ? this->getBreakpointVisibilityIcon()
                                                               : PipelineModelIconType::LAST);
      }
      break;

      default:
        break;
    }

    bool ret_val = false;
    if (this->VisibilityIcon != newIcon)
    {
      this->VisibilityIcon = newIcon;
      if (!this->InConstructor && this->Model)
      {
        this->Model->itemDataChanged(this);
      }
      ret_val = true;
    }
    if (traverse_subtree)
    {
      foreach (pqPipelineModelDataItem* child, this->Children)
      {
        child->updateVisibilityIcon(view, traverse_subtree);
      }
    }
    return ret_val;
  }

  bool isModified() const
  {
    pqProxy* proxy = qobject_cast<pqProxy*>(this->Object);
    return (proxy && (proxy->modifiedState() != pqProxy::UNMODIFIED));
  }

private:
  QString getBreakpointVisibilityIcon() const
  {
    pqLiveInsituManager* server = pqLiveInsituManager::instance();
    return (server->hasBreakpoint() ? PipelineModelIconType::INSITU_BREAKPOINT
                                    : PipelineModelIconType::LAST);
  }

  QString getVisibilityIcon(pqOutputPort* port, pqView* view) const
  {
    if (!port)
    {
      return PipelineModelIconType::LAST;
    }

    // I'm not a huge fan of this hacky way we are dealing with catalyst. We'll
    // have to come back and cleanly address how the pqPipelineModel deals with
    // "visibility" in a more generic, session-centric way.
    if (pqLiveInsituManager::isInsituServer(port->getServer()))
    {
      if (pqLiveInsituManager::isWriterParametersProxy(port->getSourceProxy()))
      {
        return PipelineModelIconType::LAST;
      }
      pqLiveInsituVisualizationManager* mgr =
        pqLiveInsituManager::managerFromInsitu(port->getServer());
      if (mgr)
      {
        return mgr->hasExtracts(port) ? PipelineModelIconType::INSITU_EXTRACT
                                      : PipelineModelIconType::INSITU_EXTRACT_GRAY;
      }
      return PipelineModelIconType::LAST;
    }

    if (!view)
    {
      // If no view is present, it implies that a suitable type of view
      // will be created.
      return PipelineModelIconType::EYEBALL_GRAY;
    }
    if (this->Controller->GetVisibility(
          port->getSourceProxy(), port->getPortNumber(), view->getViewProxy()))
    {
      return PipelineModelIconType::EYEBALL;
    }
    return view->getViewProxy()->CanDisplayData(port->getSourceProxy(), port->getPortNumber())
      ? PipelineModelIconType::EYEBALL_GRAY
      : PipelineModelIconType::LAST;
  }

  QString getIconType(pqOutputPort* port) const
  {
    if (port->getSource()->property("INSITU_EXTRACT").toBool())
    {
      return PipelineModelIconType::INSITU_EXTRACT;
    }
    else if (port->getSourceProxy()->HasAnnotation("CINEMA"))
    {
      return PipelineModelIconType::CINEMA_MARK;
    }
    else if (pqLiveInsituManager::isWriterParametersProxy(port->getSourceProxy()))
    {
      return PipelineModelIconType::INSITU_WRITER_PARAMETERS;
    }

    QString iconType =
      this->Controller->GetPipelineIcon(port->getSourceProxy(), port->getPortNumber());
    if (!iconType.isEmpty())
    {
      return iconType;
    }

    return PipelineModelIconType::GEOMETRY;
  }
};

vtkNew<vtkSMParaViewPipelineControllerWithRendering> pqPipelineModelDataItem::Controller;

//-----------------------------------------------------------------------------
class pqPipelineModelInternal
{
public:
  pqPipelineModelInternal(pqPipelineModel* parent)
    : Root(parent, NULL, pqPipelineModel::Invalid, parent)
  {
    this->ModifiedFont.setBold(true);
    this->DelayedUpdateVisibilityTimer.setSingleShot(true);
  }

  QFont ModifiedFont;
  pqPipelineModelDataItem Root;
  pqTimer DelayedUpdateVisibilityTimer;
  QList<QPointer<pqPipelineSource> > DelayedUpdateVisibilityItems;
};

//-----------------------------------------------------------------------------
void pqPipelineModel::constructor()
{
  this->FilterRoleSession = NULL;
  this->Internal = new pqPipelineModelInternal(this);
  QObject::connect(&this->Internal->DelayedUpdateVisibilityTimer, SIGNAL(timeout()), this,
    SLOT(delayedUpdateVisibilityTimeout()));

  this->Editable = true;
  this->View = NULL;

  QObject::connect(pqLiveInsituManager::instance(), SIGNAL(connectionInitiated(pqServer*)), this,
    SLOT(onInsituConnectionInitiated(pqServer*)));

  this->PixmapMap[PipelineModelIconType::SERVER].load(":/pqWidgets/Icons/pqServer16.png");
  this->PixmapMap[PipelineModelIconType::SECURE_SERVER].load(
    ":/pqWidgets/Icons/pqSecureServer16.png");
  this->PixmapMap[PipelineModelIconType::LINK].load(":/pqWidgets/Icons/pqLinkBack16.png");
  this->PixmapMap[PipelineModelIconType::GEOMETRY].load(":/pqWidgets/Icons/pq3DView16.png");
  this->PixmapMap[PipelineModelIconType::BARCHART].load(":/pqWidgets/Icons/pqHistogram16.png");
  this->PixmapMap[PipelineModelIconType::BOXCHART].load(":/pqWidgets/Icons/pqBoxChart16.png");
  this->PixmapMap[PipelineModelIconType::HISTOGRAMCHART].load(
    ":/pqWidgets/Icons/pqHistogram16.png");
  this->PixmapMap[PipelineModelIconType::LINECHART].load(":/pqWidgets/Icons/pqLineChart16.png");
  this->PixmapMap[PipelineModelIconType::PLOTMATRIX].load(":/pqWidgets/Icons/pqLineChart16.png");
  this->PixmapMap[PipelineModelIconType::TABLE].load(":/pqWidgets/Icons/pqSpreadsheet16.png");
  this->PixmapMap[PipelineModelIconType::INDETERMINATE].load(":/pqWidgets/Icons/pq3DView16.png");
  this->PixmapMap[PipelineModelIconType::NONE].load(":/pqWidgets/Icons/pq3DView16.png");
  this->PixmapMap[PipelineModelIconType::EYEBALL].load(":/pqWidgets/Icons/pqEyeball.png");
  this->PixmapMap[PipelineModelIconType::EYEBALL_GRAY].load(
    ":/pqWidgets/Icons/pqEyeballClosed.png");
  this->PixmapMap[PipelineModelIconType::INSITU_EXTRACT].load(":/pqWidgets/Icons/pqLinkIn16.png");
  this->PixmapMap[PipelineModelIconType::INSITU_EXTRACT_GRAY].load(
    ":/pqWidgets/Icons/pqLinkIn16d.png");
  this->PixmapMap[PipelineModelIconType::INSITU_SERVER_RUNNING].load(
    ":/pqWidgets/Icons/pqInsituServerRunning16.png");
  this->PixmapMap[PipelineModelIconType::INSITU_SERVER_PAUSED].load(
    ":/pqWidgets/Icons/pqInsituServerPaused16.png");
  this->PixmapMap[PipelineModelIconType::INSITU_BREAKPOINT].load(
    ":/pqWidgets/Icons/pqInsituBreakpoint16.png");
  this->PixmapMap[PipelineModelIconType::INSITU_WRITER_PARAMETERS].load(
    ":/pqWidgets/Icons/pqSave32.png");
  this->PixmapMap[PipelineModelIconType::CINEMA_MARK].load(
    ":/pqWidgets/Icons/cinemascience_mark.png");
}

//-----------------------------------------------------------------------------
pqPipelineModel::pqPipelineModel(QObject* p)
  : QAbstractItemModel(p)
  , LinkCallback(NULL)
{
  this->constructor();
}

//-----------------------------------------------------------------------------
pqPipelineModel::pqPipelineModel(const pqPipelineModel& other, QObject* parentObject)
  : QAbstractItemModel(parentObject)
  , LinkCallback(NULL)
{
  this->constructor();
  this->Internal->Root = other.Internal->Root;
  this->Internal->Root.updateLinks();
}

//-----------------------------------------------------------------------------
pqPipelineModel::pqPipelineModel(const pqServerManagerModel& other, QObject* parentObject)
  : QAbstractItemModel(parentObject)
  , LinkCallback(NULL)
{
  this->constructor();

  // Build a pipeline model from the current server manager model.
  QList<pqPipelineSource*> sources;
  QList<pqPipelineSource*>::Iterator source;
  QList<pqServer*> servers = other.findItems<pqServer*>();
  QList<pqServer*>::Iterator server = servers.begin();
  for (; server != servers.end(); ++server)
  {
    // Add the server to the model.
    this->addServer(*server);

    // Add the sources for the server.
    sources = other.findItems<pqPipelineSource*>(*server);
    for (source = sources.begin(); source != sources.end(); ++source)
    {
      this->addSource(*source);
    }

    // Set up the pipeline connections.
    for (source = sources.begin(); source != sources.end(); ++source)
    {
      int numOutputPorts = (*source)->getNumberOfOutputPorts();
      for (int port = 0; port < numOutputPorts; port++)
      {
        int numConsumers = (*source)->getNumberOfConsumers(port);
        for (int i = 0; i < numConsumers; ++i)
        {
          this->addConnection(*source, (*source)->getConsumer(port, i), port);
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
pqPipelineModel::~pqPipelineModel()
{
  // setting this->Internal to NULL keeps the ~pqPipelineModelDataItem() from
  // trying to update the link connections.
  pqPipelineModelInternal* internal = this->Internal;
  this->Internal = NULL;
  delete internal;

  if (this->LinkCallback)
  {
    this->LinkCallback->Delete();
  }
}

//-----------------------------------------------------------------------------
void pqPipelineModel::onInsituConnectionInitiated(pqServer* displaySession)
{
  pqLiveInsituVisualizationManager* manager =
    pqLiveInsituManager::instance()->managerFromDisplay(displaySession);
  vtkSMLiveInsituLinkProxy* proxy = pqLiveInsituManager::linkProxy(manager->insituSession());
  if (manager && proxy)
  {
    if (this->LinkCallback)
    {
      this->LinkCallback->Delete();
    }
    this->LinkCallback = new ModifiedLiveInsituLink(manager->insituSession(), this);
    proxy->AddObserver(vtkCommand::ModifiedEvent, this->LinkCallback);
    QObject::connect(pqLiveInsituManager::instance(), SIGNAL(breakpointAdded(pqServer*)), this,
      SLOT(updateDataServer(pqServer*)));
    QObject::connect(pqLiveInsituManager::instance(), SIGNAL(breakpointRemoved(pqServer*)), this,
      SLOT(updateDataServer(pqServer*)));
  }
}

//-----------------------------------------------------------------------------
pqPipelineModel::ItemType pqPipelineModel::getTypeFor(const QModelIndex& idx) const
{
  if (idx.isValid() && idx.model() == this)
  {
    pqPipelineModelDataItem* item =
      reinterpret_cast<pqPipelineModelDataItem*>(idx.internalPointer());
    return item->getType();
  }

  return pqPipelineModel::Invalid;
}

//-----------------------------------------------------------------------------
void pqPipelineModel::setSelectable(const QModelIndex& idx, bool selectable)
{
  if (idx.isValid() && idx.model() == this)
  {
    pqPipelineModelDataItem* item =
      reinterpret_cast<pqPipelineModelDataItem*>(idx.internalPointer());
    item->Selectable = selectable;
  }
}

//-----------------------------------------------------------------------------
void pqPipelineModel::setSubtreeSelectable(pqServerManagerModelItem* smitem, bool selectable)
{
  pqOutputPort* port = qobject_cast<pqOutputPort*>(smitem);
  if (port && port->getSource())
  {
    smitem = port->getSource();
  }

  pqPipelineModelDataItem* item;
  pqServer* server = qobject_cast<pqServer*>(smitem);
  if (server)
  {
    item = this->getDataItem(smitem, &this->Internal->Root, pqPipelineModel::Server);
  }
  else
  {
    item = this->getDataItem(smitem, &this->Internal->Root, pqPipelineModel::Proxy);
  }
  this->setSubtreeSelectable(item, selectable);
}

//-----------------------------------------------------------------------------
void pqPipelineModel::setSubtreeSelectable(pqPipelineModelDataItem* item, bool selectable)
{
  if (item)
  {
    item->Selectable = selectable;
    foreach (pqPipelineModelDataItem* link, item->Links)
    {
      link->Selectable = selectable;
    }
    foreach (pqPipelineModelDataItem* child, item->Children)
    {
      this->setSubtreeSelectable(child, selectable);
    }
  }
}

//-----------------------------------------------------------------------------
bool pqPipelineModel::isSelectable(const QModelIndex& idx) const
{
  if (idx.isValid() && idx.model() == this)
  {
    pqPipelineModelDataItem* item =
      reinterpret_cast<pqPipelineModelDataItem*>(idx.internalPointer());
    return item->Selectable;
  }

  return false;
}
//-----------------------------------------------------------------------------
QModelIndex pqPipelineModel::getNextIndex(const QModelIndex idx, const QModelIndex& root) const
{
  // If the index has children, return the first child.
  if (this->rowCount(idx) > 0)
  {
    return this->index(0, 0, idx);
  }

  // Search up the parent chain for an index with more children.
  QModelIndex current = idx;
  while (current.isValid() && current != root)
  {
    QModelIndex parentIndex = current.parent();
    if (current.row() < this->rowCount(parentIndex) - 1)
    {
      return this->index(current.row() + 1, 0, parentIndex);
    }

    current = parentIndex;
  }

  return QModelIndex();
}

//-----------------------------------------------------------------------------
int pqPipelineModel::rowCount(const QModelIndex& parentIndex) const
{
  if (parentIndex.isValid() && parentIndex.model() == this)
  {
    pqPipelineModelDataItem* item =
      reinterpret_cast<pqPipelineModelDataItem*>(parentIndex.internalPointer());
    return item->Children.size();
  }
  return this->Internal->Root.Children.size();
}

//-----------------------------------------------------------------------------
int pqPipelineModel::columnCount(const QModelIndex&) const
{
  return 2;
}

//-----------------------------------------------------------------------------
bool pqPipelineModel::hasChildren(const QModelIndex& parentIndex) const
{
  return this->rowCount(parentIndex) > 0;
}

//-----------------------------------------------------------------------------
QModelIndex pqPipelineModel::index(int row, int column, const QModelIndex& parentIndex) const
{
  // Make sure the row and column number is within range.
  int rows = this->rowCount(parentIndex);
  int columns = this->columnCount(parentIndex);
  if (row < 0 || row >= rows || column < 0 || column >= columns)
  {
    return QModelIndex();
  }

  pqPipelineModelDataItem* parentItem = 0;
  if (parentIndex.isValid())
  {
    parentItem = reinterpret_cast<pqPipelineModelDataItem*>(parentIndex.internalPointer());
  }
  else
  {
    // The parent refers to the model root.
    parentItem = &this->Internal->Root;
  }

  return this->createIndex(row, column, parentItem->Children[row]);
}

//-----------------------------------------------------------------------------
QModelIndex pqPipelineModel::parent(const QModelIndex& idx) const
{
  if (idx.isValid() && idx.model() == this)
  {
    pqPipelineModelDataItem* item =
      reinterpret_cast<pqPipelineModelDataItem*>(idx.internalPointer());

    pqPipelineModelDataItem* _parent = item->Parent;
    return this->getIndex(_parent);
  }

  return QModelIndex();
}

//-----------------------------------------------------------------------------
QVariant pqPipelineModel::data(const QModelIndex& idx, int role) const
{
  if (!idx.isValid() || idx.model() != this)
  {
    return QVariant();
  }

  pqPipelineModelDataItem* item = reinterpret_cast<pqPipelineModelDataItem*>(idx.internalPointer());

  pqServer* server = qobject_cast<pqServer*>(item->Object);
  pqPipelineSource* source = qobject_cast<pqPipelineSource*>(item->Object);
  pqOutputPort* port = qobject_cast<pqOutputPort*>(item->Object);
  switch (role)
  {
    case Qt::ToolTipRole:
      if (source && source->getProxy()->HasAnnotation("tooltip"))
      {
        return QVariant(source->getProxy()->GetAnnotation("tooltip"));
      }
      VTK_FALLTHROUGH;
    case Qt::DisplayRole:
      if (idx.column() == 1)
      {
        return QIcon(this->PixmapMap[item->VisibilityIcon]);
      }
      VTK_FALLTHROUGH;
    // *** don't break.
    case Qt::EditRole:
      if (idx.column() == 0)
      {
        if (server)
        {
          const pqServerResource& resource = server->getResource();
          const bool is_configuration_default = resource.configuration().isNameDefault();
          const auto name =
            is_configuration_default ? resource.toURI() : resource.configuration().name();
          int time = server->getRemainingLifeTime();
          QString timeLeft =
            time > -1 ? QString(" (%1min left)").arg(QString::number(time)) : QString();
          return is_configuration_default
            ? QString("%1 %3").arg(name).arg(timeLeft)
            : QString("%1 (%2)%3").arg(name).arg(resource.configuration().URI()).arg(timeLeft);
        }
        else if (source)
        {
          return QVariant(source->getSMName());
        }
        else if (port)
        {
          return port->getPortName();
        }
        else
        {
          qDebug() << "Cannot decide type.";
        }
      }
      break;

    case Qt::TextColorRole:
    {
      if (idx.column() == 0 && server && server->getRemainingLifeTime() > -1 &&
        server->getRemainingLifeTime() <= 5)
      {
        return qVariantFromValue<QColor>(QColor(Qt::red));
      }
      break;
    }

    case Qt::DecorationRole:
      if (idx.column() == 0)
      {
        if (item && item->getType() != pqPipelineModel::Invalid)
        {
          return QVariant(this->PixmapMap[item->getIconType()]);
        }
      }
      break;

    case Qt::FontRole:
    {
      if (idx.column() == 0 && item->isModified())
      {
        return qVariantFromValue<QFont>(this->Internal->ModifiedFont);
      }
      break;
    }
    case pqPipelineModel::AnnotationFilterRole:
    {
      if (!this->FilterRoleAnnotationKey.isEmpty() && source)
      {
        return QVariant(
          source->getProxy()->HasAnnotation(this->FilterRoleAnnotationKey.toLocal8Bit().data()));
      }
      return QVariant(true);
    }
    case pqPipelineModel::SessionFilterRole:
    {
      if (this->FilterRoleSession && server)
      {
        // We just want to make sure we are pointing to the same session
        return ((void*)server->session() == (void*)this->FilterRoleSession);
      }
      return QVariant(true);
    }
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
bool pqPipelineModel::setData(const QModelIndex& idx, const QVariant& value, int)
{
  if (value.toString().isEmpty())
  {
    return false;
  }

  QString name = value.toString();
  pqPipelineSource* source = qobject_cast<pqPipelineSource*>(this->getItemFor(idx));
  if (source && source->getSMName() != name)
  {
    BEGIN_UNDO_SET(QString("Rename %1 to %2").arg(source->getSMName()).arg(name));
    source->rename(name);
    END_UNDO_SET();
  }

  return true;
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqPipelineModel::flags(const QModelIndex& idx) const
{
  Qt::ItemFlags _flags = Qt::ItemIsEnabled;

  if (idx.column() == 0)
  {
    pqPipelineModelDataItem* item =
      reinterpret_cast<pqPipelineModelDataItem*>(idx.internalPointer());
    if (item->Selectable)
    {
      _flags |= Qt::ItemIsSelectable;
    }

    if (this->Editable && item->Type == pqPipelineModel::Proxy)
    {
      _flags |= Qt::ItemIsEditable;
    }
  }

  return _flags;
}

//-----------------------------------------------------------------------------
pqServerManagerModelItem* pqPipelineModel::getItemFor(const QModelIndex& idx) const
{
  if (idx.isValid() && idx.model() == this)
  {
    pqPipelineModelDataItem* item =
      reinterpret_cast<pqPipelineModelDataItem*>(idx.internalPointer());
    return item->Object;
  }
  return NULL;
}

//-----------------------------------------------------------------------------
QModelIndex pqPipelineModel::getIndexFor(pqServerManagerModelItem* item) const
{
  pqPipelineModelDataItem* dataItem = this->getDataItem(item, &this->Internal->Root);
  if (!dataItem)
  {
    pqOutputPort* port = qobject_cast<pqOutputPort*>(item);
    if (port && port->getSource()->getNumberOfOutputPorts() == 1)
    {
      return this->getIndexFor(port->getSource());
    }
  }
  return this->getIndex(dataItem);
}

//-----------------------------------------------------------------------------
pqPipelineModelDataItem* pqPipelineModel::getDataItem(
  pqServerManagerModelItem* item, pqPipelineModelDataItem* _parent = 0,
  pqPipelineModel::ItemType type /*= pqPipelineModel::Invalid*/
  ) const
{
  if (_parent == 0)
  {
    _parent = &this->Internal->Root;
  }

  if (!item)
  {
    return 0;
  }

  if (_parent->Object == item && (type == pqPipelineModel::Invalid || type == _parent->Type))
  {
    return _parent;
  }

  foreach (pqPipelineModelDataItem* child, _parent->Children)
  {
    pqPipelineModelDataItem* retVal = this->getDataItem(item, child, type);
    if (retVal && (type == pqPipelineModel::Invalid || type == retVal->Type))
    {
      return retVal;
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
QModelIndex pqPipelineModel::getIndex(pqPipelineModelDataItem* dataItem) const
{
  if (dataItem && dataItem->Parent)
  {
    int rowNo = dataItem->getIndexInParent();
    if (rowNo != -1)
    {
      return this->createIndex(rowNo, 0, dataItem);
    }
  }

  // QModelIndex() implies the ROOT.
  return QModelIndex();
}

//-----------------------------------------------------------------------------
void pqPipelineModel::addChild(pqPipelineModelDataItem* _parent, pqPipelineModelDataItem* child)
{
  if (!_parent || !child)
  {
    qDebug() << "addChild cannot have null arguments.";
    return;
  }

  QModelIndex parentIndex = this->getIndex(_parent);
  int row = _parent->Children.size();

  this->beginInsertRows(parentIndex, row, row);
  _parent->addChild(child);
  this->endInsertRows();

  // Make sure a pipeline icon exists for this item
  if (!this->checkAndLoadPipelinePixmap(child->getIconType()))
  {
    qWarning() << "Could not find icon pixmap for" << child->getIconType();
  }

  if (row == 0)
  {
    emit this->firstChildAdded(parentIndex);
  }
}

//-----------------------------------------------------------------------------
void pqPipelineModel::removeChildFromParent(pqPipelineModelDataItem* child)
{
  if (!child)
  {
    qDebug() << "removeChild cannot have null arguments.";
    return;
  }

  pqPipelineModelDataItem* _parent = child->Parent;
  if (!_parent)
  {
    qDebug() << "cannot remove ROOT.";
    return;
  }

  QModelIndex parentIndex = this->getIndex(_parent);
  int row = child->getIndexInParent();

  this->beginRemoveRows(parentIndex, row, row);
  _parent->removeChild(child);
  this->endRemoveRows();
}

//-----------------------------------------------------------------------------
void pqPipelineModel::serverDataChanged()
{
  // TODO: we should determine which server data actually chnaged
  // and invalidate only that one. FOr now, just invalidate all.

  int max = this->Internal->Root.Children.size() - 1;
  if (max >= 0)
  {
    QModelIndex minIndex = this->getIndex(this->Internal->Root.Children[0]);
    QModelIndex maxIndex = this->getIndex(this->Internal->Root.Children[max]);
    emit this->dataChanged(minIndex, maxIndex);
  }
}

//-----------------------------------------------------------------------------
void pqPipelineModel::itemDataChanged(pqPipelineModelDataItem* item)
{
  QModelIndex idx = this->getIndex(item);
  emit this->dataChanged(idx, idx);
}

//-----------------------------------------------------------------------------
void pqPipelineModel::addServer(pqServer* server)
{
  if (!server)
  {
    return;
  }

  pqPipelineModelDataItem* item =
    new pqPipelineModelDataItem(this, server, pqPipelineModel::Server, this);
  this->addChild(&this->Internal->Root, item);
  QObject::connect(server, SIGNAL(nameChanged(pqServerManagerModelItem*)), this,
    SLOT(updateData(pqServerManagerModelItem*)));
}

//-----------------------------------------------------------------------------
void pqPipelineModel::removeServer(pqServer* server)
{
  pqPipelineModelDataItem* item =
    this->getDataItem(server, &this->Internal->Root, pqPipelineModel::Server);

  if (!item)
  {
    qDebug() << "Requesting to remove a non-added server.";
    return;
  }

  this->removeChildFromParent(item);

  delete item;
}

//-----------------------------------------------------------------------------
void pqPipelineModel::addSource(pqPipelineSource* source)
{
  pqServer* server = source->getServer();
  pqPipelineModelDataItem* _parent =
    this->getDataItem(server, &this->Internal->Root, pqPipelineModel::Server);

  if (!_parent)
  {
    qDebug() << "Could not locate server on which the source is being added.";
    return;
  }

  pqPipelineModelDataItem* item =
    new pqPipelineModelDataItem(this, source, pqPipelineModel::Proxy, this);
  item->Object = source;
  item->Type = pqPipelineModel::Proxy; // source->getType();

  // Add the 'source' to the server.
  this->addChild(_parent, item);

  int numOutputPorts = source->getNumberOfOutputPorts();
  if (numOutputPorts > 1)
  {
    // add output-ports for this source.
    for (int cc = 0; cc < numOutputPorts; cc++)
    {
      pqPipelineModelDataItem* opport =
        new pqPipelineModelDataItem(this, source->getOutputPort(cc), pqPipelineModel::Port, this);
      this->addChild(item, opport);
    }
  }

  QObject::connect(source, SIGNAL(visibilityChanged(pqPipelineSource*, pqDataRepresentation*)),
    this, SLOT(delayedUpdateVisibility(pqPipelineSource*)));

  QObject::connect(source, SIGNAL(nameChanged(pqServerManagerModelItem*)), this,
    SLOT(updateData(pqServerManagerModelItem*)));
  QObject::connect(source, SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)), this,
    SLOT(updateData(pqServerManagerModelItem*)));
}

//-----------------------------------------------------------------------------
void pqPipelineModel::removeSource(pqPipelineSource* source)
{
  pqPipelineModelDataItem* item =
    this->getDataItem(source, &this->Internal->Root, pqPipelineModel::Proxy);

  if (!item)
  {
    // qDebug() << "Requesting to remove a non-added source.";
    return;
  }

  while (item->Links.size() > 0)
  {
    pqPipelineModelDataItem* link = item->Links.last();
    this->removeChildFromParent(link);
    delete link;
  }

  this->removeChildFromParent(item);
  if (item->Children.size())
  {
    // Move the children to the server.
    pqServer* server = source->getServer();
    pqPipelineModelDataItem* _parent =
      this->getDataItem(server, &this->Internal->Root, pqPipelineModel::Server);
    if (!_parent)
    {
      _parent = &this->Internal->Root;
    }

    QList<pqPipelineModelDataItem*> childrenToMove;
    foreach (pqPipelineModelDataItem* child, item->Children)
    {
      if (child->Type == pqPipelineModel::Port)
      {
        childrenToMove.append(child->Children);
      }
      else
      {
        childrenToMove.push_back(child);
      }
    }

    foreach (pqPipelineModelDataItem* child, childrenToMove)
    {
      child->Parent = NULL;
      this->addChild(_parent, child);
    }
  }

  delete item;
}

//-----------------------------------------------------------------------------
void pqPipelineModel::addConnection(
  pqPipelineSource* source, pqPipelineSource* sink, int sourceOutputPort)
{
  if (!source || !sink)
  {
    qDebug() << "Cannot connect a null source or sink.";
    return;
  }

  // Note: this slot is invoked after the connection has been set.

  // If fanIn == 1, take the sink form the server list
  // and put it under the source.
  // If fanIn == 2, take the sink from  the under the source that it was put
  //  when fanIn == 1, replace that with a link. Add a new link for the new
  //  connection and put the source under the server list.
  // If fanIn > 2, just create a link for this new connection. We have already
  //  flagged it as a link when fanIn == 2.

  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(sink);
  if (!filter)
  {
    qDebug() << "Sink has to be a filter.";
    return;
  }

  pqPipelineModelDataItem* srcItem =
    this->getDataItem(source, &this->Internal->Root, pqPipelineModel::Proxy);
  pqPipelineModelDataItem* sinkItem =
    this->getDataItem(sink, &this->Internal->Root, pqPipelineModel::Proxy);
  if (!srcItem || !sinkItem)
  {
    qDebug() << "Connection involves a non-added source. Ignoring.";
    return;
  }

  if (source->getNumberOfOutputPorts() > 1)
  {
    srcItem = srcItem->Children[sourceOutputPort];
  }

  // NOTE-TO-SELF: Never use actual values of current connections from the
  // sources/filters -- think of change input dialog to know why.

  pqPipelineModelDataItem* currentParent = sinkItem->Parent;
  if (currentParent->Type == pqPipelineModel::Server && sinkItem->Links.size() > 0)
  {
    // sink has previously been identified as a "fan-in". We simply, create a
    // new link object for it.
    pqPipelineModelDataItem* link =
      new pqPipelineModelDataItem(this, sink, pqPipelineModel::Link, this);
    this->addChild(srcItem, link);
    return;
  }

  if (currentParent->Type == pqPipelineModel::Proxy || currentParent->Type == pqPipelineModel::Port)
  {

    // this filter had just 1 input previously, now it has 2. So we need to
    // upgrade it to a fan-in.
    pqPipelineModelDataItem* linkOld =
      new pqPipelineModelDataItem(this, sink, pqPipelineModel::Link, this);
    this->addChild(currentParent, linkOld);

    pqPipelineModelDataItem* link =
      new pqPipelineModelDataItem(this, sink, pqPipelineModel::Link, this);
    this->addChild(srcItem, link);

    pqServer* server = sink->getServer();
    pqPipelineModelDataItem* serverItem =
      this->getDataItem(server, &this->Internal->Root, pqPipelineModel::Server);

    this->removeChildFromParent(sinkItem);
    this->addChild(serverItem, sinkItem);
    return;
  }

  // Adding the first input connection for this sink.
  // Remove the sink from where ever.
  this->removeChildFromParent(sinkItem);

  // Add to the children of the source.
  this->addChild(srcItem, sinkItem);
}

//-----------------------------------------------------------------------------
void pqPipelineModel::removeConnection(
  pqPipelineSource* source, pqPipelineSource* sink, int sourceOutputPort)
{
  if (!source || !sink)
  {
    qDebug() << "Cannot disconnect a null source or sink.";
    return;
  }

  pqPipelineModelDataItem* sinkItem =
    this->getDataItem(sink, &this->Internal->Root, pqPipelineModel::Proxy);
  pqPipelineModelDataItem* srcItem =
    this->getDataItem(source, &this->Internal->Root, pqPipelineModel::Proxy);

  if (!sinkItem || !srcItem)
  {
    // qDebug() << "Connection involves a non-added source. Ignoring.";
    return;
  }

  // Note: this slot is invoked after the connection has been broken.

  if (sinkItem->Links.size() == 0)
  {
    // Simplest case, sink had just 1 input.
    pqServer* server = sink->getServer();
    pqPipelineModelDataItem* serverItem =
      this->getDataItem(server, &this->Internal->Root, pqPipelineModel::Server);
    if (!serverItem)
    {
      qDebug() << "Failed to locate data item for server.";
      return;
    }

    this->removeChildFromParent(sinkItem);
    this->addChild(serverItem, sinkItem);
    return;
  }

  if (source->getNumberOfOutputPorts() > 1)
  {
    srcItem = srcItem->Children[sourceOutputPort];
  }

  // Has a fan-in for sure.
  // Remove the link item under the source.
  pqPipelineModelDataItem* linkItem = this->getDataItem(sink, srcItem, pqPipelineModel::Link);
  assert(linkItem != 0);
  this->removeChildFromParent(linkItem);
  delete linkItem;

  // locate all links for sink. If number of links == 1, remove the link and
  // move the sink to where the link was.
  if (sinkItem->Links.size() == 1)
  {
    linkItem = sinkItem->Links[0];
    pqPipelineModelDataItem* parentItem = linkItem->Parent;
    this->removeChildFromParent(linkItem);
    delete linkItem;

    this->removeChildFromParent(sinkItem);

    this->addChild(parentItem, sinkItem);
  }
}

//-----------------------------------------------------------------------------
void pqPipelineModel::setView(pqView* newview)
{
  if (this->View == newview)
  {
    return;
  }
  this->View = newview;
  // update all VisibilityIcons.
  this->Internal->Root.updateVisibilityIcon(newview, true);
}

//-----------------------------------------------------------------------------
void pqPipelineModel::delayedUpdateVisibility(pqPipelineSource* source)
{
  this->Internal->DelayedUpdateVisibilityItems.removeAll(source);
  this->Internal->DelayedUpdateVisibilityItems.push_front(source);
  this->Internal->DelayedUpdateVisibilityTimer.start(0);
}

//-----------------------------------------------------------------------------
void pqPipelineModel::delayedUpdateVisibilityTimeout()
{
  foreach (pqPipelineSource* source, this->Internal->DelayedUpdateVisibilityItems)
  {
    if (source)
    {
      this->updateVisibility(source);
    }
  }
  this->Internal->DelayedUpdateVisibilityItems.clear();
}

//-----------------------------------------------------------------------------
void pqPipelineModel::updateVisibility(pqPipelineSource* source, ItemType type /* = Proxy */)
{
  pqPipelineModelDataItem* item = this->getDataItem(source, &this->Internal->Root, type);
  if (item)
  {
    item->updateVisibilityIcon(this->View, false);
    foreach (pqPipelineModelDataItem* child, item->Children)
    {
      if (child->Type == Port)
      {
        child->updateVisibilityIcon(this->View, false);
      }
    }

    foreach (pqPipelineModelDataItem* link, item->Links)
    {
      link->updateVisibilityIcon(this->View, false);
    }
  }
}

//-----------------------------------------------------------------------------
void pqPipelineModel::updateData(pqServerManagerModelItem* source, ItemType type /* = Proxy */)
{
  pqPipelineModelDataItem* item = this->getDataItem(source, &this->Internal->Root, type);
  if (item)
  {
    item->updateVisibilityIcon(this->View, false);
    this->itemDataChanged(item);
    foreach (pqPipelineModelDataItem* link, item->Links)
    {
      item->updateVisibilityIcon(this->View, false);
      this->itemDataChanged(link);
    }
  }
  else // source is null, so update everything
  {
    foreach (pqPipelineModelDataItem* child, this->Internal->Root.Children)
    {
      this->itemDataChanged(child);
    }
  }
}

//-----------------------------------------------------------------------------
void pqPipelineModel::updateDataServer(pqServer* server)
{
  updateData(server, Invalid);
}

//-----------------------------------------------------------------------------
void pqPipelineModel::setModifiedFont(const QFont& font)
{
  this->Internal->ModifiedFont = font;
}

//-----------------------------------------------------------------------------
void pqPipelineModel::enableFilterAnnotationKey(const QString& expectedAnnotation)
{
  this->FilterRoleAnnotationKey = expectedAnnotation;
}

//-----------------------------------------------------------------------------
void pqPipelineModel::disableFilterAnnotationKey()
{
  this->FilterRoleAnnotationKey.clear();
}

//-----------------------------------------------------------------------------
void pqPipelineModel::enableFilterSession(vtkSession* session)
{
  this->FilterRoleSession = session;
}

//-----------------------------------------------------------------------------
void pqPipelineModel::disableFilterSession()
{
  this->FilterRoleSession = NULL;
}

//-----------------------------------------------------------------------------
bool pqPipelineModel::checkAndLoadPipelinePixmap(const QString& iconType)
{
  auto it = this->PixmapMap.find(iconType);
  if (it != this->PixmapMap.end())
  {
    return true;
  }

  return this->PixmapMap[iconType].load(iconType);
}
