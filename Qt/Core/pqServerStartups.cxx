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
#include "pqSettings.h"
#include "pqCommandServerStartup.h"

#include <QDomDocument>
#include <QtDebug>

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

    QDomAttr xml_server_owner = xml.createAttribute("owner");
    xml_server_owner.setValue(startup.getOwner());
    xml_server.setAttributeNode(xml_server_owner);

    xml_server.appendChild(xml.importNode(startup.getConfiguration().documentElement(), true));
    
    return xml_server;
  }
  
  static pqServerStartup* load(QDomElement xml_server)
  {
    const QString name = xml_server.toElement().attribute("name");
  
    const pqServerResource server =
      pqServerResource(xml_server.toElement().attribute("resource"));
      
    const QString owner = xml_server.toElement().attribute("owner");

    for(QDomNode xml_startup = xml_server.firstChild(); !xml_startup.isNull(); xml_startup = xml_startup.nextSibling())
      {
      if(xml_startup.isElement() && xml_startup.toElement().tagName() == "ManualStartup")
        {
        QDomDocument xml;
        xml.appendChild(xml.importNode(xml_startup, true));
        return new pqManualServerStartup(name, server, owner, xml);
        }
      else if(xml_startup.isElement() && xml_startup.toElement().tagName() == "CommandStartup")
        {
        QDomDocument xml;
        xml.appendChild(xml.importNode(xml_startup, true));
        return new pqCommandServerStartup(name, server, owner, xml);
        }
      }

    return 0;
  }

  typedef vtkstd::map<QString, pqServerStartup*> StartupsT;
  StartupsT Startups;
};

/////////////////////////////////////////////////////////////////////////////
// pqServerStartups

pqServerStartups::pqServerStartups(QObject* p) :
  QObject(p), Implementation(new pqImplementation())
{
}

pqServerStartups::~pqServerStartups()
{
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
  const pqServerResource& server,
  const QString& owner)
{
  QDomDocument configuration;
  configuration.setContent(QString("<ManualStartup/>"));
  
  this->Implementation->deleteStartup(name);
  this->Implementation->Startups.insert(
    vtkstd::make_pair(name, new pqManualServerStartup(name, server, owner, configuration)));
  emit this->changed();
}

void pqServerStartups::setCommandStartup(
  const QString& name,
  const pqServerResource& server,
  const QString& owner,
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
    name, new pqCommandServerStartup(name, server, owner, xml)));
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

void pqServerStartups::save(pqSettings& settings) const
{
  settings.remove("Servers");

  for(
    pqImplementation::StartupsT::iterator i = this->Implementation->Startups.begin();
    i != this->Implementation->Startups.end();
    ++i)
    {
    const QString name = i->first;
    const QString owner = i->second->getOwner();
    pqServerStartup* const startup = i->second;

    if(owner != "user")
      continue;

    QDomDocument xml;
    xml.appendChild(pqImplementation::save(xml, name, *startup));

    const QString server_key = "Servers/" + name;
    settings.setValue(server_key, xml.toByteArray().data());
    }
}

void pqServerStartups::save(QDomDocument& xml) const
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
    
    xml_servers.appendChild(pqImplementation::save(xml, startup_name, *startup_command));
    }
}

void pqServerStartups::load(pqSettings& settings)
{
  settings.beginGroup("Servers");
  const QStringList startups = settings.childKeys();
  for(int i = 0; i != startups.size(); ++i)
    {
    const QString name = startups[i];
    const QString value = settings.value(name).toString();
    
    QDomDocument xml_server;
    QString error_message;
    xml_server.setContent(value, &error_message);
    if(pqServerStartup* const startup = pqImplementation::load(xml_server.documentElement()))
      {
      this->Implementation->deleteStartup(name);
      this->Implementation->Startups.insert(vtkstd::make_pair(name, startup));
      }
    }
  settings.endGroup();
}

void pqServerStartups::load(QDomDocument& xml_document)
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
      if(pqServerStartup* const startup = pqImplementation::load(xml_server.toElement()))
        {
        this->Implementation->deleteStartup(name);
        this->Implementation->Startups.insert(vtkstd::make_pair(name, startup));
        }
      }
    }

  emit this->changed();
}
