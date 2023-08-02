// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
