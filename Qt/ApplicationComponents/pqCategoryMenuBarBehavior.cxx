// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCategoryMenuBarBehavior.h"

#include "pqProxyAction.h"
#include "pqProxyCategory.h"
#include "pqProxyGroupMenuManager.h"

#include <QAction>
#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QPointer>

#include <cassert>

namespace
{
// marks the menus created for categories, deleted once empty.
const char* CATEGORY_MENU_PROPERTY = "pqCategoryMenuBarBehavior_CategoryMenu";
// marks the separators added around the items of categories.
const char* CATEGORY_SEPARATOR_PROPERTY = "pqCategoryMenuBarBehavior_CategorySeparator";

QString withoutMnemonics(QString text)
{
  return text.remove('&');
}

QAction* findHelpMenuAction(QMenuBar* menubar)
{
  for (QAction* action : menubar->actions())
  {
    if (::withoutMnemonics(action->text()).toLower() == "help")
    {
      return action;
    }
  }
  return nullptr;
}

/**
 * Returns the "Exit" item of the menu, if any, that items added to existing
 * menus are inserted before, as pqPluginActionGroupBehavior does.
 */
QAction* findExitAction(QMenu* menu)
{
  for (QAction* action : menu->actions())
  {
    const QString text = ::withoutMnemonics(action->text()).toLower();
    if (text == "exit" || text == "quit")
    {
      return action;
    }
  }
  return nullptr;
}

/**
 * Returns the menu of the menu bar with the given title, ignoring mnemonics.
 */
QMenu* findMenu(QMenuBar* menubar, const QString& title)
{
  for (QAction* action : menubar->actions())
  {
    if (action->menu() && ::withoutMnemonics(action->text()) == ::withoutMnemonics(title))
    {
      return action->menu();
    }
  }
  return nullptr;
}

/**
 * Deletes the menu once control returns to the event loop, as it may be the one
 * being used, e.g. when one of its items loads a plugin. Its name is cleared so
 * that it is not found in the meantime, e.g. by tests.
 */
void deleteMenuLater(QMenu* menu)
{
  menu->setObjectName(QString());
  menu->deleteLater();
}

/**
 * The menu showing a category, and the items added to it.
 */
struct pqCategoryMenu
{
  QPointer<QMenu> Menu;
  // whether the menu was created for the category, by this behavior.
  bool Created = false;
  // the items of the category, including separators and sub-menus.
  QList<QPointer<QAction>> Actions;
  // the sub-menus, and separators around the items, created for the category.
  QList<QPointer<QObject>> Owned;
  // updates the enabled state of the items when the menu is shown.
  QMetaObject::Connection UpdateConnection;

  /**
   * Removes the items from the menu. Returns true if some were still part of
   * the menu, in which case `next` is set to the item following them, if any.
   */
  bool removeItems(QAction*& next)
  {
    int last = -1;
    if (this->Menu)
    {
      const QList<QAction*> menuActions = this->Menu->actions();
      for (const auto& action : this->Actions)
      {
        last = qMax(last, static_cast<int>(menuActions.indexOf(action.data())));
      }
      next = last >= 0 ? menuActions.value(last + 1, nullptr) : nullptr;
      for (const auto& action : this->Actions)
      {
        if (action)
        {
          this->Menu->removeAction(action);
        }
      }
    }
    for (const auto& object : this->Owned)
    {
      if (auto subMenu = qobject_cast<QMenu*>(object))
      {
        ::deleteMenuLater(subMenu);
      }
      else if (object)
      {
        object->deleteLater();
      }
    }
    QObject::disconnect(this->UpdateConnection);
    this->Actions.clear();
    this->Owned.clear();
    return last >= 0;
  }

  /**
   * Removes the items from the menu, and the menu from the menu bar if it was
   * created for a category and is now empty.
   */
  void release()
  {
    QAction* next = nullptr;
    this->removeItems(next);
    // the separator preceding the items of another category is no longer
    // needed if they are now the first ones of the menu.
    QAction* first = this->Menu ? this->Menu->actions().value(0, nullptr) : nullptr;
    if (first && first->property(CATEGORY_SEPARATOR_PROPERTY).toBool())
    {
      this->Menu->removeAction(first);
      first->deleteLater();
    }
    if (this->Menu && this->Menu->property(CATEGORY_MENU_PROPERTY).toBool() &&
      this->Menu->actions().isEmpty())
    {
      if (auto parentWidget = this->Menu->parentWidget())
      {
        parentWidget->removeAction(this->Menu->menuAction());
      }
      ::deleteMenuLater(this->Menu);
    }
    this->Menu = nullptr;
    this->Created = false;
  }
};
}

//-----------------------------------------------------------------------------
class pqCategoryMenuBarBehavior::pqInternal
{
public:
  QPointer<QMainWindow> MainWindow;
  QPointer<pqProxyGroupMenuManager> MenuManager;
  // the menus showing categories, by category name.
  QMap<QString, pqCategoryMenu> Menus;

