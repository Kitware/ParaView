// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMRenderViewExporterProxy.h"

#include "vtkExporter.h"
#include "vtkJSONSceneExporter.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkSMPropDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSession.h"

#include <map>
#include <set>
#include <string>

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
void vtkSMRenderViewExporterProxy::SetView(vtkSMViewProxy* view)
{
  this->vtkSMExporterProxy::SetView(view);

  // TODO: iterate over props to get every domain instead
  vtkSMProperty* prop = this->GetProperty("Prop");
  vtkSMProperty* prop2 = this->GetProperty("DisableNetwork");
  if (prop)
  {
    vtkSMPropDomain* domain = prop->FindDomain<vtkSMPropDomain>();
    if (domain)
    {
      domain->Update(prop);
    }
  }
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
    auto exportJSON = vtkJSONSceneExporter::SafeDownCast(exporter);
    if (exportJSON)
    {
      // Create prop <-> name association map
      std::map<std::string, vtkActor*> map;
      exportJSON->SetPropMap(map);
    }
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
