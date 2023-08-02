// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqServerConnectDialog_h
#define pqServerConnectDialog_h

#include "pqComponentsModule.h"
#include "pqServerResource.h"
#include <QDialog>

#include <QList>
class pqServerConfiguration;
class pqServerResource;
class QAuthenticator;
class QListWidgetItem;
class QNetworkReply;

/**
 * pqServerConnectDialog is a dialog that can be used to show the user a
 * selection of server configurations to connect to a server. This dialog can
 * show all the server-configurations known to pqApplicationCore or show only a
 * subset of those depending on whether a \c selector was specified in the
 * constructor.
 *
 * On successful completion, this dialog provides access to the
 * server-configuration selected by the user. It does not make an real attempts
 * to connect to that server.
 */
class PQCOMPONENTS_EXPORT pqServerConnectDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  /**
   * If \c selector is specified, only those server-configurations that match the
   * selector's scheme and hostname are shown.
   */
  pqServerConnectDialog(
    QWidget* parent = nullptr, const pqServerResource& selector = pqServerResource());
  ~pqServerConnectDialog() override;

  /**
   * returns the configuration to connect to. This returns a valid response
   * only when pqServerConnectDialog::exec() returns QDialog::Accepted.
   */
  const pqServerConfiguration& configurationToConnect() const;

  /**
   * convenience method to determine which server to connect. If the selector
   * is specified and matches a single server, then the user is not presented
   * with a dialog, otherwise the user can choose the configuration to use to
   * connect to the server. When the method returns true,
   * selected_configuration will be set to the chosen configuration.
   */
  static bool selectServer(pqServerConfiguration& selected_configuration,
    QWidget* dialogParent = nullptr, const pqServerResource& selector = pqServerResource());

Q_SIGNALS:

  /**
   * Emitted when a server is added.
   */
  void serverAdded();

  /**
   * Emitted when a server is deleted.
   */
  void serverDeleted();

protected Q_SLOTS:
  /**
   * called to update the shown server configurations.
   */
  void updateConfigurations();

  /**
   * called when user selects a server.
   */
  void onServerSelected();

  /**
   * called when user clicks "edit-server"
   */
  void editServer();

  /**
   * called when user clicks "add-server"
   */
  void addServer();

  /**
   * Called when a server has been added or deleted.
   */
  void updateButtons();

  /**
   * called when user changes the server-type.
   */
  void updateServerType();

  /**
   * called to cancel .
   */
  void goToFirstPage();

  /**
   * called when user accepts the configuration page #1.
   */
  void acceptConfigurationPage1();

  /**
   * called when user accepts the configuration page #2.
   */
  void acceptConfigurationPage2();

  /**
   * called to proceed to page that allows the user to edit the startup.
   */
  void editServerStartup();

  // called when the "name" on the edit server page changes. We ensure that the
  // user cannot set a duplicate name.
  void onNameChanged();

  /**
   * called to delete one or all servers.
   */
  void deleteServer();
  void deleteAllServers();

  /**
   * called to load/save servers.
   */
  void loadServers();
  void saveServers();

  /**
   * called to connect to selected server.
   */
  void connect();

  /**
   * called when the main-stacked widget's active page changes. We update the
   * dialog;s title text to match the page being displayed.
   */
  void updateDialogTitle(int page_number);

  /**
   * called when user clicks "Fetch Servers".
   */
  void fetchServers();

  /**
   * called when the importer needs authentication from the user.
   */
  void authenticationRequired(QNetworkReply*, QAuthenticator*);

  /**
   * called update importable configs.
   */
  void updateImportableConfigurations();

  /**
   * called to report error from importer.
   */
  void importError(const QString& message);

  /**
   * called when user selects some servers to import.
   */
  void importServersSelectionChanged();

  /**
   * called to import selected configurations.
   */
  void importServers();

  /**
   * called to edit the sources url.
   */
  void editSources();

  void saveSourcesList();
  void cancelEditSources();

protected: // NOLINT(readability-redundant-access-specifiers)
  void editConfiguration(const pqServerConfiguration&);
  bool serverNameExists(const QString& name);

private:
  Q_DISABLE_COPY(pqServerConnectDialog)

  class pqInternals;
  pqInternals* Internals;
};

#endif
