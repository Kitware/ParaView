// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMRenderViewExporterProxy.h"

#include "vtkCompositeRepresentation.h"
#include "vtkDataSet.h"
#include "vtkExporter.h"
#include "vtkGeometryRepresentation.h"
#include "vtkJSONSceneExporter.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkPVLODActor.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkSMPropDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
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

  vtkSmartPointer<vtkSMPropertyIterator> iter;
  iter.TakeReference(this->NewPropertyIterator());
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    vtkSMPropDomain* domain = iter->GetProperty()->FindDomain<vtkSMPropDomain>();
    if (domain)
    {
      domain->Update(iter->GetProperty());
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

    vtkRenderWindow* renWin = rv->GetRenderWindow(); // Uses get client-side object

    vtkRenderer* renderer = renWin->GetRenderers()->GetFirstRenderer();
    exporter->SetRenderWindow(renWin);
    exporter->SetActiveRenderer(renderer);
    auto namedActorMap = this->GetNamedActorMap(rv);
    exporter->SetNamedActorsMap(namedActorMap);
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
std::map<std::string, vtkActor*> vtkSMRenderViewExporterProxy::GetNamedActorMap(
  vtkSMRenderViewProxy* rv)
{
  std::map<std::string, vtkActor*> map;

  vtkSMPropertyHelper helper(rv, "Representations");
  std::map<vtkDataObject*, std::string> objectNames;
  for (unsigned int cc = 0, max = helper.GetNumberOfElements(); cc < max; ++cc)
  {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(helper.GetAsProxy(cc));

    if (!repr)
    {
      continue;
    }

    vtkSMPropertyHelper inputHelper(repr, "Input");
    vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy());

    vtkCompositeRepresentation* compInstance =
      vtkCompositeRepresentation::SafeDownCast(repr->GetClientSideObject());
    if (compInstance->GetVisibility())
    {
      auto dataObj = compInstance->GetRenderedDataObject(0);
      if (!dataObj)
      {
        continue;
      }
      vtkPVDataRepresentation* repr2 = compInstance->GetActiveRepresentation();

      if (vtkGeometryRepresentation::SafeDownCast(repr2))
      {
        vtkActor* actor =
          vtkActor::SafeDownCast(vtkGeometryRepresentation::SafeDownCast(repr2)->GetActor());
        vtkWarningMacro(<< dataObj << " " << input->GetLogName());
        map.insert({ input->GetLogName(), actor });
      }
    }
  }

  return map;
}

//----------------------------------------------------------------------------
void vtkSMRenderViewExporterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
