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

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqObjectBuilder.h>
#include <pqParaViewBehaviors.h>
#include <pqRecentFilesMenu.h>
#include <pqSpreadSheetView.h>
#include <pqSpreadSheetViewDecorator.h>
#include <vtkPVGeneralSettings.h>

#include "pqSpreadsheetLoadDataReaction.h"

//-----------------------------------------------------------------------------
class SpreadSheetMainWindow::pqInternals : public Ui::pqClientMainWindow
{
};

//-----------------------------------------------------------------------------
SpreadSheetMainWindow::SpreadSheetMainWindow()
  : Internals(new pqInternals())
{
  // Setup default GUI layout.
  this->Internals->setupUi(this);

  // Create a custom file menu with only Open and close
  QList<QAction*> qa = this->Internals->menu_File->actions();
  QAction* mqa = qa.at(0);
  new pqSpreadsheetLoadDataReaction(mqa);
  new pqRecentFilesMenu(*this->Internals->menuRecent_Files, this->Internals->menuRecent_Files);
  QObject::connect(
    qa.at(2), SIGNAL(triggered()), QApplication::instance(), SLOT(closeAllWindows()));

  new pqParaViewBehaviors(this, this);

  // Use auto apply to show data automatically
  vtkPVGeneralSettings::GetInstance()->SetAutoApply(true);

  // Create spread-sheet view and set it as the central Widget.
  pqSpreadSheetView* view =
    qobject_cast<pqSpreadSheetView*>(pqApplicationCore::instance()->getObjectBuilder()->createView(
      pqSpreadSheetView::spreadsheetViewType(), pqActiveObjects::instance().activeServer()));
  this->setCentralWidget(view->widget());
  new pqSpreadSheetViewDecorator(view);
}

//-----------------------------------------------------------------------------
SpreadSheetMainWindow::~SpreadSheetMainWindow() = default;
