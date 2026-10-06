// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

// Tests categories of the Filters menu shown in the menu bar and as toolbars,
// containing filters, sources, Python scripts, separators and sub-categories.

#include "pqActiveObjects.h"
#include "pqObjectBuilder.h"
#include "pqPVApplicationCore.h"
#include "pqParaViewMenuBuilders.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"

#include <QAction>
#include <QApplication>
#include <QElapsedTimer>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QThread>
#include <QToolBar>
#include <QToolButton>

#include <iostream>

namespace
{
const char* FirstXML = R"(
<ServerManagerConfiguration>
  <ParaViewFilters>
    <Category name="MyTools" menu_label="&amp;My Tools"
              show_in_menubar="1" show_in_toolbar="1" preserve_order="1">
      <Proxy group="sources" name="RTAnalyticSource" />
      <Proxy group="filters" name="ShrinkFilter" />
      <Separator />
      <Script name="CreateSphere" label="Create Sphere">
        <![CDATA[
        from paraview.simple import Sphere
        if True:
            Sphere(registrationName="SphereFromScript")
        ]]>
      </Script>
      <Category name="More" preserve_order="1">
        <Proxy group="filters" name="Cut" />
      </Category>
    </Category>
    <Category name="NotInMenuBar" show_in_toolbar="1">
      <Proxy group="sources" name="SphereSource" />
      <Category name="Sub">
        <Proxy group="filters" name="Clip" />
      </Category>
    </Category>
  </ParaViewFilters>
</ServerManagerConfiguration>)";

// adds items to the categories defined above, as a separate plugin may.
const char* SecondXML = R"(
<ParaViewFilters>
  <Category name="MyTools">
    <Category name="More">
      <Proxy group="filters" name="Contour" />
    </Category>
  </Category>
</ParaViewFilters>)";

// a category with only a script, with the same name as the one above.
const char* ThirdXML = R"(
<ParaViewFilters>
  <Category name="Other" show_in_menubar="1" show_in_toolbar="1" preserve_order="1">
    <Script name="CreateSphere" label="Create Other Sphere">
      from paraview.simple import Sphere
      Sphere(registrationName="OtherSphereFromScript")
    </Script>
  </Category>
</ParaViewFilters>)";

// adds a source, and a label, to the category above.
const char* FourthXML = R"(
<ParaViewFilters>
  <Category name="Other" menu_label="Other &amp;Tools">
    <Proxy group="sources" name="SphereSource" />
  </Category>
</ParaViewFilters>)";

#define CHECK(condition)                                                                           \
  if (!(condition))                                                                                \
  {                                                                                                \
    std::cerr << "Failed check at line " << __LINE__ << ": " #condition << std::endl;              \
    return false;                                                                                  \
  }

