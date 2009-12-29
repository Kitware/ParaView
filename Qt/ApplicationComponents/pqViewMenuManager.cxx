/*=========================================================================

   Program: ParaView
   Module:    pqViewMenuManager.cxx

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
#include "pqViewMenuManager.h"

#include "pqPVApplicationCore.h"
#include "pqSetName.h"
#include "pqViewManager.h"

#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>
#include <QToolBar>

//-----------------------------------------------------------------------------
pqViewMenuManager::pqViewMenuManager(QMainWindow* mainWindow, QMenu* menu)
  : Superclass(mainWindow)
{
  Q_ASSERT(mainWindow != NULL);
  Q_ASSERT(menu != NULL);

  this->Menu = menu;
  this->Window = mainWindow;

  // essential to ensure that the full screen shortcut is setup correctly.
  this->buildMenu();
  
  QObject::connect(menu, SIGNAL(aboutToShow()), this, SLOT(buildMenu()));
}

//-----------------------------------------------------------------------------
void pqViewMenuManager::buildMenu()
{
  this->Menu->clear();
  QList<QMenu*> child_menus = this->Menu->findChildren<QMenu*>();
  foreach (QMenu* menu, child_menus)
    {
    delete menu;
    }

  QMenu* toolbars = this->Menu->addMenu("Toolbars")
    << pqSetName("Toolbars");
  QList<QToolBar*> all_toolbars = this->Window->findChildren<QToolBar*>();
  foreach (QToolBar* toolbar, all_toolbars)
    {
    toolbars->addAction(toolbar->toggleViewAction());
    }

  this->Menu->addSeparator();
  foreach (QDockWidget* dock_widget, this->Window->findChildren<QDockWidget*>())
    {
    this->Menu->addAction(dock_widget->toggleViewAction());
    }

  this->Menu->addSeparator();

  pqViewManager* viewManager = qobject_cast<pqViewManager*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_MANAGER"));
  if (viewManager)
    {
    QAction* fullscreen = this->Menu->addAction("Full Screen");
    fullscreen->setObjectName("actionFullScreen");
    fullscreen->setShortcut(QKeySequence("F11"));
    QObject::connect(fullscreen, SIGNAL(triggered()),
      viewManager, SLOT(toggleFullScreen()));
    }
}


