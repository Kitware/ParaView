/*=========================================================================

   Program: ParaView
   Module:    pqViewMenuManager.h

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
#ifndef pqViewMenuManager_h
#define pqViewMenuManager_h

#include "pqApplicationComponentsModule.h"
#include <QObject>
#include <QPointer>

class QMenu;
class QMainWindow;
class QAction;

/**
* pqViewMenuManager keeps ParaView View menu populated with all the available
* dock widgets and toolbars. This needs special handling since new dock
* widget/toolbars may get added when plugins are loaded.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqViewMenuManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqViewMenuManager(QMainWindow* mainWindow, QMenu* menu);

protected Q_SLOTS:
  /**
   * build the menu from scratch. Clears all existing items in the menu before
   * building it.
   */
  void buildMenu();

  /**
   * This is called to update items in the menu that are not static and may
   * change as a result of loading of plugins, for example viz. actions for
   * controlling visibilities of toolbars are dock panels.
   * It removes any actions for those currently present and adds actions for
   * toolbars and panels in the application. This slot is called when the menu
   * triggers `aboutToShow` signal.
   */
  virtual void updateMenu();

protected:
  QPointer<QMenu> Menu;
  QPointer<QMenu> ToolbarsMenu;
  QPointer<QAction> DockPanelSeparators[2];
  QPointer<QAction> ShowFrameDecorationsAction;

private:
  Q_DISABLE_COPY(pqViewMenuManager)
  QMainWindow* Window;
};

#endif
