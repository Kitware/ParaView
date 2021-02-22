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
#include "pqServerConfigurationImporter.h"

#include "pqEventDispatcher.h" // For blocking test events during configuration download

#include "vtkNew.h"
#include "vtkPVConfig.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"

#include <QMap>
#include <QPointer>
#include <QtDebug>
#include <QtNetwork>

#include <cassert>

class pqServerConfigurationImporter::pqInternals
{
public:
  QMap<QString, QUrl> SourceURLs;
  QList<pqServerConfigurationImporter::Item> Configurations;
  QPointer<QNetworkReply> ActiveReply;
  QNetworkAccessManager NetworkAccessManager;
  QString ActiveFetchedData;
  QString ActiveSourceName;
  bool AbortFetch;
  pqInternals()
    : AbortFetch(false)
  {
  }

  static const char* getOS()
  {
#if defined(_WIN32)
    return "win32";
#elif defined(__APPLE__)
    return "macos";
#else
    return "nix";
#endif
  }

  // for every url, the following locations are tried.
  // 1) ${url}
  // 2) ${url}/v{major_version}.{minor_version}/[win32|macos|nix]/servers.pvsc
  // 3) ${url}/v{major_version}.{minor_version}/[win32|macos|nix]/servers.xml
  // 4) ${url}/v{major_version}.{minor_version}/servers.pvsc
  // 5) ${url}/v{major_version}.{minor_version}/servers.xml
  // 6) ${url}/servers.pvsc
  // 7) ${url}/servers.xml
  static QList<QUrl> getAlternativeURLs(const QUrl& url)
  {
    QList<QUrl> urls;
    urls.append(url);

    QUrl new_url = url;

    new_url.setPath(url.path() +
      QString("/v%1_%2/%3/servers.pvsc")
        .arg(PARAVIEW_VERSION_MAJOR)
        .arg(PARAVIEW_VERSION_MINOR)
        .arg(getOS()));
    urls.append(new_url);

    new_url.setPath(url.path() +
      QString("/v%1_%2/%3/servers.xml")
        .arg(PARAVIEW_VERSION_MAJOR)
        .arg(PARAVIEW_VERSION_MINOR)
        .arg(getOS()));
    urls.append(new_url);

    new_url.setPath(url.path() +
      QString("/v%1_%2/servers.pvsc").arg(PARAVIEW_VERSION_MAJOR).arg(PARAVIEW_VERSION_MINOR));
    urls.append(new_url);

    new_url.setPath(url.path() +
      QString("/v%1_%2/servers.xml").arg(PARAVIEW_VERSION_MAJOR).arg(PARAVIEW_VERSION_MINOR));
    urls.append(new_url);

    new_url.setPath(url.path() + QString("/servers.pvsc"));
    urls.append(new_url);

    new_url.setPath(url.path() + QString("/servers.xml"));
    urls.append(new_url);
    return urls;
  }
};

//-----------------------------------------------------------------------------
pqServerConfigurationImporter::pqServerConfigurationImporter(QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqInternals())
{
  QObject::connect(&this->Internals->NetworkAccessManager,
    SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)), this,
    SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)));
}

//-----------------------------------------------------------------------------
pqServerConfigurationImporter::~pqServerConfigurationImporter()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
const QList<pqServerConfigurationImporter::Item>& pqServerConfigurationImporter::configurations()
  const
{
  return this->Internals->Configurations;
}

//-----------------------------------------------------------------------------
void pqServerConfigurationImporter::addSource(
  const QString& name, const QUrl& url, pqServerConfigurationImporter::SourceMode /*mode=PVSC*/)
{
  if (url.isValid())
  {
    this->Internals->SourceURLs[name] = url;
  }
  else
  {
    qWarning() << "Invalid url: " << url;
  }
}

//-----------------------------------------------------------------------------
void pqServerConfigurationImporter::clearSources()
{
  this->Internals->SourceURLs.clear();
}

