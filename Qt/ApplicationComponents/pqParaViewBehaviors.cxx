/*=========================================================================

   Program: ParaView
   Module:    pqParaViewBehaviors.cxx

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
#include "pqParaViewBehaviors.h"

#include "pqAlwaysConnectedBehavior.h"
#include "pqApplicationCore.h"
#include "pqAutoLoadPluginXMLBehavior.h"
#include "pqCommandLineOptionsBehavior.h"
#include "pqCrashRecoveryBehavior.h"
#include "pqDataTimeStepBehavior.h"
#include "pqDefaultViewBehavior.h"
#include "pqDeleteBehavior.h"
#include "pqFixPathsInStateFilesBehavior.h"
#include "pqInterfaceTracker.h"
#include "pqObjectPickingBehavior.h"
#include "pqPersistentMainWindowStateBehavior.h"
#include "pqPipelineContextMenuBehavior.h"
#include "pqPluginActionGroupBehavior.h"
#include "pqPluginDockWidgetsBehavior.h"
#include "pqPVNewSourceBehavior.h"
#include "pqQtMessageHandlerBehavior.h"
#include "pqSpreadSheetVisibilityBehavior.h"
#include "pqStandardViewModules.h"
#include "pqUndoRedoBehavior.h"
#include "pqVerifyRequiredPluginBehavior.h"
#include "pqViewFrameActionsBehavior.h"

#include <QShortcut>
#include <QMainWindow>

//-----------------------------------------------------------------------------
pqParaViewBehaviors::pqParaViewBehaviors(
  QMainWindow* mainWindow, QObject* parentObject)
  : Superclass(parentObject)
{
  // Register ParaView interfaces.
  pqInterfaceTracker* pgm = pqApplicationCore::instance()->interfaceTracker();

  // * adds support for standard paraview views.
  pgm->addInterface(new pqStandardViewModules(pgm));

  // Load plugins distributed with application.
  pqApplicationCore::instance()->loadDistributedPlugins();

  // Define application behaviors.
  new pqQtMessageHandlerBehavior(this);
  new pqDataTimeStepBehavior(this);
  new pqViewFrameActionsBehavior(this);
  new pqSpreadSheetVisibilityBehavior(this);
  new pqPipelineContextMenuBehavior(this);
  new pqDefaultViewBehavior(this);
  new pqAlwaysConnectedBehavior(this);
  new pqPVNewSourceBehavior(this);
  new pqDeleteBehavior(this);
  new pqUndoRedoBehavior(this);
  new pqCrashRecoveryBehavior(this);
  new pqAutoLoadPluginXMLBehavior(this);
  new pqPluginDockWidgetsBehavior(mainWindow);
  new pqVerifyRequiredPluginBehavior(this);
  new pqPluginActionGroupBehavior(mainWindow);
  new pqFixPathsInStateFilesBehavior(this);
  new pqCommandLineOptionsBehavior(this);
  new pqPersistentMainWindowStateBehavior(mainWindow);
  new pqObjectPickingBehavior(this);

  // Setup quick-launch shortcuts.
  QShortcut *ctrlSpace = new QShortcut(Qt::CTRL + Qt::Key_Space,
    mainWindow);
  QObject::connect(ctrlSpace, SIGNAL(activated()),
    pqApplicationCore::instance(), SLOT(quickLaunch()));
  QShortcut *altSpace = new QShortcut(Qt::ALT + Qt::Key_Space,
    mainWindow);
  QObject::connect(altSpace, SIGNAL(activated()),
    pqApplicationCore::instance(), SLOT(quickLaunch()));
}

