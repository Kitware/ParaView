// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCategoryToolbarsBehavior.h"

#include "pqApplicationCore.h"
#include "pqEventDispatcher.h"
#include "pqEventTranslator.h"
#include "pqProxyCategory.h"
#include "pqProxyGroupMenuManager.h"

#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QPointer>
#include <QToolBar>
#include <QToolButton>

#include <cassert>

namespace
{
// marks the menus created for the sub-categories shown in toolbars.
const char* SUBCATEGORY_MENU_PROPERTY = "pqCategoryToolbarsBehavior_SubCategoryMenu";
}

class pqCategoryToolbarsBehavior::pqInternal
{
public:
  QList<QAction*> ToolbarsToHide;
  QList<QPointer<QToolBar>> CachedToolBars;
  QPointer<QMainWindow> MainWindow;
  QPointer<pqProxyGroupMenuManager> MenuManager;

  pqInternal(QMainWindow* mainWindow, pqProxyGroupMenuManager* menuManager)
    : MainWindow(mainWindow)
    , MenuManager(menuManager)
  {
  }

  /**
   * Remove deleted toolbars from cache.
   */
  void cleanCache() { this->CachedToolBars.removeAll(nullptr); }

  /**
   * Find a toolbar with the matching name. Use inner cache.
   * Return nullptr if no such toolbar exists.
   */
  QToolBar* findToolbar(pqProxyCategory* category)
  {
    this->cleanCache();
    auto toolbarName = this->MenuManager->getToolbarName(category);
    for (const auto& toolbar : this->CachedToolBars)
    {
      if (toolbar->objectName() == toolbarName)
      {
        return toolbar;
      }
    }

    return nullptr;
  }

  /**
   * Create a toolbar from the category.
   * Add it to the main window.
   */
  QToolBar* createToolbar(pqProxyCategory* category)
  {
    const QString& categoryLabel = category->label();

    auto toolbarName = this->MenuManager->getToolbarName(category);
    auto toolbar = new QToolBar(this->MainWindow);
    toolbar->setObjectName(toolbarName);
    toolbar->setOrientation(Qt::Horizontal);
    toolbar->setWindowTitle(categoryLabel);
    this->MainWindow->addToolBar(toolbar);
    this->CachedToolBars << toolbar;

    return toolbar;
  }

  /**
   * Populate toolbar with up-to-date action list.
   */
  void updateActions(QToolBar* toolbar, pqProxyCategory* category)
  {
    const QString& categoryName = category->name();
    QList<QAction*> toolbarActions = this->MenuManager->categoryActions(category);
    toolbar->clear();
    // delete the menus of the previous drop-down buttons, once control returns
    // to the event loop as one may be in use, e.g. when one of its items loads
    // a plugin. Their name is cleared so that they are not found meanwhile.
    for (QMenu* menu : toolbar->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly))
    {
      if (menu->property(SUBCATEGORY_MENU_PROPERTY).toBool())
      {
        menu->setProperty(SUBCATEGORY_MENU_PROPERTY, false);
        menu->setObjectName(QString());
        menu->deleteLater();
      }
    }
    for (int cc = 0; cc < toolbarActions.size(); cc++)
    {
      QVariant omitList = toolbarActions[cc]->property("OmitFromToolbar");
      if (omitList.isValid() && omitList.toStringList().contains(categoryName))
      {
        continue;
      }
      toolbar->addAction(toolbarActions[cc]);
    }

    // toolbars of categories shown in the menu bar mirror their menu:
    // sub-categories are shown as drop-down buttons.
    if (!category->showInMenuBar())
    {
      return;
    }
    for (auto subCategory : category->getCategoriesAlphabetically())
    {
      if (subCategory->isEmpty())
      {
        continue;
      }
      auto menu = new QMenu(subCategory->label(), toolbar);
      menu->setObjectName(subCategory->name());
      menu->menuAction()->setMenuRole(QAction::NoRole);
      menu->setProperty(SUBCATEGORY_MENU_PROPERTY, true);
      this->MenuManager->populateMenuFromCategory(menu, subCategory);
      toolbar->addAction(menu->menuAction());
      if (auto button = qobject_cast<QToolButton*>(toolbar->widgetForAction(menu->menuAction())))
      {
        button->setPopupMode(QToolButton::InstantPopup);
      }
    }
  }

  /**
   * Remove obsolete toolbars from the window.
   * Toolbars to keep in place are passed as parameters.
   * Use cache to determine previous toolbars that should be removed.
   */
  void removeObsoleteToolbars(QList<QToolBar*> newToolbars)
  {
    for (const auto& toolbar : this->CachedToolBars)
    {
      if (!newToolbars.contains(toolbar))
      {
        toolbar->deleteLater();
      }
    }

    this->CachedToolBars.removeAll(nullptr);
  }
};

//-----------------------------------------------------------------------------
pqCategoryToolbarsBehavior::pqCategoryToolbarsBehavior(
  pqProxyGroupMenuManager* menuManager, QMainWindow* mainWindow)
  : Superclass(menuManager)
  , Internal(new pqInternal(mainWindow, menuManager))
{
  assert(menuManager);
  assert(mainWindow);

  QObject::connect(menuManager, SIGNAL(menuPopulated()), this, SLOT(updateToolbars()));
  QObject::connect(menuManager, SIGNAL(categoriesUpdated()), this, SLOT(updateToolbars()));
}

//-----------------------------------------------------------------------------
// needed to destroy unique_ptr of forward declared class.
pqCategoryToolbarsBehavior::~pqCategoryToolbarsBehavior() = default;

//-----------------------------------------------------------------------------
void pqCategoryToolbarsBehavior::updateToolbars()
{
  auto appcategory = this->Internal->MenuManager->getMenuCategory();
  QList<QToolBar*> newToolbars;
  for (auto category : appcategory->getSubCategoriesRecursive())
  {
    if (!category->showInToolbar())
    {
      continue;
    }

    auto toolbar = this->Internal->findToolbar(category);
    if (!toolbar)
    {
      toolbar = this->Internal->createToolbar(category);
    }

    this->Internal->updateActions(toolbar, category);

    newToolbars << toolbar;
  }

  this->Internal->removeObsoleteToolbars(newToolbars);
}
