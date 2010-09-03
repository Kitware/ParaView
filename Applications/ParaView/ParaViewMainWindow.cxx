/*=========================================================================

   Program: ParaView
   Module:    ParaViewMainWindow.cxx

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
#include "ParaViewMainWindow.h"
#include "ui_ParaViewMainWindow.h"

#include "pqHelpReaction.h"
#include "pqObjectInspectorWidget.h"
#include "pqParaViewBehaviors.h"
#include "pqParaViewMenuBuilders.h"
#include "vtkPVPlugin.h"
#include "ParaViewVRPN.h"
#include "ParaViewVRUI.h"



class ParaViewMainWindow::pqInternals : public Ui::pqClientMainWindow
{
public:
  ParaViewVRPN *InputDevice;
  ParaViewVRUI *InputDevice2;
};

//-----------------------------------------------------------------------------
ParaViewMainWindow::ParaViewMainWindow()
{
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  // Setup default GUI layout.

  // Set up the dock window corners to give the vertical docks more room.
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  this->Internals->animationViewDock->hide();
  this->Internals->statisticsDock->hide();
  this->Internals->selectionInspectorDock->hide();
  this->Internals->comparativePanelDock->hide();
  this->tabifyDockWidget(this->Internals->animationViewDock,
    this->Internals->statisticsDock);

  // Enable automatic creation of representation on accept.
  this->Internals->proxyTabWidget->setShowOnAccept(true);

  // Enable help for from the object inspector.
  QObject::connect(this->Internals->proxyTabWidget->getObjectInspector(),
    SIGNAL(helpRequested(QString)),
    this, SLOT(showHelpForProxy(const QString&)));

  // Populate application menus with actions.
  pqParaViewMenuBuilders::buildFileMenu(*this->Internals->menu_File);
  pqParaViewMenuBuilders::buildEditMenu(*this->Internals->menu_Edit);

  // Populate sources menu.
  pqParaViewMenuBuilders::buildSourcesMenu(*this->Internals->menuSources, this);

  // Populate filters menu.
  pqParaViewMenuBuilders::buildFiltersMenu(*this->Internals->menuFilters, this);

  // Populate Tools menu.
  pqParaViewMenuBuilders::buildToolsMenu(*this->Internals->menuTools);

  // setup the context menu for the pipeline browser.
  pqParaViewMenuBuilders::buildPipelineBrowserContextMenu(
    *this->Internals->pipelineBrowser);

  pqParaViewMenuBuilders::buildToolbars(*this);

  // Setup the View menu. This must be setup after all toolbars and dockwidgets
  // have been created.
  pqParaViewMenuBuilders::buildViewMenu(*this->Internals->menu_View, *this);

  // Setup the menu to show macros.
  pqParaViewMenuBuilders::buildMacrosMenu(*this->Internals->menu_Macros);

  // Setup the help menu.
  pqParaViewMenuBuilders::buildHelpMenu(*this->Internals->menu_Help);

#if 1
  // VRPN input events.
  this->VRPNTimer=new QTimer(this);
  this->VRPNTimer->setInterval(40); // in ms
  // to define: obj and callback()
  this->Internals->InputDevice=new ParaViewVRPN;
  this->Internals->InputDevice->SetName("Tracker0@localhost");
  this->Internals->InputDevice->Init();
  connect(this->VRPNTimer,SIGNAL(timeout()),
          this->Internals->InputDevice,SLOT(callback()));
  this->VRPNTimer->start();
#endif

#if 1
  // VRUI input events.
  this->VRUITimer=new QTimer(this);
  this->VRUITimer->setInterval(40); // in ms
  // to define: obj and callback()
  this->Internals->InputDevice2=new ParaViewVRUI;
  this->Internals->InputDevice2->SetName("localhost");
  this->Internals->InputDevice2->SetPort(8555);
  this->Internals->InputDevice2->Init();
  connect(this->VRUITimer,SIGNAL(timeout()),
          this->Internals->InputDevice2,SLOT(callback()));
  this->VRUITimer->start();
#endif

  // Final step, define application behaviors. Since we want all ParaView
  // behaviors, we use this convenience method.
  new pqParaViewBehaviors(this, this);
}

//-----------------------------------------------------------------------------
ParaViewMainWindow::~ParaViewMainWindow()
{
  delete this->Internals;
}


//-----------------------------------------------------------------------------
void ParaViewMainWindow::showHelpForProxy(const QString& proxyname)
{
  pqHelpReaction::showHelp(
    QString("qthelp://paraview.org/paraview/%1.html").arg(proxyname));
}
