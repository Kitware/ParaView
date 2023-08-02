// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqParaViewMenuBuilders_h
#define pqParaViewMenuBuilders_h

#include "pqApplicationComponentsModule.h"

class QMenu;
class QWidget;
class QMainWindow;

class pqPropertiesPanel;

/**
 * pqParaViewMenuBuilders provides helper methods to build menus that are
 * exactly as used by ParaView client. Simply call the appropriate method with
 * the menu as an argument, and it will be populated with actions and reactions
 * for standard ParaView behavior.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqParaViewMenuBuilders
{
public:
  /**
   * Builds standard File menu.
   */
  static void buildFileMenu(QMenu& menu);

  /**
   * Builds the standard Edit menu.
   */
  static void buildEditMenu(QMenu& menu, pqPropertiesPanel* propertiesPanel = nullptr);

  /**
   * Builds "Sources" menu.
   * If you want to automatically add toolbars for sources as requested in the
   * configuration pass in a non-null main window.
   */
  static void buildSourcesMenu(QMenu& menu, QMainWindow* mainWindow = nullptr);

  /**
   * Builds "Filters" menu.
   * If you want to automatically add toolbars for filters as requested in the
   * configuration pass in a non-null main window.
   * If you do not want to add the actions from the filters menu to quick launch
   * maintained by pqApplicationCore (see pqPVApplicationCore::registerForQuicklaunch),
   * then pass quickLaunchable == false.
   */
  static void buildFiltersMenu(QMenu& menu, QMainWindow* mainWindow = nullptr,
    bool hideDisabled = false, bool quickLaunchable = true);

  /**
   * Builds "Extractors" menu.
   */
  static void buildExtractorsMenu(QMenu& menu, QMainWindow* mainWindow = nullptr,
    bool hideDisabled = false, bool quickLaunchable = true);

  /**
   * Builds the "Tools" menu.
   */
  static void buildToolsMenu(QMenu& menu);

  /**
   * Builds the "Catalyst" menu
   */
  static void buildCatalystMenu(QMenu& menu);

  /**
   * Builds the "View" menu.
   */
  static void buildViewMenu(QMenu& menu, QMainWindow& window);

  /**
   * Builds the "Macros" menu. This menu is automatically hidden is python
   * support is not enabled.
   */
  static void buildMacrosMenu(QMenu& menu);

  /**
   * Builds the help menu.
   */
  static void buildHelpMenu(QMenu& menu);

  /**
   * Builds the context menu shown over the pipeline browser for some common
   * pipeline operations.
   */
  static void buildPipelineBrowserContextMenu(QMenu& menu, QMainWindow* window = nullptr);

  /**
   * Builds and adds all standard ParaView toolbars.
   */
  static void buildToolbars(QMainWindow& mainWindow);
};

#endif
