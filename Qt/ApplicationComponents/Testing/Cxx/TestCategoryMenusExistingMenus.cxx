// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

// Tests categories shown in the menu bar that have the title of an existing
// menu: their items are added to this menu, e.g. "View", instead of creating
// another menu with the same title. Categories of sources and of filters with
// the same title also share a menu.

#include "pqActiveObjects.h"
#include "pqObjectBuilder.h"
#include "pqPVApplicationCore.h"
#include "pqParaViewMenuBuilders.h"
#include "pqProxyCategory.h"
#include "pqProxyGroupMenuManager.h"
#include "pqServer.h"
#include "vtkSMSession.h"

#include <QAction>
#include <QApplication>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QStringList>

#include <iostream>

namespace
{
const char* FirstXML = R"(
<ServerManagerConfiguration>
  <ParaViewFilters>
    <Category name="View" show_in_menubar="1" preserve_order="1">
      <Proxy group="filters" name="ShrinkFilter" />
      <Category name="More">
        <Proxy group="filters" name="Cut" />
      </Category>
    </Category>
    <Category name="File" show_in_menubar="1">
      <Proxy group="filters" name="Clip" />
    </Category>
    <Category name="Shared" menu_label="&amp;Shared Tools" show_in_menubar="1">
      <Proxy group="filters" name="Clip" />
    </Category>
  </ParaViewFilters>
  <ParaViewSources>
    <Category name="Shared" menu_label="Shared Tools" show_in_menubar="1">
      <Proxy group="sources" name="SphereSource" />
    </Category>
  </ParaViewSources>
</ServerManagerConfiguration>)";

// adds an item to a category shown in an existing menu.
const char* SecondXML = R"(
<ParaViewFilters>
  <Category name="View">
    <Proxy group="filters" name="Contour" />
  </Category>
</ParaViewFilters>)";

#define CHECK(condition)                                                                           \
  if (!(condition))                                                                                \
  {                                                                                                \
    std::cerr << "Failed check at line " << __LINE__ << ": " #condition << std::endl;              \
    return false;                                                                                  \
  }

#define CHECK_ITEMS(menu, ...)                                                                     \
  if (texts(menu) != QStringList({ __VA_ARGS__ }))                                                 \
  {                                                                                                \
    std::cerr << "Unexpected menu items at line " << __LINE__ << ": "                              \
              << texts(menu).join(", ").toStdString() << std::endl;                                \
    return false;                                                                                  \
  }

QStringList texts(QWidget* widget)
{
  QStringList result;
  for (QAction* action : widget->actions())
  {
    result << (action->isSeparator() ? QString("---") : action->text());
  }
  return result;
}

QAction* findAction(QWidget* widget, const QString& text)
{
  for (QAction* action : widget->actions())
  {
    if (action->text() == text)
    {
      return action;
    }
  }
  return nullptr;
}

/**
 * Sets whether the top-level category is shown in the menu bar, as done using
 * the Configure Filter Categories dialog, and updates the menus.
 */
void setShowInMenuBar(QMenu* managerMenu, const QString& category, bool show)
{
  auto manager = managerMenu->findChild<pqProxyGroupMenuManager*>();
  manager->getMenuCategory()->findSubCategory(category)->setShowInMenuBar(show);
  Q_EMIT managerMenu->aboutToShow();
}

