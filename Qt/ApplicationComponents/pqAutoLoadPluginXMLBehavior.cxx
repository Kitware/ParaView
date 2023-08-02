// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAutoLoadPluginXMLBehavior.h"

#include "pqApplicationCore.h"
#include "pqPluginManager.h"
#include "vtkPVPlugin.h"
#include "vtkPVPluginTracker.h"
#include "vtkPVServerManagerPluginInterface.h"

#include "vtkObject.h"
#include <QDir>

void getAllParaViewResourcesDirs(const QString& prefix, QSet<QString>& set)
{
  QDir dir(prefix);
  if (!dir.exists())
  {
    return;
  }
  if (prefix.endsWith("/ParaViewResources"))
  {
    QStringList contents = dir.entryList(QDir::Files);
    Q_FOREACH (QString file, contents)
    {
      set.insert(prefix + "/" + file);
    }
    return;
  }
  QStringList contents = dir.entryList(QDir::AllDirs);
  Q_FOREACH (QString sub_dir, contents)
  {
    getAllParaViewResourcesDirs(prefix + "/" + sub_dir, set);
  }
}

//-----------------------------------------------------------------------------
pqAutoLoadPluginXMLBehavior::pqAutoLoadPluginXMLBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(pqApplicationCore::instance()->getPluginManager(), SIGNAL(pluginsUpdated()),
    this, SLOT(updateResources()));
  this->updateResources();
}

//-----------------------------------------------------------------------------
void pqAutoLoadPluginXMLBehavior::updateResources()
{
  QSet<QString> xml_files;
  ::getAllParaViewResourcesDirs(":", xml_files);

  Q_FOREACH (QString dir, xml_files)
  {
    if (!this->PreviouslyParsedResources.contains(dir))
    {
      pqApplicationCore::instance()->loadConfiguration(dir);
      this->PreviouslyParsedResources.insert(dir);
    }
  }

  // Plugins can also embed gui configuration XMLs.
  vtkPVPluginTracker* tracker = vtkPVPluginTracker::GetInstance();
  for (unsigned int cc = 0; cc < tracker->GetNumberOfPlugins(); cc++)
  {
    vtkPVPlugin* plugin = tracker->GetPlugin(cc);
    if (plugin && strcmp(plugin->GetPluginName(), "vtkPVInitializerPlugin") != 0 &&
      !this->PreviouslyParsedPlugins.contains(plugin->GetPluginName()))
    {
      this->PreviouslyParsedPlugins.insert(plugin->GetPluginName());
      vtkPVServerManagerPluginInterface* smplugin =
        dynamic_cast<vtkPVServerManagerPluginInterface*>(plugin);
      if (smplugin)
      {
        std::vector<std::string> xmls;
        smplugin->GetXMLs(xmls);
        for (size_t kk = 0; kk < xmls.size(); kk++)
        {
          pqApplicationCore::instance()->loadConfigurationXML(xmls[kk].c_str());
        }
      }
    }
  }
}
