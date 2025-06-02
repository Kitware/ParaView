// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqLinksModel.h"

// Std includes
#include <cassert>
#include <map>

// Qt includes
#include <QDebug>
#include <QPointer>

// vtk includes
#include <vtkCollection.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkSmartPointer.h>

// Server manager includes
#include "vtkPVXMLElement.h"
#include "vtkSMCameraLink.h"
#include "vtkSMCameraProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSelectionLink.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSMViewLink.h"

// pqCore includes
#include "pqApplicationCore.h"
#include "pqCameraWidgetViewLink.h"
#include "pqInteractiveViewLink.h"
#include "pqProxy.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"

namespace
{
constexpr char CAMERA_WIDGET_VIEW_LINK_XML_NAME[] = "CameraWidgetViewLink";
constexpr char CAMERA_WIDGET_VIEW_LINKS_XML_NAME[] = "CameraWidgetViewLinks";
constexpr char INTERACTIVE_VIEW_LINK_XML_NAME[] = "InteractiveViewLink";
constexpr char INTERACTIVE_VIEW_LINKS_XML_NAME[] = "InteractiveViewLinks";
}

class pqLinksModel::pqInternal : public vtkCommand
{
public:
  pqLinksModel* Model;
  QPointer<pqServer> Server;
  QMap<QString, pqInteractiveViewLink*> InteractiveViewLinks;
  QMap<QString, pqCameraWidgetViewLink*> CameraWidgetViewLinks;
  static pqInternal* New() { return new pqInternal; }
  static const char* columnHeaders[];

  void Execute(vtkObject*, unsigned long eid, void* callData) override
  {
    vtkSMProxyManager::RegisteredProxyInformation* info =
      reinterpret_cast<vtkSMProxyManager::RegisteredProxyInformation*>(callData);

    if (!info || info->Type != vtkSMProxyManager::RegisteredProxyInformation::LINK)
    {
      return;
    }

    QString linkName = info->ProxyName;

    if (eid == vtkCommand::RegisterEvent)
    {
      this->Model->beginResetModel();
      this->LinkObjects.append(new pqLinksModelObject(linkName, this->Model, this->Server));
      this->Model->endResetModel();
    }
    else if (eid == vtkCommand::UnRegisterEvent)
    {
      QList<pqLinksModelObject*>::iterator iter;
      for (iter = this->LinkObjects.begin(); iter != this->LinkObjects.end(); ++iter)
      {
        if ((*iter)->name() == linkName)
        {
          this->Model->beginResetModel();
          delete *iter;
          this->Model->emitLinkRemoved(linkName);
          this->LinkObjects.erase(iter);
          this->Model->endResetModel();
          CLEAR_UNDO_STACK();
          break;
        }
      }
    }
  }

protected:
  pqInternal() = default;
  ~pqInternal() override
  {
    // clean up interactiveViewLinks
    for (auto interLink : this->InteractiveViewLinks)
    {
      delete interLink;
    }
    this->InteractiveViewLinks.clear();
    // clean up cameraWidgetViewLinks
    for (auto cameraLink : this->CameraWidgetViewLinks)
    {
      delete cameraLink;
    }
    this->CameraWidgetViewLinks.clear();
  }
  QList<pqLinksModelObject*> LinkObjects;

private:
  pqInternal(const pqInternal&);
  pqInternal& operator=(const pqInternal&);
};

const char* pqLinksModel::pqInternal::columnHeaders[] = { "Name", "Object 1", "Property",
  "Object 2", "Property" };

//------------------------------------------------------------------------------
pqLinksModel::pqLinksModel(QObject* p)
  : Superclass(p)
{
  this->Internal = pqInternal::New();
  this->Internal->Model = this;

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(
    smmodel, SIGNAL(serverAdded(pqServer*)), this, SLOT(onSessionCreated(pqServer*)));
  QObject::connect(
    smmodel, SIGNAL(serverRemoved(pqServer*)), this, SLOT(onSessionRemoved(pqServer*)));

  // Connect to state to create/store interactive view links
  QObject::connect(pqApplicationCore::instance(),
    SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)), this,
    SLOT(onStateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)));
  QObject::connect(pqApplicationCore::instance(), SIGNAL(stateSaved(vtkPVXMLElement*)), this,
    SLOT(onStateSaved(vtkPVXMLElement*)));
}

//------------------------------------------------------------------------------
pqLinksModel::~pqLinksModel()
{
  this->Internal->Model = nullptr;
  this->Internal->Delete();
}

//------------------------------------------------------------------------------
void pqLinksModel::onSessionCreated(pqServer* server)
{
  this->Internal->Server = server;
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  pxm->AddObserver(vtkCommand::RegisterEvent, this->Internal);
  pxm->AddObserver(vtkCommand::UnRegisterEvent, this->Internal);
}

