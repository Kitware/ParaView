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
#ifndef pqServerLauncher_h
#define pqServerLauncher_h

#include "pqComponentsModule.h"
#include <QObject>
#include <QProcess> // needed for QProcess::ProcessError.

class pqServer;
class pqServerConfiguration;
class QDialog;
class QProcessEnvironment;
class vtkObject;

/**
* pqServerLauncher manages launching of server process using the details
* specified in the server configuration.
* pqServerConfiguration can be simple, i.e the user is expected to launch the
* pvserver, or complex, i.e. the user is prompted for several options a
* pvserver process is launched automatically. All this is handled by this
* class.
*
* When launching processes or during reverse-connect, this class also shows
* message box that can be used by the user to abort the waiting for
* connection.
*/
class PQCOMPONENTS_EXPORT pqServerLauncher : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqServerLauncher(const pqServerConfiguration& configuration, QObject* parent = 0);
  ~pqServerLauncher() override;

  /**
  * Custom applications may want to extend the pqServerLauncher to customize
  * connecting to servers. Such application can set the QMetaObject to use
  * using this method. If none is set, pqServerLauncher will be created, by
  * default.
  * Returns the previous QMetaObject instance set, if any.
  */
  static const QMetaObject* setServerDefaultLauncherType(const QMetaObject*);
  static const QMetaObject* defaultServerLauncherType();

  /**
  * Creates a new instance using the default launcher type specified. If none
  * is specified, pqServerLauncher is created.
  */
  static pqServerLauncher* newInstance(
    const pqServerConfiguration& configuration, QObject* parent = NULL);

  /**
  * This method will launch the server process based on the configuration and
  * connect to the server. Returns true if the connection was successful,
  * otherwise returns false.
  */
  bool connectToServer();

  /**
  * on successful call to connectToServer() this method can be used to obtain
  * pqServer connection.
  */
  pqServer* connectedServer() const;

protected Q_SLOTS:
  void processFailed(QProcess::ProcessError);
  void readStandardOutput();
  void readStandardError();
  void launchServerForReverseConnection();

protected:
  /**
  * Request the user for user-configurable options, if any. Returns false if
  * the user cancelled the dialog asking the options. Returns true if there
  * are not user-configurable options or the user has accepted the values.
  */
  bool promptOptions();

  /**
  * Called when starting a server processes using a command-startup.
  * Returns true if launch was successful else returns false.
  */
  virtual bool launchServer(bool show_status_dialog);

  /**
  * An utility method to execute a command using a QProcess
  */
  bool processCommand(
    QString command, double timeout, double delay, const QProcessEnvironment* options = NULL);

  virtual bool connectToPrelaunchedServer();

  bool isReverseConnection() const;

  /**
  * Subclasses can override this method to further customize the dialog being
  * shown to the user to prompt for options in
  * pqServerLauncher::promptOptions.
  */
  virtual void prepareDialogForPromptOptions(QDialog&) {}

  /**
  * Provides access to the pqServerConfiguration passed to the constructor.
  * Note this is clone of the pqServerConfiguration passed to the constructor and
  * not the same instance.
  */
  pqServerConfiguration& configuration() const;

  /**
  * Provide access to the QProcessEnvironment.
  */
  QProcessEnvironment& options() const;

  /**
  * Use this method to update the process environment using current user selections.
  */
  virtual void updateOptionsUsingUserSelections();

  /**
  * Subclasses can override this to handle output and error messages from the
  * QProcess launched for command-startup configurations. Default
  * implementation simply dumps the text to the debug/error console.
  */
  virtual void handleProcessStandardOutput(const QByteArray& data);
  virtual void handleProcessErrorOutput(const QByteArray& data);

private:
  Q_DISABLE_COPY(pqServerLauncher)

  class pqInternals;
  pqInternals* Internals;
  static const QMetaObject* DefaultServerLauncherType;
  bool WrongConnectId;
};

#endif
