// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

// Tests that the tree of the Configure Filter Categories dialog mirrors the
// menu bar: top-level categories are under a "Filters" item, unless shown as
// menus of the menu bar, in which case they are beside it.

#include "pqActiveObjects.h"
#include "pqConfigureCategoriesDialog.h"
#include "pqObjectBuilder.h"
#include "pqPVApplicationCore.h"
#include "pqParaViewMenuBuilders.h"
#include "pqProxyGroupMenuManager.h"
#include "pqServer.h"
#include "vtkSMSession.h"

#include <QApplication>
#include <QCheckBox>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QToolButton>
#include <QTreeWidget>

#include <iostream>

namespace
{
const char* XML = R"(
<ParaViewFilters>
  <Category name="MyTools" menu_label="My Tools" show_in_menubar="1">
    <Proxy group="filters" name="ShrinkFilter" />
  </Category>
  <Category name="Regular">
    <Proxy group="filters" name="Clip" />
    <Category name="Sub">
      <Proxy group="filters" name="Cut" />
    </Category>
  </Category>
</ParaViewFilters>)";

#define CHECK(condition)                                                                           \
  if (!(condition))                                                                                \
  {                                                                                                \
    std::cerr << "Failed check at line " << __LINE__ << ": " #condition << std::endl;              \
    return false;                                                                                  \
  }

QTreeWidgetItem* findChild(QTreeWidgetItem* parent, const QString& text)
{
  for (int cc = 0; cc < parent->childCount(); cc++)
  {
    if (parent->child(cc)->text(0) == text)
    {
      return parent->child(cc);
    }
  }
  return nullptr;
}

QMenu* findMenu(QWidget* parent, const QString& name)
{
  for (QAction* action : parent->actions())
  {
    if (action->menu() && action->menu()->objectName() == name)
    {
      return action->menu();
    }
  }
  return nullptr;
}

/**
 * The widgets of the dialog used by the test.
 */
struct Widgets
{
  QTreeWidget* Tree;
  QTreeWidgetItem* Filters;
  QCheckBox* MenuBar;
  QCheckBox* Toolbar;
  QToolButton* Remove;
  QToolButton* AddFilter;
  QToolButton* AddCategory;
  QToolButton* AddSubCategory;

  Widgets(pqConfigureCategoriesDialog& dialog)
    : Tree(dialog.findChild<QTreeWidget*>("customCategoriesTree"))
    , Filters(this->Tree->topLevelItem(0))
    , MenuBar(dialog.findChild<QCheckBox*>("useAsMenuBarMenu"))
    , Toolbar(dialog.findChild<QCheckBox*>("useAsToolbar"))
    , Remove(dialog.findChild<QToolButton*>("remove"))
    , AddFilter(dialog.findChild<QToolButton*>("addFilter"))
    , AddCategory(dialog.findChild<QToolButton*>("addCategory"))
    , AddSubCategory(dialog.findChild<QToolButton*>("addSubCategory"))
  {
  }
};

/**
 * Checks the item of the menu, and that it cannot be modified.
 */
bool testFiltersItem(pqConfigureCategoriesDialog& dialog, Widgets& ui)
{
  CHECK(ui.Filters && ui.Filters->text(0) == "Filters" && ui.Filters->isExpanded());
  CHECK(!(ui.Filters->flags() & (Qt::ItemIsEditable | Qt::ItemIsDragEnabled)));

  // filters cannot be added to it, only categories.
  auto filters = dialog.findChild<QTreeWidget*>("defaultCategoriesTree");
  filters->setCurrentItem(filters->topLevelItem(0));
  ui.Tree->setCurrentItem(ui.Filters);
  CHECK(!ui.MenuBar->isEnabled() && !ui.Toolbar->isEnabled());
  CHECK(!ui.Remove->isEnabled() && !ui.AddFilter->isEnabled() && ui.AddSubCategory->isEnabled());

  // nor removed, e.g. using the Delete key.
  const int numCategories = ui.Filters->childCount();
  QMetaObject::invokeMethod(&dialog, "onRemovePressed");
  CHECK(ui.Tree->topLevelItem(0) == ui.Filters && ui.Filters->childCount() == numCategories);
  filters->setCurrentItem(nullptr);
  return true;
}