//------------------------------------------------------------------------------
void pqLinksModel::onSessionRemoved(pqServer* server)
{
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  pxm->RemoveObserver(this->Internal);
}

//------------------------------------------------------------------------------
void pqLinksModel::onStateLoaded(vtkPVXMLElement* root, vtkSMProxyLocator* locator)
{
  if (!root || !locator)
  {
    return;
  }

  vtkPVXMLElement* xml = root->FindNestedElementByName(INTERACTIVE_VIEW_LINKS_XML_NAME);
  if (xml && xml->GetName() && strcmp(xml->GetName(), INTERACTIVE_VIEW_LINKS_XML_NAME) == 0)
  {
    for (unsigned cc = 0; cc < xml->GetNumberOfNestedElements(); cc++)
    {
      vtkPVXMLElement* child = xml->GetNestedElement(cc);
      if (!child || !child->GetName() ||
        strcmp(child->GetName(), INTERACTIVE_VIEW_LINK_XML_NAME) != 0)
      {
        continue;
      }
      const char* name = child->GetAttribute("name");
      if (!name)
      {
        qWarning() << "Dropping an unammed InteractiveViewLink";
        continue;
      }

      int displayViewProxyId;
      if (!child->GetScalarAttribute("DisplayViewProxy", &displayViewProxyId))
      {
        qWarning() << "InteractiveViewLink named :" << name
                   << " is missing a DisplayViewProxy. Dropping.";
        continue;
      }
      vtkSMProxy* displayViewProxy = locator->LocateProxy(displayViewProxyId);
      if (!displayViewProxy)
      {
        qWarning() << "Failed to locate InteractiveViewLink DisplayViewProxy with ID: "
                   << displayViewProxyId << " . Dropping";
        continue;
      }

      int linkedViewProxyId;
      if (!child->GetScalarAttribute("LinkedViewProxy", &linkedViewProxyId))
      {
        qWarning() << "InteractiveViewLink named :" << name
                   << " is missing a LinkedViewProxy. Dropping.";
        continue;
      }
      vtkSMProxy* linkedViewProxy = locator->LocateProxy(linkedViewProxyId);
      if (!linkedViewProxy)
      {
        qWarning() << "Failed to locate InteractiveViewLink LinkedViewProxy with ID: "
                   << linkedViewProxyId << " . Dropping";
        continue;
      }

      double xPos, yPos, xSize, ySize;
      if (!child->GetScalarAttribute("positionX", &xPos))
      {
        qWarning() << "InteractiveViewLink named :" << name << " is missing a positionX. Dropping.";
        continue;
      }
      if (!child->GetScalarAttribute("positionY", &yPos))
      {
        qWarning() << "InteractiveViewLink named :" << name << " is missing a positionY. Dropping.";
        continue;
      }
      if (!child->GetScalarAttribute("sizeX", &xSize))
      {
        qWarning() << "InteractiveViewLink named :" << name << " is missing a sizeX. Dropping.";
        continue;
      }
      if (!child->GetScalarAttribute("sizeY", &ySize))
      {
        qWarning() << "InteractiveViewLink named :" << name << " is missing a sizeY. Dropping.";
        continue;
      }
      this->createInteractiveViewLink(
        name, displayViewProxy, linkedViewProxy, xPos, yPos, xSize, ySize);
    }
  }

  xml = root->FindNestedElementByName(CAMERA_WIDGET_VIEW_LINKS_XML_NAME);
  if (!xml || !xml->GetName() || strcmp(xml->GetName(), CAMERA_WIDGET_VIEW_LINKS_XML_NAME) != 0)
  {
    return;
  }

  for (unsigned cc = 0; cc < xml->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* child = xml->GetNestedElement(cc);
    if (!child || !child->GetName() ||
      strcmp(child->GetName(), CAMERA_WIDGET_VIEW_LINK_XML_NAME) != 0)
    {
      continue;
    }
    const char* name = child->GetAttribute("name");
    if (!name)
    {
      qWarning() << "Dropping an unnamed CameraWidgetViewLink";
      continue;
    }

    int displayViewProxyId;
    if (!child->GetScalarAttribute("DisplayViewProxy", &displayViewProxyId))
    {
      qWarning() << "CameraWidgetViewLink named :" << name
                 << " is missing a DisplayViewProxy. Dropping.";
      continue;
    }
    vtkSMProxy* displayViewProxy = locator->LocateProxy(displayViewProxyId);
    if (!displayViewProxy)
    {
      qWarning() << "Failed to locate CameraWidgetViewLink DisplayViewProxy with ID: "
                 << displayViewProxyId << " . Dropping";
      continue;
    }

    int linkedViewProxyId;
    if (!child->GetScalarAttribute("LinkedViewProxy", &linkedViewProxyId))
    {
      qWarning() << "CameraWidgetViewLink named :" << name
                 << " is missing a LinkedViewProxy. Dropping.";
      continue;
    }
    vtkSMProxy* linkedViewProxy = locator->LocateProxy(linkedViewProxyId);
    if (!linkedViewProxy)
    {
      qWarning() << "Failed to locate CameraWidgetViewLink LinkedViewProxy with ID: "
                 << linkedViewProxyId << " . Dropping";
      continue;
    }
    this->createCameraWidgetViewLink(name, displayViewProxy, linkedViewProxy, /*loadState=*/true);
  }
}