  /**
   * Adds the items of the category, followed by sub-menus for its
   * sub-categories, to the menu of `entry`, before the `before` item if any.
   * Items added among the ones of an existing menu are enclosed in separators.
   */
  void addItems(pqCategoryMenu& entry, pqProxyCategory* category, QAction* before, QObject* context)
  {
    QMenu* menu = entry.Menu;
    QList<QAction*> actions = this->MenuManager->categoryActions(category);
    for (auto subCategory : category->getCategoriesAlphabetically())
    {
      if (subCategory->isEmpty())
      {
        continue;
      }
      auto subMenu = new QMenu(subCategory->label(), menu);
      subMenu->setObjectName(subCategory->name());
      subMenu->menuAction()->setMenuRole(QAction::NoRole);
      this->MenuManager->populateMenuFromCategory(subMenu, subCategory);
      actions << subMenu->menuAction();
      entry.Owned << subMenu;
    }

    const QList<QAction*> menuActions = menu->actions();
    const int index = before ? menuActions.indexOf(before) : menuActions.size();
    QAction* previous = index > 0 ? menuActions[index - 1] : nullptr;
    auto insertSeparator = [&]()
    {
      QAction* separator = menu->insertSeparator(before);
      separator->setProperty(CATEGORY_SEPARATOR_PROPERTY, true);
      entry.Actions << separator;
      entry.Owned << separator;
    };
    if (previous && !previous->isSeparator())
    {
      insertSeparator();
    }
    menu->insertActions(before, actions);
    for (QAction* action : actions)
    {
      entry.Actions << action;
    }
    if (before && !before->isSeparator())
    {
      insertSeparator();
    }

    // like the Filters menu, update the enabled state of filters when shown.
    const QString name = category->name();
    entry.UpdateConnection = QObject::connect(menu, &QMenu::aboutToShow, context,
      [this, name]()
      {
        QList<QAction*> items;
        for (const auto& action : this->Menus.value(name).Actions)
        {
          if (action)
          {
            items << action;
          }
        }
        pqProxyAction::updateActionsState(items);
      });
  }
};

//-----------------------------------------------------------------------------
pqCategoryMenuBarBehavior::pqCategoryMenuBarBehavior(
  pqProxyGroupMenuManager* menuManager, QMainWindow* mainWindow)
  : Superclass(menuManager)
  , Internal(new pqInternal())
{
  assert(menuManager);
  assert(mainWindow);
  this->Internal->MainWindow = mainWindow;
  this->Internal->MenuManager = menuManager;

  QObject::connect(menuManager, &pqProxyGroupMenuManager::menuPopulated, this,
    &pqCategoryMenuBarBehavior::updateMenus);
  QObject::connect(menuManager, &pqProxyGroupMenuManager::categoriesUpdated, this,
    &pqCategoryMenuBarBehavior::updateMenus);
  this->updateMenus();
}

//-----------------------------------------------------------------------------
pqCategoryMenuBarBehavior::~pqCategoryMenuBarBehavior()
{
  // the categories are shown in the menu of the manager again.
  if (this->Internal->MenuManager)
  {
    this->Internal->MenuManager->setShowCategoriesInMenuBar(false);
  }
  for (auto& entry : this->Internal->Menus)
  {
    entry.release();
  }
}

//-----------------------------------------------------------------------------
void pqCategoryMenuBarBehavior::updateMenus()
{
  auto& internal = *this->Internal;
  if (!internal.MainWindow || !internal.MenuManager)
  {
    return;
  }

  // use the menu bar of the window, if any: QMainWindow::menuBar() would create
  // one, replacing e.g. a custom menu widget. Without menu bar, the categories
  // are shown in the menu of the manager instead.
  auto menubar = qobject_cast<QMenuBar*>(internal.MainWindow->menuWidget());
  internal.MenuManager->setShowCategoriesInMenuBar(menubar != nullptr);

  QList<pqProxyCategory*> categories;
  for (auto category : internal.MenuManager->getMenuCategory()->getCategoriesAlphabetically())
  {
    if (menubar && category->showInMenuBar() && !category->isEmpty())
    {
      categories << category;
    }
  }

  // remove the items of the categories no longer shown in the menu bar. Their
  // menus are removed afterwards, as they may be used for other categories.
  QMap<QString, pqCategoryMenu> previousMenus;
  for (auto category : categories)
  {
    if (internal.Menus.contains(category->name()))
    {
      previousMenus[category->name()] = internal.Menus.take(category->name());
    }
  }
  QList<pqCategoryMenu> obsoleteMenus = internal.Menus.values();
  internal.Menus.clear();
  for (auto& entry : obsoleteMenus)
  {
    QAction* next = nullptr;
    entry.removeItems(next);
  }

  for (auto category : categories)
  {
    // a category is shown in the menu of the menu bar with its title, if any,
    // e.g. "View", or the menu created for a category of another manager.
    pqCategoryMenu entry = previousMenus.take(category->name());
    QMenu* menu = ::findMenu(menubar, category->label());
    if (!menu && entry.Menu && entry.Created)
    {
      // the label of the category changed.
      menu = entry.Menu;
      menu->setTitle(category->label());
    }

    QAction* before = nullptr;
    if (!menu || menu != entry.Menu)
    {
      entry.release();
    }
    if (!menu)
    {
      menu = new QMenu(category->label(), menubar);
      menu->setObjectName(category->name());
      menu->menuAction()->setMenuRole(QAction::NoRole);
      menu->setProperty(CATEGORY_MENU_PROPERTY, true);
      // insert new menus before the Help menu, if any.
      menubar->insertMenu(::findHelpMenuAction(menubar), menu);
      entry.Created = true;
    }
    else if (menu != entry.Menu || !entry.removeItems(before))
    {
      // items are added at the end of existing menus. Otherwise, they are kept
      // where they were, as other items may have been added after them.
      before = ::findExitAction(menu);
    }
    entry.Menu = menu;
    internal.addItems(entry, category, before, this);
    internal.Menus[category->name()] = entry;

    // the menu of another manager, e.g. "Sources", is cleared when populated.
    auto manager = menu->findChild<pqProxyGroupMenuManager*>(QString(), Qt::FindDirectChildrenOnly);
    if (manager && manager != internal.MenuManager)
    {
      QObject::connect(manager, &pqProxyGroupMenuManager::menuPopulated, this,
        &pqCategoryMenuBarBehavior::updateMenus, Qt::UniqueConnection);
    }
  }

  for (auto& entry : obsoleteMenus)
  {
    entry.release();
  }
}
