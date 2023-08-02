// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqServerConfigurationImporter.h"

#include "pqEventDispatcher.h" // For blocking test events during configuration download

#include "vtkNew.h"
#include "vtkPVVersion.h"
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
    Q_FOREACH (const QUrl& cur_url, alternative_urls)
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
  if (!parser->Parse(this->Internals->ActiveFetchedData.toUtf8().data()))
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
