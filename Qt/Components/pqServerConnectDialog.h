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
  * called to delete a server.
  */
  void deleteServer();

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

protected:
  void editConfiguration(const pqServerConfiguration&);

private:
  Q_DISABLE_COPY(pqServerConnectDialog)

  class pqInternals;
  pqInternals* Internals;
};

#endif
