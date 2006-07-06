/*=========================================================================

   Program: ParaView
   Module:    pqViewMenu.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqViewMenu.h
/// \date 7/3/2006

#ifndef _pqViewMenu_h
#define _pqViewMenu_h


#include "pqWidgetsExport.h"
#include <QObject>

class pqViewMenuInternal;
class QAction;
class QDockWidget;
class QEvent;
class QIcon;
class QMenu;
class QMenuBar;
class QString;
class QToolBar;


/// \class pqViewMenu
/// \brief
///   The pqViewMenu class encapsulates the functionality in the view
///   menu.
class PQWIDGETS_EXPORT pqViewMenu : public QObject
{
  Q_OBJECT

public:
  pqViewMenu(QObject *parent=0);
  virtual ~pqViewMenu();

  /// \brief
  ///   Used to watch for show and hide events.
  ///
  /// The show and hide events for dock window and toolbars are used
  /// to update the action associated with them in the view menu.
  ///
  /// \param watched The dock window or toolbar receiving the event.
  /// \param e The event that is about to be sent.
  /// \return
  ///   True if the event should be filtered out.
  virtual bool eventFilter(QObject* watched, QEvent* e);

  /// \brief
  ///   Creates a new view menu with all the actions.
  /// \param menubar The menu bar to add the menu to.
  /// \sa pqViewMenu::addActionsToMenu(QMenu *)
  void addActionsToMenuBar(QMenuBar *menubar) const;

  /// \brief
  ///   Adds all the actions to the given menu.
  ///
  /// This method should only be called once by the application. The
  /// menu pointer is saved in order to add new dock window and toolbar
  /// visibility actions.
  ///
  /// \param menu The menu to add the actions to.
  void addActionsToMenu(QMenu *menu) const;

  /// \brief
  ///   Gets the menu action associated with the dock window.
  /// \param dock The dock window to look up.
  /// \return
  ///   A pointer to the action associated with the dock window.
  QAction *getMenuAction(QDockWidget *dock) const;

  /// \brief
  ///   Gets the menu action associated with the toolbar.
  /// \param tool The toolbar to look up.
  /// \return
  ///   A pointer to the action associated with the toolbar.
  QAction *getMenuAction(QToolBar *tool) const;

  /// \brief
  ///   Adds a menu item for the dock window to the view menu.
  ///
  /// Pass in a null icon if there is no icon for the dock window. The
  /// text can be the window title for the dock window. The text can
  /// also include a menu shortcut key.
  ///
  /// \param dock The dock window to add.
  /// \param icon An icon to display in the menu.
  /// \param text The text to display in the menu.
  void addDockWindow(QDockWidget *dock, const QIcon &icon,
      const QString &text);

  /// \brief
  ///   Removes the dock window from the view menu.
  /// \param dock The dock window to remove.
  void removeDockWindow(QDockWidget *dock);

  /// \brief
  ///   Adds a menu item for the toolbar to the view menu.
  ///
  /// The text can be the window title for the toolbar. The text can
  /// also include a menu shortcut key.
  ///
  /// \param tool The toolbar to add.
  /// \param text The text to display in the menu.
  void addToolBar(QToolBar *tool, const QString &text);

  /// \brief
  ///   Removes the toolbar from the view menu.
  /// \param tool The toolbar to remove.
  void removeToolBar(QToolBar *tool);

private:
  pqViewMenuInternal *Internal; ///< Stores the dock windows and tool bars.
};

#endif
