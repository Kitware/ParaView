// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqServerResource.h"

#include "pqServerConfiguration.h"

#include <QDir>
#include <QPointer>
#include <QRegularExpression>
#include <QUrl>
#include <QtDebug>

//////////////////////////////////////////////////////////////////////////////
// pqServerResource::pqImplementation

class pqServerResource::pqImplementation
{
public:
  pqImplementation()
    : Port(-1)
    , DataServerPort(-1)
    , RenderServerPort(-1)
  {
  }

  pqImplementation(
    const QString& rhs, const pqServerConfiguration& config = pqServerConfiguration())
    : Port(-1)
    , DataServerPort(-1)
    , RenderServerPort(-1)
    , Configuration(config)
  {
    QStringList strings = rhs.split(";");

    QUrl temp(strings[0]);
    this->Scheme = temp.scheme();

    if (this->Scheme == "cdsrs" || this->Scheme == "cdsrsrc")
    {
      this->DataServerHost = temp.host();
      this->DataServerPort = temp.port();

      // we had conflicting documentation!!! The doc said that the render server
      // is separated by a "/" while the code looks for "//". Updating the
      // regular expressions so that both cases are handled.
      QRegularExpression render_server_host("//?([^:/]*)");
      QRegularExpressionMatch match = render_server_host.match(temp.path());
      if (match.hasMatch())
      {
        this->RenderServerHost = match.captured(1);
      }

      QRegularExpression render_server_port("//?[^:]*:([^/]*)");
      match = render_server_port.match(temp.path());
      if (match.hasMatch())
      {
        bool ok = false;
        const int port = match.captured(1).toInt(&ok);
        if (ok)
          this->RenderServerPort = port;
      }

      QRegularExpression path("(//[^/]*)(.*)");
      match = path.match(temp.path());
      if (match.hasMatch())
      {
        this->Path = match.captured(2);
      }
    }
    else
    {
      this->Host = temp.host();
      this->Port = temp.port();
      this->Path = temp.path();
    }
    this->ServerName = temp.fragment();

    // Use configuration name if provided
    if (this->ServerName.isEmpty() && !config.isNameDefault())
    {
      this->ServerName = config.name();
    }

    if (this->Path.size() > 2 && this->Path[0] == '/' && this->Path[2] == ':')
    {
      this->Path = this->Path.mid(1);
    }

    if (!this->Path.isEmpty())
    {
      // Reverse the ; and # replacement. This means filenames can't have "%3B" or "%23" in them,
      // but they can have "%", which wouldn't work with the more general
      // QUrl::fromPercentEncoding()
      this->Path.replace("%3B", ";").replace("%23", "#");
    }

    if (this->Path.size() > 1 && this->Path[1] == ':')
    {
      this->Path = QDir::toNativeSeparators(this->Path);
    }

    strings.removeFirst();
    Q_FOREACH (QString str, strings)
    {
      QStringList data = str.split(":");
      this->ExtraData[data[0]] = data[1];
    }
  }

  bool operator==(const pqImplementation& rhs)
  {
    return this->Scheme == rhs.Scheme && this->Host == rhs.Host && this->Port == rhs.Port &&
      this->DataServerHost == rhs.DataServerHost && this->DataServerPort == rhs.DataServerPort &&
      this->RenderServerHost == rhs.RenderServerHost &&
      this->RenderServerPort == rhs.RenderServerPort && this->Path == rhs.Path &&
      this->ServerName == rhs.ServerName;
  }

  bool operator!=(const pqImplementation& rhs) { return !(*this == rhs); }

  bool operator<(const pqImplementation& rhs)
  {
    if (this->Scheme != rhs.Scheme)
      return this->Scheme < rhs.Scheme;
    if (this->Host != rhs.Host)
      return this->Host < rhs.Host;
    if (this->Port != rhs.Port)
      return this->Port < rhs.Port;
    if (this->DataServerHost != rhs.DataServerHost)
      return this->DataServerHost < rhs.DataServerHost;
    if (this->DataServerPort != rhs.DataServerPort)
      return this->DataServerPort < rhs.DataServerPort;
    if (this->RenderServerHost != rhs.RenderServerHost)
      return this->RenderServerHost < rhs.RenderServerHost;
    if (this->RenderServerPort != rhs.RenderServerPort)
      return this->RenderServerPort < rhs.RenderServerPort;
    if (this->Path != rhs.Path)
      return this->Path < rhs.Path;
    return this->ServerName < rhs.ServerName;
  }

  QString Scheme;
  QString Host;
  int Port;
  QString DataServerHost;
  int DataServerPort;
  QString RenderServerHost;
  int RenderServerPort;
  QString Path;
  QString ServerName;
  QMap<QString, QString> ExtraData;
  pqServerConfiguration Configuration;
};

