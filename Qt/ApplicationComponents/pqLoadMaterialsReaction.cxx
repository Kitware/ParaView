// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLoadMaterialsReaction.h"

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
pqLoadMaterialsReaction::~pqLoadMaterialsReaction() = default;

//-----------------------------------------------------------------------------
bool pqLoadMaterialsReaction::loadMaterials()
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  pqFileDialog fileDialog(server, pqCoreUtilities::mainWidget(), tr("Load Materials:"), QString(),
    tr("OSPRay Material Files") + QString(" (*.json);;") + tr("Wavefront Material Files") +
      QString(" (*.mtl)"),
    false);
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

  server = (server != nullptr) ? server : pqActiveObjects::instance().activeServer();

  vtkSMSession* session = server->session();
  if (session)
  {
    vtkNew<vtkSMParaViewPipelineController> controller;
    vtkSMMaterialLibraryProxy* mlp =
      vtkSMMaterialLibraryProxy::SafeDownCast(controller->FindMaterialLibrary(session));
    vtkSMPropertyHelper(mlp, "LoadMaterials").Set(dbase.toUtf8().data());
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
