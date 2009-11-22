/*=========================================================================

   Program: ParaView
   Module:    pqBrandPluginsLoader.cxx

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
#include "pqBrandPluginsLoader.h"

#include "pqApplicationCore.h"
#include "pqPluginManager.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include "vtkObjectBase.h"

//-----------------------------------------------------------------------------
pqBrandPluginsLoader::pqBrandPluginsLoader(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
static QString locatePlugin(const QStringList& paths, const QString& name)
{
  QString filename;
#if defined(Q_WS_WIN)
  filename = name + ".dll";
#elif defined(Q_WS_MAC)
  filename = "lib" + name + ".dylib";
#else
  filename = "lib" + name + ".so";
#endif
 foreach (QString path, paths)
   {
   QFileInfo info(path + "/" + filename);
   if (info.exists() && info.isReadable())
     {
     return (path + "/" + filename);
     }
   }
 return QString();
}

//-----------------------------------------------------------------------------
bool pqBrandPluginsLoader::loadPlugins(const QStringList& plugins,
  bool skip_missing_plugins)
{
  pqPluginManager* pluginManager =
    pqApplicationCore::instance()->getPluginManager();

  // with "/" as the separator on all platforms.
  QString app_dir =
    QDir::fromNativeSeparators(QApplication::applicationDirPath());

  foreach (QString plugin, plugins)
    {
    QStringList paths_to_search;
    paths_to_search << app_dir;
    paths_to_search << app_dir + "/plugins/" + plugin;
#if defined(Q_WS_MAC)
    paths_to_search << app_dir + "/../Plugins";
    paths_to_search << app_dir + "/../../.." ;
#endif
    QString plugin_library = locatePlugin(paths_to_search, plugin);
    if (plugin_library.isEmpty())
      {
      if (skip_missing_plugins)
        {
        continue;
        }
      qDebug() << "Failed to locate plugin: " << plugin;
      }
    else if (pluginManager->loadExtension(NULL, plugin_library) ==
      pqPluginManager::NOTLOADED)
      {
      if (skip_missing_plugins)
        {
        continue;
        }
      qCritical() << "Failed to load plugin: " << plugin;
      return false;
      }
    else
      {
      cout << "Loaded plugin: " << plugin_library.toAscii().data() << endl;
      }
    }
  return true;
}

