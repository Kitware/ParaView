/*=========================================================================

   Program: ParaView
   Module:  pqLoadMaterialsReaction.cxx

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
#include "pqLoadMaterialsReaction.h"
#include "vtkPVConfig.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqServer.h"
#include "pqStandardRecentlyUsedResourceLoaderImplementation.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkNew.h"
#include "vtkSMMaterialLibraryProxy.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"

#include <sstream>

//-----------------------------------------------------------------------------
pqLoadMaterialsReaction::pqLoadMaterialsReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqLoadMaterialsReaction::~pqLoadMaterialsReaction()
{
}

//-----------------------------------------------------------------------------
bool pqLoadMaterialsReaction::loadMaterials()
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  pqFileDialog fileDialog(server, pqCoreUtilities::mainWidget(), tr("Load Materials:"), QString(),
    "OSPRay Material Files (*.json);;Wavefront Material Files (*.mtl)");
  fileDialog.setObjectName("FileOpenDialog");
  fileDialog.setFileMode(pqFileDialog::ExistingFiles);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    return pqLoadMaterialsReaction::loadMaterials(fileDialog.getSelectedFiles(0)[0]);
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqLoadMaterialsReaction::loadMaterials(const QString& dbase, pqServer* server)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  CLEAR_UNDO_STACK();

  server = (server != NULL) ? server : pqActiveObjects::instance().activeServer();

  vtkSMSession* session = server->session();
  if (session)
  {
    vtkNew<vtkSMParaViewPipelineController> controller;
    vtkSMMaterialLibraryProxy* mlp =
      vtkSMMaterialLibraryProxy::SafeDownCast(controller->FindMaterialLibrary(session));
    vtkSMPropertyHelper(mlp, "LoadMaterials").Set(dbase.toLatin1().data());
    mlp->UpdateVTKObjects();
  }

  CLEAR_UNDO_STACK();
  return true;
#else
  (void)dbase;
  (void)server;
  return false;
#endif
}
