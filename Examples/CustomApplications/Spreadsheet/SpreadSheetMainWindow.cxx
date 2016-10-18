/*=========================================================================

   Program: ParaView
   Module:    SpreadSheetMainWindow.cxx

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
#include "SpreadSheetMainWindow.h"
#include "ui_SpreadSheetMainWindow.h"

#include "pqActiveObjects.h"
#include "pqActiveObjects.h"
#include "pqAlwaysConnectedBehavior.h"
#include "pqApplicationCore.h"
#include "pqDisplayPolicy.h"
#include "pqLoadDataReaction.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPersistentMainWindowStateBehavior.h"
#include "pqPipelineSource.h"
#include "pqRecentFilesMenu.h"
#include "pqSpreadSheetView.h"
#include "pqSpreadSheetViewDecorator.h"

//-----------------------------------------------------------------------------
SpreadSheetMainWindow::SpreadSheetMainWindow(QWidget* parentObject, Qt::WindowFlags wflags)
  : Superclass(parentObject, wflags)
{
  Ui::SpreadSheetMainWindow ui;
  ui.setupUi(this);

  // Define default behaviors - create only small subset, since this application
  // is not really a ParaView-like application at all.
  new pqAlwaysConnectedBehavior(this);
  new pqPersistentMainWindowStateBehavior(this);

  /// We do need the recent files menu, so set it up.
  new pqRecentFilesMenu(*ui.menuRecent_Datasets, ui.menuRecent_Datasets);

  // Create spread-sheet view and set it as the central Widget.
  pqSpreadSheetView* view =
    qobject_cast<pqSpreadSheetView*>(pqApplicationCore::instance()->getObjectBuilder()->createView(
      pqSpreadSheetView::spreadsheetViewType(), pqActiveObjects::instance().activeServer()));
  this->setCentralWidget(view->widget());
  new pqSpreadSheetViewDecorator(view);

  pqActiveObjects::instance().setActiveView(view);

  // Reactions
  new pqLoadDataReaction(ui.action_Open_Dataset);
  QObject::connect(
    ui.action_Exit, SIGNAL(triggered()), pqApplicationCore::instance(), SLOT(quit()));

  // We need to destroy the previously opened source and show the new one
  // every time a new source is created.
  QObject::connect(pqApplicationCore::instance()->getObjectBuilder(),
    SIGNAL(sourceCreated(pqPipelineSource*)), this, SLOT(showData(pqPipelineSource*)));
}

//-----------------------------------------------------------------------------
SpreadSheetMainWindow::~SpreadSheetMainWindow()
{
}

//-----------------------------------------------------------------------------
void SpreadSheetMainWindow::showData(pqPipelineSource* source)
{
  pqActiveObjects& activeObjects = pqActiveObjects::instance();

  if (activeObjects.activeSource())
  {
    pqApplicationCore::instance()->getObjectBuilder()->destroy(activeObjects.activeSource());
  }
  activeObjects.setActiveSource(source);
  pqApplicationCore::instance()->getDisplayPolicy()->setRepresentationVisibility(
    source->getOutputPort(0), activeObjects.activeView(), true);
}
