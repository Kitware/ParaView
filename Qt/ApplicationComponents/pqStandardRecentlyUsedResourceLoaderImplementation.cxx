// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqStandardRecentlyUsedResourceLoaderImplementation.h"

#include "pqApplicationCore.h"
#include "pqFileDialogModel.h"
#include "pqLoadDataReaction.h"
#include "pqLoadStateReaction.h"
#include "pqRecentlyUsedResourcesList.h"
#include "pqServer.h"
#include "pqServerConfiguration.h"
#include "pqServerResource.h"
#include "vtkPVSession.h"

#include <QFileInfo>
#include <QtDebug>

#include <cassert>

//-----------------------------------------------------------------------------
pqStandardRecentlyUsedResourceLoaderImplementation::
  pqStandardRecentlyUsedResourceLoaderImplementation(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqStandardRecentlyUsedResourceLoaderImplementation::
  ~pqStandardRecentlyUsedResourceLoaderImplementation() = default;

//-----------------------------------------------------------------------------
bool pqStandardRecentlyUsedResourceLoaderImplementation::canLoad(const pqServerResource& resource)
{
  return (resource.hasData("PARAVIEW_STATE") ||
    (resource.hasData("PARAVIEW_DATA") && resource.hasData("smgroup") &&
      resource.hasData("smname")));
}

//-----------------------------------------------------------------------------
bool pqStandardRecentlyUsedResourceLoaderImplementation::load(
  const pqServerResource& resource, pqServer* server)
{
  assert(this->canLoad(resource));
  if (resource.hasData("PARAVIEW_STATE"))
  {
    if (resource.hasData("FILE_LOCATION"))
    {
      return this->loadState(resource, server, resource.data("FILE_LOCATION").toUInt());
    }
    else
    {
      return this->loadState(resource, server, vtkPVSession::CLIENT);
    }
  }
  else if (resource.hasData("PARAVIEW_DATA"))
  {
    return this->loadData(resource, server);
  }
  return false;
}

//-----------------------------------------------------------------------------
QIcon pqStandardRecentlyUsedResourceLoaderImplementation::icon(const pqServerResource& resource)
{
  if (resource.hasData("PARAVIEW_STATE"))
  {
    return QIcon(":/pqWidgets/Icons/pvIcon.svg");
  }
  else if (resource.hasData("PARAVIEW_DATA"))
  {
    return QIcon(":/pqWidgets/Icons/pqMultiBlockData16.png");
  }
  return QIcon();
}

//-----------------------------------------------------------------------------
QString pqStandardRecentlyUsedResourceLoaderImplementation::label(const pqServerResource& resource)
{
  return resource.path();
}

//-----------------------------------------------------------------------------
bool pqStandardRecentlyUsedResourceLoaderImplementation::loadState(
  const pqServerResource& resource, pqServer* server, vtkTypeUInt32 location)
{
  QString stateFile = resource.path();
  pqLoadStateReaction::loadState(stateFile, false, server, location);
  return true;
}

//-----------------------------------------------------------------------------
bool pqStandardRecentlyUsedResourceLoaderImplementation::loadData(
  const pqServerResource& resource, pqServer* server)
{
  QString filename = resource.path();
  QString readerGroup = resource.data("smgroup");
  QString readerName = resource.data("smname");

  QStringList files;
  if (filename.isEmpty())
  {
    qCritical() << "Empty file name specified!";
    return false;
  }
  files.push_back(filename);
  QString extrafilesCount = resource.data("extrafilesCount", "-1");
  if (extrafilesCount.toInt() > 0)
  {
    for (int cc = 0; cc < extrafilesCount.toInt(); cc++)
    {
      QString extrafile = resource.data(QString("file.%1").arg(cc));
      if (!extrafile.isEmpty())
      {
        // pqLoadDataReaction validates each file, so we'll don't validate it
        // here.
        files.push_back(extrafile);
      }
    }
  }
  return pqLoadDataReaction::loadData(files, readerGroup, readerName, server);
}

//-----------------------------------------------------------------------------
bool pqStandardRecentlyUsedResourceLoaderImplementation::addDataFilesToRecentResources(
  pqServer* server, const QStringList& files, const QString& smgroup, const QString& smname)
{
  if (server)
  {
    // Needed to get the display resource in case of port forwarding
    pqServerResource resource = server->getResource();
    pqServerConfiguration config = resource.configuration();
    if (!config.isNameDefault())
    {
      resource = config.resource();
    }

    resource.setPath(files[0]);
    resource.addData("PARAVIEW_DATA", "1");
    resource.addData("smgroup", smgroup);
    resource.addData("smname", smname);
    resource.addData("extrafilesCount", QString("%1").arg(files.size() - 1));
    for (int cc = 1; cc < files.size(); cc++)
    {
      resource.addData(QString("file.%1").arg(cc - 1), files[cc]);
    }
    pqApplicationCore* core = pqApplicationCore::instance();
    core->recentlyUsedResources().add(resource);
    core->recentlyUsedResources().save(*core->settings());
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqStandardRecentlyUsedResourceLoaderImplementation::addStateFileToRecentResources(
  pqServer* server, const QString& filename, vtkTypeUInt32 location)
{
  if (server)
  {
    // Needed to get the display resource in case of port forwarding
    pqServerResource resource = server->getResource();
    pqServerConfiguration config = resource.configuration();
    if (!config.isNameDefault())
    {
      resource = config.resource();
    }

    // Add this to the list of recent server resources ...
    resource.setPath(filename);
    resource.addData("PARAVIEW_STATE", "1");
    resource.addData("FILE_LOCATION", QString::number(location));
    pqApplicationCore* core = pqApplicationCore::instance();
    core->recentlyUsedResources().add(resource);
    core->recentlyUsedResources().save(*core->settings());
    return true;
  }

  return false;
}