//-----------------------------------------------------------------------------
void pqServerConfigurationImporter::fetchConfigurations()
{
  if (this->Internals->ActiveReply)
  {
    qWarning() << "fetchConfigurations() already is progress.";
    return;
  }

  this->Internals->Configurations.clear();
  this->Internals->AbortFetch = false;

  // Block test events until all configurations are downloaded.
  pqEventDispatcher::deferEventsIfBlocked(true);

  for (QMapIterator<QString, QUrl> iter(this->Internals->SourceURLs); iter.hasNext();)
  {
    // this is funny, but evidently, that's how QMapIterator it to be used.
    iter.next();

    QUrl url = iter.value();

    this->Internals->ActiveSourceName = iter.key();

    QList<QUrl> alternative_urls = pqInternals::getAlternativeURLs(url);
    foreach (const QUrl& cur_url, alternative_urls)
    {
      if (this->fetch(cur_url))
      {
        break;
      }
    }
    if (this->Internals->AbortFetch)
    {
      break;
    }
  }

  // Unblock test events
  pqEventDispatcher::deferEventsIfBlocked(false);

  Q_EMIT this->configurationsUpdated();
}

//-----------------------------------------------------------------------------
bool pqServerConfigurationImporter::fetch(const QUrl& url)
{
  if (this->Internals->AbortFetch)
  {
    return false;
  }

  QNetworkReply* reply = this->Internals->NetworkAccessManager.get(QNetworkRequest(url));
  this->Internals->ActiveReply = reply;

  this->Internals->ActiveFetchedData.clear();

  QEventLoop eventLoop;
  // setup so that the loop quits when network communication ends or
  // abortFetch() is called.
  QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
  QObject::connect(
    this, SIGNAL(abortFetchTriggered()), &eventLoop, SLOT(quit()), Qt::QueuedConnection);
  // setup to read fetched data.
  QObject::connect(reply, SIGNAL(readyRead()), this, SLOT(readCurrentData()));

  // start the event loop.
  eventLoop.exec();

  bool return_value = false;
  QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
  if (reply->error() != QNetworkReply::NoError)
  {
    Q_EMIT this->message(QString("Request failed: %1").arg(reply->errorString()));
  }
  else if (!redirectionTarget.isNull())
  {
    // handle re-direction
    return_value = this->fetch(url.resolved(redirectionTarget.toUrl()));
  }
  else if (!this->Internals->AbortFetch)
  {
    // we've successfully downloaded the file.
    return_value = this->processDownloadedContents();
  }

  // this will set ActiveReply to nullptr automatically.
  delete reply;
  reply = nullptr;
  return return_value;
}

//-----------------------------------------------------------------------------
void pqServerConfigurationImporter::abortFetch()
{
  if (this->Internals->ActiveReply)
  {
    this->Internals->AbortFetch = true;
    this->Internals->ActiveReply->abort();
    Q_EMIT this->abortFetchTriggered();
  }
}
//-----------------------------------------------------------------------------
void pqServerConfigurationImporter::readCurrentData()
{
  assert(this->Internals->ActiveReply != nullptr);
  this->Internals->ActiveFetchedData.append(this->Internals->ActiveReply->readAll());
}

//-----------------------------------------------------------------------------
bool pqServerConfigurationImporter::processDownloadedContents()
{
  vtkNew<vtkPVXMLParser> parser;
  if (!parser->Parse(this->Internals->ActiveFetchedData.toLocal8Bit().data()))
  {
    return false;
  }

  vtkPVXMLElement* root = parser->GetRootElement();
  if (QString(root->GetName()) != "Servers")
  {
    return false;
  }

  bool appended = false;
  // FIXME: We may want to add some version-number checking here.
  for (unsigned int cc = 0; cc < root->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* child = root->GetNestedElement(cc);
    if (child->GetName() && strcmp(child->GetName(), "Server") == 0)
    {
      pqServerConfiguration config(child);
      config.setMutable(true);
      Item item;
      item.Configuration = config;
      item.SourceName = this->Internals->ActiveSourceName;
      this->Internals->Configurations.append(item);
      appended = true;
    }
  }

  if (appended)
  {
    Q_EMIT this->incrementalUpdate();
  }
  return true;
}
