// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMSaveAnimationExtractsProxy.h"

#include "vtkCompositeAnimationPlayer.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVProgressHandler.h"
#include "vtkSMAnimationScene.h"
#include "vtkSMAnimationSceneWriter.h"
#include "vtkSMExtractsController.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"

#include <algorithm>

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
    this->AnimationScene->SetOverrideStillRender(true);
    return true;
  }

  bool SaveFinalize() override
  {
    this->AnimationScene->SetOverrideStillRender(false);
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
vtkSMSaveAnimationExtractsProxy::vtkSMSaveAnimationExtractsProxy() = default;

//----------------------------------------------------------------------------
vtkSMSaveAnimationExtractsProxy::~vtkSMSaveAnimationExtractsProxy() = default;

//----------------------------------------------------------------------------
bool vtkSMSaveAnimationExtractsProxy::SaveExtracts()
{
  auto session = this->GetSession();

  vtkSMProxy* sceneProxy = vtkSMPropertyHelper(this, "AnimationScene").GetAsProxy();
  if (!sceneProxy)
  {
    vtkErrorMacro("No animation scene found. Cannot generate extracts.");
    return false;
  }

  SM_SCOPED_TRACE(SaveLayoutSizes);
  SM_SCOPED_TRACE(SaveCameras);
  SM_SCOPED_TRACE(SaveAnimationExtracts).arg("proxy", this);

  vtkNew<ExtractsWriter> writer;
  writer->Initialize(this);
  writer->SetAnimationScene(sceneProxy);
  writer->SetStride(vtkSMPropertyHelper(this, "FrameStride").GetAsInt());
  // Convert frame window to PlaybackTimeWindow; FrameWindow is an integral
  // value indicating the frame number of timestep; PlaybackTimeWindow is double
  // values as animation time.
  int frameWindow[2] = { 0, 0 };
  vtkSMPropertyHelper(this, "FrameWindow").Get(frameWindow, 2);
  double playbackTimeWindow[2] = { -1, 0 };
  vtkSMAnimationScene* scene = vtkSMAnimationScene::SafeDownCast(sceneProxy->GetClientSideObject());
  scene->SanitizeFrameWindow(frameWindow, playbackTimeWindow);
  writer->SetStartFileCount(frameWindow[0]);
  writer->SetPlaybackTimeWindow(playbackTimeWindow);

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
