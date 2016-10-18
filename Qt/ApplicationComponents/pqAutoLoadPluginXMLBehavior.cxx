/*=========================================================================

   Program: ParaView
   Module:    pqAutoLoadPluginXMLBehavior.cxx

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
    foreach (QString file, contents)
    {
      set.insert(prefix + "/" + file);
    }
    return;
  }
  QStringList contents = dir.entryList(QDir::AllDirs);
  foreach (QString sub_dir, contents)
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

  foreach (QString dir, xml_files)
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
