/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSaveAnimationExtractsProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSaveAnimationExtractsProxy.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVProgressHandler.h"
#include "vtkSMAnimationScene.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMAnimationSceneWriter.h"
#include "vtkSMExtractsController.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkWeakPointer.h"

namespace
{
class ExtractsWriter : public vtkSMAnimationSceneWriter
{
public:
  static ExtractsWriter* New();
  vtkTypeMacro(ExtractsWriter, vtkSMAnimationSceneWriter);

  void Initialize(vtkSMSaveAnimationExtractsProxy* options)
  {
    this->SetFileName("/tmp/not-used");
    this->ProxyManager = options->GetSessionProxyManager();
    this->Controller->SetExtractsOutputDirectory(
      vtkSMPropertyHelper(options, "ExtractsOutputDirectory").GetAsString());
    this->GenerateCinemaSpecification =
      (vtkSMPropertyHelper(options, "GenerateCinemaSpecification").GetAsInt() != 0);
  }

protected:
  ExtractsWriter() = default;
  ~ExtractsWriter() override = default;

  bool SaveInitialize(int startCount) override
  {
    this->TimeStep = startCount;

    // Animation scene call render on each tick. We override that render call
    // since it's a waste of rendering, the code to save the images will call
    // render anyways.
    this->AnimationScene->SetOverrideStillRender(1);
    return true;
  }

  bool SaveFinalize() override
  {
    this->AnimationScene->SetOverrideStillRender(0);
    if (this->GenerateCinemaSpecification)
    {
      this->Controller->SaveSummaryTable("data.csv", this->ProxyManager);
    }
    return true;
  }

  bool SaveFrame(double time) override
  {
    this->Controller->SetTime(time);
    this->Controller->SetTimeStep(this->TimeStep);
    this->Controller->Extract(this->ProxyManager);
    ++this->TimeStep;
    return true;
  }

private:
  ExtractsWriter(const ExtractsWriter&) = delete;
  void operator=(const ExtractsWriter&) = delete;

  vtkNew<vtkSMExtractsController> Controller;
  vtkSMSessionProxyManager* ProxyManager = nullptr;
  int TimeStep = 0;
  bool GenerateCinemaSpecification = false;
};

vtkStandardNewMacro(ExtractsWriter);
}

vtkStandardNewMacro(vtkSMSaveAnimationExtractsProxy);
//----------------------------------------------------------------------------
vtkSMSaveAnimationExtractsProxy::vtkSMSaveAnimationExtractsProxy()
{
}

//----------------------------------------------------------------------------
vtkSMSaveAnimationExtractsProxy::~vtkSMSaveAnimationExtractsProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMSaveAnimationExtractsProxy::SaveExtracts()
{
  auto session = this->GetSession();

  vtkNew<vtkSMParaViewPipelineController> controller;
  auto scene = controller->FindAnimationScene(session);
  if (!scene)
  {
    vtkErrorMacro("No animation scene found. Cannot generate extracts.");
    return false;
  }

  SM_SCOPED_TRACE(SaveLayoutSizes);
  SM_SCOPED_TRACE(SaveCameras);
  SM_SCOPED_TRACE(SaveAnimationExtracts).arg("proxy", this);

  vtkNew<ExtractsWriter> writer;
  writer->Initialize(this);
  writer->SetAnimationScene(scene);
  // register with progress handler so we monitor progress events.
  session->GetProgressHandler()->RegisterProgressEvent(
    writer.Get(), static_cast<int>(this->GetGlobalID()));
  session->PrepareProgress();
  bool status = writer->Save();
  session->CleanupPendingProgress();
  return status;
}

//----------------------------------------------------------------------------
void vtkSMSaveAnimationExtractsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
