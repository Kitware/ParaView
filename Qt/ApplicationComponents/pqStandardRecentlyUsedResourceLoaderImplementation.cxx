/*=========================================================================

   Program: ParaView
   Module:  pqStandardRecentlyUsedResourceLoaderImplementation.cxx

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
#include "pqStandardRecentlyUsedResourceLoaderImplementation.h"

#include "pqApplicationCore.h"
#include "pqFileDialogModel.h"
#include "pqLoadDataReaction.h"
#include "pqLoadStateReaction.h"
#include "pqRecentlyUsedResourcesList.h"
#include "pqServer.h"
#include "pqServerConfiguration.h"
#include "pqServerResource.h"

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
  ~pqStandardRecentlyUsedResourceLoaderImplementation()
{
}

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
    return this->loadState(resource, server);
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
  const pqServerResource& resource, pqServer* server)
{
  QString stateFile = resource.path();

  QFileInfo finfo(stateFile);
  if (!finfo.isFile() || !finfo.isReadable())
  {
    qCritical() << "State file no longer exists: '" << stateFile << "'";
    return false;
  }
  pqLoadStateReaction::loadState(stateFile, server);
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
  pqServer* server, const QString& filename)
{
  if (server)
  {
    // Needed to get the display resource in case of port forwarding
    pqServerResource tmpResource = server->getResource();
    pqServerConfiguration config = tmpResource.configuration();
    if (!config.isNameDefault())
    {
      tmpResource = config.resource();
    }

    // Add this to the list of recent server resources ...
    pqServerResource resource;
    resource.setScheme("session");
    resource.setPath(filename);
    resource.setSessionServer(tmpResource);
    resource.addData("PARAVIEW_STATE", "1");
    pqApplicationCore* core = pqApplicationCore::instance();
    core->recentlyUsedResources().add(resource);
    core->recentlyUsedResources().save(*core->settings());
    return true;
  }

  return false;
}
