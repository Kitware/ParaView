/*=========================================================================

   Program: ParaView
   Module:    pqParaViewMenuBuilders.h

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
  static void buildSourcesMenu(QMenu& menu, QMainWindow* mainWindow = 0);

  /**
  * Builds "Filters" menu.
  * If you want to automatically add toolbars for filters as requested in the
  * configuration pass in a non-null main window.
  * If you do not want to add the actions from the filters menu to quick launch
  * maintained by pqApplicationCore (see pqPVApplicationCore::registerForQuicklaunch),
  * then pass quickLaunchable == false.
  */
  static void buildFiltersMenu(QMenu& menu, QMainWindow* mainWindow = 0, bool hideDisabled = false,
    bool quickLaunchable = true);

  /**
  * Builds the "Tools" menu.
  */
  static void buildToolsMenu(QMenu& menu);

  /**
  * Builds the "Catalyst" menu
  */
  static void buildCatalystMenu(QMenu& menu, QWidget* confpanel);

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
  static void buildPipelineBrowserContextMenu(QMenu& menu);

  /**
  * Builds and adds all standard ParaView toolbars.
  */
  static void buildToolbars(QMainWindow& mainWindow);
};

#endif
