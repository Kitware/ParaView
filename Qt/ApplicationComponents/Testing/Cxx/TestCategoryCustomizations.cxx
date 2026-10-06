// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

// Tests that user customizations of categories are saved as differences with
// the categories defined by the application and plugins, and applied over
// them, so that categories added later on, e.g. by plugins, are shown.

#include "pqActiveObjects.h"
#include "pqObjectBuilder.h"
#include "pqPVApplicationCore.h"
#include "pqProxyCategory.h"
#include "pqProxyGroupMenuManager.h"
#include "pqServer.h"
#include "pqSettings.h"
#include "vtkSMSession.h"

#include <QApplication>
#include <QMenu>

#include <iostream>
#include <memory>

namespace
{
const char* CustomizationsKey = "categoryCustomizations.ParaViewFilters/";
const char* LegacyKey = "categories.ParaViewFilters/";

// categories defined by the application.
const char* BaseXML = R"(
<ParaViewFilters>
  <Category name="A" show_in_toolbar="1">
    <Proxy group="filters" name="Clip" />
    <Proxy group="filters" name="Cut" />
  </Category>
  <Category name="B">
    <Proxy group="filters" name="Contour" />
  </Category>
</ParaViewFilters>)";

// categories defined by a plugin loaded later on.
const char* PluginXML = R"(
<ParaViewFilters>
  <Category name="A">
    <Proxy group="filters" name="Threshold" />
  </Category>
  <Category name="P" menu_label="Plugin">
    <Proxy group="filters" name="DataSetSurfaceFilter" />
    <Script name="PluginScript">print("From the plugin")</Script>
  </Category>
</ParaViewFilters>)";

#define CHECK(condition)                                                                           \
  if (!(condition))                                                                                \
  {                                                                                                \
    std::cerr << "Failed check at line " << __LINE__ << ": " #condition << std::endl;              \
    return false;                                                                                  \
  }

std::unique_ptr<pqProxyGroupMenuManager> newManager(QMenu& menu)
{
  return std::unique_ptr<pqProxyGroupMenuManager>(
    new pqProxyGroupMenuManager(&menu, "ParaViewFilters", false, false));
}

/**
 * Checks the categories are the ones from BaseXML and PluginXML, with the
 * customizations done by the user below.
 */
bool checkCustomized(pqProxyCategory* categories)
{
  pqProxyCategory* a = categories->findSubCategory("A");
  CHECK(a != nullptr);
  CHECK(!a->showInToolbar());
  CHECK(a->findProxy("Clip") && a->findProxy("Clip")->hideFromMenu());
  CHECK(a->findProxy("Cut") && !a->findProxy("Cut")->hideFromMenu());
  CHECK(categories->findSubCategory("B") == nullptr);
  pqProxyCategory* mine = categories->findSubCategory("Mine");
  CHECK(mine != nullptr && mine->hasProxy("ShrinkFilter"));
  return true;
}

