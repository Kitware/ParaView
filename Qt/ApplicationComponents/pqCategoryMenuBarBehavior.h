// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCategoryMenuBarBehavior_h
#define pqCategoryMenuBarBehavior_h

#include "pqApplicationComponentsModule.h"

#include <QObject>

#include <memory> // for std::unique_ptr

class pqProxyGroupMenuManager;
class QMainWindow;

/**
 * @ingroup Behaviors
 * pqCategoryMenuBarBehavior shows the top-level categories of a
 * pqProxyGroupMenuManager marked with `show_in_menubar` as menus of the menu
 * bar, inserted before the "Help" menu, if any. This lets plugins add menus
 * gathering their tools, e.g.:
 *
 * ```xml
 * <ParaViewFilters>
 *   <Category name="MyTools" menu_label="My Tools" show_in_menubar="1" preserve_order="1">
 *     <Proxy group="sources" name="SphereSource" />
 *     <Proxy group="filters" name="Cut" />
 *     <Separator />
 *     <Script name="Hello" label="Say Hello">print("Hello!")</Script>
 *   </Category>
 * </ParaViewFilters>
 * ```
 *
 * Menus list the items of the category followed by sub-menus for its
 * sub-categories. Menus are updated when categories change, e.g. when plugins
 * are loaded.
 *
 * If the menu bar already has a menu with the title of the category, ignoring
 * mnemonics, e.g. "View", the items are added at the end of this menu, after a
 * separator, instead of creating another menu with the same title. This is
 * also how categories of sources and of filters with the same title share a
 * menu.
 *
 * The categories are no longer shown in the menu of the pqProxyGroupMenuManager
 * (see pqProxyGroupMenuManager::setShowCategoriesInMenuBar()), unless the main
 * window has no menu bar, which is not created.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCategoryMenuBarBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqCategoryMenuBarBehavior(pqProxyGroupMenuManager* menuManager, QMainWindow* mainWindow);
  ~pqCategoryMenuBarBehavior() override;

protected Q_SLOTS:
  /**
   * Create, update and delete menus.
   */
  void updateMenus();

private:
  Q_DISABLE_COPY(pqCategoryMenuBarBehavior)
  class pqInternal;
  std::unique_ptr<pqInternal> Internal;
};

#endif
