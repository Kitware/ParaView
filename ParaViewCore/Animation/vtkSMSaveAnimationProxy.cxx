/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSaveAnimationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSaveAnimationProxy.h"

#include "vtkCompositeAnimationPlayer.h"
#include "vtkErrorCode.h"
#include "vtkGenericMovieWriter.h"
#include "vtkImageData.h"
#include "vtkImageWriter.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVProgressHandler.h"
#include "vtkPVRenderingCapabilitiesInformation.h"
#include "vtkPVServerInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMAnimationScene.h"
#include "vtkSMAnimationSceneWriter.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMSessionClient.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"

#include <sstream>
#include <vtksys/SystemTools.hxx>

namespace vtkSMSaveAnimationProxyNS
{

class SceneGrabber
{
public:
  static vtkSmartPointer<vtkImageData> Grab(vtkSMSaveAnimationProxy* proxy)
  {
    return proxy ? proxy->CapturePreppedImage() : vtkSmartPointer<vtkImageData>();
  }
};

template <class T>
class SceneImageWriter : public vtkSMAnimationSceneWriter
{
  vtkSmartPointer<T> Writer;
  vtkWeakPointer<vtkSMSaveAnimationProxy> Helper;

public:
  vtkTemplateTypeMacro(SceneImageWriter, vtkSMAnimationSceneWriter);
  /**
   * Set the vtkSMSaveAnimationProxy proxy.
   */
  void SetHelper(vtkSMSaveAnimationProxy* helper) { this->Helper = helper; }

  /**
   * Set the writer to use.
   */
  void SetWriter(T* writer) { this->Writer = writer; }
  T* GetWriter() { return this->Writer; }

protected:
  SceneImageWriter() {}
  ~SceneImageWriter() {}
  bool SaveInitialize(int vtkNotUsed(startCount)) override
  {
    // Animation scene call render on each tick. We override that render call
    // since it's a waste of rendering, the code to save the images will call
    // render anyways.
    this->AnimationScene->SetOverrideStillRender(1);
    return true;
  }

  bool SaveFrame(double time) override
  {
    vtkSmartPointer<vtkImageData> image = SceneGrabber::Grab(this->Helper);

    // Now, in symmetric batch mode, while this method will get called on all
    // ranks, we really only to save the image on root node.
    // Note, the call to CapturePreppedImage() still needs to happen on all
    // ranks, since otherwise we may get mismatched renders.
    vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
    if (image == nullptr || (controller && controller->GetLocalProcessId() != 0))
    {
      // don't actually save anything on this rank.
      return true;
    }

    return this->WriteFrameImage(time, image);
  }

  bool SaveFinalize() override
  {
    this->AnimationScene->SetOverrideStillRender(0);
    return true;
  }

  virtual bool WriteFrameImage(double time, vtkImageData* data) = 0;

private:
  SceneImageWriter(const SceneImageWriter&) = delete;
  void operator=(const SceneImageWriter&) = delete;
};

class SceneImageWriterMovie : public SceneImageWriter<vtkGenericMovieWriter>
{
public:
  static SceneImageWriterMovie* New();
  vtkTypeMacro(SceneImageWriterMovie, SceneImageWriter<vtkGenericMovieWriter>);

protected:
  SceneImageWriterMovie()
    : Started(false)
  {
  }
  ~SceneImageWriterMovie() {}

  bool SaveInitialize(int startCount) override
  {
    if (auto* writer = this->GetWriter())
    {
      writer->SetFileName(this->GetFileName());
      return this->Superclass::SaveInitialize(startCount);
    }
    this->Started = false;
    return false;
  }

  bool WriteFrameImage(double vtkNotUsed(time), vtkImageData* data) override
  {
    assert(data);
    auto* writer = this->GetWriter();
    writer->SetInputData(data);
    if (!this->Started)
    {
      this->Started = true;
      writer->Start(); // start needs input data, hence we do it here.
    }
    writer->Write();
    writer->SetInputData(nullptr);
    return (writer->GetError() == 0 && writer->GetError() == vtkErrorCode::NoError);
  }