bool doTest(QMainWindow& window, QMenu* sourcesMenu, QMenu* filtersMenu)
{
  QMenuBar* menubar = window.menuBar();
  QMenu* fileMenu = menubar->actions()[0]->menu();
  QMenu* viewMenu = menubar->actions()[3]->menu();
  const QStringList menus = texts(menubar);
  pqApplicationCore::instance()->loadConfigurationXML(FirstXML);

  // a single menu is added, before the "Help" menu: the "View" and "File"
  // categories use the existing menus.
  CHECK_ITEMS(menubar, "&File", "&Sources", "&Filters", "&View", "Shared Tools", "&Help");

  // items are added at the end of the menu, after a separator, followed by the
  // sub-menus.
  CHECK_ITEMS(viewMenu, "Toolbars", "---", "Shrink", "More");
  QMenu* subMenu = viewMenu->actions().back()->menu();
  CHECK(subMenu && subMenu->objectName() == "More");
  CHECK_ITEMS(subMenu, "Slice");

  // but before the "Exit" item, if any.
  CHECK_ITEMS(fileMenu, "Open", "---", "Clip", "---", "Exit");

  // categories of sources and of filters with the same title share a menu.
  QMenu* sharedMenu = menubar->actions()[4]->menu();
  CHECK(sharedMenu && sharedMenu->objectName() == "Shared");
  CHECK_ITEMS(sharedMenu, "Sphere", "---", "Clip");

  // only the items of the category are updated when the menu is shown: filters
  // require an input.
  QAction* toolbars = findAction(viewMenu, "Toolbars");
  QAction* shrink = findAction(viewMenu, "Shrink");
  CHECK(toolbars && shrink);
  Q_EMIT viewMenu->aboutToShow();
  CHECK(toolbars->isEnabled() && !shrink->isEnabled());

  // items of the category stay where they are when it is updated, e.g. when a
  // plugin is loaded, as other items may have been added to the menu since.
  viewMenu->addAction("Full Screen");
  pqApplicationCore::instance()->loadConfigurationXML(SecondXML);
  CHECK(menubar->actions()[3]->menu() == viewMenu);
  CHECK_ITEMS(viewMenu, "Toolbars", "---", "Shrink", "Contour", "More", "---", "Full Screen");
  CHECK_ITEMS(fileMenu, "Open", "---", "Clip", "---", "Exit");
  CHECK_ITEMS(sharedMenu, "Sphere", "---", "Clip");

  // the items of the menu are kept when the menu is populated again.
  Q_EMIT sourcesMenu->aboutToShow();
  Q_EMIT filtersMenu->aboutToShow();
  CHECK_ITEMS(viewMenu, "Toolbars", "---", "Shrink", "Contour", "More", "---", "Full Screen");
  CHECK_ITEMS(sharedMenu, "Sphere", "---", "Clip");

  // when categories are no longer shown in the menu bar, their items are
  // removed from the existing menus, which are kept.
  setShowInMenuBar(filtersMenu, "View", false);
  setShowInMenuBar(filtersMenu, "File", false);
  CHECK_ITEMS(viewMenu, "Toolbars", "Full Screen");
  CHECK_ITEMS(fileMenu, "Open", "---", "Exit");
  CHECK_ITEMS(menubar, "&File", "&Sources", "&Filters", "&View", "Shared Tools", "&Help");

  // the shared menu is removed once none of the categories use it.
  setShowInMenuBar(sourcesMenu, "Shared", false);
  CHECK_ITEMS(sharedMenu, "Clip");
  CHECK_ITEMS(menubar, "&File", "&Sources", "&Filters", "&View", "Shared Tools", "&Help");
  setShowInMenuBar(filtersMenu, "Shared", false);
  CHECK(texts(menubar) == menus);

  // and created again, for the first category shown.
  setShowInMenuBar(filtersMenu, "Shared", true);
  CHECK_ITEMS(menubar, "&File", "&Sources", "&Filters", "&View", "&Shared Tools", "&Help");
  CHECK_ITEMS(menubar->actions()[4]->menu(), "Clip");
  setShowInMenuBar(sourcesMenu, "Shared", true);
  CHECK_ITEMS(menubar->actions()[4]->menu(), "Clip", "---", "Sphere");
  return true;
}
}

extern int TestCategoryMenusExistingMenus(int argc, char* argv[])
{
  QApplication app(argc, argv);
  pqPVApplicationCore appCore(argc, argv);

  QMainWindow window;
  QMenu* fileMenu = window.menuBar()->addMenu("&File");
  fileMenu->addAction("Open");
  fileMenu->addSeparator();
  fileMenu->addAction("Exit")->setMenuRole(QAction::NoRole);
  QMenu* sourcesMenu = window.menuBar()->addMenu("&Sources");
  QMenu* filtersMenu = window.menuBar()->addMenu("&Filters");
  window.menuBar()->addMenu("&View")->addAction("Toolbars");
  window.menuBar()->addMenu("&Help");
  pqParaViewMenuBuilders::buildSourcesMenu(*sourcesMenu, &window);
  pqParaViewMenuBuilders::buildFiltersMenu(*filtersMenu, &window);

  pqObjectBuilder* builder = appCore.getObjectBuilder();
  pqServer* server = builder->createServer(pqServerResource(vtkSMSession::GetBuiltinName()));
  pqActiveObjects::instance().setActiveServer(server);

  const bool success = doTest(window, sourcesMenu, filtersMenu);
  builder->destroyAllProxies(server);
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
