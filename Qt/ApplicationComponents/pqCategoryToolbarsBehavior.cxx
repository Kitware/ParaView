/*=========================================================================

   Program: ParaView
   Module:    pqCategoryToolbarsBehavior.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqCategoryToolbarsBehavior.h"

#include "pqApplicationCore.h"
#include "pqEventDispatcher.h"
#include "pqEventTranslator.h"
#include "pqProxyGroupMenuManager.h"
#include "pqTestUtility.h"

#include <QMainWindow>
#include <QToolBar>

#include <cassert>
#include <iostream>

//-----------------------------------------------------------------------------
pqCategoryToolbarsBehavior::pqCategoryToolbarsBehavior(
  pqProxyGroupMenuManager* menuManager, QMainWindow* mainWindow)
  : Superclass(menuManager)
{
  assert(menuManager != 0);
  assert(mainWindow != 0);

  this->MainWindow = mainWindow;
  this->MenuManager = menuManager;

  // When tests start, hide toolbars that have asked to be off by default.
  // Do the same when starting to record events for a test.
  pqTestUtility* testUtil = pqApplicationCore::instance()->testUtility();
  pqEventDispatcher* testPlayer = testUtil ? testUtil->dispatcher() : NULL;
  pqEventTranslator* testRecorder = testUtil ? testUtil->eventTranslator() : NULL;
  if (testPlayer)
  {
    QObject::connect(testPlayer, SIGNAL(restarted()), this, SLOT(prepareForTest()));
  }
  if (testRecorder)
  {
    QObject::connect(testRecorder, SIGNAL(started()), this, SLOT(prepareForTest()));
  }

  QObject::connect(menuManager, SIGNAL(menuPopulated()), this, SLOT(updateToolbars()));
  this->updateToolbars();
}

//-----------------------------------------------------------------------------
void pqCategoryToolbarsBehavior::updateToolbars()
{
  QStringList toolbarCategories = this->MenuManager->getToolbarCategories();
  foreach (QString category, toolbarCategories)
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
    QList<QAction*> toolbarActions = this->MenuManager->actions(category);
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

void pqCategoryToolbarsBehavior::prepareForTest()
{
  foreach (QAction* toolbar, this->ToolbarsToHide)
  {
    if (toolbar && toolbar->isChecked())
    {
      toolbar->trigger();
    }
  }
}
