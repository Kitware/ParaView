// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqServerConfigurationCollection.h"

#include "pqCoreUtilities.h"
#include "pqServerConfiguration.h"
#include "pqServerResource.h"
#include "vtkNew.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkProcessModule.h"
#include "vtkRemotingCoreConfiguration.h"
#include "vtkResourceFileLocator.h"
#include "vtkVersion.h"

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
  return pqCoreUtilities::getParaViewUserDirectory() + "/servers.pvsc";
}

static const std::vector<std::string>& customServers()
{
  return vtkRemotingCoreConfiguration::GetInstance()->GetServerConfigurationsFiles();
}
// get path to shared system servers.
static QString systemServers()
{
  QString settingsRoot;
#if defined(Q_OS_WIN)
  settingsRoot = QString::fromUtf8(getenv("COMMON_APPDATA"));
#else
  settingsRoot = QString::fromUtf8("/usr/share");
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
  std::vector<std::string> prefixes = { ".", "bin", "lib", "MacOS" };

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

  if (!vtkRemotingCoreConfiguration::GetInstance()->GetDisableRegistry())
  {
    this->load(defaultServers(), false);
    this->load(systemServers(), false);
    for (auto& fname : customServers())
    {
      this->load(fname.c_str(), false);
    }
    this->load(userServers(), true);
  }
}

//-----------------------------------------------------------------------------
pqServerConfigurationCollection::~pqServerConfigurationCollection()
{
  auto config = vtkRemotingCoreConfiguration::GetInstance();
  // save to servers.pvsc only if --dr/--disable-registry flag is absent and configurations exist
  if (!config->GetDisableRegistry() && !this->configurations().empty())
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
  auto config = vtkRemotingCoreConfiguration::GetInstance();
  if (!config->GetDisableRegistry())

  {
    return this->save(userServers(), true);
  }
  else
  {
    static bool warned = false;
    if (!warned)
    {
      qWarning() << "When running with the `--dr` flag the server settings will not be saved.";
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
    file.write(contents.toUtf8().data());
    file.close();
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqServerConfigurationCollection::loadContents(const QString& contents, bool mutable_configs)
{
  vtkNew<vtkPVXMLParser> parser;
  if (!parser->Parse(contents.toUtf8().data()))
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
  Q_FOREACH (const pqServerConfiguration& config, this->Configurations)
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
  Q_FOREACH (const pqServerConfiguration& config, this->Configurations)
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
  Q_FOREACH (const pqServerConfiguration& config, this->Configurations)
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

void pqServerConfigurationCollection::removeUserConfigurations()
{
  Q_FOREACH (pqServerConfiguration config, this->configurations())
  {
    this->removeConfiguration(config.name());
  }
  pqCoreUtilities::remove(::userServers());
}