//------------------------------------------------------------------------------
void pqLinksModel::onStateSaved(vtkPVXMLElement* root)
{
  assert(root != nullptr);
  vtkNew<vtkPVXMLElement> interLinkParentXML;
  interLinkParentXML->SetName(INTERACTIVE_VIEW_LINKS_XML_NAME);
  vtkNew<vtkPVXMLElement> camWidgetLinkParentXML;
  camWidgetLinkParentXML->SetName(CAMERA_WIDGET_VIEW_LINKS_XML_NAME);

  // Save state of each stored pqInteractiveViewLink
  for (const QString& linkName : this->Internal->InteractiveViewLinks.keys())
  {
    pqInteractiveViewLink* interLink = this->Internal->InteractiveViewLinks[linkName];
    if (!interLink)
    {
      continue;
    }
    vtkNew<vtkPVXMLElement> interLinkXML;
    interLinkXML->SetName(INTERACTIVE_VIEW_LINK_XML_NAME);
    interLinkXML->AddAttribute("name", linkName.toUtf8().data());
    interLink->saveXMLState(interLinkXML);
    interLinkParentXML->AddNestedElement(interLinkXML);
  }
  // Save state of each stored pqCameraWidgetViewLink
  for (const QString& linkName : this->Internal->CameraWidgetViewLinks.keys())
  {
    pqCameraWidgetViewLink* camWidgetLink = this->Internal->CameraWidgetViewLinks[linkName];
    if (!camWidgetLink)
    {
      continue;
    }
    vtkNew<vtkPVXMLElement> camWidgetLinkXML;
    camWidgetLinkXML->SetName(CAMERA_WIDGET_VIEW_LINK_XML_NAME);
    camWidgetLinkXML->AddAttribute("name", linkName.toUtf8().data());
    camWidgetLink->saveXMLState(camWidgetLinkXML);
    camWidgetLinkParentXML->AddNestedElement(camWidgetLinkXML);
  }

  root->AddNestedElement(interLinkParentXML);
  root->AddNestedElement(camWidgetLinkParentXML);
}

