/*=========================================================================

   Program: ParaView
   Module:    pqServerResource.cxx

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

#include "pqServerResource.h"

#include <QDir>
#include <QRegExp>
#include <QUrl>
#include <QtDebug>

//////////////////////////////////////////////////////////////////////////////
// pqServerResource::pqImplementation

class pqServerResource::pqImplementation
{
public:
  pqImplementation() :
    Port(-1),
    DataServerPort(-1),
    RenderServerPort(-1)
  {
  }
  
  pqImplementation(const QString& rhs) :
    Port(-1),
    DataServerPort(-1),
    RenderServerPort(-1)
  {
    QUrl temp(rhs);
    this->Scheme = temp.scheme();
    
    if(this->Scheme == "cdsrs" || this->Scheme == "cdsrsrc")
      {
      this->DataServerHost = temp.host();
      this->DataServerPort = temp.port();

      QRegExp render_server_host("//([^:/]*)");
      if(0 == render_server_host.indexIn(temp.path()))
        {
        this->RenderServerHost = render_server_host.cap(1);
        }

      QRegExp render_server_port("//[^:]*:([^/]*)");
      if(0 == render_server_port.indexIn(temp.path()))
        {
        bool ok = false;
        const int port = render_server_port.cap(1).toInt(&ok);
        if(ok)
          this->RenderServerPort = port;
        }

      QRegExp path("(//[^/]*)(.*)");
      if(0 == path.indexIn(temp.path()))
        {
        this->Path = path.cap(2);
        }
      }
    else
      {
      this->Host = temp.host();
      this->Port = temp.port();
      this->Path = temp.path();
      }

  if(this->Path.size() > 2 && this->Path[0] == '/' && this->Path[2] == ':')
    {
    this->Path = this->Path.mid(1);
    }
    
  if(this->Path.size() > 1 && this->Path[1] == ':')
    {
    this->Path = QDir::convertSeparators(this->Path);
    }
  
  }

  const bool operator==(const pqImplementation& rhs)
  {
    return this->Scheme == rhs.Scheme
      && this->Host == rhs.Host
      && this->Port == rhs.Port
      && this->DataServerHost == rhs.DataServerHost
      && this->DataServerPort == rhs.DataServerPort
      && this->RenderServerHost == rhs.RenderServerHost
      && this->RenderServerPort == rhs.RenderServerPort
      && this->Path == rhs.Path;
  }
  
  const bool operator!=(const pqImplementation& rhs)
  {
    return !(*this == rhs);
  }
  
  const bool operator<(const pqImplementation& rhs)
  {
    if(this->Scheme != rhs.Scheme)
      return this->Scheme < rhs.Scheme;
    if(this->Host != rhs.Host)
      return this->Host < rhs.Host;
    if(this->Port != rhs.Port)
      return this->Port < rhs.Port;
    if(this->DataServerHost != rhs.DataServerHost)
      return this->DataServerHost < rhs.DataServerHost;
    if(this->DataServerPort != rhs.DataServerPort)
      return this->DataServerPort < rhs.DataServerPort;
    if(this->RenderServerHost != rhs.RenderServerHost)
      return this->RenderServerHost < rhs.RenderServerHost;
    if(this->RenderServerPort != rhs.RenderServerPort)
      return this->RenderServerPort < rhs.RenderServerPort;

    return this->Path < rhs.Path;
  }

  QString Scheme;
  QString Host;
  int Port;
  QString DataServerHost;
  int DataServerPort;
  QString RenderServerHost;
  int RenderServerPort;
  QString Path;
};

pqServerResource::pqServerResource() :
  Implementation(new pqImplementation())
{
}

pqServerResource::pqServerResource(const QString& rhs) :
  Implementation(new pqImplementation(rhs))
{
}

pqServerResource::pqServerResource(const pqServerResource& rhs) :
  Implementation(new pqImplementation(*rhs.Implementation))
{
}

pqServerResource& pqServerResource::operator=(const pqServerResource& rhs)
{
  if(this != &rhs)
    {
    *this->Implementation = *rhs.Implementation;
    }
    
  return *this;
}

pqServerResource::~pqServerResource()
{
  delete this->Implementation;
}

const QString pqServerResource::toString() const
{
  QString result;
  
  result += this->Implementation->Scheme + ":";
  
  if(this->Implementation->Scheme == "builtin")
    {
    }
  else if(this->Implementation->Scheme == "cdsrs" ||
    this->Implementation->Scheme == "cdsrsrc")
    {
    result += "//" + this->Implementation->DataServerHost;
    if(-1 != this->Implementation->DataServerPort)
      {
      result += ":" + QString::number(this->Implementation->DataServerPort);
      }
    result += "//" + this->Implementation->RenderServerHost;
    if(-1 != this->Implementation->RenderServerPort)
      {
      result += ":" + QString::number(this->Implementation->RenderServerPort);
      }
    }
  else
    {
    result += "//" + this->Implementation->Host;
    if(-1 != this->Implementation->Port)
      {
      result += ":" + QString::number(this->Implementation->Port);
      }
    }
    
  if(!this->Implementation->Path.isEmpty())
    {
    if(this->Implementation->Path[0] != '/')
      {
      result += "/";
      }
      result += this->Implementation->Path;
    }
    
  return result;
}

const QString pqServerResource::scheme() const
{
  return this->Implementation->Scheme;
}

void pqServerResource::setScheme(const QString& rhs)
{
  this->Implementation->Scheme = rhs;
}

const QString pqServerResource::host() const
{
  if(this->Implementation->Scheme == "cdsrs" ||
    this->Implementation->Scheme == "cdsrsrc")
    {
    return "";
    }
    
  return this->Implementation->Host;
}

void pqServerResource::setHost(const QString& rhs)
{
  if(this->Implementation->Scheme == "cdsrs" ||
    this->Implementation->Scheme == "cdsrsrc")
    {
    return;
    }
    
  this->Implementation->Host = rhs;
}

const int pqServerResource::port() const
{
  if(this->Implementation->Scheme == "cdsrs" ||
    this->Implementation->Scheme == "cdsrsrc")
    {
    return -1;
    }
    
  return this->Implementation->Port;
}

const int pqServerResource::port(int default_port) const
{
  if(this->Implementation->Scheme == "cdsrs" ||
    this->Implementation->Scheme == "cdsrsrc")
    {
    return -1;
    }
  
  if(-1 == this->Implementation->Port)
    {
    return default_port;
    }
    
  return this->Implementation->Port;
}

void pqServerResource::setPort(int rhs)
{
  if(this->Implementation->Scheme == "cdsrs" ||
    this->Implementation->Scheme == "cdsrsrc")
    {
    return;
    }

  this->Implementation->Port = rhs;
}

const QString pqServerResource::dataServerHost() const
{
  if(this->Implementation->Scheme != "cdsrs" &&
    this->Implementation->Scheme != "cdsrsrc")
    {
    return "";
    }
    
  return this->Implementation->DataServerHost;
}

void pqServerResource::setDataServerHost(const QString& rhs)
{
  if(this->Implementation->Scheme != "cdsrs" &&
    this->Implementation->Scheme != "cdsrsrc")
    {
    return;
    }
    
  this->Implementation->DataServerHost = rhs;
}

const int pqServerResource::dataServerPort() const
{
  if(this->Implementation->Scheme != "cdsrs" &&
    this->Implementation->Scheme != "cdsrsrc")
    {
    return -1;
    }
  
  return this->Implementation->DataServerPort;
}

const int pqServerResource::dataServerPort(int default_port) const
{
  if(this->Implementation->Scheme != "cdsrs" &&
    this->Implementation->Scheme != "cdsrsrc")
    {
    return -1;
    }
  
  if(-1 == this->Implementation->DataServerPort)
    {
    return default_port;
    }
  
  return this->Implementation->DataServerPort;
}

void pqServerResource::setDataServerPort(int rhs)
{
  if(this->Implementation->Scheme != "cdsrs" &&
    this->Implementation->Scheme != "cdsrsrc")
    {
    return;
    }
  
  this->Implementation->DataServerPort = rhs;
}

const QString pqServerResource::renderServerHost() const
{
  if(this->Implementation->Scheme != "cdsrs" &&
    this->Implementation->Scheme != "cdsrsrc")
    {
    return "";
    }

  return this->Implementation->RenderServerHost;
}

void pqServerResource::setRenderServerHost(const QString& rhs)
{
  if(this->Implementation->Scheme != "cdsrs" &&
    this->Implementation->Scheme != "cdsrsrc")
    {
    return;
    }
    
  this->Implementation->RenderServerHost = rhs;
}

const int pqServerResource::renderServerPort() const
{
  if(this->Implementation->Scheme != "cdsrs" &&
    this->Implementation->Scheme != "cdsrsrc")
    {
    return -1;
    }

  return this->Implementation->RenderServerPort;
}

const int pqServerResource::renderServerPort(int default_port) const
{
  if(this->Implementation->Scheme != "cdsrs" &&
    this->Implementation->Scheme != "cdsrsrc")
    {
    return -1;
    }

  if(-1 == this->Implementation->RenderServerPort)
    {
    return default_port;
    }
    
  return this->Implementation->RenderServerPort;
}

void pqServerResource::setRenderServerPort(int rhs)
{
  if(this->Implementation->Scheme != "cdsrs" &&
    this->Implementation->Scheme != "cdsrsrc")
    {
    return;
    }
  
  this->Implementation->RenderServerPort = rhs;
}

const QString pqServerResource::path() const
{
  return this->Implementation->Path;
}

void pqServerResource::setPath(const QString& rhs)
{
  this->Implementation->Path = rhs;
}

const pqServerResource pqServerResource::server() const
{
  if(this->Implementation->Scheme == "builtin")
    {
    pqServerResource result;
    result.setScheme(this->Implementation->Scheme);
    return result;
    }
  else if(this->Implementation->Scheme == "cs" || this->Implementation->Scheme == "csrc")
    {
    pqServerResource result;
    result.setScheme(this->Implementation->Scheme);
    result.setHost(this->Implementation->Host);
    if(-1 != this->Implementation->Port)
      {
      result.setPort(this->Implementation->Port);
      }
    return result;
    }
  else if(this->Implementation->Scheme == "cdsrs" || this->Implementation->Scheme == "cdsrsrc")
    {
    pqServerResource result;
    result.setScheme(this->Implementation->Scheme);
    result.setDataServerHost(this->Implementation->DataServerHost);
    if(-1 != this->Implementation->DataServerPort)
      result.setDataServerPort(this->Implementation->DataServerPort);
    result.setRenderServerHost(this->Implementation->RenderServerHost);
    if(-1 != this->Implementation->RenderServerPort)
      result.setRenderServerPort(this->Implementation->RenderServerPort);
    return result;
    }
  else
    {
    qWarning() << "Unknown resource scheme: " << this->Implementation->Scheme;
    }
    
  return pqServerResource();
}

const bool pqServerResource::operator==(const pqServerResource& rhs) const
{
  return *this->Implementation == *rhs.Implementation;
}

const bool pqServerResource::operator!=(const pqServerResource& rhs) const
{
  return *this->Implementation != *rhs.Implementation;
}

const bool pqServerResource::operator<(const pqServerResource& rhs) const
{
  return *this->Implementation < *rhs.Implementation;
}
