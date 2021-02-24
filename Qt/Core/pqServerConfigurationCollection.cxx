/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqServerConfigurationCollection.h"

#include "pqCoreUtilities.h"
#include "pqOptions.h"
#include "pqServerConfiguration.h"
#include "pqServerResource.h"
#include "vtkNew.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkProcessModule.h"
#include "vtkResourceFileLocator.h"

#include <vtksys/SystemTools.hxx>

#include <QApplication>
#include <QDir>
#include <QTextStream>
#include <QtDebug>
#include <sstream>

namespace
{
// get path to user-servers
static QString userServers()
{
  const char* serversFileName =
    vtkProcessModule::GetProcessModule()->GetOptions()->GetServersFileName();

  return serversFileName ? serversFileName
                         : pqCoreUtilities::getParaViewUserDirectory() + "/servers.pvsc";
}

// get path to shared system servers.
static QString systemServers()
{
  QString settingsRoot;
#if defined(Q_OS_WIN)
  settingsRoot = QString::fromLocal8Bit(getenv("COMMON_APPDATA"));
#else
  settingsRoot = QString::fromLocal8Bit("/usr/share");
#endif
  QString settingsPath = QString("%2%1%3%1%4");
  settingsPath = settingsPath.arg(QDir::separator());
  settingsPath = settingsPath.arg(settingsRoot);
  settingsPath = settingsPath.arg(QApplication::organizationName());
  settingsPath = settingsPath.arg("servers.pvsc");
  return settingsPath;
}

static QString defaultServers()
{
  auto vtk_libs = vtkGetLibraryPathForSymbol(GetVTKVersion);
  std::vector<std::string> prefixes = { ".", "bin", "lib" };

  vtkNew<vtkResourceFileLocator> locator;
  locator->SetLogVerbosity(vtkPVLogger::GetApplicationVerbosity());
  auto path = locator->Locate(vtk_libs, prefixes, "default_servers.pvsc");
  if (!path.empty())
  {
    return vtksys::SystemTools::CollapseFullPath("default_servers.pvsc", path).c_str();
  }
  return QString();
}
}

//-----------------------------------------------------------------------------
pqServerConfigurationCollection::pqServerConfigurationCollection(QObject* parentObject)
  : Superclass(parentObject)
{
  // Add a configuration for the builtin server.
  pqServerResource resource("builtin:");
  pqServerConfiguration config;
  config.setName("builtin");
  config.setResource(resource);
  config.setMutable(false);
  this->Configurations["builtin"] = config;

  pqOptions* options = pqOptions::SafeDownCast(vtkProcessModule::GetProcessModule()->GetOptions());
  if (!options || !options->GetDisableRegistry())
  {
    this->load(defaultServers(), false);
    this->load(systemServers(), false);
    this->load(userServers(), true);
  }
}

//-----------------------------------------------------------------------------
pqServerConfigurationCollection::~pqServerConfigurationCollection()
{
  pqOptions* options = pqOptions::SafeDownCast(vtkProcessModule::GetProcessModule()->GetOptions());
  if (!options || !options->GetDisableRegistry())
  {
    this->save(userServers(), true);
  }
}

