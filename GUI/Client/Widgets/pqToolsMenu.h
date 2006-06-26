/*=========================================================================

   Program: ParaView
   Module:    pqToolsMenu.h

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

/// \file pqToolsMenu.h
/// \date 6/23/2006

#ifndef _pqToolsMenu_h
#define _pqToolsMenu_h


#include "pqWidgetsExport.h"
#include <QObject>

class pqBundleManager;
class pqBundleManagerModel;
class QAction;
class QMenu;
class QMenuBar;
class QStringList;


/// \class pqToolsMenu
/// \brief
///   The pqToolsMenu class encapsulates the functionality in the
///   tools menu.
///
/// The actions and their associated slots are contained in the class.
/// The pqToolsMenu can be used to create the default ParaView tools
/// menu. The actions can also be added to a custom menu using the
/// \c getMenuAction method.
class PQWIDGETS_EXPORT pqToolsMenu : public QObject
{
  Q_OBJECT

public:
  enum ActionName
    {
    InvalidAction = -1,
    CreateBundle = 0,
    ManageBundles,
    LinkEditor,
    DumpNames,
    RecordTest,
    TestScreenshot,
    PlayTest,
    PythonShell,
    Options,
    LastAction = Options
    };

public:
  pqToolsMenu(QObject *parent=0);
  virtual ~pqToolsMenu();

  /// \brief
  ///   Creates a new tools menu with all the actions.
  /// \param menubar The menu bar to add the menu to.
  void addActionsToMenuBar(QMenuBar *menubar) const;

  /// \brief
  ///   Adds all the actions to the given menu.
  /// \param menu The menu to add the actions to.
  void addActionsToMenu(QMenu *menu) const;

  /// \brief
  ///   Gets an action from the menu.
  /// \param name The name of the action to get.
  /// \return
  ///   A pointer to the requested menu action.
  QAction *getMenuAction(ActionName name) const;

public slots:
  /// Opens the bundle definition wizard.
  void openBundleWizard();

  /// Opens the bundle definition manager.
  void openBundleManager();

  /// Opens the link editor.
  void openLinkEditor();

  /// \name Testing Methods
  //@{
  void dumpWidgetNames();
  void recordTest();
  void recordTest(const QStringList &fileNames);
  void recordTestScreenshot();
  void recordTestScreenshot(const QStringList &fileNames);
  void playTest();
  void playTest(const QStringList &fileNames);
  //@}

  /// Opens the Python shell.
  void openPythonShell();

  /// Opens the options dialog.
  void openOptionsDialog();

private:
  pqBundleManager *BundleManager; ///< Bundle manager dialog.
  pqBundleManagerModel *Bundles;  ///< Keeps track of registered bundles.
  QAction **MenuList;             ///< Stores the list of menu actions.
};

#endif
