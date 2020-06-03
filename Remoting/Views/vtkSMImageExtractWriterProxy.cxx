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

#include "vtkObjectFactory.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMExtractsController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSaveScreenshotProxy.h"
#include "vtkSMViewProxy.h"

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
  if (view)
  {
    vtkSMPropertyHelper helper(view, "ViewTime");
    old_time = helper.GetAsDouble();
    helper.Set(extractor->GetTime());
    view->UpdateVTKObjects();
  }

  if (view != nullptr && vtkSMPropertyHelper(this, "ResetDisplay").GetAsInt() == 1)
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

  auto convertedname = this->GenerateImageExtractsFileName(fname, extractor);
  const bool status = writer->WriteImage(convertedname.c_str());
  if (old_time != VTK_DOUBLE_MAX && view != nullptr)
  {
    vtkSMPropertyHelper(view, "ViewTime").Set(old_time);
    view->UpdateVTKObjects();
  }

  return status;
}

//----------------------------------------------------------------------------
void vtkSMImageExtractWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