//------------------------------------------------------------------------------
static vtkSMProxy* getProxyFromLink(vtkSMLink* link, int desiredDir)
{
  int numLinks = link->GetNumberOfLinkedObjects();
  for (int i = 0; i < numLinks; i++)
  {
    vtkSMProxy* proxy = link->GetLinkedProxy(i);
    int dir = link->GetLinkedObjectDirection(i);
    if (dir == desiredDir)
    {
      return proxy;
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
// TODO: fix this so it isn't dependent on the order of links
QString pqLinksModel::getPropertyFromIndex(const QModelIndex& idx, int desiredDir) const
{
  QString name = this->getLinkName(idx);
  vtkSMLink* link = this->getLink(name);
  vtkSMPropertyLink* propertyLink = vtkSMPropertyLink::SafeDownCast(link);

  if (propertyLink)
  {
    int numLinks = propertyLink->GetNumberOfLinkedObjects();
    for (int i = 0; i < numLinks; i++)
    {
      int dir = propertyLink->GetLinkedObjectDirection(i);
      if (dir == desiredDir)
      {
        return propertyLink->GetLinkedPropertyName(i);
      }
    }
  }
  return QString();
}

//------------------------------------------------------------------------------
// TODO: fix this so it isn't dependent on the order of links
vtkSMProxy* pqLinksModel::getProxyFromIndex(const QModelIndex& idx, int dir) const
{
  QString name = this->getLinkName(idx);
  vtkSMLink* link = this->getLink(name);
  return getProxyFromLink(link, dir);
}

//------------------------------------------------------------------------------
vtkSMProxy* pqLinksModel::getProxy1(const QModelIndex& idx) const
{
  return this->getProxyFromIndex(idx, vtkSMLink::INPUT);
}

//------------------------------------------------------------------------------
vtkSMProxy* pqLinksModel::getProxy2(const QModelIndex& idx) const
{
  return this->getProxyFromIndex(idx, vtkSMLink::OUTPUT);
}

//------------------------------------------------------------------------------
QString pqLinksModel::getProperty1(const QModelIndex& idx) const
{
  return this->getPropertyFromIndex(idx, vtkSMLink::INPUT);
}

//------------------------------------------------------------------------------
QString pqLinksModel::getProperty2(const QModelIndex& idx) const
{
  return this->getPropertyFromIndex(idx, vtkSMLink::OUTPUT);
}

//------------------------------------------------------------------------------
int pqLinksModel::rowCount(const QModelIndex&) const
{
  return this->Internal->Server ? this->Internal->Server->proxyManager()->GetNumberOfLinks() : 0;
}

//------------------------------------------------------------------------------
int pqLinksModel::columnCount(const QModelIndex&) const
{
  // name, master object, property, slave object, property
  return sizeof(pqInternal::columnHeaders) / sizeof(char*);
}

//------------------------------------------------------------------------------
QVariant pqLinksModel::data(const QModelIndex& idx, int role) const
{
  if (role == Qt::DisplayRole)
  {
    QString name = this->getLinkName(idx);
    vtkSMLink* link = this->getLink(name);
    ItemType type = this->getLinkType(link);

    if (idx.column() == 0)
    {
      return name.isNull() ? "Unknown" : name;
    }
    else if (idx.column() == 1)
    {
      vtkSMProxy* pxy = this->getProxy1(idx);
      pqProxy* qpxy = this->representativeProxy(pxy);
      return qpxy ? qpxy->getSMName() : "Unknown";
    }
    else if (idx.column() == 2)
    {
      vtkSMProxy* pxy = this->getProxy1(idx);
      pqProxy* qpxy = this->representativeProxy(pxy);
      if (type == pqLinksModel::Proxy && qpxy->getProxy() == pxy)
      {
        return "All";
      }
      else if (type == pqLinksModel::Proxy && qpxy)
      {
        vtkSMProxyListDomain* d = this->proxyListDomain(qpxy->getProxy());
        if (d)
        {
          int numProxies = d->GetNumberOfProxies();
          for (int i = 0; i < numProxies; i++)
          {
            if (pxy == d->GetProxy(i))
            {
              return d->GetProxyName(i);
            }
          }
        }
      }
      else if (type == pqLinksModel::Selection)
      {
        return "Selection";
      }
      QString prop = this->getProperty1(idx);
      return prop.isEmpty() ? "Unknown" : prop;
    }
    else if (idx.column() == 3)
    {
      vtkSMProxy* pxy = this->getProxy2(idx);
      pqProxy* qpxy = this->representativeProxy(pxy);
      return qpxy ? qpxy->getSMName() : "Unknown";
    }
    else if (idx.column() == 4)
    {
      vtkSMProxy* pxy = this->getProxy2(idx);
      pqProxy* qpxy = this->representativeProxy(pxy);
      if (type == pqLinksModel::Proxy && qpxy->getProxy() == pxy)
      {
        return "All";
      }
      else if (type == pqLinksModel::Proxy && qpxy)
      {
        vtkSMProxyListDomain* d = this->proxyListDomain(qpxy->getProxy());
        if (d)
        {
          int numProxies = d->GetNumberOfProxies();
          for (int i = 0; i < numProxies; i++)
          {
            if (pxy == d->GetProxy(i))
            {
              return d->GetProxyName(i);
            }
          }
        }
      }
      else if (type == pqLinksModel::Selection)
      {
        return "Selection";
      }
      QString prop = this->getProperty2(idx);
      return prop.isEmpty() ? "Unknown" : prop;
    }
  }
  return QVariant();
}

//------------------------------------------------------------------------------
QVariant pqLinksModel::headerData(int section, Qt::Orientation orient, int role) const
{
  if (role == Qt::DisplayRole && orient == Qt::Horizontal && section >= 0 &&
    section < this->columnCount())
  {
    // column headers
    return QString(pqInternal::columnHeaders[section]);
  }
  else if (role == Qt::DisplayRole && orient == Qt::Vertical)
  {
    // row headers, just use numbers 1-n
    return QString("%1").arg(section + 1);
  }

  return QVariant();
}

//------------------------------------------------------------------------------
QString pqLinksModel::getLinkName(const QModelIndex& idx) const
{
  if (this->Internal->Server)
  {
    vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
    QString linkName = pxm->GetLinkName(idx.row());
    return linkName;
  }
  return QString();
}

//------------------------------------------------------------------------------
QModelIndex pqLinksModel::findLink(vtkSMLink* link) const
{
  int numRows = this->rowCount();
  for (int i = 0; i < numRows; i++)
  {
    QModelIndex idx = this->index(i, 0, QModelIndex());
    if (this->getLink(idx) == link)
    {
      return idx;
    }
  }
  return QModelIndex();
}

//------------------------------------------------------------------------------
int pqLinksModel::FindLinksFromProxy(vtkSMProxy* proxy, int direction, vtkCollection* links) const
{
  int nFoundLinks = 0;
  if (this->Internal->Server)
  {
    vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
    const char* tmpName;
    vtkSMLink* tmpLink;
    for (int i = 0; i < pxm->GetNumberOfLinks(); i++)
    {
      tmpName = pxm->GetLinkName(i);
      tmpLink = pxm->GetRegisteredLink(tmpName);
      for (unsigned int j = 0; j < tmpLink->GetNumberOfLinkedObjects(); j++)
      {
        if ((direction == vtkSMLink::NONE || tmpLink->GetLinkedObjectDirection(j) == direction) &&
          tmpLink->GetLinkedProxy(j) == proxy)
        {
          links->AddItem(tmpLink);
          nFoundLinks++;
          break;
        }
      }
    }
  }
  return nFoundLinks;
}

//------------------------------------------------------------------------------
vtkSMLink* pqLinksModel::getLink(const QString& name) const
{
  if (this->Internal->Server)
  {
    vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
    vtkSMLink* link = pxm->GetRegisteredLink(name.toUtf8().data());
    return link;
  }
  return nullptr;
}

//------------------------------------------------------------------------------
vtkSMLink* pqLinksModel::getLink(const QModelIndex& idx) const
{
  return this->getLink(this->getLinkName(idx));
}

//------------------------------------------------------------------------------
pqLinksModel::ItemType pqLinksModel::getLinkType(vtkSMLink* link) const
{
  if (vtkSMPropertyLink::SafeDownCast(link))
  {
    return Property;
  }
  else if (vtkSMCameraLink::SafeDownCast(link))
  {
    return Camera;
  }
  else if (vtkSMProxyLink::SafeDownCast(link))
  {
    return Proxy;
  }
  else if (vtkSMSelectionLink::SafeDownCast(link))
  {
    return Selection;
  }
  return Unknown;
}

//------------------------------------------------------------------------------
pqLinksModel::ItemType pqLinksModel::getLinkType(const QModelIndex& idx) const
{
  vtkSMLink* link = this->getLink(idx);
  return this->getLinkType(link);
}

//------------------------------------------------------------------------------
void pqLinksModel::addProxyLink(
  const QString& name, vtkSMProxy* inputProxy, vtkSMProxy* outputProxy)
{
  vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
  vtkNew<vtkSMProxyLink> link;

  pxm->RegisterLink(name.toUtf8().data(), link);
  link->LinkProxies(inputProxy, outputProxy);

  // any proxy property doesn't participate in the link
  // instead, these proxies are linkable themselves
  vtkNew<vtkSMPropertyIterator> iter;
  iter->SetProxy(inputProxy);
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    if (vtkSMProxyProperty::SafeDownCast(iter->GetProperty()))
    {
      link->AddException(iter->GetKey());
    }
  }

  Q_EMIT this->linkAdded(pqLinksModel::Proxy);
  CLEAR_UNDO_STACK();

  SM_SCOPED_TRACE(CallFunction)
    .arg("AddProxyLink")
    .arg(inputProxy)
    .arg(outputProxy)
    .arg(name.toUtf8().data())
    .arg("comment", qPrintable(tr("link proxies")));
}

//------------------------------------------------------------------------------
void pqLinksModel::addCameraWidgetLink(
  const QString& name, vtkSMProxy* inputProxy, vtkSMProxy* outputProxy)
{
  vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
  vtkNew<vtkSMViewLink> link;

  pxm->RegisterLink(name.toUtf8().data(), link);
  link->LinkProxies(inputProxy, outputProxy);
  link->SetExceptionBehaviorToWhitelist();
  // Ensure that nothing stay linked. (especially "Representations" property)
  link->ClearExceptions();

  this->createCameraWidgetViewLink(name, inputProxy, outputProxy, /*loadState=*/false);

  Q_EMIT this->linkAdded(pqLinksModel::CameraWidget);
  CLEAR_UNDO_STACK();
}

//------------------------------------------------------------------------------
void pqLinksModel::addCameraLink(
  const QString& name, vtkSMProxy* inputProxy, vtkSMProxy* outputProxy, bool interactiveViewLink)
{
  vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
  vtkNew<vtkSMCameraLink> link;

  pxm->RegisterLink(name.toUtf8().data(), link);
  link->LinkProxies(inputProxy, outputProxy);

  Q_EMIT this->linkAdded(pqLinksModel::Camera);
  CLEAR_UNDO_STACK();

  SM_SCOPED_TRACE(CallFunction)
    .arg("AddCameraLink")
    .arg(inputProxy)
    .arg(outputProxy)
    .arg(name.toUtf8().data())
    .arg("comment", qPrintable(tr("link cameras in two views")));

  if (interactiveViewLink)
  {
    this->createInteractiveViewLink(name, inputProxy, outputProxy);
  }
}

//------------------------------------------------------------------------------
void pqLinksModel::createInteractiveViewLink(const QString& name, vtkSMProxy* displayView,
  vtkSMProxy* linkedView, double xPos, double yPos, double xSize, double ySize)
{
  // Look for corresponding pqRenderView
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqRenderView* firstView = nullptr;
  pqRenderView* otherView = nullptr;
  QList<pqRenderView*> views = smModel->findItems<pqRenderView*>();
  for (auto view : views)
  {
    if (view && view->getRenderViewProxy() == displayView)
    {
      firstView = view;
    }
    if (view && view->getRenderViewProxy() == linkedView)
    {
      otherView = view;
    }
  }

  // If found, create the pqInteractiveViewLink
  if (firstView && otherView)
  {
    this->Internal->InteractiveViewLinks[name] =
      new pqInteractiveViewLink(firstView, otherView, xPos, yPos, xSize, ySize);
  }
}

//------------------------------------------------------------------------------
void pqLinksModel::createCameraWidgetViewLink(
  const QString& name, vtkSMProxy* displayView, vtkSMProxy* linkedView, bool loadState)
{
  // Look for corresponding pqRenderView
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqRenderView* firstView = nullptr;
  pqRenderView* otherView = nullptr;
  QList<pqRenderView*> views = smModel->findItems<pqRenderView*>();
  for (auto view : views)
  {
    if (view && view->getRenderViewProxy() == displayView)
    {
      firstView = view;
    }
    if (view && view->getRenderViewProxy() == linkedView)
    {
      otherView = view;
    }
  }

  // If found, create the pqCameraWidgetViewLink
  if (firstView && otherView)
  {
    this->Internal->CameraWidgetViewLinks[name] =
      new pqCameraWidgetViewLink(firstView, otherView, loadState);
  }
}

//------------------------------------------------------------------------------
bool pqLinksModel::hasInteractiveViewLink(const QString& name)
{
  return this->Internal->InteractiveViewLinks.contains(name);
}

//------------------------------------------------------------------------------
pqInteractiveViewLink* pqLinksModel::getInteractiveViewLink(const QString& name)
{
  return this->Internal->InteractiveViewLinks.contains(name)
    ? this->Internal->InteractiveViewLinks[name]
    : nullptr;
}

//------------------------------------------------------------------------------
bool pqLinksModel::hasCameraWidgetViewLink(const QString& name)
{
  return this->Internal->CameraWidgetViewLinks.contains(name);
}

//------------------------------------------------------------------------------
pqCameraWidgetViewLink* pqLinksModel::getCameraWidgetViewLink(const QString& name)
{
  return this->Internal->CameraWidgetViewLinks.contains(name)
    ? this->Internal->CameraWidgetViewLinks[name]
    : nullptr;
}

//------------------------------------------------------------------------------
void pqLinksModel::addPropertyLink(const QString& name, vtkSMProxy* inputProxy,
  const QString& inputProp, vtkSMProxy* outputProxy, const QString& outputProp)
{
  vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
  vtkSMPropertyLink* link = vtkSMPropertyLink::New();
  pxm->RegisterLink(name.toUtf8().data(), link);

  // bi-directional link
  link->AddLinkedProperty(inputProxy, inputProp.toUtf8().data(), vtkSMLink::INPUT);
  link->AddLinkedProperty(outputProxy, outputProp.toUtf8().data(), vtkSMLink::OUTPUT);
  link->AddLinkedProperty(outputProxy, outputProp.toUtf8().data(), vtkSMLink::INPUT);
  link->AddLinkedProperty(inputProxy, inputProp.toUtf8().data(), vtkSMLink::OUTPUT);
  link->Delete();
  Q_EMIT this->linkAdded(pqLinksModel::Property);
  CLEAR_UNDO_STACK();
}

//------------------------------------------------------------------------------
void pqLinksModel::addSelectionLink(
  const QString& name, vtkSMProxy* inputProxy, vtkSMProxy* outputProxy, bool convertToIndices)
{
  vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
  vtkSMSelectionLink* link = vtkSMSelectionLink::New();
  link->SetConvertToIndices(convertToIndices);
  pxm->RegisterLink(name.toUtf8().data(), link);

  // bi-directional link
  link->AddLinkedSelection(inputProxy, vtkSMLink::OUTPUT);
  link->AddLinkedSelection(outputProxy, vtkSMLink::OUTPUT);
  link->AddLinkedSelection(outputProxy, vtkSMLink::INPUT);
  link->AddLinkedSelection(inputProxy, vtkSMLink::INPUT);

  link->Delete();
  Q_EMIT this->linkAdded(pqLinksModel::Selection);
  CLEAR_UNDO_STACK();

  SM_SCOPED_TRACE(CallFunction)
    .arg("AddSelectionLink")
    .arg(inputProxy)
    .arg(outputProxy)
    .arg(name.toUtf8().data())
    .arg(convertToIndices)
    .arg("comment", qPrintable(tr("link selection between two objects")));
}

//------------------------------------------------------------------------------
void pqLinksModel::removeLink(const QModelIndex& idx)
{
  if (!idx.isValid())
  {
    return;
  }

  // we want an index for the first column
  QModelIndex removeIdx = this->index(idx.row(), 0, idx.parent());
  // get the name from the first column
  QString name = this->data(removeIdx, Qt::DisplayRole).toString();

  this->removeLink(name);
}

//------------------------------------------------------------------------------
void pqLinksModel::removeLink(const QString& name)
{
  if (!name.isNull())
  {
    vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
    pxm->UnRegisterLink(name.toUtf8().data());
    this->emitLinkRemoved(name);
    CLEAR_UNDO_STACK();

    SM_SCOPED_TRACE(CallFunction)
      .arg("RemoveLink")
      .arg(name.toUtf8().data())
      .arg("comment", qPrintable(tr("Remove any link given its name")));
  }
}

//------------------------------------------------------------------------------
void pqLinksModel::emitLinkRemoved(const QString& name)
{
  if (this->Internal->CameraWidgetViewLinks.contains(name))
  {
    delete this->Internal->CameraWidgetViewLinks[name];
    this->Internal->CameraWidgetViewLinks.remove(name);
  }
  if (this->Internal->InteractiveViewLinks.contains(name))
  {
    delete this->Internal->InteractiveViewLinks[name];
    this->Internal->InteractiveViewLinks.remove(name);
  }
  Q_EMIT this->linkRemoved(name);
}

//------------------------------------------------------------------------------
pqProxy* pqLinksModel::representativeProxy(vtkSMProxy* pxy)
{
  // assume internal proxies don't have pqProxy counterparts
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqProxy* rep = smModel->findItem<pqProxy*>(pxy);

  if (!rep)
  {
    // get the owner of this internal proxy
    int numConsumers = pxy->GetNumberOfConsumers();
    for (int i = 0; rep == nullptr && i < numConsumers; i++)
    {
      vtkSMProxy* consumer = pxy->GetConsumerProxy(i);
      rep = smModel->findItem<pqProxy*>(consumer);
    }
  }
  return rep;
}

//------------------------------------------------------------------------------
vtkSMProxyListDomain* pqLinksModel::proxyListDomain(vtkSMProxy* pxy)
{
  vtkSMProxyListDomain* pxyDomain = nullptr;

  if (pxy == nullptr)
  {
    return nullptr;
  }

  vtkSMPropertyIterator* iter = vtkSMPropertyIterator::New();
  iter->SetProxy(pxy);
  for (iter->Begin(); pxyDomain == nullptr && !iter->IsAtEnd(); iter->Next())
  {
    vtkSMProxyProperty* pxyProperty = vtkSMProxyProperty::SafeDownCast(iter->GetProperty());
    if (pxyProperty)
    {
      pxyDomain = pxyProperty->FindDomain<vtkSMProxyListDomain>();
    }
  }
  iter->Delete();
  return pxyDomain;
}

//------------------------------------------------------------------------------
class pqLinksModelObject::pqInternal
{
public:
  QPointer<pqServer> Server;
  // a list of proxies involved in the link
  QList<pqProxy*> OutputProxies;
  QList<pqProxy*> InputProxies;
  vtkSmartPointer<vtkEventQtSlotConnect> Connection;
  // name of link
  QString Name;
  vtkSmartPointer<vtkSMLink> Link;
  bool Setting;
};

//------------------------------------------------------------------------------
pqLinksModelObject::pqLinksModelObject(QString linkName, pqLinksModel* p, pqServer* server)
  : QObject(p)
{
  this->Internal = new pqInternal;
  this->Internal->Connection = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Internal->Server = server;
  this->Internal->Name = linkName;
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  this->Internal->Link = pxm->GetRegisteredLink(linkName.toUtf8().data());
  this->Internal->Setting = false;
  this->Internal->Connection->Connect(
    this->Internal->Link, vtkCommand::ModifiedEvent, this, SLOT(refresh()));
  this->refresh();
}

//------------------------------------------------------------------------------
pqLinksModelObject::~pqLinksModelObject()
{
  if (vtkSMCameraLink::SafeDownCast(this->Internal->Link))
  {
    for (auto proxy : this->Internal->InputProxies)
    {
      // For render module links, we have to ensure that we remove
      // the links between their interaction undo stacks as well.
      pqRenderView* ren = qobject_cast<pqRenderView*>(proxy);
      if (ren)
      {
        this->unlinkUndoStacks(ren);
      }
    }
  }

  delete this->Internal;
}

//------------------------------------------------------------------------------
QString pqLinksModelObject::name() const
{
  return this->Internal->Name;
}

//------------------------------------------------------------------------------
vtkSMLink* pqLinksModelObject::link() const
{
  return this->Internal->Link;
}

//------------------------------------------------------------------------------
void pqLinksModelObject::proxyModified(pqServerManagerModelItem* item)
{
  if (this->Internal->Setting)
  {
    return;
  }

  this->Internal->Setting = true;
  pqProxy* source = qobject_cast<pqProxy*>(item);
  if (source && source->modifiedState() == pqProxy::MODIFIED)
  {
    for (auto proxy : this->Internal->OutputProxies)
    {
      if (proxy != source && proxy->modifiedState() != pqProxy::MODIFIED)
      {
        proxy->setModifiedState(pqProxy::MODIFIED);
      }
    }
  }
  this->Internal->Setting = false;
}

//------------------------------------------------------------------------------
void pqLinksModelObject::remove()
{
  vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
  pxm->UnRegisterLink(this->name().toUtf8().data());
}

//------------------------------------------------------------------------------
void pqLinksModelObject::unlinkUndoStacks(pqRenderView* ren)
{
  for (auto proxy : this->Internal->OutputProxies)
  {
    // assume all are render modules because some might be deleted already
    pqRenderView* other = static_cast<pqRenderView*>(proxy);
    if (other && other != ren)
    {
      ren->unlinkUndoStack(other);
    }
  }
}

//------------------------------------------------------------------------------
void pqLinksModelObject::linkUndoStacks()
{
  for (auto proxy : this->Internal->InputProxies)
  {
    pqRenderView* src = qobject_cast<pqRenderView*>(proxy);
    if (src)
    {
      for (int cc = 0; cc < this->Internal->OutputProxies.size(); cc++)
      {
        pqRenderView* dest = qobject_cast<pqRenderView*>(this->Internal->OutputProxies[cc]);
        if (dest && src != dest)
        {
          src->linkUndoStack(dest);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void pqLinksModelObject::refresh()
{
  for (auto proxy : this->Internal->InputProxies)
  {
    QObject::disconnect(proxy, SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)), this,
      SLOT(proxyModified(pqServerManagerModelItem*)));

    // For render module links, we have to ensure that we remove
    // the links between their interaction undo stacks as well.
    pqRenderView* ren = qobject_cast<pqRenderView*>(proxy);
    if (ren)
    {
      this->unlinkUndoStacks(ren);
    }
  }

  this->Internal->InputProxies.clear();
  this->Internal->OutputProxies.clear();

  QList<vtkSMProxy*> tmpInputs, tmpOutputs;

  int numLinks = this->link()->GetNumberOfLinkedObjects();
  for (int i = 0; i < numLinks; i++)
  {
    vtkSMProxy* pxy = this->link()->GetLinkedProxy(i);
    int dir = this->link()->GetLinkedObjectDirection(i);
    if (dir == vtkSMLink::INPUT)
    {
      tmpInputs.append(pxy);
    }
    else if (dir == vtkSMLink::OUTPUT)
    {
      tmpOutputs.append(pxy);
    }
  }

  for (auto proxy : tmpInputs)
  {
    pqProxy* reprProxy = pqLinksModel::representativeProxy(proxy);
    if (reprProxy)
    {
      this->Internal->InputProxies.append(reprProxy);
      QObject::connect(reprProxy, SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)), this,
        SLOT(proxyModified(pqServerManagerModelItem*)));
      QObject::connect(reprProxy, SIGNAL(destroyed(QObject*)), this, SLOT(remove()));
    }
  }

  for (auto proxy : tmpOutputs)
  {
    pqProxy* reprProxy = pqLinksModel::representativeProxy(proxy);
    if (reprProxy)
    {
      this->Internal->OutputProxies.append(reprProxy);
      QObject::connect(reprProxy, SIGNAL(destroyed(QObject*)), this, SLOT(remove()));
    }
  }

  if (vtkSMCameraLink::SafeDownCast(this->link()))
  {
    this->linkUndoStacks();
  }
}
