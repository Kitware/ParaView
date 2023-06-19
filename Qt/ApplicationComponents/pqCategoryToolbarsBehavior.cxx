// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCategoryToolbarsBehavior.h"

#include "pqApplicationCore.h"
#include "pqEventDispatcher.h"
#include "pqEventTranslator.h"
#include "pqProxyGroupMenuManager.h"

#include <QMainWindow>
#include <QToolBar>

#include <cassert>

//-----------------------------------------------------------------------------
pqCategoryToolbarsBehavior::pqCategoryToolbarsBehavior(
  pqProxyGroupMenuManager* menuManager, QMainWindow* mainWindow)
  : Superclass(menuManager)
{
  assert(menuManager != 0);
  assert(mainWindow != 0);

  this->MainWindow = mainWindow;
  this->MenuManager = menuManager;

  QObject::connect(menuManager, SIGNAL(menuPopulated()), this, SLOT(updateToolbars()));
  this->updateToolbars();
}

//-----------------------------------------------------------------------------
void pqCategoryToolbarsBehavior::updateToolbars()
{
  QStringList toolbarCategories = this->MenuManager->getToolbarCategories();
  for (const QString& category : toolbarCategories)
  {
    QToolBar* toolbar = this->MainWindow->findChild<QToolBar*>(category);
    if (!toolbar)
    {
      if (category == "Common")
      {
        this->MainWindow->addToolBarBreak();
      }
      toolbar = new QToolBar(this->MainWindow);
      toolbar->setObjectName(category);
      toolbar->setOrientation(Qt::Horizontal);
      QString categoryLabel = this->MenuManager->categoryLabel(category);
      toolbar->setWindowTitle(categoryLabel.size() > 0 ? categoryLabel : category);
      this->MainWindow->addToolBar(toolbar);
      if (this->MenuManager->hideForTests(category))
      {
        this->ToolbarsToHide << toolbar->toggleViewAction();
      }
    }
    QList<QAction*> toolbarActions = this->MenuManager->categoryActions(category);
    toolbar->clear();
    for (int cc = 0; cc < toolbarActions.size(); cc++)
    {
      QVariant omitList = toolbarActions[cc]->property("OmitFromToolbar");
      if (omitList.isValid() && omitList.toStringList().contains(category))
      {
        continue;
      }
      toolbar->addAction(toolbarActions[cc]);
    }
  }
}
