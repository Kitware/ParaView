/*=========================================================================

   Program: ParaView
   Module:    pqCommandLineOptionsBehavior.cxx

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
#include "pqCommandLineOptionsBehavior.h"

#include "pqActiveObjects.h"
#include "pqComponentsTestUtility.h"
#include "pqCoreUtilities.h"
#include "pqDeleteReaction.h"
#include "pqEventDispatcher.h"
#include "pqFileDialog.h"
#include "pqFixPathsInStateFilesBehavior.h"
#include "pqLoadDataReaction.h"
#include "pqLoadStateReaction.h"
#include "pqObjectBuilder.h"
#include "pqOptions.h"
#include "pqPersistentMainWindowStateBehavior.h"
#include "pqPVApplicationCore.h"
#include "pqPythonShellReaction.h"
#include "pqRenderView.h"
#include "pqScalarsToColors.h"
#include "pqServerConnectReaction.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqTimeKeeper.h"
#include "pqUndoStack.h"
#include "pqViewManager.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"

#include <QApplication>
#include <QDebug>
#include <QMainWindow>
#include <QStringList>
#include <QTimer>

//-----------------------------------------------------------------------------
pqCommandLineOptionsBehavior::pqCommandLineOptionsBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QTimer::singleShot(100, this, SLOT(processCommandLineOptions()));
}

//-----------------------------------------------------------------------------
void pqCommandLineOptionsBehavior::processCommandLineOptions()
{
  pqOptions* options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());
  
  // check for --server.
  const char* serverresource_name = options->GetServerResourceName();
  if (serverresource_name)
    {
    pqServerConnectReaction::connectToServer(serverresource_name); 
    if (!pqActiveObjects::instance().activeServer())
      {
      // FIXME: This warning is not showing up since pqAlwaysConnectedBehavior
      // connects to a server. We need to check if active server is not same as
      // requested server, then warn.
      qCritical() << "Could not connect to requested server \"" 
        << serverresource_name 
        << "\". Creating default builtin connection.";
      }
    }

  // Now we are assured that some default server connection has been made
  // (either the one requested by the user on the command line or simply the
  // default one).
  Q_ASSERT(pqActiveObjects::instance().activeServer() != 0);

  // check for --data option.
  if (options->GetParaViewDataName())
    {
    // We don't directly set the data file name instead use the dialog. This
    // makes it possible to select a file group.
    pqFileDialog dialog (
      pqActiveObjects::instance().activeServer(),
      pqCoreUtilities::mainWidget(),
      tr("Internal Open File"), QString(),
      QString());
    dialog.setFileMode(pqFileDialog::ExistingFiles);
    if (!dialog.selectFile(options->GetParaViewDataName()))
      {
      qCritical() << "Cannot open data file \""
        << options->GetParaViewDataName() << "\"";
      }
    QList<QStringList> files = dialog.getAllSelectedFiles();
    QStringList file;
    foreach(file,files)
      {
      if (pqLoadDataReaction::loadData(file) == NULL)
        {
        qCritical() << "Failed to load data file: " <<
          options->GetParaViewDataName();
        }
      }
    }
  else if (options->GetStateFileName())
    {
    // check for --state option. (Bug #5711)
    // NOTE: --data and --state cannnot be specifed at the same time.

    // HACK: We need to disable popping up of the fix-filenames dialog when
    // loading a state file from the command line.
    bool prev = pqFixPathsInStateFilesBehavior::blockDialog(true);
    pqLoadStateReaction::loadState(options->GetStateFileName());
    pqFixPathsInStateFilesBehavior::blockDialog(prev);
    }

  if (options->GetPythonScript())
    {
#ifdef PARAVIEW_ENABLE_PYTHON
    pqPythonShellReaction::executeScript(options->GetPythonScript());
#else
    qCritical() << "Python support not enabled. Cannot run python scripts.";
#endif
    }

  if (options->GetNumberOfTestScripts() > 0)
    {
    QTimer::singleShot(1000, this, SLOT(playTests()));
    }
}

//-----------------------------------------------------------------------------
void pqCommandLineOptionsBehavior::playTests()
{
  pqOptions* options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());

  pqPersistentMainWindowStateBehavior::saveState(
    qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget()));

  bool success = true;
  for (int cc=0; success &&  cc < options->GetNumberOfTestScripts(); cc++)
    {
    if (cc > 0)
      {
      this->resetApplication();
      }

    // Play the test script if specified.
    pqTestUtility* testUtility = pqApplicationCore::instance()->testUtility();
    options->SetCurrentImageThreshold(options->GetTestImageThreshold(cc));
    cout << "Playing: " << options->GetTestScript(cc).toAscii().data() << endl;
    success = testUtility->playTests(options->GetTestScript(cc));

    if (success && !options->GetTestBaseline(cc).isEmpty())
      {
      success = pqComponentsTestUtility::CompareView(
        options->GetTestBaseline(cc), options->GetTestImageThreshold(cc),
        options->GetTestDirectory());
      }
    }

  if (options->GetExitAppWhenTestsDone())
    {
    QApplication::instance()->exit(success? 0 : 1);
    }
}

//-----------------------------------------------------------------------------
void pqCommandLineOptionsBehavior::resetApplication()
{
  BEGIN_UNDO_EXCLUDE();

  // delete all sources and representations
  pqDeleteReaction::deleteAll();

  // delete all views
  QList<pqView*> current_views = 
    pqApplicationCore::instance()->getServerManagerModel()->findItems<pqView*>();
  foreach (pqView* view, current_views)
    {
    pqApplicationCore::instance()->getObjectBuilder()->destroy(view);
    }

  // delete all looktables.
  QList<pqScalarsToColors*> luts = 
    pqApplicationCore::instance()->getServerManagerModel()->findItems<pqScalarsToColors*>();
  foreach (pqScalarsToColors* lut, luts)
    {
    pqApplicationCore::instance()->getObjectBuilder()->destroy(lut);
    }

  // reset view layout.
  pqViewManager* viewManager = qobject_cast<pqViewManager*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_MANAGER"));
  if (viewManager)
    {
    viewManager->reset();
    }
  // create default render view.
  pqApplicationCore::instance()->getObjectBuilder()->createView(
    pqRenderView::renderViewType(),
    pqActiveObjects::instance().activeServer());

  // reset animation time.
  pqActiveObjects::instance().activeServer()->getTimeKeeper()->setTime(0.0);

  // restore panels etc.
  pqPersistentMainWindowStateBehavior::restoreState(
    qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget()));

  pqEventDispatcher::processEventsAndWait(10);

  END_UNDO_EXCLUDE();
  CLEAR_UNDO_STACK();
}
