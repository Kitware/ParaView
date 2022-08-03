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

#include "vtkCompositeAnimationPlayer.h"
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
vtkSMSaveAnimationExtractsProxy::vtkSMSaveAnimationExtractsProxy() = default;

//----------------------------------------------------------------------------
vtkSMSaveAnimationExtractsProxy::~vtkSMSaveAnimationExtractsProxy() = default;

//----------------------------------------------------------------------------
bool vtkSMSaveAnimationExtractsProxy::SaveExtracts()
{
  auto session = this->GetSession();

  vtkSMProxy* scene = vtkSMPropertyHelper(this, "AnimationScene").GetAsProxy();
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
  writer->SetStride(vtkSMPropertyHelper(this, "FrameStride").GetAsInt());
  // Convert frame window to PlaybackTimeWindow; FrameWindow is an integral
  // value indicating the frame number of timestep; PlaybackTimeWindow is double
  // values as animation time.
  int frameWindow[2] = { 0, 0 };
  vtkSMPropertyHelper(this, "FrameWindow").Get(frameWindow, 2);
  double playbackTimeWindow[2] = { -1, 0 };
  switch (vtkSMPropertyHelper(scene, "PlayMode").GetAsInt())
  {
    case vtkCompositeAnimationPlayer::SEQUENCE:
    {
      int numFrames = vtkSMPropertyHelper(scene, "NumberOfFrames").GetAsInt();
      double startTime = vtkSMPropertyHelper(scene, "StartTime").GetAsDouble();
      double endTime = vtkSMPropertyHelper(scene, "EndTime").GetAsDouble();
      frameWindow[0] = frameWindow[0] < 0 ? 0 : frameWindow[0];
      frameWindow[1] = frameWindow[1] >= numFrames ? numFrames - 1 : frameWindow[1];
      playbackTimeWindow[0] =
        startTime + ((endTime - startTime) * frameWindow[0]) / (numFrames - 1);
      playbackTimeWindow[1] =
        startTime + ((endTime - startTime) * frameWindow[1]) / (numFrames - 1);
    }
    break;
    case vtkCompositeAnimationPlayer::SNAP_TO_TIMESTEPS:
    {
      vtkSMProxy* timeKeeper = vtkSMPropertyHelper(scene, "TimeKeeper").GetAsProxy();
      vtkSMPropertyHelper tsValuesHelper(timeKeeper, "TimestepValues");
      int numTS = tsValuesHelper.GetNumberOfElements();
      frameWindow[0] = frameWindow[0] < 0 ? 0 : frameWindow[0];
      frameWindow[1] = frameWindow[1] >= numTS ? numTS - 1 : frameWindow[1];
      playbackTimeWindow[0] = tsValuesHelper.GetAsDouble(frameWindow[0]);
      playbackTimeWindow[1] = tsValuesHelper.GetAsDouble(frameWindow[1]);
    }

    break;
    case vtkCompositeAnimationPlayer::REAL_TIME:
      // this should not happen. vtkSMSaveAnimationProxy::Prepare() should have
      // changed the play mode to SEQUENCE or SNAP_TO_TIMESTEPS.
      abort();
  }
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