//-----------------------------------------------------------------------------
bool pqServerConfigurationCollection::load(const QString& filename, bool mutable_configs)
{
  if (!filename.isEmpty())
  {
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly))
    {
      return this->loadContents(file.readAll().data(), mutable_configs);
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqServerConfigurationCollection::saveNow()
{
  pqOptions* options = pqOptions::SafeDownCast(vtkProcessModule::GetProcessModule()->GetOptions());
  if (!options || !options->GetDisableRegistry())
  {
    return this->save(userServers(), true);
  }
  else
  {
    static bool warned = false;
    if (!warned)
    {
      qWarning() << "When running with the -dr flag the server settings will not be saved.";
      warned = true;
    }
    return true;
  }
}

//-----------------------------------------------------------------------------
bool pqServerConfigurationCollection::save(const QString& filename, bool only_mutable)
{
  QString contents = this->saveContents(only_mutable);
  QFile file(filename);
  if (!contents.isEmpty() && file.open(QIODevice::WriteOnly))
  {
    file.write(contents.toLocal8Bit().data());
    file.close();
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqServerConfigurationCollection::loadContents(const QString& contents, bool mutable_configs)
{
  vtkNew<vtkPVXMLParser> parser;
  if (!parser->Parse(contents.toLocal8Bit().data()))
  {
    qWarning() << "Configuration not a valid xml.";
    return false;
  }

  vtkPVXMLElement* root = parser->GetRootElement();
  if (QString(root->GetName()) != "Servers")
  {
    qWarning() << "Not a ParaView server configuration file. Missing <Servers /> root.";
    return false;
  }

  bool prev = this->blockSignals(true);
  for (unsigned int cc = 0; cc < root->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* child = root->GetNestedElement(cc);
    if (child->GetName() && strcmp(child->GetName(), "Server") == 0)
    {
      this->addConfiguration(child, mutable_configs);
    }
  }

  this->blockSignals(prev);
  Q_EMIT this->changed();
  return true;
}

//-----------------------------------------------------------------------------
QString pqServerConfigurationCollection::saveContents(bool only_mutable) const
{
  QString xml;
  QTextStream stream(&xml);
  stream << "<Servers>\n";
  foreach (const pqServerConfiguration& config, this->Configurations)
  {
    if (only_mutable == false || config.isMutable())
    {
      stream << config.toString(vtkIndent().GetNextIndent());
    }
  }
  stream << "</Servers>";
  return xml;
}

//-----------------------------------------------------------------------------
void pqServerConfigurationCollection::addConfiguration(
  vtkPVXMLElement* arg_configuration, bool mutable_config)
{
  pqServerConfiguration config(arg_configuration);
  config.setMutable(mutable_config);
  this->addConfiguration(config);
}

//-----------------------------------------------------------------------------
void pqServerConfigurationCollection::addConfiguration(const pqServerConfiguration& config)
{
  if (config.resource().scheme() == "builtin")
  {
    // skip configs with "builtin" resource. Only 1 such config is present and
    // we add that during constructor time.
    return;
  }

  if (this->Configurations.contains(config.name()))
  {
    qWarning() << "Replacing existing server configuration named : " << config.name();
  }

  this->Configurations[config.name()] = config;
  Q_EMIT this->changed();
}

//-----------------------------------------------------------------------------
void pqServerConfigurationCollection::removeConfiguration(const QString& toremove)
{
  if (toremove == "builtin")
  {
    // don't accidentally remove the only builtin config.
    return;
  }

  if (this->Configurations.remove(toremove) > 0)
  {
    Q_EMIT this->changed();
  }
}

//-----------------------------------------------------------------------------
QList<pqServerConfiguration> pqServerConfigurationCollection::configurations() const
{
  QList<pqServerConfiguration> reply;
  foreach (const pqServerConfiguration& config, this->Configurations)
  {
    // skip builtin from this list.
    if (config.name() != "builtin")
    {
      reply.append(config);
    }
  }
  return reply;
}

//-----------------------------------------------------------------------------
QList<pqServerConfiguration> pqServerConfigurationCollection::configurations(
  const pqServerResource& selector) const
{
  QList<pqServerConfiguration> reply;
  foreach (const pqServerConfiguration& config, this->Configurations)
  {
    if (config.resource().schemeHosts() == selector.schemeHosts())
    {
      reply.append(config);
    }
  }
  return reply;
}

//-----------------------------------------------------------------------------
const pqServerConfiguration* pqServerConfigurationCollection::configuration(
  const char* configuration_name) const
{
  QMap<QString, pqServerConfiguration>::const_iterator iter =
    this->Configurations.find(configuration_name);
  if (iter != this->Configurations.constEnd())
  {
    return &iter.value();
  }

  return nullptr;
}
