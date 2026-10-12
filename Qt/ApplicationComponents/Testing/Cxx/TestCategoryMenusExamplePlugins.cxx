// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

// Tests the example plugins in Examples/Plugins/CustomMenus, which add items to
// the same menu and toolbar using an XML-only plugin and a Python plugin. The
// directory containing the examples is provided using the
// CUSTOM_MENUS_EXAMPLE_DIR environment variable.

#include "pqActiveObjects.h"
#include "pqAutoLoadPluginXMLBehavior.h"
#include "pqObjectBuilder.h"
#include "pqPVApplicationCore.h"
#include "pqParaViewMenuBuilders.h"
#include "pqPipelineFilter.h"
#include "pqPluginManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkPythonInterpreter.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"

#include <QAction>
#include <QApplication>
#include <QElapsedTimer>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QStringList>
#include <QThread>
#include <QToolBar>

#include <iostream>

namespace
{
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

QAction* findAction(QMenu* menu, const QString& text)
{
  for (QAction* action : menu->actions())
  {
    if (action->text() == text)
    {
      return action;
    }
  }
  return nullptr;
}

/**
 * Processes events until the action is enabled, or a timeout.
 */
bool waitUntilEnabled(QAction* action)
{
  QElapsedTimer timer;
  timer.start();
  while (action && !action->isEnabled() && timer.elapsed() < 5000)
  {
    QCoreApplication::processEvents();
    QThread::msleep(10);
  }
  return action && action->isEnabled();
}

/**
 * Creates a source or filter using the action, then applies it the same way as
 * pqApplyBehavior: the pipeline is updated, as the data is shown, before the
 * source is marked as unmodified.
 */
pqPipelineSource* createUsing(QAction* action)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  const int count = smmodel->findItems<pqPipelineSource*>().size();
  action->trigger();
  QList<pqPipelineSource*> sources = smmodel->findItems<pqPipelineSource*>();
  if (sources.size() != count + 1)
  {
    return nullptr;
  }
  pqPipelineSource* source = sources.back();
  pqActiveObjects::instance().setActiveSource(source);
  source->updatePipeline();
  source->setModifiedState(pqProxy::UNMODIFIED);
  Q_EMIT pqApplicationCore::instance() -> forceFilterMenuRefresh();
  return source;
}

bool doTest(QMainWindow& window, pqServer* server, const QString& exampleDir)
{
  pqPluginManager* pluginManager = pqApplicationCore::instance()->getPluginManager();
  CHECK(pluginManager->loadExtension(server, exampleDir + "/AnalysisTools.xml", nullptr, false) ==
    pqPluginManager::LOADED);
  CHECK(pluginManager->loadExtension(server, exampleDir + "/AnalysisToolsPython.py", nullptr,
          false) == pqPluginManager::LOADED);

  // both plugins add items to the same menu, followed by the sub-menus.
  QMenu* menu = nullptr;
  for (QAction* action : window.menuBar()->actions())
  {
    if (action->menu() && action->menu()->objectName() == "Analysis Tools")
    {
      menu = action->menu();
    }
  }
  CHECK(menu != nullptr);
  const QStringList expected = { "Wavelet", "---", "Slice", "Surface With Normals", "---",
    "Print Data Summary", "Normalized Height", "Color By Normalized Height", "Display", "Reports" };
  if (texts(menu->actions()) != expected)
  {
    std::cerr << "Unexpected menu items: " << texts(menu->actions()).join(", ").toStdString()
              << std::endl;
    return false;
  }

  // and to the same toolbar.
  QToolBar* toolbar = nullptr;
  for (QToolBar* candidate : window.findChildren<QToolBar*>())
  {
    if (candidate->objectName().endsWith(".Analysis Tools"))
    {
      toolbar = candidate;
    }
  }
  CHECK(toolbar != nullptr);
  CHECK(texts(toolbar->actions()) == expected);

  // create a source, and filters from the XML-only and Python plugins.
  pqPipelineSource* wavelet = createUsing(findAction(menu, "Wavelet"));
  CHECK(wavelet != nullptr);

  // filters from both plugins are enabled once the source is applied.
  CHECK(waitUntilEnabled(findAction(menu, "Normalized Height")));
  CHECK(waitUntilEnabled(findAction(menu, "Surface With Normals")));
  auto surface =
    qobject_cast<pqPipelineFilter*>(createUsing(findAction(menu, "Surface With Normals")));
  CHECK(surface && surface->getInput(0) == wavelet);
  CHECK(QString(surface->getProxy()->GetXMLName()) == "SurfaceWithNormals");

  pqActiveObjects::instance().setActiveSource(wavelet);
  CHECK(waitUntilEnabled(findAction(menu, "Normalized Height")));
  auto height = qobject_cast<pqPipelineFilter*>(createUsing(findAction(menu, "Normalized Height")));
  CHECK(height && height->getInput(0) == wavelet);
  CHECK(QString(height->getProxy()->GetXMLName()) == "NormalizedHeight");

  // Python actions call the functions from the plugin module.
  CHECK(vtkPythonInterpreter::RunSimpleString(
          "import AnalysisToolsPython as m\n"
          "m.calls = []\n"
          "m.print_array_ranges = lambda: m.calls.append('print_array_ranges')\n") == 0);
  QMenu* reports = findAction(menu, "Reports")->menu();
  CHECK(reports != nullptr && reports->actions().size() == 1);
  reports->actions()[0]->trigger();
  // scripts are run once control returns to the event loop.
  QCoreApplication::processEvents();
  CHECK(vtkPythonInterpreter::RunSimpleString(
          "import AnalysisToolsPython as m\n"
          "assert m.calls == ['print_array_ranges'], m.calls\n") == 0);
  return true;
}
}

extern int TestCategoryMenusExamplePlugins(int argc, char* argv[])
{
  const QString exampleDir = QString::fromLocal8Bit(qgetenv("CUSTOM_MENUS_EXAMPLE_DIR"));
  if (exampleDir.isEmpty())
  {
    std::cerr << "CUSTOM_MENUS_EXAMPLE_DIR must be set." << std::endl;
    return EXIT_FAILURE;
  }

  QApplication app(argc, argv);
  pqPVApplicationCore appCore(argc, argv);

  QMainWindow window;
  QMenu* filtersMenu = window.menuBar()->addMenu("&Filters");
  window.menuBar()->addMenu("&Help");
  pqParaViewMenuBuilders::buildFiltersMenu(*filtersMenu, &window);
  new pqAutoLoadPluginXMLBehavior(&window);

  pqObjectBuilder* builder = appCore.getObjectBuilder();
  pqServer* server = builder->createServer(pqServerResource(vtkSMSession::GetBuiltinName()));
  pqActiveObjects::instance().setActiveServer(server);

  const bool success = doTest(window, server, exampleDir);
  builder->destroyAllProxies(server);
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