bool doTest(QMainWindow& window, QMenu* filtersMenu)
{
  auto manager = filtersMenu->findChild<pqProxyGroupMenuManager*>();
  pqApplicationCore::instance()->loadConfigurationXML(XML);
  CHECK(findMenu(window.menuBar(), "MyTools") != nullptr);

  {
    pqConfigureCategoriesDialog dialog(manager);
    Widgets ui(dialog);

    // the category shown in the menu bar is beside the item of the menu, which
    // contains the other ones.
    CHECK(ui.Tree->topLevelItemCount() == 2);
    QTreeWidgetItem* myTools = ui.Tree->topLevelItem(1);
    CHECK(myTools->text(0) == "My Tools" && findChild(myTools, "Shrink"));
    QTreeWidgetItem* regular = findChild(ui.Filters, "Regular");
    CHECK(regular && findChild(regular, "Clip") && findChild(ui.Filters, "&Favorites"));
    CHECK(!findChild(ui.Filters, "My Tools"));
    CHECK(testFiltersItem(dialog, ui));

    // the item of the category is moved under the item of the menu when it is
    // no longer shown in the menu bar.
    ui.Tree->setCurrentItem(myTools);
    CHECK(ui.MenuBar->isEnabled() && ui.MenuBar->isChecked() && ui.Remove->isEnabled());
    ui.MenuBar->click();
    CHECK(ui.Tree->topLevelItemCount() == 1 && myTools->parent() == ui.Filters);
    CHECK(ui.Tree->currentItem() == myTools);
    CHECK(ui.MenuBar->isEnabled() && !ui.MenuBar->isChecked());

    // and beside it when shown in the menu bar, with its children.
    ui.Tree->setCurrentItem(regular);
    CHECK(ui.MenuBar->isEnabled() && !ui.MenuBar->isChecked());
    ui.MenuBar->click();
    CHECK(ui.Tree->topLevelItemCount() == 2 && ui.Tree->topLevelItem(1) == regular);
    CHECK(ui.Tree->currentItem() == regular && ui.MenuBar->isChecked());
    QTreeWidgetItem* sub = findChild(regular, "Sub");
    CHECK(sub && findChild(sub, "Slice") && findChild(regular, "Clip"));

    // only top-level categories can be shown in the menu bar.
    ui.Tree->setCurrentItem(sub);
    CHECK(!ui.MenuBar->isEnabled() && ui.Toolbar->isEnabled());

    // categories are created in the menu, whatever the selection.
    ui.AddCategory->click();
    CHECK(ui.Tree->topLevelItemCount() == 2);
    CHECK(ui.Filters->child(0)->text(0) == "New Category");
    CHECK(ui.Tree->currentItem() == ui.Filters->child(0));
    CHECK(ui.MenuBar->isEnabled() && !ui.MenuBar->isChecked());
    ui.Tree->setCurrentItem(nullptr);
    ui.Tree->clearSelection();
    QMetaObject::invokeMethod(&dialog, "createNewCategory");
    CHECK(ui.Tree->topLevelItemCount() == 2);
    CHECK(findChild(ui.Filters, "New Category_0") != nullptr);

    // sub-categories are created in the selected category.
    ui.Tree->setCurrentItem(regular);
    ui.AddSubCategory->click();
    CHECK(ui.Tree->topLevelItemCount() == 2 && findChild(regular, "New Category"));
    dialog.accept();
  }

  // the menus are updated.
  CHECK(findMenu(window.menuBar(), "MyTools") == nullptr);
  CHECK(findMenu(window.menuBar(), "Regular") != nullptr);
  Q_EMIT filtersMenu->aboutToShow();
  CHECK(findMenu(filtersMenu, "MyTools") != nullptr);
  CHECK(findMenu(filtersMenu, "Regular") == nullptr);

  // and the dialog reflects them, until reset.
  pqConfigureCategoriesDialog dialog(manager);
  Widgets ui(dialog);
  CHECK(ui.Tree->topLevelItemCount() == 2);
  CHECK(ui.Filters->text(0) == "Filters" && ui.Tree->topLevelItem(1)->text(0) == "Regular");
  CHECK(findChild(ui.Filters, "My Tools") && !findChild(ui.Filters, "Regular"));
  dialog.findChild<QToolButton*>("resetAll")->click();
  ui = Widgets(dialog);
  CHECK(ui.Tree->topLevelItemCount() == 2);
  CHECK(ui.Filters->text(0) == "Filters" && ui.Filters->isExpanded());
  CHECK(ui.Tree->topLevelItem(1)->text(0) == "My Tools");
  CHECK(findChild(ui.Filters, "Regular") && !findChild(ui.Filters, "My Tools"));
  return true;
}
}

extern int TestConfigureCategoriesDialog(int argc, char* argv[])
{
  QApplication app(argc, argv);
  pqPVApplicationCore appCore(argc, argv);

  QMainWindow window;
  QMenu* filtersMenu = window.menuBar()->addMenu("&Filters");
  window.menuBar()->addMenu("&Help");
  pqParaViewMenuBuilders::buildFiltersMenu(*filtersMenu, &window);

  pqObjectBuilder* builder = appCore.getObjectBuilder();
  pqServer* server = builder->createServer(pqServerResource(vtkSMSession::GetBuiltinName()));
  pqActiveObjects::instance().setActiveServer(server);

  const bool success = doTest(window, filtersMenu);
  builder->destroyAllProxies(server);
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