pqServerResource::pqServerResource()
  : Implementation(new pqImplementation())
{
}

pqServerResource::pqServerResource(const QString& rhs)
  : Implementation(new pqImplementation(rhs))
{
}

pqServerResource::pqServerResource(const QString& rhs, const pqServerConfiguration& config)
  : Implementation(new pqImplementation(rhs, config))
{
}

pqServerResource::pqServerResource(const pqServerResource& rhs)
  : Implementation(new pqImplementation(*rhs.Implementation))
{
}

pqServerResource& pqServerResource::operator=(const pqServerResource& rhs)
{
  if (this != &rhs)
  {
    *this->Implementation = *rhs.Implementation;
  }

  return *this;
}

pqServerResource::~pqServerResource()
{
  delete this->Implementation;
}

QString pqServerResource::toURI() const
{
  QString result;
  result += this->Implementation->Scheme + ":";

  if (this->Implementation->Scheme == "builtin")
  {
  }
  else if (this->Implementation->Scheme == "cs" || this->Implementation->Scheme == "csrc")
  {
    result += "//" + this->Implementation->Host;
    if (-1 != this->Implementation->Port)
    {
      result += ":" + QString::number(this->Implementation->Port);
    }
  }
  else if (this->Implementation->Scheme == "cdsrs" || this->Implementation->Scheme == "cdsrsrc")
  {
    result += "//" + this->Implementation->DataServerHost;
    if (-1 != this->Implementation->DataServerPort)
    {
      result += ":" + QString::number(this->Implementation->DataServerPort);
    }
    result += "//" + this->Implementation->RenderServerHost;
    if (-1 != this->Implementation->RenderServerPort)
    {
      result += ":" + QString::number(this->Implementation->RenderServerPort);
    }
  }

  if (!this->Implementation->Path.isEmpty())
  {
    if (this->Implementation->Path[0] != '/')
    {
      result += "/";
    }
    result += this->Implementation->Path;
    // Replace ; and # with percent encodings to avoid characters messing up the parsing later.
    result.replace(";", "%3B").replace("#", "%23");
  }

  if (!this->Implementation->ServerName.isEmpty())
  {
    result += "#" + this->Implementation->ServerName;
  }

  return result;
}

QString pqServerResource::scheme() const
{
  return this->Implementation->Scheme;
}

void pqServerResource::setScheme(const QString& rhs)
{
  this->Implementation->Scheme = rhs;
}

bool pqServerResource::isReverse() const
{
  return (this->Implementation->Scheme == "csrc" || this->Implementation->Scheme == "cdsrsrc");
}

QString pqServerResource::host() const
{
  if (this->Implementation->Scheme == "cdsrs" || this->Implementation->Scheme == "cdsrsrc")
  {
    return "";
  }

  return this->Implementation->Host;
}

void pqServerResource::setHost(const QString& rhs)
{
  if (this->Implementation->Scheme == "cdsrs" || this->Implementation->Scheme == "cdsrsrc")
  {
    return;
  }

  this->Implementation->Host = rhs;
}

int pqServerResource::port() const
{
  if (this->Implementation->Scheme == "cdsrs" || this->Implementation->Scheme == "cdsrsrc")
  {
    return -1;
  }

  return this->Implementation->Port;
}

int pqServerResource::port(int default_port) const
{
  if (this->Implementation->Scheme == "cdsrs" || this->Implementation->Scheme == "cdsrsrc")
  {
    return -1;
  }

  if (-1 == this->Implementation->Port)
  {
    return default_port;
  }

  return this->Implementation->Port;
}

void pqServerResource::setPort(int rhs)
{
  if (this->Implementation->Scheme == "cdsrs" || this->Implementation->Scheme == "cdsrsrc")
  {
    return;
  }

  this->Implementation->Port = rhs;
}

QString pqServerResource::dataServerHost() const
{
  if (this->Implementation->Scheme != "cdsrs" && this->Implementation->Scheme != "cdsrsrc")
  {
    return "";
  }

  return this->Implementation->DataServerHost;
}

void pqServerResource::setDataServerHost(const QString& rhs)
{
  if (this->Implementation->Scheme != "cdsrs" && this->Implementation->Scheme != "cdsrsrc")
  {
    return;
  }

  this->Implementation->DataServerHost = rhs;
}

int pqServerResource::dataServerPort() const
{
  if (this->Implementation->Scheme != "cdsrs" && this->Implementation->Scheme != "cdsrsrc")
  {
    return -1;
  }

  return this->Implementation->DataServerPort;
}

