// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMImageExtractWriterProxy.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkPVStringFormatter.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMExtractsController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSaveScreenshotProxy.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkStringFormatter.h"
#include "vtkVector.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <utility>

namespace
{
class ScopedCamera
{
  vtkVector3d FocalPoint;
  vtkVector3d Position;
  vtkVector3d ViewUp;
  double ParallelScale;
  vtkCamera* Camera;

public:
  ScopedCamera(vtkCamera* camera)
    : Camera(camera)
  {
    camera->GetFocalPoint(this->FocalPoint.GetData());
    camera->GetPosition(this->Position.GetData());
    camera->GetViewUp(this->ViewUp.GetData());
    this->ParallelScale = camera->GetParallelScale();
  }

  ~ScopedCamera() { this->CopyTo(this->Camera); }

  vtkCamera* GetCamera() const { return this->Camera; }

  void Restore() { this->CopyTo(this->Camera); }
  void CopyTo(vtkCamera* camera)
  {
    camera->SetFocalPoint(this->FocalPoint.GetData());
    camera->SetPosition(this->Position.GetData());
    camera->SetViewUp(this->ViewUp.GetData());
    camera->SetParallelScale(this->ParallelScale);
  }

  void ResetPhiTheta()
  {
    this->Restore();
    const auto fdistance = (this->FocalPoint - this->Position).Norm();
    auto pos = this->FocalPoint + vtkVector3d(0, 0, fdistance);
    this->Camera->SetPosition(pos.GetData());
    this->Camera->SetViewUp(0, 1.0, 0);
  }
};

class ScopedViewTime
{
  std::vector<std::pair<vtkSMProxy*, double>> Times;

public:
  ScopedViewTime(const std::vector<vtkSMViewProxy*>& views, double newTime)
  {
    for (auto view : views)
    {
      vtkSMPropertyHelper helper(view, "ViewTime");
      this->Times.emplace_back(view, helper.GetAsDouble());
      helper.Set(newTime);
      view->UpdateVTKObjects();
    }
  }

  ~ScopedViewTime()
  {
    for (const auto& viewTime : this->Times)
    {
      vtkSMPropertyHelper(viewTime.first, "ViewTime").Set(viewTime.second);
      viewTime.first->UpdateVTKObjects();
    }
  }

  ScopedViewTime(const ScopedViewTime&) = delete;
  void operator=(const ScopedViewTime&) = delete;
};
}

vtkStandardNewMacro(vtkSMImageExtractWriterProxy);
//----------------------------------------------------------------------------
vtkSMImageExtractWriterProxy::vtkSMImageExtractWriterProxy() = default;

//----------------------------------------------------------------------------
vtkSMImageExtractWriterProxy::~vtkSMImageExtractWriterProxy() = default;

//----------------------------------------------------------------------------
void vtkSMImageExtractWriterProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }

  this->Superclass::CreateVTKObjects();

  if (!this->ObjectsCreated || this->IsPrototype())
  {
    return;
  }

  if (auto writer = vtkSMSaveScreenshotProxy::SafeDownCast(this->GetSubProxy("Writer")))
  {
    writer->UpdateDefaultsAndVisibilities(vtkSMPropertyHelper(this, "FileName").GetAsString());
  }
}

//----------------------------------------------------------------------------
bool vtkSMImageExtractWriterProxy::CanExtract(vtkSMProxy* proxy)
{
  return vtkSMViewProxy::SafeDownCast(proxy) != nullptr;
}

//----------------------------------------------------------------------------
bool vtkSMImageExtractWriterProxy::IsExtracting(vtkSMProxy* proxy)
{
  vtkSMPropertyHelper viewHelper(this, "View");
  return (viewHelper.GetAsProxy() == proxy);
}

