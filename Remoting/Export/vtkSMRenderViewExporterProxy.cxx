// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMRenderViewExporterProxy.h"

#include "vtkExporter.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSession.h"

vtkStandardNewMacro(vtkSMRenderViewExporterProxy);
//----------------------------------------------------------------------------
vtkSMRenderViewExporterProxy::vtkSMRenderViewExporterProxy() = default;

//----------------------------------------------------------------------------
vtkSMRenderViewExporterProxy::~vtkSMRenderViewExporterProxy() = default;

//----------------------------------------------------------------------------
bool vtkSMRenderViewExporterProxy::CanExport(vtkSMProxy* view)
{
  return (view && view->IsA("vtkSMRenderViewProxy"));
}

//----------------------------------------------------------------------------
void vtkSMRenderViewExporterProxy::Write()
{
  this->CreateVTKObjects();
  vtkExporter* exporter = vtkExporter::SafeDownCast(this->GetClientSideObject());
  vtkSMRenderViewProxy* rv = vtkSMRenderViewProxy::SafeDownCast(this->View);

  this->View->GetSession()->PrepareProgress();
  if (exporter && rv)
  {
    int old_threshold = -1;
    if (rv->GetProperty("RemoteRenderThreshold"))
    {
      vtkSMPropertyHelper helper(rv, "RemoteRenderThreshold");
      old_threshold = helper.GetAsInt();
      helper.Set(VTK_INT_MAX);
      rv->StillRender();
    }

    vtkRenderWindow* renWin = rv->GetRenderWindow();
    exporter->SetRenderWindow(renWin);
    exporter->SetActiveRenderer(renWin->GetRenderers()->GetFirstRenderer());
    exporter->Write();
    exporter->SetRenderWindow(nullptr);
    if (rv->GetProperty("RemoteRenderThreshold"))
    {
      vtkSMPropertyHelper(rv, "RemoteRenderThreshold").Set(old_threshold);
    }
  }
  this->View->GetSession()->CleanupPendingProgress();
}

//----------------------------------------------------------------------------
void vtkSMRenderViewExporterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
