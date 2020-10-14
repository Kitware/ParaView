/*=========================================================================

  Program:   ParaView
  Module:    vtkSMImageExtractWriterProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMImageExtractWriterProxy.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMExtractsController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSaveScreenshotProxy.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <algorithm>
#include <sstream>

namespace
{
class ScopedCamera
{
  vtkVector3d FocalPoint;
  vtkVector3d Position;
  vtkVector3d ViewUp;
  vtkCamera* Camera;

public:
  ScopedCamera(vtkCamera* camera)
    : Camera(camera)
  {
    camera->GetFocalPoint(this->FocalPoint.GetData());
    camera->GetPosition(this->Position.GetData());
    camera->GetViewUp(this->ViewUp.GetData());
  }

  ~ScopedCamera() { this->CopyTo(this->Camera); }

  void Restore() { this->CopyTo(this->Camera); }
  void CopyTo(vtkCamera* camera)
  {
    camera->SetFocalPoint(this->FocalPoint.GetData());
    camera->SetPosition(this->Position.GetData());
    camera->SetViewUp(this->ViewUp.GetData());
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
  double Time;
  vtkSMProxy* View;

public:
  ScopedViewTime(vtkSMProxy* view, double newTime)
    : View(view)
  {
    vtkSMPropertyHelper helper(this->View, "ViewTime");
    this->Time = helper.GetAsDouble();
    helper.Set(newTime);
    this->View->UpdateVTKObjects();
  }

  ~ScopedViewTime()
  {
    vtkSMPropertyHelper(this->View, "ViewTime").Set(this->Time);
    this->View->UpdateVTKObjects();
  }
};
}

vtkStandardNewMacro(vtkSMImageExtractWriterProxy);
//----------------------------------------------------------------------------
vtkSMImageExtractWriterProxy::vtkSMImageExtractWriterProxy()
{
}

//----------------------------------------------------------------------------
vtkSMImageExtractWriterProxy::~vtkSMImageExtractWriterProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMImageExtractWriterProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }

  this->Superclass::CreateVTKObjects();
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

  auto view = vtkSMViewProxy::SafeDownCast(vtkSMPropertyHelper(writer, "View").GetAsProxy());
  if (!view)
  {
    vtkErrorMacro("No view provided to generate extract from!");
    return false;
  }

  const ScopedViewTime viewState(view, extractor->GetTime());

  if (vtkSMPropertyHelper(this, "ResetDisplay").GetAsInt() == 1)
  {
    // BUG: currently, if we have more than 1 extract for the same view,
    // and only 1 of them resetting the display, we don't have a means to restore
    // the display faithfully. need to fix that.
    if (auto rv = vtkSMRenderViewProxy::SafeDownCast(view))
    {
      rv->ResetCamera();
    }
    else if (auto cv = vtkSMContextViewProxy::SafeDownCast(view))
    {
      cv->ResetDisplay();
    }
  }

  return this->WriteInternal(extractor);
}

//----------------------------------------------------------------------------
bool vtkSMImageExtractWriterProxy::WriteInternal(vtkSMExtractsController* extractor,
  const vtkSMImageExtractWriterProxy::SummaryParametersT& params)
{
  auto writer = vtkSMSaveScreenshotProxy::SafeDownCast(this->GetSubProxy("Writer"));
  assert(writer != nullptr);

  auto view = vtkSMViewProxy::SafeDownCast(vtkSMPropertyHelper(writer, "View").GetAsProxy());
  assert(view != nullptr);

  if (vtkSMPropertyHelper(this, "CameraMode").GetAsInt() ==
    vtkSMImageExtractWriterProxy::CameraMode::PhiTheta)
  {
    auto rv = vtkSMRenderViewProxy::SafeDownCast(view);
    if (rv == nullptr)
    {
      vtkErrorMacro("PhiTheta camera mode is not supported on this view of type '"
        << view->GetXMLName() << "'");
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

    auto camera = rv->GetActiveCamera();
    ScopedCamera cameraState(camera);

    for (double phi = 0; phi < 360; phi += 360.0 / phi_resolution)
    {
      cameraState.ResetPhiTheta();
      camera->Azimuth(phi);
      for (double theta = 0; theta < 360; theta += 360.0 / theta_resolution)
      {
        const double elevation = theta > 0.0 ? (360.0 / theta_resolution) : 0.0;
        camera->Elevation(elevation);
        camera->OrthogonalizeViewUp();

        vtkSMExtractsController::SummaryParametersT tparams = params;
        char buffer[128];
        std::snprintf(buffer, 128, "%06.2f", phi);
        tparams["phi"] = buffer;

        std::snprintf(buffer, 128, "%06.2f", theta);
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
  auto convertedname = this->GenerateImageExtractsFileName(fname, sparams.str(), extractor);
  const bool status = writer->WriteImage(convertedname.c_str(), vtkPVSession::DATA_SERVER_ROOT);
  if (status)
  {
    // add to summary
    extractor->AddSummaryEntry(this, convertedname, cameraParams);
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
