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
#ifndef __pqServerLauncher_h 
#define __pqServerLauncher_h

#include <QObject>
#include <QProcess> // needed for QProcess::ProcessError.
#include "pqComponentsExport.h"

class pqServerConfiguration;
class pqServer;

/// pqServerLauncher manages launching of server process using the details
/// specified in the server configuration.
/// pqServerConfiguration can be simple, i.e the user is expected to launch the
/// pvserver, or complex, i.e. the user is prompted for several options a
/// pvserver process is launched automatically. All this is handled by this
/// class.
///
/// When launching processes or during reverse-connect, this class also shows
/// message box that can be used by the user to abort the waiting for
/// connection.
class PQCOMPONENTS_EXPORT pqServerLauncher : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqServerLauncher(
    const pqServerConfiguration& configuration,
    QObject* parent=0);
  virtual ~pqServerLauncher();

  /// This method will launch the server process based on the configuration and
  /// connect to the server. Returns true if the connection was successful,
  /// otherwise returns false.
  bool connectToServer();

  /// on successful call to connectToServer() this method can be used to obtain
  /// pqServer connection.
  pqServer* connectedServer() const;

protected slots:
  void processFailed(QProcess::ProcessError);
  void readStandardOutput();
  void readStandardError();
  void launchServerForReverseConnection();

protected:
  /// Request the user for user-configurable options, if any. Returns false if
  /// the user cancelled the dialog asking the options. Returns true if there
  /// are not user-configurable options or the user has accepted the values.
  bool promptOptions();

  bool launchServer(bool show_status_dialog);

  bool connectToPrelaunchedServer();

  bool isReverseConnection() const;

private:
  Q_DISABLE_COPY(pqServerLauncher)

  class pqInternals;
  pqInternals* Internals;
};

#endif
