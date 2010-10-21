/*=========================================================================

   Program: ParaView
   Module:    pqSaveStateReaction.cxx

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
#include "pqSaveStateReaction.h"
#include "vtkPVConfig.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqPVApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqServer.h"
#include "pqServerResource.h"
#include "pqServerResources.h"

#ifdef PARAVIEW_ENABLE_PYTHON
#include "pqPythonManager.h"
#endif

//-----------------------------------------------------------------------------
pqSaveStateReaction::pqSaveStateReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  // save state enable state depends on whether we are connected to an active
  // server or not and whether
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(activeObjects, SIGNAL(serverChanged(pqServer*)),
    this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqSaveStateReaction::updateEnableState()
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  this->parentAction()->setEnabled(activeObjects->activeServer() != NULL);
}

//-----------------------------------------------------------------------------
void pqSaveStateReaction::saveState()
{
#ifdef PARAVIEW_ENABLE_PYTHON
  QString fileExt = tr("ParaView state file (*.pvsm);;Python state file (*.py);;All files (*)");
#else
  QString fileExt = tr("ParaView state file (*.pvsm);;All files (*)");
#endif
  pqFileDialog fileDialog(NULL,
                          pqCoreUtilities::mainWidget(),
                          tr("Save State File"), QString(), fileExt);

  fileDialog.setObjectName("FileSaveServerStateDialog");
  fileDialog.setFileMode(pqFileDialog::AnyFile);

  if (fileDialog.exec() == QDialog::Accepted)
    {
    QString selectedFile = fileDialog.getSelectedFiles()[0];
    if(selectedFile.endsWith(".py"))
      {
      pqSaveStateReaction::savePythonState(selectedFile);
      }
    else
      {
      pqSaveStateReaction::saveState(selectedFile);
      }
    }
}

//-----------------------------------------------------------------------------
void pqSaveStateReaction::saveState(const QString& filename)
{
  pqApplicationCore::instance()->saveState(filename);
  pqServer *server = pqActiveObjects::instance().activeServer();
  // Add this to the list of recent server resources ...
  pqServerResource resource;
  resource.setScheme("session");
  resource.setPath(filename);
  resource.setSessionServer(server->getResource());
  pqApplicationCore::instance()->serverResources().add(resource);
  pqApplicationCore::instance()->serverResources().save(
    *pqApplicationCore::instance()->settings());
}
//-----------------------------------------------------------------------------
void pqSaveStateReaction::savePythonState(const QString& filename)
{
#ifdef PARAVIEW_ENABLE_PYTHON
  pqPythonManager *pythonManager = pqPVApplicationCore::instance()->pythonManager();
  if(!pythonManager)
    {
    qCritical("No application wide python manager.");
    return;
    }
  pythonManager->saveTraceState(filename);
#endif
}
