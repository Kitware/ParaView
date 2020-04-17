/*=========================================================================

   Program: ParaView
   Module:    pqCategoryToolbarsBehavior.h

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
#ifndef pqCategoryToolbarsBehavior_h
#define pqCategoryToolbarsBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QList>
#include <QObject>
#include <QPointer>

class pqProxyGroupMenuManager;
class QMainWindow;
class QAction;

/**
* @ingroup Behaviors
* pqCategoryToolbarsBehavior is used when the application wants to enable
* categories from a pqProxyGroupMenuManager to show up in a toolbar.
* ex. One may want to have a toolbar listing all the filters in "Common"
* category. This behavior also ensures that as plugins are loaded, if new
* categories request that the be added as a toolbar, new toolbars for those
* are added and also if new items get added to a category already shown as a
* toolbar, then the toolbar is updated.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqCategoryToolbarsBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqCategoryToolbarsBehavior(pqProxyGroupMenuManager* menuManager, QMainWindow* mainWindow);

protected Q_SLOTS:
  /**
  * Called when menuManager fires the menuPopulated() signal.
  */
  void updateToolbars();

  /**
  * This slot gets attached to a pqEventDispatcher so that some toolbars
  * can be hidden before each test starts (to prevent small test-image differences
  * due to layout differences between machines).
  */
  void prepareForTest();

private:
  Q_DISABLE_COPY(pqCategoryToolbarsBehavior)

  QPointer<QMainWindow> MainWindow;
  QPointer<pqProxyGroupMenuManager> MenuManager;
  QList<QAction*> ToolbarsToHide;
};

#endif