  bool SaveFinalize() override
  {
    if (this->Started)
    {
      this->GetWriter()->End();
    }
    this->Started = false;
    return this->Superclass::SaveFinalize();
  }

private:
  SceneImageWriterMovie(const SceneImageWriterMovie&) = delete;
  void operator=(const SceneImageWriterMovie&) = delete;
  bool Started;
};
vtkStandardNewMacro(SceneImageWriterMovie);

class SceneImageWriterImageSeries : public SceneImageWriter<vtkImageWriter>
{
public:
  static SceneImageWriterImageSeries* New();
  vtkTypeMacro(SceneImageWriterImageSeries, SceneImageWriter<vtkImageWriter>);

  /**
   * The suffix format to use to format the counter
   */
  vtkSetStringMacro(SuffixFormat);
  vtkGetStringMacro(SuffixFormat);

protected:
  SceneImageWriterImageSeries()
    : Counter(0)
    , SuffixFormat(nullptr)
  {
  }
  ~SceneImageWriterImageSeries() { this->SetSuffixFormat(nullptr); }

  bool SaveInitialize(int startCount) override
  {
    this->Counter = startCount;
    auto path = vtksys::SystemTools::GetFilenamePath(this->FileName);
    auto prefix = vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);
    this->Prefix = path.empty() ? prefix : path + "/" + prefix;
    this->Extension = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
    return this->Superclass::SaveInitialize(startCount);
  }

  bool WriteFrameImage(double vtkNotUsed(time), vtkImageData* data) override
  {
    auto writer = this->GetWriter();
    assert(data);
    assert(this->SuffixFormat);
    assert(writer);

    char buffer[1024];
    snprintf(buffer, 1024, this->SuffixFormat, this->Counter);

    std::ostringstream str;
    str << this->Prefix << buffer << this->Extension;
    writer->SetInputData(data);
    writer->SetFileName(str.str().c_str());
    writer->Write();
    writer->SetInputData(nullptr);

    const bool success = writer->GetErrorCode() == vtkErrorCode::NoError;
    this->Counter += success ? 1 : 0;
    return success;
  }

private:
  SceneImageWriterImageSeries(const SceneImageWriterImageSeries&) = delete;
  void operator=(const SceneImageWriterImageSeries&) = delete;
  int Counter;
  char* SuffixFormat;
  std::string Prefix;
  std::string Extension;
};
vtkStandardNewMacro(SceneImageWriterImageSeries);
}

