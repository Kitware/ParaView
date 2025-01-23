// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
namespace
{
bool iconAndLabel(const QList<pqRecentlyUsedResourceLoaderInterface*>& ifaces,
  const pqServerResource& resource, QIcon& icon, QString& label)
{
  // using Q_FOREACH here was causing failures on VS
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
  // using Q_FOREACH here was causing failures on VS
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
  typedef QMap<QString, QList<pqServerResource>> ClusteredResourcesType;
  ClusteredResourcesType clusteredResources;

  for (int cc = 0; cc < resources.size(); cc++)
  {
    const pqServerResource& resource = resources[cc];
    QString key;
    if (this->SortByServers)
    {
      const pqServerConfiguration& config = resource.configuration();
      if (config.isNameDefault())
      {
        QString serverName = resource.serverName();
        if (!serverName.isEmpty())
        {
          key = serverName;
        }
        else
        {
          key = resource.schemeHostsPorts().toURI();
        }
      }
      else
      {
        key = resource.configuration().name();
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
      if (::iconAndLabel(ifaces, item, icon, label))
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
    this->onOpenResource(pqServerResource(data));
  }
}

//-----------------------------------------------------------------------------
void pqRecentFilesMenu::onOpenResource(const pqServerResource& resource)
{
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqServer* server;

  // If a server name is set, use it to find the server, if not, use the schemehostsports instead
  if (resource.serverName().isEmpty())
  {
    const pqServerResource serverResource = resource.schemeHostsPorts();
    server = smModel->findServer(serverResource);
  }
  else
  {
    server = smModel->findServer(resource.serverName());
  }

  if (!server)
  {
    int ret =
      QMessageBox::warning(pqCoreUtilities::mainWidget(), tr("Disconnect from current server?"),
        tr("The file you opened requires connecting to a new server.\n"
           "The current connection will be closed.\n\n"
           "Are you sure you want to continue?"),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::No)
    {
      return;
    }
    pqServerConfiguration config_to_connect;
    if (pqServerConnectDialog::selectServer(
          config_to_connect, pqCoreUtilities::mainWidget(), resource))
    {
      QScopedPointer<pqServerLauncher> launcher(pqServerLauncher::newInstance(config_to_connect));
      if (launcher->connectToServer())
      {
        server = launcher->connectedServer();
      }
    }
  }

  if (server)
  {
    this->open(server, resource);
  }
}

//-----------------------------------------------------------------------------
bool pqRecentFilesMenu::open(pqServer* server, const pqServerResource& resource) const
{
  pqInterfaceTracker* itk = pqApplicationCore::instance()->interfaceTracker();
  QList<pqRecentlyUsedResourceLoaderInterface*> ifaces =
    itk->interfaces<pqRecentlyUsedResourceLoaderInterface*>();
  if (::load(ifaces, resource, server))
  {
    pqRecentlyUsedResourcesList& mruList = pqApplicationCore::instance()->recentlyUsedResources();
    mruList.add(resource);
    mruList.save(*pqApplicationCore::instance()->settings());
    return true;
  }
  return false;
}
