// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqServerConfigurationImporter_h
#define pqServerConfigurationImporter_h

#include "pqComponentsModule.h"
#include "pqServerConfiguration.h"
#include <QList>
#include <QObject>

class QAuthenticator;
class QNetworkReply;
class QUrl;

/**
 * pqServerConfigurationImporter class can be used to import remote server
 * configurations.
 */
class PQCOMPONENTS_EXPORT pqServerConfigurationImporter : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqServerConfigurationImporter(QObject* parent = nullptr);
  ~pqServerConfigurationImporter() override;

  enum SourceMode
  {
    PVSC
  };

  /**
   * Add source URL. Currently, we only support pvsc sources. In future we may
   * support other types of sources.
   * For every url added, the following locations are tried, in the specific
   * order:
   * \li ${url}
   * \li ${url}/v{major_version}.{minor_version}/[win32|macos|nix]/servers.pvsc
   * \li ${url}/v{major_version}.{minor_version}/[win32|macos|nix]/servers.xml
   * \li ${url}/v{major_version}.{minor_version}/servers.pvsc
   * \li ${url}/v{major_version}.{minor_version}/servers.xml
   * \li ${url}/servers.pvsc
   * \li ${url}/servers.xml
   * The search stops as soon as a url returns a valid servers configuration
   * file. Note all paths are case-sensitive.
   */
  void addSource(const QString& name, const QUrl& url, SourceMode mode = PVSC);

  /**
   * Remove all added sources.
   */
  void clearSources();

  class Item
  {
  public:
    /**
     * Points to the server configuration.
     */
    pqServerConfiguration Configuration;

    /**
     * Refers to the user-friendly name for the source defined in the
     * configurations file.
     */
    QString SourceName;
  };

  /**
   * Returns the fetched configurations. If fetchConfigurations() was aborted
   * using abortFetch(), then this list may be partially filled.
   * Also, this list include all server configurations fetched, without
   * checking if some of these configurations are already present in
   * pqServerConfigurationCollection maintained by pqApplicationCore.
   */
  const QList<Item>& configurations() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Use this method to fetch server configurations from urls specified.
   * This call blocks until all configurations are fetched. It uses a
   * QEventLoop internally to wait for the network communication. One can
   * abort the waiting by using abortFetch().
   * Refer to pqServerConnectDialog::fetchServers() for details on using
   * fetchConfigurations() and abortFetch().
   */
  void fetchConfigurations();

  /**
   * Aborts blocking in fetchConfigurations(). Has any effect only when
   * fetchConfigurations() has blocked.
   */
  void abortFetch();

Q_SIGNALS:
  /**
   * fired as source configurations are fetched. Makes it possible to build
   * interfaces that are populated as new configurations arrive, rather than
   * waiting for the entire fetchConfigurations() to complete.
   */
  void incrementalUpdate();

  /**
   * fired at end of fetchConfigurations() call.
   */
  void configurationsUpdated();

  /**
   * this signal is fired whenever the source-url requests authentication
   * before delivering the contents. Refer to
   * QNetworkAccessManager::authenticationRequired() for details.
   */
  void authenticationRequired(QNetworkReply*, QAuthenticator*);

  /**
   * fired when abortFetch() is called. An internal signal, not really useful
   * for external components.
   */
  void abortFetchTriggered();

  /**
   * this class posts miscellaneous information about the network communication
   * using this signal.
   */
  void message(const QString& message);

protected:
  /**
   * Called after data from each source is fetched fully. This will process the
   * fetched data and update the configurations list. May fire
   * incrementalUpdate() if configurations were updated.
   */
  bool processDownloadedContents();

  /**
   * Returns true if valid config was fetched and processed from the URL,
   * otherwise returns false.
   */
  bool fetch(const QUrl& url);

private Q_SLOTS:
  /**
   * called data is available on the network request.
   */
  void readCurrentData();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqServerConfigurationImporter)

  class pqInternals;
  pqInternals* Internals;
};

#endif
