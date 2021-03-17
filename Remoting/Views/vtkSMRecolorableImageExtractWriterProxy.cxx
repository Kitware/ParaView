/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRecolorableImageExtractWriterProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRecolorableImageExtractWriterProxy.h"

#include "vtkDataObject.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkProcessModule.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSaveScreenshotProxy.h"
#include "vtkSMSessionClient.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

#include <cassert>
#include <vector>

namespace
{

class vtkHelper
{
  vtkPVRenderView* RenderView = nullptr;
  std::vector<vtkSMProxy*> Representations;

public:
  vtkHelper() = default;
  ~vtkHelper()
  {
    if (this->RenderView)
    {
      this->RenderView->EndValuePassForRendering();
    }

    for (const auto& repr : this->Representations)
    {
      vtkSMPropertyHelper(repr, "Visibility").Set(1);
      repr->UpdateVTKObjects();
    }
  }

  bool Initialize(vtkSMViewProxy* view, vtkSMRecolorableImageExtractWriterProxy* self)
  {
    auto renderView = vtkPVRenderView::SafeDownCast(view->GetClientSideObject());
    if (!renderView)
    {
      vtkLogF(ERROR, "Only 'vtkPVRenderView' proxies are currently supported.");
      return false;
    }

    auto source =
      vtkSMSourceProxy::SafeDownCast(vtkSMPropertyHelper(self, "DataSource").GetAsProxy());
    auto port = vtkSMPropertyHelper(self, "DataSource").GetOutputPort();
    if (source == nullptr)
    {
      vtkLogF(ERROR, "'DataSource' must be specified.");
      return false;
    }

    auto repr = vtkSMPVRepresentationProxy::SafeDownCast(view->FindRepresentation(source, port));
    if (!repr || vtkSMPropertyHelper(repr, "Visibility").GetAsInt() == 0)
    {
      vtkLogF(ERROR, "'DataSource' is not visible in view!");
      return false;
    }

    if (!repr->GetUsingScalarColoring())
    {
      vtkLogF(ERROR, "Chosen 'DataSource' is not using scalar coloring.");
      return false;
    }

    vtkSMPropertyHelper reprHelper(view, "Representations");
    for (unsigned int cc = 0, max = reprHelper.GetNumberOfElements(); cc < max; ++cc)
    {
      auto arepr = reprHelper.GetAsProxy(cc);
      if (arepr && arepr != repr && vtkSMPropertyHelper(arepr, "Visibility").GetAsInt() == 1)
      {
        vtkSMPropertyHelper(arepr, "Visibility").Set(0);
        arepr->UpdateVTKObjects();
        this->Representations.push_back(arepr);
      }
    }

    int component = 0; // FIXME: pick the component to export.
    vtkSMPropertyHelper colorArrayName(repr, "ColorArrayName");
    if (renderView->BeginValuePassForRendering(colorArrayName.GetInputArrayAssociation(),
          colorArrayName.GetInputArrayNameToProcess(), component))
    {
      this->RenderView = renderView;
      return true;
    }
    else
    {
      vtkLogF(ERROR, "BeginValuePassForRendering failed!");
    }
    return false;
  }
};
}

vtkStandardNewMacro(vtkSMRecolorableImageExtractWriterProxy);
//----------------------------------------------------------------------------
vtkSMRecolorableImageExtractWriterProxy::vtkSMRecolorableImageExtractWriterProxy() = default;

//----------------------------------------------------------------------------
vtkSMRecolorableImageExtractWriterProxy::~vtkSMRecolorableImageExtractWriterProxy() = default;

//----------------------------------------------------------------------------
bool vtkSMRecolorableImageExtractWriterProxy::WriteInternal(
  vtkSMExtractsController* extractor, const SummaryParametersT& params)
{
  auto session = this->GetSession();
  if (vtkSMSessionClient::SafeDownCast(session))
  {
    vtkErrorMacro("This extractor does not support client-server confugurations yet.");
    return false;
  }

  auto pm = vtkProcessModule::GetProcessModule();
  assert(pm != nullptr);
  if (pm->GetNumberOfLocalPartitions() > 1 && !pm->GetSymmetricMPIMode())
  {
    vtkErrorMacro("This extractor does not support non-symmetric parallel execution yet.");
    return false;
  }

  auto writer = this->GetSubProxy("Writer");
  assert(writer != nullptr);

  auto view = vtkSMViewProxy::SafeDownCast(vtkSMPropertyHelper(writer, "View").GetAsProxy());
  assert(view != nullptr);

  vtkHelper helper;
  if (!helper.Initialize(view, this))
  {
    return false;
  }

  return this->Superclass::WriteInternal(extractor, params);
}

//----------------------------------------------------------------------------
void vtkSMRecolorableImageExtractWriterProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }

  this->Superclass::CreateVTKObjects();

  if (!this->ObjectsCreated)
  {
    return;
  }

  // a small hack to make vtkSMSaveScreenshotProxy save floating point buffers
  // instead of standard images.
  if (auto writer = vtkSMSaveScreenshotProxy::SafeDownCast(this->GetSubProxy("Writer")))
  {
    writer->SetUseFloatingPointBuffers(true);
  }
}

//----------------------------------------------------------------------------
void vtkSMRecolorableImageExtractWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
