/*=========================================================================

   Program: ParaView
   Module:    pqServerBrowser.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#ifndef _pqServerBrowser_h
#define _pqServerBrowser_h

#include "pqComponentsExport.h"

#include <QDialog>

class pqServerStartups;
class pqServerStartup;
class pqSettings;
class QListWidgetItem;

/**
Provides a user-interface component for browsing through the set of
configured servers for editing and/or choosing connections.

To use, create an instance of pqServerBrowser, and connect its
serverSelected() signal to a slot.  If the user selects a server,
the slot will be called with information sufficient to start
the associated server.

You may want to use the pqServerStartupBrowser dialog, which
does all of the above, and starts the server for you.

\sa pqServer, pqServerStartupBrowser.
*/

class PQCOMPONENTS_EXPORT pqServerBrowser :
  public QDialog
{
  typedef QDialog Superclass;

  Q_OBJECT

public:
  pqServerBrowser(pqServerStartups& startups, QWidget* parent = 0);
  ~pqServerBrowser();

  /// Sets a message to be displayed to the user
  void setMessage(const QString& message);

  /// Returns the startup selected by the user, or NULL
  pqServerStartup* getSelectedServer();

  /// Set the names of startups that form the ignore list.
  /// All startups in the ignore list are not
  /// shown in the browser.
  void setIgnoreList(const QStringList& startupName);

signals:
  /// This signal will be emitted if the user picks a server to be started.
  void serverSelected(pqServerStartup&);

private slots:
  void onStartupsChanged();
  void onCurrentItemChanged(QListWidgetItem*, QListWidgetItem*);
  void onItemDoubleClicked(QListWidgetItem*);
  
  void onAddServer();
  void onEditServer();
  void onDeleteServer();
  void onSave();
  void onSave(const QStringList&);
  void onLoad();
  void onLoad(const QStringList&);
  
  void onConnect();
  void onClose();

private:
  pqServerBrowser(const pqServerBrowser&);
  pqServerBrowser& operator=(const pqServerBrowser&);

  void emitServerSelected(pqServerStartup&);
  virtual void onServerSelected(pqServerStartup&);

  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqServerBrowser_h
