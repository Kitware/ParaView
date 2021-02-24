/*=========================================================================

   Program: ParaView
   Module:    pqSaveExtractsReaction.cxx

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
#include "pqSaveExtractsReaction.h"

#include "pqActiveObjects.h"
#include "pqAnimationProgressDialog.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqProgressManager.h"
#include "pqProxyWidgetDialog.h"
#include "pqProxyWidgetDialog.h"
#include "vtkNew.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMSaveAnimationExtractsProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

//-----------------------------------------------------------------------------
pqSaveExtractsReaction::pqSaveExtractsReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqSaveExtractsReaction::~pqSaveExtractsReaction() = default;

//-----------------------------------------------------------------------------
bool pqSaveExtractsReaction::generateExtracts()
{
  vtkNew<vtkSMParaViewPipelineController> controller;
  auto pxm = pqActiveObjects::instance().proxyManager();
  auto proxy = vtkSmartPointer<vtkSMProxy>::Take(pxm->NewProxy("misc", "SaveAnimationExtracts"));
  auto exporter = vtkSMSaveAnimationExtractsProxy::SafeDownCast(proxy);
  if (exporter)
  {
    controller->PreInitializeProxy(exporter);
    controller->PostInitializeProxy(exporter);

    pqProxyWidgetDialog dialog(exporter, pqCoreUtilities::mainWidget());
    dialog.setWindowTitle("Save Extracts Options");
    dialog.setSettingsKey("SaveAnimationExtracts");
    if (dialog.exec() == QDialog::Accepted)
    {
      pqAnimationProgressDialog progress(
        "Save Extracts Progress", "Abort", 0, 100, pqCoreUtilities::mainWidget());
      progress.setWindowTitle("Saving Extracts ...");
      progress.setAnimationScene(controller->FindAnimationScene(pxm->GetSession()));
      progress.show();

      auto appcore = pqApplicationCore::instance();
      auto pgm = appcore->getProgressManager();
      // this is essential since pqProgressManager blocks all interaction
      // events when progress events are pending. since we have a QProgressDialog
      // as modal, we don't need to that. Plus, we want the cancel button on the
      // dialog to work.
      const auto prev = pgm->unblockEvents(true);
      const auto status = exporter->SaveExtracts();
      pgm->unblockEvents(prev);
      return status;
    }
  }
  return false;
}
