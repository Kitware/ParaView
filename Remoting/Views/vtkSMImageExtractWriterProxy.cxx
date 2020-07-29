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
#include "vtkSMContextViewProxy.h"
#include "vtkSMExtractsController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSaveScreenshotProxy.h"

#include <sstream>

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

  // Let's create the output directory.
  if (!extractor->CreateImageExtractsOutputDirectory())
  {
    return false;
  }

  double old_time = VTK_DOUBLE_MAX;
  auto view = vtkSMViewProxy::SafeDownCast(vtkSMPropertyHelper(writer, "View").GetAsProxy());
  if (!view)
  {
    vtkErrorMacro("No view provided to generate extract from!");
    return false;
  }

  vtkSMPropertyHelper helper(view, "ViewTime");
  old_time = helper.GetAsDouble();
  helper.Set(extractor->GetTime());
  view->UpdateVTKObjects();

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

    for (int phi = 0; phi <= 360; phi += 360 / phi_resolution)
    {
      auto camera = rv->GetActiveCamera();
      camera->Azimuth(phi_resolution);
      camera->OrthogonalizeViewUp();
      for (int theta = 0; theta <= 360; theta += 360 / theta_resolution)
      {
        camera->Elevation(theta_resolution);
        camera->OrthogonalizeViewUp();
        if (phi == 360 || theta == 360)
        {
          // no need to write this image since it's same as the one with phi=0
          // or theta=0.
          continue;
        }
        std::ostringstream str;
        str << "p=" << phi << "t=" << theta;
        auto convertedname = this->GenerateImageExtractsFileName(fname, str.str(), extractor);
        const bool status = writer->WriteImage(convertedname.c_str());
        if (old_time != VTK_DOUBLE_MAX && view != nullptr)
        {
          vtkSMPropertyHelper(view, "ViewTime").Set(old_time);
          view->UpdateVTKObjects();
        }
        if (!status)
        {
          return false;
        }
      }
    }
    return true;
  }
  else
  {
    auto convertedname = this->GenerateImageExtractsFileName(fname, extractor);
    const bool status = writer->WriteImage(convertedname.c_str());
    if (old_time != VTK_DOUBLE_MAX && view != nullptr)
    {
      vtkSMPropertyHelper(view, "ViewTime").Set(old_time);
      view->UpdateVTKObjects();
    }
    return status;
  }
}

//----------------------------------------------------------------------------
void vtkSMImageExtractWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