//----------------------------------------------------------------------------
void vtkSMImageExtractWriterProxy::SetInput(vtkSMProxy* proxy)
{
  if (proxy == nullptr)
  {
    vtkErrorMacro("Input cannot be nullptr.");
    return;
  }

  vtkSMPropertyHelper viewHelper(this, "View");
  viewHelper.Set(proxy);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMImageExtractWriterProxy::GetInput()
{
  return vtkSMPropertyHelper(this, "View").GetAsProxy();
}

//----------------------------------------------------------------------------
bool vtkSMImageExtractWriterProxy::Write(vtkSMExtractsController* extractor)
{
  auto fname = vtkSMPropertyHelper(this, "FileName").GetAsString();
  if (!fname)
  {
    vtkErrorMacro("Missing \"FileName\"!");
    return false;
  }

  auto writer = vtkSMSaveScreenshotProxy::SafeDownCast(this->GetSubProxy("Writer"));
  if (!writer)
  {
    vtkErrorMacro("Missing writer sub proxy.");
    return false;
  }

  const auto views = this->GetViewsToCapture();
  if (views.empty())
  {
    return false;
  }

  const ScopedViewTime viewState(views, extractor->GetTime());

  // cameras are restored after the capture so that views are left unchanged,
  // e.g. when generating extracts from the GUI.
  std::vector<std::unique_ptr<ScopedCamera>> cameraStates;
  if (vtkSMPropertyHelper(this, "ResetDisplay").GetAsInt() == 1)
  {
    for (auto view : views)
    {
      if (auto rv = vtkSMRenderViewProxy::SafeDownCast(view))
      {
        cameraStates.push_back(std::make_unique<ScopedCamera>(rv->GetActiveCamera()));
        rv->ResetCamera();
      }
      else if (auto cv = vtkSMContextViewProxy::SafeDownCast(view))
      {
        cv->ResetDisplay();
      }
    }
  }

  return this->WriteInternal(extractor);
}

//----------------------------------------------------------------------------
std::vector<vtkSMViewProxy*> vtkSMImageExtractWriterProxy::GetViewsToCapture()
{
  auto writer = this->GetSubProxy("Writer");
  auto view = writer
    ? vtkSMViewProxy::SafeDownCast(vtkSMPropertyHelper(writer, "View").GetAsProxy())
    : nullptr;
  if (!view)
  {
    vtkErrorMacro("No view provided to generate extract from!");
    return {};
  }

  if (vtkSMPropertyHelper(writer, "SaveAllViews").GetAsInt() == 0)
  {
    return { view };
  }

  // same layout as the one vtkSMSaveScreenshotProxy captures when saving all views.
  auto layout =
    vtkSMViewLayoutProxy::SafeDownCast(vtkSMPropertyHelper(writer, "Layout").GetAsProxy());
  if (!layout)
  {
    layout = vtkSMViewLayoutProxy::FindLayout(view);
  }
  if (!layout)
  {
    vtkErrorMacro("'SaveAllViews' is enabled but the view is not assigned to any layout!");
    return {};
  }
  return layout->GetViews();
}

//----------------------------------------------------------------------------
bool vtkSMImageExtractWriterProxy::WriteInternal(vtkSMExtractsController* extractor,
  const vtkSMImageExtractWriterProxy::SummaryParametersT& params)
{
  if (vtkSMPropertyHelper(this, "CameraMode").GetAsInt() ==
    vtkSMImageExtractWriterProxy::CameraMode::PhiTheta)
  {
    // move the camera in every render view being captured; other views, if
    // any, are left unchanged.
    const auto views = this->GetViewsToCapture();
    std::vector<std::unique_ptr<ScopedCamera>> cameraStates;
    for (auto view : views)
    {
      if (auto rv = vtkSMRenderViewProxy::SafeDownCast(view))
      {
        cameraStates.push_back(std::make_unique<ScopedCamera>(rv->GetActiveCamera()));
      }
    }
    if (cameraStates.empty())
    {
      if (views.size() == 1)
      {
        vtkErrorMacro("PhiTheta camera mode is not supported on this view of type '"
          << views[0]->GetXMLName() << "'");
      }
      else
      {
        vtkErrorMacro("PhiTheta camera mode requires at least one render view.");
      }
      return false;
    }

    const int phi_resolution = vtkSMPropertyHelper(this, "PhiResolution").GetAsInt();
    if (phi_resolution < 1 || phi_resolution > 360)
    {
      vtkErrorMacro("PhiResolution must be in range [1, 360]");
      return false;
    }

    const int theta_resolution = vtkSMPropertyHelper(this, "ThetaResolution").GetAsInt();
    if (theta_resolution < 1 || theta_resolution > 360)
    {
      vtkErrorMacro("ThetaResolution must be in range [1, 360]");
      return false;
    }

    for (double phi = 0; phi < 360; phi += 360.0 / phi_resolution)
    {
      for (auto& cameraState : cameraStates)
      {
        cameraState->ResetPhiTheta();
        cameraState->GetCamera()->Azimuth(phi);
      }
      for (double theta = 0; theta < 360; theta += 360.0 / theta_resolution)
      {
        const double elevation = theta > 0.0 ? (360.0 / theta_resolution) : 0.0;
        for (auto& cameraState : cameraStates)
        {
          auto camera = cameraState->GetCamera();
          camera->Elevation(elevation);
          camera->OrthogonalizeViewUp();
        }

        vtkSMExtractsController::SummaryParametersT tparams = params;
        char buffer[128];
        auto result = vtk::format_to_n(buffer, 128, "{:06.2f}", phi);
        *result.out = '\0';
        tparams["phi"] = buffer;

        result = vtk::format_to_n(buffer, 128, "{:06.2f}", theta);
        *result.out = '\0';
        tparams["theta"] = buffer;

        if (!this->WriteImage(extractor, tparams))
        {
          return false;
        }
      }
    }

    return true;
  }
  else
  {
    return this->WriteImage(extractor, params);
  }
}

//----------------------------------------------------------------------------
bool vtkSMImageExtractWriterProxy::WriteImage(
  vtkSMExtractsController* extractor, const SummaryParametersT& cameraParams)
{
  auto fname = vtkSMPropertyHelper(this, "FileName").GetAsString();
  if (!fname)
  {
    vtkErrorMacro("Missing \"FileName\"!");
    return false;
  }

  auto writer = vtkSMSaveScreenshotProxy::SafeDownCast(this->GetSubProxy("Writer"));
  if (!writer)
  {
    vtkErrorMacro("Missing writer sub proxy.");
    return false;
  }

  auto view = vtkSMViewProxy::SafeDownCast(vtkSMPropertyHelper(writer, "View").GetAsProxy());
  if (!view)
  {
    vtkErrorMacro("No view provided to generate extract from!");
    return false;
  }

  std::ostringstream sparams;
  for (const auto& pair : cameraParams)
  {
    sparams << this->GetShortName(pair.first) << "=" << pair.second;
  }
  // define scope of arguments with camera params
  PV_STRING_FORMATTER_NAMED_SCOPE("EXTRACT", fmt::arg("camera", sparams.str()));

  auto convertedName =
    this->GenerateExtractsFileName(fname, extractor->GetRealExtractsOutputDirectory());

  const bool status = writer->WriteImage(convertedName.c_str(), vtkPVSession::DATA_SERVER_ROOT);
  if (status)
  {
    // add to summary
    extractor->AddSummaryEntry(this, convertedName, cameraParams);
  }
  return status;
}

//----------------------------------------------------------------------------
const char* vtkSMImageExtractWriterProxy::GetShortName(const std::string& key) const
{
  if (key == "phi")
  {
    return "p";
  }
  else if (key == "theta")
  {
    return "t";
  }
  return key.c_str();
}

//----------------------------------------------------------------------------
void vtkSMImageExtractWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