QStringList texts(const QList<QAction*>& actions)
{
  QStringList result;
  for (QAction* action : actions)
  {
    result << (action->isSeparator() ? QString("---") : action->text());
  }
  return result;
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

QToolBar* findToolBar(QMainWindow& window, const QString& categoryName)
{
  for (QToolBar* toolbar : window.findChildren<QToolBar*>())
  {
    if (toolbar->objectName().endsWith("." + categoryName))
    {
      return toolbar;
    }
  }
  return nullptr;
}

/**
 * Mimics pqApplyBehavior applying a newly created source: the pipeline is
 * updated, as the data is shown, before the source is marked as unmodified.
 */
void apply(pqPipelineSource* source)
{
  pqActiveObjects::instance().setActiveSource(source);
  source->updatePipeline();
  source->setModifiedState(pqProxy::UNMODIFIED);
  Q_EMIT pqApplicationCore::instance() -> forceFilterMenuRefresh();
}

/**
 * Processes events until the action is enabled, or a timeout.
 */
bool waitUntilEnabled(QAction* action)
{
  QElapsedTimer timer;
  timer.start();
  while (!action->isEnabled() && timer.elapsed() < 5000)
  {
    QCoreApplication::processEvents();
    QThread::msleep(10);
  }
  return action->isEnabled();
}

/**
 * Checks categories defined by several configurations: scripts with the same
 * name, and a category created without label, then given one.
 */
bool testSeveralConfigurations(QMainWindow& window, QMenu* myToolsMenu)
{
  pqApplicationCore::instance()->loadConfigurationXML(ThirdXML);
  QMenu* menu = findMenu(window.menuBar(), "Other");
  CHECK(menu && menu->title() == "Other");
  CHECK(texts(menu->actions()) == QStringList({ "Create Other Sphere" }));
  QToolBar* toolbar = findToolBar(window, "Other");
  CHECK(toolbar && toolbar->objectName() == "filters.Other");

  // the label is the one given, and the toolbar is the same as its name does
  // not depend on the items of the category.
  pqApplicationCore::instance()->loadConfigurationXML(FourthXML);
  CHECK(findMenu(window.menuBar(), "Other") == menu);
  CHECK(menu->title() == "Other &Tools");
  CHECK(texts(menu->actions()) == QStringList({ "Create Other Sphere", "Sphere" }));
  CHECK(findToolBar(window, "Other") == toolbar && toolbar->objectName() == "filters.Other");

  // scripts with the same name have their own action, running their code.
  QAction* script = menu->actions()[0];
  CHECK(script != myToolsMenu->actions()[3]);
  CHECK(myToolsMenu->actions()[3]->text() == "Create Sphere");
#if VTK_MODULE_ENABLE_ParaView_pqPython
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  script->trigger();
  QCoreApplication::processEvents();
  CHECK(smmodel->findItem<pqPipelineSource*>("OtherSphereFromScript") != nullptr);
#endif
  return true;
}

bool doTest(QMainWindow& window, QMenu* filtersMenu, QMenu* contextMenu,
  QMainWindow& windowWithoutMenuBar, QMenu* otherFiltersMenu)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  pqApplicationCore::instance()->loadConfigurationXML(FirstXML);
  pqApplicationCore::instance()->loadConfigurationXML(SecondXML);

  // the menu is added before the "Help" menu.
  QMenuBar* menubar = window.menuBar();
  QMenu* menu = findMenu(menubar, "MyTools");
  CHECK(menu != nullptr);
  CHECK(menu->title() == "&My Tools");
  CHECK(menubar->actions().size() == 3 && menubar->actions()[1] == menu->menuAction());
  CHECK(findMenu(menubar, "NotInMenuBar") == nullptr);

  // items, in order, followed by sub-menus.
  const QStringList expected = { "Wavelet", "Shrink", "---", "Create Sphere", "More" };
  if (texts(menu->actions()) != expected)
  {
    std::cerr << "Unexpected menu items: " << texts(menu->actions()).join(", ").toStdString()
              << std::endl;
    return false;
  }

  // the sub-menu has items from both configurations.
  QMenu* subMenu = menu->actions().back()->menu();
  CHECK(subMenu != nullptr);
  CHECK(texts(subMenu->actions()) == QStringList({ "Slice", "Contour" }));

  // categories in the menu bar are not in the Filters menu.
  Q_EMIT filtersMenu->aboutToShow();
  CHECK(findMenu(filtersMenu, "MyTools") == nullptr);
  CHECK(findMenu(filtersMenu, "NotInMenuBar") != nullptr);

  // but they are in menus of filters not associated with a menu bar, e.g. the
  // context menu of the pipeline browser, or of a window without menu bar,
  // which is not created.
  Q_EMIT contextMenu->aboutToShow();
  CHECK(findMenu(contextMenu, "MyTools") != nullptr);
  CHECK(windowWithoutMenuBar.menuWidget() == nullptr);
  Q_EMIT otherFiltersMenu->aboutToShow();
  CHECK(findMenu(otherFiltersMenu, "MyTools") != nullptr);

  // the toolbar has the same items, with the sub-menu as a drop-down button.
  // Toolbars of categories that are not in the menu bar only have their items.
  QToolBar* toolbar = findToolBar(window, "MyTools");
  CHECK(toolbar != nullptr);
  CHECK(texts(toolbar->actions()) == expected);
  auto button = qobject_cast<QToolButton*>(toolbar->widgetForAction(toolbar->actions().back()));
  CHECK(button && button->popupMode() == QToolButton::InstantPopup);
  QToolBar* otherToolbar = findToolBar(window, "NotInMenuBar");
  CHECK(otherToolbar && texts(otherToolbar->actions()) == QStringList({ "Sphere" }));

  // sources are enabled, but filters require an input.
  QAction* wavelet = menu->actions()[0];
  QAction* shrink = menu->actions()[1];
  CHECK(wavelet->isEnabled());
  Q_EMIT menu->aboutToShow();
  CHECK(!shrink->isEnabled());

  // create a source using the menu.
  const int numSources = smmodel->findItems<pqPipelineSource*>().size();
  wavelet->trigger();
  QList<pqPipelineSource*> sources = smmodel->findItems<pqPipelineSource*>();
  CHECK(sources.size() == numSources + 1);
  pqPipelineSource* waveletSource = sources.back();
  CHECK(QString(waveletSource->getProxy()->GetXMLName()) == "RTAnalyticSource");
  apply(waveletSource);

  // create a filter using the menu, once enabled for the source.
  CHECK(waitUntilEnabled(shrink));
  shrink->trigger();
  sources = smmodel->findItems<pqPipelineSource*>();
  CHECK(sources.size() == numSources + 2);
  auto shrinkFilter = qobject_cast<pqPipelineFilter*>(sources.back());
  CHECK(shrinkFilter && QString(shrinkFilter->getProxy()->GetXMLName()) == "ShrinkFilter");
  CHECK(shrinkFilter->getInput(0) == waveletSource);

#if VTK_MODULE_ENABLE_ParaView_pqPython
  // run the inline script, once control returns to the event loop, as scripts
  // may e.g. load plugins, which rebuilds the menus.
  menu->actions()[3]->trigger();
  CHECK(smmodel->findItem<pqPipelineSource*>("SphereFromScript") == nullptr);
  QCoreApplication::processEvents();
  CHECK(smmodel->findItem<pqPipelineSource*>("SphereFromScript") != nullptr);
#else
  CHECK(!menu->actions()[3]->isEnabled());
#endif
  return testSeveralConfigurations(window, menu);
}
}

extern int TestCategoryMenus(int argc, char* argv[])
{
  QApplication app(argc, argv);
  pqPVApplicationCore appCore(argc, argv);

  QMainWindow window;
  QMenu* filtersMenu = window.menuBar()->addMenu("&Filters");
  window.menuBar()->addMenu("&Help");
  pqParaViewMenuBuilders::buildFiltersMenu(*filtersMenu, &window);

  // like the "Add Filter" context menu of the pipeline browser.
  QMenu contextMenu;
  pqParaViewMenuBuilders::buildFiltersMenu(contextMenu, nullptr, true, false);

  // e.g. the window of a custom application, without menu bar.
  QMainWindow windowWithoutMenuBar;
  QMenu otherFiltersMenu;
  pqParaViewMenuBuilders::buildFiltersMenu(otherFiltersMenu, &windowWithoutMenuBar, false, false);

  pqObjectBuilder* builder = appCore.getObjectBuilder();
  pqServer* server = builder->createServer(pqServerResource(vtkSMSession::GetBuiltinName()));
  pqActiveObjects::instance().setActiveServer(server);

  const bool success =
    doTest(window, filtersMenu, &contextMenu, windowWithoutMenuBar, &otherFiltersMenu);
  builder->destroyAllProxies(server);
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