int pqServerResource::dataServerPort(int default_port) const
{
  if (this->Implementation->Scheme != "cdsrs" && this->Implementation->Scheme != "cdsrsrc")
  {
    return -1;
  }

  if (-1 == this->Implementation->DataServerPort)
  {
    return default_port;
  }

  return this->Implementation->DataServerPort;
}

void pqServerResource::setDataServerPort(int rhs)
{
  if (this->Implementation->Scheme != "cdsrs" && this->Implementation->Scheme != "cdsrsrc")
  {
    return;
  }

  this->Implementation->DataServerPort = rhs;
}

QString pqServerResource::renderServerHost() const
{
  if (this->Implementation->Scheme != "cdsrs" && this->Implementation->Scheme != "cdsrsrc")
  {
    return "";
  }

  return this->Implementation->RenderServerHost;
}

void pqServerResource::setRenderServerHost(const QString& rhs)
{
  if (this->Implementation->Scheme != "cdsrs" && this->Implementation->Scheme != "cdsrsrc")
  {
    return;
  }

  this->Implementation->RenderServerHost = rhs;
}

int pqServerResource::renderServerPort() const
{
  if (this->Implementation->Scheme != "cdsrs" && this->Implementation->Scheme != "cdsrsrc")
  {
    return -1;
  }

  return this->Implementation->RenderServerPort;
}

int pqServerResource::renderServerPort(int default_port) const
{
  if (this->Implementation->Scheme != "cdsrs" && this->Implementation->Scheme != "cdsrsrc")
  {
    return -1;
  }

  if (-1 == this->Implementation->RenderServerPort)
  {
    return default_port;
  }

  return this->Implementation->RenderServerPort;
}

void pqServerResource::setRenderServerPort(int rhs)
{
  if (this->Implementation->Scheme != "cdsrs" && this->Implementation->Scheme != "cdsrsrc")
  {
    return;
  }

  this->Implementation->RenderServerPort = rhs;
}

QString pqServerResource::path() const
{
  return this->Implementation->Path;
}

void pqServerResource::setPath(const QString& rhs)
{
  this->Implementation->Path = rhs;
}

QString pqServerResource::serverName() const
{
  return this->Implementation->ServerName;
}

void pqServerResource::setServerName(const QString& name)
{
  this->Implementation->ServerName = name;
}

pqServerResource pqServerResource::schemeHostsPorts() const
{
  pqServerResource result = this->schemeHosts();

  result.setPort(this->Implementation->Port);
  result.setDataServerPort(this->Implementation->DataServerPort);
  result.setRenderServerPort(this->Implementation->RenderServerPort);

  return result;
}

pqServerResource pqServerResource::schemeHosts() const
{
  pqServerResource result;

  result.setScheme(this->Implementation->Scheme);
  result.setHost(this->Implementation->Host);
  result.setDataServerHost(this->Implementation->DataServerHost);
  result.setRenderServerHost(this->Implementation->RenderServerHost);

  return result;
}

pqServerResource pqServerResource::hostPath() const
{
  pqServerResource result;

  result.setHost(this->Implementation->Host);
  result.setDataServerHost(this->Implementation->DataServerHost);
  result.setRenderServerHost(this->Implementation->RenderServerHost);
  result.setPath(this->Implementation->Path);

  return result;
}

pqServerResource pqServerResource::pathServerName() const
{
  pqServerResource result;

  result.setPath(this->Implementation->Path);
  result.setServerName(this->Implementation->ServerName);

  return result;
}

bool pqServerResource::operator==(const pqServerResource& rhs) const
{
  return *this->Implementation == *rhs.Implementation;
}

bool pqServerResource::operator!=(const pqServerResource& rhs) const
{
  return *this->Implementation != *rhs.Implementation;
}

bool pqServerResource::operator<(const pqServerResource& rhs) const
{
  return *this->Implementation < *rhs.Implementation;
}

void pqServerResource::addData(const QString& key, const QString& value)
{
  this->Implementation->ExtraData[key] = value;
}

QString pqServerResource::data(const QString& key) const
{
  return this->data(key, QString());
}

QString pqServerResource::data(const QString& key, const QString& default_value) const
{
  return this->Implementation->ExtraData.contains(key) ? this->Implementation->ExtraData[key]
                                                       : default_value;
}

bool pqServerResource::hasData(const QString& key) const
{
  return this->Implementation->ExtraData.contains(key);
}

QString pqServerResource::serializeString() const
{
  QString uri = this->toURI();
  QMap<QString, QString>::iterator iter;
  for (iter = this->Implementation->ExtraData.begin();
       iter != this->Implementation->ExtraData.end(); ++iter)
  {
    uri += QString(";%1:%2").arg(iter.key()).arg(iter.value());
  }
  return uri;
}

const pqServerConfiguration& pqServerResource::configuration() const
{
  return this->Implementation->Configuration;
}
