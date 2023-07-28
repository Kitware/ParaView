// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "myMainWindow.h"
#include "ui_myMainWindow.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqObjectBuilder.h>
#include <pqRenderView.h>
#include <pqServerResource.h>

//-----------------------------------------------------------------------------
myMainWindow::myMainWindow(QWidget* parentObject, Qt::WindowFlags wflags)
  : Superclass(parentObject, wflags)
{
  Ui::myMainWindow ui;
  ui.setupUi(this);

  // Make a connection to the builtin server
  pqApplicationCore* core = pqApplicationCore::instance();
  core->getObjectBuilder()->createServer(pqServerResource("builtin:"));

  // Create render view
  pqRenderView* view =
    qobject_cast<pqRenderView*>(pqApplicationCore::instance()->getObjectBuilder()->createView(
      pqRenderView::renderViewType(), pqActiveObjects::instance().activeServer()));
  pqActiveObjects::instance().setActiveView(view);

  // Set it as the central widget
  this->setCentralWidget(view->widget());
}

//-----------------------------------------------------------------------------
myMainWindow::~myMainWindow() = default;
