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
#include "pqShellServerStartup.h"

#include <QtDebug>

#include <vtkstd/map>

/////////////////////////////////////////////////////////////////////////////
// pqServerStartups::pqImplementation

class pqServerStartups::pqImplementation
{
public:
  pqImplementation()
  {
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
  
  void deleteStartup(const pqServerResource& server)
  {
    if(this->Startups.count(server))
    {
    delete this->Startups[server];
    this->Startups.erase(server);
    }
  }
  
  typedef vtkstd::map<pqServerResource, pqServerStartup*> StartupsT;
  StartupsT Startups;
};

/////////////////////////////////////////////////////////////////////////////
// pqServerStartups

pqServerStartups::pqServerStartups() :
  Implementation(new pqImplementation())
{
  // Add some default startups ...
/*
  this->Startups.insert(vtkstd::make_pair(
    pqServerResource("cs://localhost"),
    new pqShellStartup("cmd /c start /b pvserver --server-port=%PV_SERVER_PORT% --use-offscreen-rendering")));
*/
}

pqServerStartups::~pqServerStartups()
{
  delete this->Implementation;
}

bool pqServerStartups::startupRequired(const pqServerResource& server)
{
  // Nothing has to be started if it's a builtin server ...
  if(server.scheme() == "builtin")
    {
    return false;
    }
  
  /** \todo Ideally, we should test here to see if the server is already running.
  If so, return false.  Currently, there's no way to "ping" the PV server.
  */
    
  return true;
}

bool pqServerStartups::startupAvailable(const pqServerResource& server)
{
  // Look for a startup that matches the requested scheme and host
  for(
    pqImplementation::StartupsT::iterator startup = this->Implementation->Startups.begin();
    startup != this->Implementation->Startups.end();
    ++startup)
    {
    const pqServerResource startup_server = startup->first;
    if(startup_server.schemeHosts() == server.schemeHosts())
      {
      return true;
      }
    }

  return false;
}

pqServerStartup* pqServerStartups::getStartup(const pqServerResource& server)
{
  // Look for a startup that matches the requested scheme and host
  for(
    pqImplementation::StartupsT::iterator startup = this->Implementation->Startups.begin();
    startup != this->Implementation->Startups.end();
    ++startup)
    {
    const pqServerResource startup_server = startup->first;
    if(startup_server.schemeHosts() == server.schemeHosts())
      {
      return startup->second;
      }
    }

  return 0;
}

const pqServerStartups::ServersT pqServerStartups::servers() const
{
  ServersT results;

  for(
    pqImplementation::StartupsT::iterator startup = this->Implementation->Startups.begin();
    startup != this->Implementation->Startups.end();
    ++startup)
    {
    results.push_back(startup->first);
    }
    
  return results;
}

void pqServerStartups::setManualStartup(const pqServerResource& server)
{
  this->Implementation->deleteStartup(server);
  this->Implementation->Startups.insert(vtkstd::make_pair(server.schemeHosts(), new pqManualServerStartup()));
  emit this->changed();
}

void pqServerStartups::setShellStartup(const pqServerResource& server, const QString& command_line, double delay)
{
  this->Implementation->deleteStartup(server);
  this->Implementation->Startups.insert(vtkstd::make_pair(server.schemeHosts(), new pqShellServerStartup(command_line, delay)));
  emit this->changed();
}

void pqServerStartups::deleteStartups(const ServersT& startups)
{
  for(ServersT::const_iterator startup = startups.begin(); startup != startups.end(); ++startup) 
    {
    this->Implementation->deleteStartup(*startup);
    }
    
  emit this->changed();
}

void pqServerStartups::load(pqSettings& settings)
{
  settings.beginGroup("ServerStartups");
  const QStringList startups = settings.childGroups();
  for(int i = 0; i != startups.size(); ++i)
    {
    const QString encoded_server = startups[i];
    const QString server_type = settings.value(encoded_server + "/type").toString();

    QString server = encoded_server;
    server.replace("|", "/");
    
    if(server_type == "manual")
      {
      this->setManualStartup(server);
      }
    else if(server_type == "shell")
      {
      const QString command_line = settings.value(encoded_server + "/command_line").toString();
      const double delay = settings.value(encoded_server + "/delay").toDouble();
      this->setShellStartup(server, command_line, delay);
      }
    }
  settings.endGroup();
}

void pqServerStartups::save(pqSettings& settings)
{
  settings.remove("ServerStartups");

  for(
    pqImplementation::StartupsT::iterator startup = this->Implementation->Startups.begin();
    startup != this->Implementation->Startups.end();
    ++startup)
    {
    const pqServerResource startup_server = startup->first;
    pqServerStartup* const startup_command = startup->second;

    QString encoded_startup_server = startup_server.toString();
    encoded_startup_server.replace("/", "|");
    
    const QString server_key = "ServerStartups/" + encoded_startup_server;
    if(dynamic_cast<pqManualServerStartup*>(startup_command))
      {
      settings.setValue(server_key + "/type", "manual");
      }
    else if(pqShellServerStartup* const shell_startup = dynamic_cast<pqShellServerStartup*>(startup_command))
      {
      settings.setValue(server_key + "/type", "shell");
      settings.setValue(server_key + "/command_line", shell_startup->CommandLine);
      settings.setValue(server_key + "/delay", shell_startup->Delay);
      }
    }
}
