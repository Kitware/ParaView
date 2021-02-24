/*=========================================================================

   Program: ParaView
   Module:    pqRecentFilesMenu.cxx

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
#include "pqRecentFilesMenu.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqInterfaceTracker.h"
#include "pqRecentlyUsedResourceLoaderInterface.h"
#include "pqRecentlyUsedResourcesList.h"
#include "pqServerConfiguration.h"
#include "pqServerConnectDialog.h"
#include "pqServerLauncher.h"
#include "pqServerManagerModel.h"
#include "pqServerResource.h"

#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QScopedPointer>
#include <QtDebug>

#include <algorithm>
#include <cassert>

//=============================================================================
namespace rfm
{
bool canLoad(
  const QList<pqRecentlyUsedResourceLoaderInterface*>& ifaces, const pqServerResource& resource)
{
  // using foreach here was causing failures on VS
  for (int cc = 0, max = ifaces.size(); cc < max; ++cc)
  {
    pqRecentlyUsedResourceLoaderInterface* iface = ifaces[cc];
    if (iface->canLoad(resource))
    {
      return true;
    }
  }
  return false;
}

bool iconAndLabel(const QList<pqRecentlyUsedResourceLoaderInterface*>& ifaces,
  const pqServerResource& resource, QIcon& icon, QString& label)
{
  // using foreach here was causing failures on VS
  for (int cc = 0, max = ifaces.size(); cc < max; ++cc)
  {
    pqRecentlyUsedResourceLoaderInterface* iface = ifaces[cc];
    if (iface->canLoad(resource))
    {
      icon = iface->icon(resource);
      label = iface->label(resource);
      return true;
    }
  }
  return false;
}

bool load(const QList<pqRecentlyUsedResourceLoaderInterface*>& ifaces,
  const pqServerResource& resource, pqServer* server)
{
  // using foreach here was causing failures on VS
  for (int cc = 0, max = ifaces.size(); cc < max; ++cc)
  {
    pqRecentlyUsedResourceLoaderInterface* iface = ifaces[cc];
    if (iface->canLoad(resource))
    {
      return iface->load(resource, server);
    }
  }
  return false;
}
}

//=============================================================================
pqRecentFilesMenu::pqRecentFilesMenu(QMenu& menu, QObject* p)
  : QObject(p)
  , Menu(&menu)
  , SortByServers(true)
{
  this->connect(this->Menu, SIGNAL(aboutToShow()), SLOT(buildMenu()));
  this->connect(this->Menu, SIGNAL(triggered(QAction*)), SLOT(onOpenResource(QAction*)));
}

//-----------------------------------------------------------------------------
pqRecentFilesMenu::~pqRecentFilesMenu() = default;

//-----------------------------------------------------------------------------
void pqRecentFilesMenu::buildMenu()
{
  if (!this->Menu)
  {
    return;
  }

  this->Menu->clear();

  pqInterfaceTracker* itk = pqApplicationCore::instance()->interfaceTracker();
  QList<pqRecentlyUsedResourceLoaderInterface*> ifaces =
    itk->interfaces<pqRecentlyUsedResourceLoaderInterface*>();

  // Get the set of all resources in most-recently-used order ...
  const pqRecentlyUsedResourcesList::ListT& resources =
    pqApplicationCore::instance()->recentlyUsedResources().list();

  // Sort resources to cluster them by servers.
  typedef QMap<QString, QList<pqServerResource> > ClusteredResourcesType;
  ClusteredResourcesType clusteredResources;

  for (int cc = 0; cc < resources.size(); cc++)
  {
    const pqServerResource& resource = resources[cc];
    QString key;
    if (this->SortByServers)
    {
      pqServerConfiguration config = resource.configuration();
      if (config.isNameDefault())
      {
        pqServerResource hostResource = (resource.scheme() == "session")
          ? resource.sessionServer().schemeHostsPorts()
          : resource.schemeHostsPorts();
        key = hostResource.toURI();
      }
      else
      {
        key = resource.configuration().URI();
      }
    }
    clusteredResources[key].push_back(resource);
  }

  // Display the servers ...
  for (ClusteredResourcesType::const_iterator criter = clusteredResources.begin();
       criter != clusteredResources.end(); ++criter)
  {
    if (!criter.key().isEmpty())
    {
      assert(this->SortByServers == true);

      // Add a separator for the server.
      QAction* const action = new QAction(criter.key(), this->Menu);
      action->setIcon(QIcon(":/pqWidgets/Icons/pqConnect.svg"));

      // ensure that the server stands out
      QFont font = action->font();
      font.setBold(true);
      action->setFont(font);
      this->Menu->addAction(action);
    }

    // now add actions for the recent items.
    for (int kk = 0; kk < criter.value().size(); ++kk)
    {
      const pqServerResource& item = criter.value()[kk];
      QString label;
      QIcon icon;
      if (rfm::iconAndLabel(ifaces, item, icon, label))
      {
        QAction* const act = new QAction(label, this->Menu);
        act->setData(item.serializeString());
        act->setIcon(icon);
        this->Menu->addAction(act);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqRecentFilesMenu::onOpenResource(QAction* action)
{
  QString data = action ? action->data().toString() : QString();
  if (!data.isEmpty())
  {
    this->onOpenResource(pqServerResource(action->data().toString()));
  }
}

//-----------------------------------------------------------------------------
void pqRecentFilesMenu::onOpenResource(const pqServerResource& resource)
{
  const pqServerResource server = resource.scheme() == "session"
    ? resource.sessionServer().schemeHostsPorts()
    : resource.schemeHostsPorts();

  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqServer* pq_server = smModel->findServer(server);
  if (!pq_server)
  {
    int ret =
      QMessageBox::warning(pqCoreUtilities::mainWidget(), tr("Disconnect from current server?"),
        tr("The file you opened requires connecting to a new server. \n"
           "The current connection will be closed.\n\n"
           "Are you sure you want to continue?"),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::No)
    {
      return;
    }
    pqServerConfiguration config_to_connect;
    if (pqServerConnectDialog::selectServer(
          config_to_connect, pqCoreUtilities::mainWidget(), server))
    {
      QScopedPointer<pqServerLauncher> launcher(pqServerLauncher::newInstance(config_to_connect));
      if (launcher->connectToServer())
      {
        pq_server = launcher->connectedServer();
      }
    }
  }

  if (pq_server)
  {
    this->open(pq_server, resource);
  }
}

//-----------------------------------------------------------------------------
bool pqRecentFilesMenu::open(pqServer* server, const pqServerResource& resource) const
{
  pqInterfaceTracker* itk = pqApplicationCore::instance()->interfaceTracker();
  QList<pqRecentlyUsedResourceLoaderInterface*> ifaces =
    itk->interfaces<pqRecentlyUsedResourceLoaderInterface*>();
  if (rfm::load(ifaces, resource, server))
  {
    pqRecentlyUsedResourcesList& mruList = pqApplicationCore::instance()->recentlyUsedResources();
    mruList.add(resource);
    mruList.save(*pqApplicationCore::instance()->settings());
    return true;
  }
  return false;
}
