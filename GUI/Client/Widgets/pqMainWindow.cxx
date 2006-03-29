/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

=========================================================================*/

#include "pqMainWindow.h"

#include <pqConnect.h>
#include <pqSetName.h>

#include <QAction>
#include <QDockWidget>
#include <QEvent>
#include <QMenu>
#include <QMenuBar>

///////////////////////////////////////////////////////////////////////////
// pqMainWindow::pqImplementation

/// Private implementation details for pqMainWindow
class pqMainWindow::pqImplementation
{
public:
  pqImplementation() :
    FileMenu(0),
    ViewMenu(0)
  {
  }

  /// Stores the (optional) standard "File" menu
  QMenu* FileMenu;
  /// Stores the (optional) standard "View" menu
  QMenu* ViewMenu;
  /// Stores a mapping from dockable widgets to menu actions
  QMap<QDockWidget*, QAction*> DockWidgetVisibleActions;
};

///////////////////////////////////////////////////////////////////////////
// pqMainWindow

pqMainWindow::pqMainWindow() :
  Implementation(new pqImplementation())
{
  this->setObjectName("mainWindow");
  this->setWindowTitle("ParaQ");
}

pqMainWindow::~pqMainWindow()
{
  delete Implementation;
}

QMenu* pqMainWindow::createFileMenu()
{
  if(!this->Implementation->FileMenu)
    {
    this->Implementation->FileMenu = this->menuBar()->addMenu(tr("&File"));
    }
    
  return this->Implementation->FileMenu;
}

QMenu* pqMainWindow::createViewMenu()
{
  if(!this->Implementation->ViewMenu)
    {
    this->Implementation->ViewMenu = this->menuBar()->addMenu(tr("&View"));
    }
    
  return this->Implementation->ViewMenu;
}

void pqMainWindow::simpleAddDockWidget(Qt::DockWidgetArea area, QDockWidget* dockwidget, const QIcon& icon, bool visible)
{
  dockwidget->setVisible(visible);

  this->addDockWidget(area, dockwidget);
  
  QAction* const action = icon.isNull() ?
    this->createViewMenu()->addAction(dockwidget->windowTitle()) :
    this->createViewMenu()->addAction(icon, dockwidget->windowTitle());
    
  action << pqSetName(dockwidget->windowTitle());
  action->setCheckable(true);
  action->setChecked(visible);
  action << pqConnect(SIGNAL(triggered(bool)), dockwidget, SLOT(setVisible(bool)));
    
  this->Implementation->DockWidgetVisibleActions[dockwidget] = action;
    
  dockwidget->installEventFilter(this);
}

bool pqMainWindow::eventFilter(QObject* watched, QEvent* e)
{
  if(e->type() == QEvent::Hide || e->type() == QEvent::Show)
    {
    if(QDockWidget* const dockwidget = qobject_cast<QDockWidget*>(watched))
      {
      if(this->Implementation->DockWidgetVisibleActions.contains(dockwidget))
        {
        this->Implementation->DockWidgetVisibleActions[dockwidget]->setChecked(e->type() == QEvent::Show);
        }
      }
    }

  return QMainWindow::eventFilter(watched, e);
}