vtkStandardNewMacro(vtkSMSaveAnimationProxy);
//----------------------------------------------------------------------------
vtkSMSaveAnimationProxy::vtkSMSaveAnimationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMSaveAnimationProxy::~vtkSMSaveAnimationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMSaveAnimationProxy::EnforceSizeRestrictions(const char* filename)
{
  std::string ext = vtksys::SystemTools::GetFilenameLastExtension(filename ? filename : "");
  if (vtksys::SystemTools::LowerCase(ext) == ".avi")
  {
    vtkVector2i newsize;
    vtkSMPropertyHelper(this, "ImageResolution").Get(newsize.GetData(), 2);

    const vtkVector2i size(newsize);
    if (newsize[0] % 4 != 0)
    {
      newsize[0] -= (newsize[0] % 4);
    }
    if (newsize[1] % 4 != 0)
    {
      newsize[1] -= (newsize[1] % 4);
    }

    if (newsize != size)
    {
      vtkWarningMacro("The requested resolution '("
        << size[0] << ", " << size[1] << ")' has been changed to '(" << newsize[0] << ", "
        << newsize[1] << ")' to match format specification.");
      vtkSMPropertyHelper(this, "ImageResolution").Set(newsize.GetData(), 2);
      return true; // size changed.
    }
  }
  // nothing  changed.
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMSaveAnimationProxy::WriteAnimation(const char* filename)
{
  vtkSMViewLayoutProxy* layout = this->GetLayout();
  vtkSMViewProxy* view = this->GetView();

  // view and layout are mutually exclusive.
  assert(layout == NULL || view == NULL);
  if (layout == NULL && view == NULL)
  {
    vtkErrorMacro("Cannot WriteImage without a view or layout.");
    return false;
  }

  // Enforce any restrictions on the image size based on the file format
  this->EnforceSizeRestrictions(filename);

  SM_SCOPED_TRACE(SaveCameras)
    .arg("proxy", view != NULL ? static_cast<vtkSMProxy*>(view) : static_cast<vtkSMProxy*>(layout));

  SM_SCOPED_TRACE(SaveScreenshotOrAnimation)
    .arg("helper", this)
    .arg("filename", filename)
    .arg("view", view)
    .arg("layout", layout)
    .arg("mode_screenshot", 0);
  return this->WriteAnimationLocally(filename);
}

//----------------------------------------------------------------------------
bool vtkSMSaveAnimationProxy::WriteAnimationLocally(const char* filename)
{
  if (!this->Prepare())
  {
    return false;
  }

  vtkSmartPointer<vtkSMAnimationSceneWriter> writer;

  vtkSMProxy* sceneProxy = this->GetAnimationScene();
  auto formatProxy = this->GetFormatProxy(filename);
  if (!formatProxy)
  {
    vtkErrorMacro("Failed to determine format for '" << filename);
    return false;
  }

  // ideally, frame rate is directly set on the format proxy, but due to odd
  // interactions between frame rate and window, we need frame rate on `this`.
  // until we get around to cleaning that, I am letting this be.
  vtkSMPropertyHelper(formatProxy, "FrameRate", true)
    .Set(vtkSMPropertyHelper(this, "FrameRate").GetAsInt());
  formatProxy->UpdateVTKObjects();

  // based on the format, we create an appropriate SceneImageWriter.
  auto formatObj = formatProxy->GetClientSideObject();
  if (auto imgWriter = vtkImageWriter::SafeDownCast(formatObj))
  {
    vtkNew<vtkSMSaveAnimationProxyNS::SceneImageWriterImageSeries> realWriter;
    realWriter->SetWriter(imgWriter);
    realWriter->SetSuffixFormat(vtkSMPropertyHelper(formatProxy, "SuffixFormat").GetAsString());
    realWriter->SetHelper(this);
    writer = realWriter;
  }
  else if (auto movieWriter = vtkGenericMovieWriter::SafeDownCast(formatObj))
  {
    vtkNew<vtkSMSaveAnimationProxyNS::SceneImageWriterMovie> realWriter;
    realWriter->SetWriter(movieWriter);
    realWriter->SetHelper(this);
    writer = realWriter;
  }
  else
  {
    vtkErrorMacro("Unknown format type "
      << (formatObj ? formatObj->GetClassName() : "")
      << ". Currently, on vtkImageWriter or vtkGenericMovieWriter subclasses "
      << "are supported.");
    return false;
  }

  writer->SetAnimationScene(sceneProxy);
  writer->SetFileName(filename);

  // FIXME: we should consider cleaning up this API on vtkSMAnimationSceneWriter. For now,
  //        keeping it unchanged. This largely lifted from old code in
  //        pqAnimationManager.
  //
  // Convert frame window to PlaybackTimeWindow; FrameWindow is an integral
  // value indicating the frame number of timestep; PlaybackTimeWindow is double
  // values as animation time.
  int frameWindow[2] = { 0, 0 };
  vtkSMPropertyHelper(this, "FrameWindow").Get(frameWindow, 2);
  double playbackTimeWindow[2] = { -1, 0 };
  switch (vtkSMPropertyHelper(sceneProxy, "PlayMode").GetAsInt())
  {
    case vtkCompositeAnimationPlayer::SEQUENCE:
    {
      int numFrames = vtkSMPropertyHelper(sceneProxy, "NumberOfFrames").GetAsInt();
      double startTime = vtkSMPropertyHelper(sceneProxy, "StartTime").GetAsDouble();
      double endTime = vtkSMPropertyHelper(sceneProxy, "EndTime").GetAsDouble();
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
      vtkSMProxy* timeKeeper = vtkSMPropertyHelper(sceneProxy, "TimeKeeper").GetAsProxy();
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
  this->GetSession()->GetProgressHandler()->RegisterProgressEvent(
    writer.Get(), static_cast<int>(this->GetGlobalID()));
  this->GetSession()->PrepareProgress();
  bool status = writer->Save();
  this->GetSession()->CleanupPendingProgress();

  this->Cleanup();
  return status;
}

//----------------------------------------------------------------------------
vtkSMViewLayoutProxy* vtkSMSaveAnimationProxy::GetLayout()
{
  if (vtkSMPropertyHelper(this, "SaveAllViews").GetAsInt() != 0)
  {
    return vtkSMViewLayoutProxy::SafeDownCast(vtkSMPropertyHelper(this, "Layout").GetAsProxy());
  }
  return NULL;
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMSaveAnimationProxy::GetView()
{
  if (vtkSMPropertyHelper(this, "SaveAllViews").GetAsInt() == 0)
  {
    return vtkSMViewProxy::SafeDownCast(vtkSMPropertyHelper(this, "View").GetAsProxy());
  }
  return NULL;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMSaveAnimationProxy::GetAnimationScene()
{
  return vtkSMPropertyHelper(this, "AnimationScene").GetAsProxy();
}

//----------------------------------------------------------------------------
void vtkSMSaveAnimationProxy::UpdateDefaultsAndVisibilities(const char* filename)
{
  this->Superclass::UpdateDefaultsAndVisibilities(filename);

  if (filename)
  {
    // pick correct "format" as the default.
    if (auto proxy = this->GetFormatProxy(filename))
    {
      if (proxy->GetProperty("FrameRate"))
      {
        this->GetProperty("FrameRate")->SetPanelVisibility("default");
      }
    }
  }
}

//----------------------------------------------------------------------------
bool vtkSMSaveAnimationProxy::Prepare()
{
  // Let the superclass deal with preparing the scene for saving out images for the correct
  // size and form.
  if (!this->Superclass::Prepare())
  {
    return false;
  }

  // Now let's deal with ensuring the animation behaves as per our requirements.
  vtkSMProxy* scene = this->GetAnimationScene();
  if (!scene)
  {
    vtkErrorMacro("Missing animation scene.");
    return false;
  }

  // save scene state to restore later.
  this->SceneState.TakeReference(scene->SaveXMLState(nullptr));

  int frameRate = vtkSMPropertyHelper(this, "FrameRate").GetAsInt();

  // Update playmode and duration.
  vtkSMPropertyHelper playModeHelper(scene, "PlayMode");

  // If current playMode was real-time, we change it to sequence since we cannot
  // really save in real-time mode, and adjust the frames.
  if (playModeHelper.GetAsInt() == vtkCompositeAnimationPlayer::REAL_TIME)
  {
    playModeHelper.Set(vtkCompositeAnimationPlayer::SEQUENCE);
    // change length of seq. animation to match the length in real-time mode.
    // (see #17031)
    vtkSMPropertyHelper(scene, "NumberOfFrames")
      .Set(1 + frameRate * vtkSMPropertyHelper(scene, "Duration").GetAsInt());
  }
  else
  {
    // for SEQUENCE and SNAP_TO_TIMESTEPS, we don't need to change anything as far as
    // playing of the animation goes.
  }
  scene->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSaveAnimationProxy::Cleanup()
{
  if (vtkSMProxy* scene = this->GetAnimationScene())
  {
    // pass nullptr for vtkSMProxyLocator leaves all proxy properties unchanged.
    scene->LoadXMLState(this->SceneState.Get(), nullptr);
    scene->UpdateVTKObjects();
    this->SceneState = nullptr;
  }
  return this->Superclass::Cleanup();
}

//----------------------------------------------------------------------------
void vtkSMSaveAnimationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
