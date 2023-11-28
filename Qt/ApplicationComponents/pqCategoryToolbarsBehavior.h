// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCategoryToolbarsBehavior_h
#define pqCategoryToolbarsBehavior_h

#include "pqApplicationComponentsModule.h"

#include <QObject>

#include <memory>

#include "vtkParaViewDeprecation.h" // for deprecation macro

class pqProxyGroupMenuManager;
class QMainWindow;
class QAction;

/**
 * @ingroup Behaviors
 * pqCategoryToolbarsBehavior is used when the application wants to enable
 * categories from a pqProxyGroupMenuManager to show up in a toolbar.
 * ex. One may want to have a toolbar listing all the filters in "Common"
 * category. This behavior also ensures that as plugins are loaded, if new
 * categories request that the be added as a toolbar, new toolbars for those
 * are added and also if new items get added to a category already shown as a
 * toolbar, then the toolbar is updated.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCategoryToolbarsBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqCategoryToolbarsBehavior(pqProxyGroupMenuManager* menuManager, QMainWindow* mainWindow);
  ~pqCategoryToolbarsBehavior() override;

protected Q_SLOTS:
  /**
   * Create, delete toolbars.
   */
  void updateToolbars();

  /**
   * This slot gets attached to a pqEventDispatcher so that some toolbars
   * can be hidden before each test starts (to prevent small test-image differences
   * due to layout differences between machines).
   */
  PARAVIEW_DEPRECATED_IN_5_13_0(
    "This was mostly unused. Also it is better to avoid test-dedicated code paths.")
  void prepareForTest(){};

private:
  Q_DISABLE_COPY(pqCategoryToolbarsBehavior)

  class pqInternal;
  std::unique_ptr<pqInternal> Internal;
};

#endif
