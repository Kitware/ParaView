/*=========================================================================

   Program: ParaView
   Module:    pqCPWritersMenuManager.h

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
#ifndef __pqCPWritersMenuManager_h
#define __pqCPWritersMenuManager_h

#include <QObject>
#include <QTimer>

class QMenu;
class QAction;

/// pqCPWritersMenuManager is responsible for managing the menu for "Writers".
/// pqCPPluginManager calls createMenu() when the plugin is initialized, then
/// pqCPWritersMenuManager creates and setups up the co-processing writers menu.
/// If another plugin is loaded after this one is then it rechecks to see
/// if any writers were added with the CoProcessing hint in the XML file
/// and if they were then the writers get added to the Writers menu.  See
/// Resources/servermanager.xml for an example of how to do that.
class pqCPWritersMenuManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqCPWritersMenuManager(QObject* parent=0);
  ~pqCPWritersMenuManager();

public slots:
  /// Creates a new "Writers" menu and adds the co-processing writers to it.
  void createMenu();

protected slots:
  /// Updates the enable state for the writers menu.
  void updateEnableState();

  /// Called when user requests to create a writer.
  void onActionTriggered(QAction*);
protected:
  void createWriter(const QString& xmlgroup, const QString& xmlname);

private:
  QMenu* Menu;

private:
  pqCPWritersMenuManager(const pqCPWritersMenuManager&); // Not implemented.
  void operator=(const pqCPWritersMenuManager&); // Not implemented.

  QTimer Timer;
};

#endif


