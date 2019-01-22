/*=========================================================================

   Program: ParaView
   Module:  pqImportCinemaReaction.cxx

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
#include "pqImportCinemaReaction.h"
#include "vtkPVConfig.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqServer.h"
#include "pqStandardRecentlyUsedResourceLoaderImplementation.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkNew.h"

#if VTK_MODULE_ENABLE_ParaView_CinemaReader
#include "vtkSMCinemaDatabaseImporter.h"
#endif

#include <sstream>

//-----------------------------------------------------------------------------
pqImportCinemaReaction::pqImportCinemaReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(
    activeObjects, SIGNAL(serverChanged(pqServer*)), this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqImportCinemaReaction::~pqImportCinemaReaction()
{
}

//-----------------------------------------------------------------------------
void pqImportCinemaReaction::updateEnableState()
{
#if VTK_MODULE_ENABLE_ParaView_CinemaReader
  bool enable_state = false;
  pqActiveObjects& activeObjects = pqActiveObjects::instance();
  vtkSMSession* session =
    activeObjects.activeServer() ? activeObjects.activeServer()->session() : NULL;
  if (session)
  {
    vtkNew<vtkSMCinemaDatabaseImporter> importer;
    enable_state = importer->SupportsCinema(session);
  }
  this->parentAction()->setEnabled(enable_state);
#else
  this->parentAction()->setEnabled(true);
#endif
}

//-----------------------------------------------------------------------------
bool pqImportCinemaReaction::loadCinemaDatabase()
{
#if VTK_MODULE_ENABLE_ParaView_CinemaReader
  pqServer* server = pqActiveObjects::instance().activeServer();
  pqFileDialog fileDialog(server, pqCoreUtilities::mainWidget(), tr("Open Cinema Database:"),
    QString(), "Cinema Database Files (info.json);;All files(*)");
  fileDialog.setObjectName("FileOpenDialog");
  fileDialog.setFileMode(pqFileDialog::ExistingFiles);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    return pqImportCinemaReaction::loadCinemaDatabase(fileDialog.getSelectedFiles(0)[0]);
  }
  return false;
#else
  pqCoreUtilities::promptUser("pqImportCinemaReaction::NoPython", QMessageBox::Critical,
    tr("Python support not enabled"), tr("Python support is required to load a Cinema database, "
                                         "but is not available in this build."),
    QMessageBox::Ok | QMessageBox::Save);
  return false;
#endif
}

//-----------------------------------------------------------------------------
bool pqImportCinemaReaction::loadCinemaDatabase(const QString& dbase, pqServer* server)
{
#if VTK_MODULE_ENABLE_ParaView_CinemaReader
  CLEAR_UNDO_STACK();

  server = (server != NULL) ? server : pqActiveObjects::instance().activeServer();
  pqView* view = pqActiveObjects::instance().activeView();

  vtkNew<vtkSMCinemaDatabaseImporter> importer;
  if (!importer->ImportCinema(
        dbase.toLatin1().data(), server->session(), (view ? view->getViewProxy() : NULL)))
  {
    qCritical("Failed to import Cinema database.");
    return false;
  }

  pqStandardRecentlyUsedResourceLoaderImplementation::addCinemaDatabaseToRecentResources(
    server, dbase);

  if (view)
  {
    view->render();
  }

  CLEAR_UNDO_STACK();
  return true;
#else
  (void)dbase;
  (void)server;
  return false;
#endif
}
