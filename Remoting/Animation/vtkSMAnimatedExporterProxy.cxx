// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMAnimatedExporterProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMAnimationSceneSeriesWriter.h"
#include "vtkSMExporterProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"

vtkStandardNewMacro(vtkSMAnimatedExporterProxy);

//----------------------------------------------------------------------------
bool vtkSMAnimatedExporterProxy::CanExport(vtkSMProxy* proxy)
{
  auto realExporter = vtkSMExporterProxy::SafeDownCast(this->GetSubProxy("Exporter"));
  if (realExporter && proxy)
  {
    return realExporter->CanExport(proxy);
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkSMAnimatedExporterProxy::SetView(vtkSMViewProxy* view)
{
  this->Superclass::SetView(view);

  this->CreateVTKObjects();
  auto realExporter = vtkSMExporterProxy::SafeDownCast(this->GetSubProxy("Exporter"));
  if (realExporter)
  {
    realExporter->SetView(this->GetView());
  }
}

//----------------------------------------------------------------------------
void vtkSMAnimatedExporterProxy::Write()
{
  this->CreateVTKObjects();

  auto realExporter = vtkSMExporterProxy::SafeDownCast(this->GetSubProxy("Exporter"));
  auto sceneWriter = vtkSMAnimationSceneWriter::SafeDownCast(this->GetClientSideObject());
  if (realExporter && sceneWriter)
  {
    sceneWriter->SetFrameExporterDelegate(realExporter);
  }

  sceneWriter->Save();
}

//----------------------------------------------------------------------------
void vtkSMAnimatedExporterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