bool doTest()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  settings->remove(CustomizationsKey);
  settings->remove(LegacyKey);

  QMenu menu1;
  auto manager1 = newManager(menu1);
  core->loadConfigurationXML(BaseXML);

  // customize the categories, as done using the Configure Categories dialog.
  pqProxyCategory* categories = manager1->getMenuCategory();
  categories->findSubCategory("A")->setShowInToolbar(false);
  categories->findSubCategory("A")->findProxy("Clip")->setHideFromMenu(true);
  categories->removeCategory("B");
  auto mine = new pqProxyCategory(categories, "Mine", "Mine");
  mine->addProxy(new pqProxyInfo(mine, "ShrinkFilter", "filters", ""));
  manager1->writeCategoryToSettings();

  // only differences are saved.
  const QString saved = settings->value(CustomizationsKey).toString();
  CHECK(saved.contains("Clip") && saved.contains("ShrinkFilter"));
  CHECK(!saved.contains("Cut"));
  CHECK(!settings->contains(LegacyKey));

  // the customizations are applied when categories are loaded again.
  QMenu menu2;
  auto manager2 = newManager(menu2);
  core->loadConfigurationXML(BaseXML);
  CHECK(checkCustomized(manager2->getMenuCategory()));

  // categories from plugins are added, with the customizations kept.
  core->loadConfigurationXML(PluginXML);
  for (auto manager : { manager1.get(), manager2.get() })
  {
    categories = manager->getMenuCategory();
    CHECK(checkCustomized(categories));
    CHECK(categories->findSubCategory("A")->hasProxy("Threshold"));
    pqProxyCategory* plugin = categories->findSubCategory("P");
    CHECK(plugin && plugin->label() == "Plugin" && plugin->hasProxy("DataSetSurfaceFilter"));
  }

  // customizations of categories from plugins are applied once they are loaded.
  manager2->getMenuCategory()->findSubCategory("P")->setShowInToolbar(true);
  manager2->writeCategoryToSettings();
  QMenu menu3;
  auto manager3 = newManager(menu3);
  core->loadConfigurationXML(BaseXML);
  CHECK(checkCustomized(manager3->getMenuCategory()));
  CHECK(manager3->getMenuCategory()->findSubCategory("P") == nullptr);
  core->loadConfigurationXML(PluginXML);
  CHECK(manager3->getMenuCategory()->findSubCategory("P") &&
    manager3->getMenuCategory()->findSubCategory("P")->showInToolbar());

  // customizations of categories and items from plugins that are not loaded
  // are kept when customizations are saved meanwhile, e.g. when adding a
  // favorite or using the Configure Categories dialog, which edits a copy.
  categories = manager3->getMenuCategory();
  categories->findSubCategory("A")->findProxy("Threshold")->setHideFromMenu(true);
  pqProxyCategory* mineCategory = categories->findSubCategory("Mine");
  mineCategory->addProxy(new pqProxyInfo(
    mineCategory, manager3->getApplicationCategory()->findProxy("PluginScript", true)));
  manager3->writeCategoryToSettings();

  QMenu menu4;
  auto manager4 = newManager(menu4);
  core->loadConfigurationXML(BaseXML);
  categories = manager4->getMenuCategory();
  CHECK(checkCustomized(categories));
  CHECK(categories->findSubCategory("P") == nullptr);
  CHECK(!categories->findSubCategory("Mine")->hasProxy("PluginScript"));
  manager4->writeCategoryToSettings();
  pqProxyCategory copy;
  copy.deepCopy(categories);
  copy.writeSettings("ParaViewFilters", manager4->getApplicationCategory());
  const QString kept = settings->value(CustomizationsKey).toString();
  CHECK(
    kept.contains(R"(name="P")") && kept.contains("Threshold") && kept.contains("PluginScript"));

  core->loadConfigurationXML(PluginXML);
  categories = manager4->getMenuCategory();
  CHECK(checkCustomized(categories));
  CHECK(categories->findSubCategory("P") && categories->findSubCategory("P")->showInToolbar());
  pqProxyInfo* threshold = categories->findSubCategory("A")->findProxy("Threshold");
  CHECK(threshold && threshold->hideFromMenu());
  pqProxyInfo* script = categories->findSubCategory("Mine")->findProxy("PluginScript");
  CHECK(script && script->isScript() && script->script().contains("From the plugin"));

  // categories are only rebuilt when their customizations or definitions
  // change, not whenever settings are modified or definitions looked for.
  settings->setValue("TestCategoryCustomizations.unrelated", 1);
  settings->alertSettingsModified();
  CHECK(manager4->getMenuCategory() == categories);
  manager4->addProxyDefinitionUpdateListener("filters");
  categories = manager4->getMenuCategory();
  manager4->lookForNewDefinitions();
  CHECK(manager4->getMenuCategory() == categories);
  settings->remove("TestCategoryCustomizations.unrelated");

  // categories saved by previous versions, as a copy of all categories, are
  // merged into the application categories.
  settings->remove(CustomizationsKey);
  settings->setValue(LegacyKey, R"(
<ParaViewFilters>
  <Category name="A" show_in_toolbar="1">
    <Proxy group="filters" name="Clip" hide_from_menu="1" />
    <Proxy group="filters" name="Cut" />
  </Category>
  <Category name="Legacy">
    <Proxy group="filters" name="Contour" />
  </Category>
</ParaViewFilters>)");
  settings->alertSettingsModified();
  categories = manager1->getMenuCategory();
  CHECK(categories->findSubCategory("A") && categories->findSubCategory("A")->showInToolbar());
  CHECK(categories->findSubCategory("A")->findProxy("Clip")->hideFromMenu());
  CHECK(categories->findSubCategory("A")->hasProxy("Threshold"));
  CHECK(categories->findSubCategory("Legacy") != nullptr);
  CHECK(categories->findSubCategory("B") != nullptr);
  CHECK(categories->findSubCategory("P") != nullptr);

  settings->remove(LegacyKey);
  return true;
}
}

extern int TestCategoryCustomizations(int argc, char* argv[])
{
  QApplication app(argc, argv);
  pqPVApplicationCore appCore(argc, argv);

  pqObjectBuilder* builder = appCore.getObjectBuilder();
  pqServer* server = builder->createServer(pqServerResource(vtkSMSession::GetBuiltinName()));
  pqActiveObjects::instance().setActiveServer(server);

  const bool success = doTest();
  builder->destroyAllProxies(server);
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
