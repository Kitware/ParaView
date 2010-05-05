/*=========================================================================

   Program: ParaView
   Module:    pqServerStartups.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#include "pqManualServerStartup.h"
#include "pqServerResource.h"
#include "pqServerStartups.h"
#include "pqCommandServerStartup.h"
#include "pqOptions.h"
#include "pqApplicationCore.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QtDebug>

#include <vtkPVXMLElement.h>
#include <vtkPVXMLParser.h>
#include <vtkProcessModule.h>
#include <vtkstd/map>
#include <vtksys/ios/sstream>

/////////////////////////////////////////////////////////////////////////////
// pqServerStartups::pqImplementation

class pqServerStartups::pqImplementation
{
public:
  pqImplementation()
  {
    vtkSmartPointer<vtkPVXMLElement> configuration =
      vtkSmartPointer<vtkPVXMLElement>::New();
    configuration->SetName("ManualStartup");

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

  static vtkSmartPointer<vtkPVXMLElement> save(const QString& name, pqServerStartup& startup)
  {
    vtkSmartPointer<vtkPVXMLElement> xml_server =
      vtkSmartPointer<vtkPVXMLElement>::New();
    xml_server->SetName("Server");
    xml_server->AddAttribute("name", name.toAscii().data());
    xml_server->AddAttribute("resource",
      startup.getServer().toURI().toAscii().data());
    xml_server->AddNestedElement(startup.getConfiguration());
    return xml_server;
  }

  pqServerStartup* load(vtkPVXMLElement* xml_server, bool userPrefs)
  {
    const QString name = xml_server->GetAttribute("name");

    const pqServerResource server =
      pqServerResource(xml_server->GetAttribute("resource"));

    int num = xml_server->GetNumberOfNestedElements();
    for (int i=0; i<num; i++)
      {
      vtkPVXMLElement* xml_startup = xml_server->GetNestedElement(i);
      if(QString(xml_startup->GetName()) == "ManualStartup")
        {
        return new pqManualServerStartup(name, server, userPrefs, xml_startup);
        }
      else if(QString(xml_startup->GetName()) == "CommandStartup")
        {
        return new pqCommandServerStartup(name, server, userPrefs, xml_startup);
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
  settingsPath = settingsPath.arg(QApplication::organizationName());
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
  else if (options && options->GetDisableRegistry())
    {
    // load the testing servers resource.
    this->load(":/pqCoreTesting/pqTestingServers.pvsc", false);
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
  vtkSmartPointer<vtkPVXMLElement> configuration =
    vtkSmartPointer<vtkPVXMLElement>::New();
  configuration->SetName("ManualStartup");

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
  vtkSmartPointer<vtkPVXMLElement> configuration =
    vtkSmartPointer<vtkPVXMLElement>::New();
  configuration->SetName("CommandStartup");

  vtkSmartPointer<vtkPVXMLElement> xml_command =
    vtkSmartPointer<vtkPVXMLElement>::New();
  xml_command->SetName("Command");
  configuration->AddNestedElement(xml_command);

  xml_command->AddAttribute("exec", executable.toAscii().data());
  xml_command->AddAttribute("timeout", timeout);
  xml_command->AddAttribute("delay", delay);

  xml_command->AddAttribute("Arguments", delay);

  vtkSmartPointer<vtkPVXMLElement> xml_arguments =
    vtkSmartPointer<vtkPVXMLElement>::New();
  xml_arguments->SetName("Arguments");
  xml_command->AddNestedElement(xml_arguments);

  for(int i = 0; i != arguments.size(); ++i)
    {
    vtkSmartPointer<vtkPVXMLElement> xml_argument =
      vtkSmartPointer<vtkPVXMLElement>::New();
    xml_argument->SetName("Argument");
    xml_arguments->AddNestedElement(xml_argument);
    xml_argument->AddAttribute("value", arguments[i].toAscii().data());
    }

  this->Implementation->deleteStartup(name);
  this->Implementation->Startups.insert(vtkstd::make_pair(
    name, new pqCommandServerStartup(name, server, true, configuration)));
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

void pqServerStartups::save(vtkPVXMLElement* xml, bool userPrefs) const
{
  vtkPVXMLElement* xml_servers = vtkPVXMLElement::New();
  xml_servers->SetName("Servers");
  xml->AddNestedElement(xml_servers);
  xml_servers->Delete();

  for(
    pqImplementation::StartupsT::iterator startup = this->Implementation->Startups.begin();
    startup != this->Implementation->Startups.end();
    ++startup)
    {
    const QString startup_name = startup->first;
    pqServerStartup* const startup_command = startup->second;
    if ( (userPrefs && startup_command->shouldSave()) || !userPrefs)
      {
      xml_servers->AddNestedElement(pqImplementation::save(startup_name, *startup_command));
      }
    }
}

void pqServerStartups::save(const QString& path, bool userPrefs) const
{
  vtkSmartPointer<vtkPVXMLElement> xml =
    vtkSmartPointer<vtkPVXMLElement>::New();
  this->save(xml, userPrefs);

  vtksys_ios::ostringstream xml_stream;
  xml->GetNestedElement(0)->PrintXML(xml_stream, vtkIndent());

  QFile file(path);
  if(file.open(QIODevice::WriteOnly))
    {
    file.write(xml_stream.str().c_str());
    }
  else
    {
    qCritical() << "Error opening " << path << "for writing";
    }
}

void pqServerStartups::load(vtkPVXMLElement* xml_servers, bool userPrefs)
{
  if(QString(xml_servers->GetName()) != "Servers")
    {
    qCritical() << "Not a ParaView server configuration document";
    return;
    }

  int num = xml_servers->GetNumberOfNestedElements();
  for(int i=0; i<num; i++)
    {
    vtkPVXMLElement* xml_server = xml_servers->GetNestedElement(i);
    if(QString(xml_server->GetName()) == "Server")
      {
      const QString name = xml_server->GetAttribute("name");
      if(pqServerStartup* const startup =
        this->Implementation->load(xml_server, userPrefs))
        {
        this->Implementation->deleteStartup(name);
        this->Implementation->Startups.insert(vtkstd::make_pair(name, startup));
        }
      }
    }

  emit this->changed();
}

void pqServerStartups::load(const QString& path, bool userPrefs)
{
  QFile file(path);
  if (file.open(QIODevice::ReadOnly))
    {
    QByteArray dat = file.readAll();
    vtkSmartPointer<vtkPVXMLParser> parser =
      vtkSmartPointer<vtkPVXMLParser>::New();
    if (parser->Parse(dat.data()))
      {
      this->load(parser->GetRootElement(), userPrefs);
      }
    else
      {
      QString warn("Failed to parse ");
      warn += path;
      qWarning() << warn;
      }
    }
}

