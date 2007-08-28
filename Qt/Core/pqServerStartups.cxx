/*=========================================================================

   Program: ParaView
   Module:    pqServerStartups.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "pqManualServerStartup.h"
#include "pqServerResource.h"
#include "pqServerStartups.h"
#include "pqCommandServerStartup.h"
#include "pqOptions.h"
#include "pqApplicationCore.h"

#include <QDomDocument>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QtDebug>

#include <vtkProcessModule.h>
#include <vtkstd/map>

/////////////////////////////////////////////////////////////////////////////
// pqServerStartups::pqImplementation

class pqServerStartups::pqImplementation
{
public:
  pqImplementation()
  {
    QDomDocument configuration;
    configuration.setContent(QString("<ManualStartup/>"));
  
    // Setup a builtin server as a hard-coded default ...
    Startups["builtin"] = new pqManualServerStartup(
      "builtin",
      pqServerResource("builtin:"),
      "builtin",
      configuration);
  }
  
  ~pqImplementation()
  {
    for(
      StartupsT::iterator startup = this->Startups.begin();
      startup != this->Startups.end();
      ++startup)
      {
      delete startup->second;
      }
  }
  
  void deleteStartup(const QString& startup)
  {
    if(this->Startups.count(startup))
    {
    delete this->Startups[startup];
    this->Startups.erase(startup);
    }
  }
  
  static QDomElement save(QDomDocument& xml, const QString& name, pqServerStartup& startup)
  {
    QDomElement xml_server = xml.createElement("Server");

    QDomAttr xml_server_name = xml.createAttribute("name");
    xml_server_name.setValue(name);
    xml_server.setAttributeNode(xml_server_name);

    QDomAttr xml_server_resource = xml.createAttribute("resource");
    xml_server_resource.setValue(startup.getServer().toURI());
    xml_server.setAttributeNode(xml_server_resource);

    xml_server.appendChild(xml.importNode(startup.getConfiguration().documentElement(), true));
    
    return xml_server;
  }

  static pqServerStartup* load(QDomElement xml_server, bool save)
  {
    const QString name = xml_server.toElement().attribute("name");
  
    const pqServerResource server =
      pqServerResource(xml_server.toElement().attribute("resource"));
      
    for(QDomNode xml_startup = xml_server.firstChild(); !xml_startup.isNull(); xml_startup = xml_startup.nextSibling())
      {
      if(xml_startup.isElement() && xml_startup.toElement().tagName() == "ManualStartup")
        {
        QDomDocument xml;
        xml.appendChild(xml.importNode(xml_startup, true));
        return new pqManualServerStartup(name, server, save, xml);
        }
      else if(xml_startup.isElement() && xml_startup.toElement().tagName() == "CommandStartup")
        {
        QDomDocument xml;
        xml.appendChild(xml.importNode(xml_startup, true));
        return new pqCommandServerStartup(name, server, save, xml);
        }
      }

    return 0;
  }

  typedef vtkstd::map<QString, pqServerStartup*> StartupsT;
  StartupsT Startups;
};

/////////////////////////////////////////////////////////////////////////////
// pqServerStartups

static QString userSettings()
{
  QString settingsRoot;
#if defined(Q_OS_WIN)
  settingsRoot = QString::fromLocal8Bit(getenv("APPDATA"));
#else
  settingsRoot = QString::fromLocal8Bit(getenv("HOME")) + 
                 QDir::separator() + QString::fromLocal8Bit(".config");
#endif
  QString settingsPath = QString("%2%1%3%1%4");
  settingsPath = settingsPath.arg(QDir::separator());
  settingsPath = settingsPath.arg(settingsRoot);
  settingsPath = settingsPath.arg(QApplication::organizationName());
  settingsPath = settingsPath.arg("servers.pvsc");
  return settingsPath;
}

static QString systemSettings()
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
  settingsPath = settingsPath.arg(QApplication::applicationName());
  settingsPath = settingsPath.arg("servers.pvsc");
  return settingsPath;
}

pqServerStartups::pqServerStartups(QObject* p) :
  QObject(p), Implementation(new pqImplementation())
{
  pqOptions* options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());
  if(!options || !options->GetDisableRegistry())
    {
    // load from application dir
    this->load(QApplication::applicationDirPath() + QDir::separator() +
               "default_servers.pvsc", false);
    // load from system dir (/usr/share/..., %COMMON_APPDATA%/...)
    this->load(systemSettings(), false);
    // load user settings
    this->load(userSettings(), true);
    }
}

pqServerStartups::~pqServerStartups()
{
  pqOptions* options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());
  if(!options || !options->GetDisableRegistry())
    {
    this->save(userSettings(),true);
    }
  delete this->Implementation;
}

const pqServerStartups::StartupsT pqServerStartups::getStartups() const
{
  StartupsT results;

  for(
    pqImplementation::StartupsT::iterator startup = this->Implementation->Startups.begin();
    startup != this->Implementation->Startups.end();
    ++startup)
    {
    results.push_back(startup->first);
    }
    
  return results;
}

const pqServerStartups::StartupsT pqServerStartups::getStartups(const pqServerResource& server) const
{
  StartupsT results;

  for(
    pqImplementation::StartupsT::iterator startup = this->Implementation->Startups.begin();
    startup != this->Implementation->Startups.end();
    ++startup)
    {
    if(startup->second->getServer().schemeHosts() == server.schemeHosts())
      {
      results.push_back(startup->first);
      }
    }
    
  return results;
}

pqServerStartup* pqServerStartups::getStartup(const QString& startup) const
{
  return this->Implementation->Startups.count(startup)
    ? this->Implementation->Startups[startup]
    : 0;
}

void pqServerStartups::setManualStartup(
  const QString& name,
  const pqServerResource& server)
{
  QDomDocument configuration;
  configuration.setContent(QString("<ManualStartup/>"));
  
  this->Implementation->deleteStartup(name);
  this->Implementation->Startups.insert(
    vtkstd::make_pair(name, new pqManualServerStartup(name, server, true, configuration)));
  emit this->changed();
}

void pqServerStartups::setCommandStartup(
  const QString& name,
  const pqServerResource& server,
  const QString& executable,
  double timeout,
  double delay,
  const QStringList& arguments)
{
  QDomDocument xml;
  QDomElement xml_command_startup = xml.createElement("CommandStartup");
  xml.appendChild(xml_command_startup);

  QDomElement xml_command = xml.createElement("Command");
  xml_command_startup.appendChild(xml_command);
  
  QDomAttr xml_command_exec = xml.createAttribute("exec");
  xml_command_exec.setValue(executable);
  xml_command.setAttributeNode(xml_command_exec);
  
  QDomAttr xml_command_timeout = xml.createAttribute("timeout");
  xml_command_timeout.setValue(QString::number(timeout));
  xml_command.setAttributeNode(xml_command_timeout);
  
  QDomAttr xml_command_delay = xml.createAttribute("delay");
  xml_command_delay.setValue(QString::number(delay));
  xml_command.setAttributeNode(xml_command_delay);

  QDomElement xml_arguments = xml.createElement("Arguments");
  xml_command.appendChild(xml_arguments);
  
  for(int i = 0; i != arguments.size(); ++i)
    {
    QDomElement xml_argument = xml.createElement("Argument");
    xml_arguments.appendChild(xml_argument);
    
    QDomAttr xml_argument_value = xml.createAttribute("value");
    xml_argument_value.setValue(arguments[i]);
    xml_argument.setAttributeNode(xml_argument_value);
    }

  this->Implementation->deleteStartup(name);
  this->Implementation->Startups.insert(vtkstd::make_pair(
    name, new pqCommandServerStartup(name, server, true, xml)));
  emit this->changed();
}

void pqServerStartups::deleteStartups(const StartupsT& startups)
{
  for(
    StartupsT::const_iterator startup = startups.begin();
    startup != startups.end();
    ++startup)
    {
    this->Implementation->deleteStartup(*startup);
    }
    
  emit this->changed();
}

void pqServerStartups::save(QDomDocument& xml, bool saveOnly) const
{
  QDomElement xml_servers = xml.createElement("Servers");
  xml.appendChild(xml_servers);

  for(
    pqImplementation::StartupsT::iterator startup = this->Implementation->Startups.begin();
    startup != this->Implementation->Startups.end();
    ++startup)
    {
    const QString startup_name = startup->first;
    pqServerStartup* const startup_command = startup->second;
    if(saveOnly && !startup_command->shouldSave())
      continue;
    
    xml_servers.appendChild(pqImplementation::save(xml, startup_name, *startup_command));
    }
}

void pqServerStartups::save(const QString& path, bool saveOnly) const
{
  QDomDocument xml;
  this->save(xml, saveOnly);
  
  QFile file(path);
  if(file.open(QIODevice::WriteOnly))
    {
    file.write(xml.toByteArray());
    }
  else
    {
    qCritical() << "Error opening " << path << "for writing";
    }
}

void pqServerStartups::load(QDomDocument& xml_document, bool s)
{
  QDomElement xml_servers = xml_document.documentElement();
  if(xml_servers.nodeName() != "Servers")
    {
    qCritical() << "Not a ParaView server configuration document";
    return;
    }

  for(QDomNode xml_server = xml_servers.firstChild(); !xml_server.isNull(); xml_server = xml_server.nextSibling())
    {
    if(xml_server.isElement() && xml_server.toElement().tagName() == "Server")
      {
      const QString name = xml_server.toElement().attribute("name");
      if(pqServerStartup* const startup = pqImplementation::load(xml_server.toElement(), s))
        {
        this->Implementation->deleteStartup(name);
        this->Implementation->Startups.insert(vtkstd::make_pair(name, startup));
        }
      }
    }

  emit this->changed();
}

void pqServerStartups::load(const QString& path, bool s)
{
  QFile file(path);
  if(file.exists())
    {
    QDomDocument xml;
    QString error_message;
    int error_line = 0;
    int error_column = 0;
    if(xml.setContent(&file, false, &error_message, &error_line, &error_column))
      {
      this->load(xml, s);
      }
    else
      {
      qWarning() << "Error parsing " << path;
      }
    }
}

