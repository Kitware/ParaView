/*=========================================================================

   Program: ParaView
   Module:    pqPluginDockWidgetsBehavior.cxx

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
#include "pqPluginDockWidgetsBehavior.h"

#include "pqApplicationCore.h"
#include "pqDockWindowInterface.h"
#include "pqInterfaceTracker.h"

#include <QDockWidget>
#include <QMainWindow>

//-----------------------------------------------------------------------------
pqPluginDockWidgetsBehavior::pqPluginDockWidgetsBehavior(QMainWindow* parentObject)
  : Superclass(parentObject)
{
  pqInterfaceTracker* pm = pqApplicationCore::instance()->interfaceTracker();
  QObject::connect(
    pm, SIGNAL(interfaceRegistered(QObject*)), this, SLOT(addPluginInterface(QObject*)));
  foreach (QObject* iface, pm->interfaces())
  {
    this->addPluginInterface(iface);
  }
}

//-----------------------------------------------------------------------------
void pqPluginDockWidgetsBehavior::addPluginInterface(QObject* iface)
{
  pqDockWindowInterface* dwi = qobject_cast<pqDockWindowInterface*>(iface);
  if (!dwi)
  {
    return;
  }
  QMainWindow* mainWindow = qobject_cast<QMainWindow*>(this->parent());
  if (!mainWindow)
  {
    qWarning("Could not find MainWindow. Cannot load dock widgets from the plugin.");
    return;
  }

  // Get the dock area.
  QString area = dwi->dockArea();
  Qt::DockWidgetArea dArea = Qt::LeftDockWidgetArea;
  if (area.compare("Right", Qt::CaseInsensitive) == 0)
  {
    dArea = Qt::RightDockWidgetArea;
  }
  else if (area.compare("Top", Qt::CaseInsensitive) == 0)
  {
    dArea = Qt::TopDockWidgetArea;
  }
  else if (area.compare("Bottom", Qt::CaseInsensitive) == 0)
  {
    dArea = Qt::BottomDockWidgetArea;
  }

  // Create the dock window.
  QDockWidget* dock = dwi->dockWindow(mainWindow);
  mainWindow->addDockWidget(dArea, dock);
}
