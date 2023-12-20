// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkPVServerInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkRemoteWriterHelper.h"
#include "vtkRenderWindow.h"
#include "vtkSMAnimationScene.h"
#include "vtkSMAnimationSceneWriter.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMSessionClient.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"

#include <sstream>
#include <vtksys/SystemTools.hxx>

namespace vtkSMSaveAnimationProxyNS
{

class Friendship
{
public:
  static std::pair<vtkSmartPointer<vtkImageData>, vtkSmartPointer<vtkImageData>> Grab(
    vtkSMSaveAnimationProxy* proxy)
  {
    return proxy ? proxy->CapturePreppedImages()
                 : std::pair<vtkSmartPointer<vtkImageData>, vtkSmartPointer<vtkImageData>>();
  }

  static std::string GetStereoFileName(
    vtkSMSaveAnimationProxy* proxy, const std::string& filename, bool left)
  {
    return proxy ? proxy->GetStereoFileName(filename, left) : filename;
  }
};

class SceneImageWriter : public vtkSMAnimationSceneWriter
{
  vtkWeakPointer<vtkSMSaveAnimationProxy> Helper;

public:
  vtkTemplateTypeMacro(SceneImageWriter, vtkSMAnimationSceneWriter);
  /**
   * Set the vtkSMSaveAnimationProxy proxy.
   */
  void SetHelper(vtkSMSaveAnimationProxy* helper) { this->Helper = helper; }

  /**
   * Get the vtkRemoteWriterHelper proxy.
   */
  vtkSmartPointer<vtkSMSourceProxy> GetRemoteWriterHelper(
    vtkSMProxy* formatProxy, vtkTypeUInt32 location)
  {
    assert(formatProxy);
    const auto pxm = formatProxy->GetSessionProxyManager();
    auto remoteWriter = vtkSmartPointer<vtkSMSourceProxy>::Take(
      vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("misc", "RemoteWriterHelper")));
    vtkSMPropertyHelper(remoteWriter, "Writer").Set(formatProxy);
    vtkSMPropertyHelper(remoteWriter, "OutputDestination").Set(static_cast<int>(location));
    vtkSMPropertyHelper(remoteWriter, "TryWritingInBackground").Set(0);
    remoteWriter->UpdateVTKObjects();
    return remoteWriter;
  }

protected:
  SceneImageWriter() = default;
  ~SceneImageWriter() override = default;
  bool SaveInitialize(int vtkNotUsed(startCount)) override
  {
    // Animation scene call render on each tick. We override that render call
    // since it's a waste of rendering, the code to save the images will call
    // render regardless.
    this->AnimationScene->SetOverrideStillRender(1);
    return true;
  }

  bool SaveFrame(double time) override
  {
    auto image_pair = Friendship::Grab(this->Helper);

    // Now, in symmetric batch mode, while this method will get called on all
    // ranks, we really only to save the image on root node.
    // Note, the call to CapturePreppedImages() still needs to happen on all
    // ranks, since otherwise we may get mismatched renders.
    vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
    if (image_pair.first == nullptr || (controller && controller->GetLocalProcessId() != 0))
    {
      // don't actually save anything on this rank.
      return true;
    }

    return this->WriteFrameImage(time, image_pair.first, image_pair.second);
  }

  bool SaveFinalize() override
  {
    this->AnimationScene->SetOverrideStillRender(0);
    return true;
  }

  virtual bool WriteFrameImage(double time, vtkImageData* dataLeft, vtkImageData* dataRight) = 0;

  std::string GetStereoFileName(const std::string& filename, bool left)
  {
    return Friendship::GetStereoFileName(this->Helper, filename, left);
  }

private:
  SceneImageWriter(const SceneImageWriter&) = delete;
  void operator=(const SceneImageWriter&) = delete;
};

class SceneImageWriterMovie : public SceneImageWriter
{
  vtkSmartPointer<vtkSMSourceProxy> RemoteWriterHelpers[2] = { nullptr, nullptr };

public:
  static SceneImageWriterMovie* New();
  vtkTypeMacro(SceneImageWriterMovie, SceneImageWriter);

  /**
   * Set format proxy
   */
  void SetFormatProxy(int index, vtkSMProxy* formatProxy, vtkTypeUInt32 location)
  {
    this->RemoteWriterHelpers[index] = this->GetRemoteWriterHelper(formatProxy, location);
  }

protected:
  SceneImageWriterMovie()
    : Started(false)
  {
  }
  ~SceneImageWriterMovie() override = default;

  bool SaveInitialize(int startCount) override
  {
    const std::string filename = this->GetFileName();
    if (this->RemoteWriterHelpers[1])
    {
      // right writer
      const auto rFormat = vtkSMPropertyHelper(this->RemoteWriterHelpers[1], "Writer").GetAsProxy();
      vtkSMPropertyHelper(rFormat, "FileName")
        .Set(this->GetStereoFileName(filename, /*left=*/false).c_str());
      rFormat->UpdateVTKObjects();

      // left writer
      const auto lFormat = vtkSMPropertyHelper(this->RemoteWriterHelpers[0], "Writer").GetAsProxy();
      vtkSMPropertyHelper(lFormat, "FileName")
        .Set(this->GetStereoFileName(filename, /*left=*/true).c_str());
      lFormat->UpdateVTKObjects();
    }
    else
    {
      // left writer
      const auto lFormat = vtkSMPropertyHelper(this->RemoteWriterHelpers[0], "Writer").GetAsProxy();
      vtkSMPropertyHelper(lFormat, "FileName").Set(filename.c_str());
      lFormat->UpdateVTKObjects();
    }
    return this->Superclass::SaveInitialize(startCount);
  }

  bool WriteFrameImage(
    double vtkNotUsed(time), vtkImageData* dataLeft, vtkImageData* dataRight) override
  {
    vtkImageData* data[] = { dataLeft, dataRight };
    bool status = true;
    for (int cc = 0; cc < 2; ++cc)
    {
      if (auto remoteWriterHelper = this->RemoteWriterHelpers[cc])
      {
        assert(data[cc] != nullptr);
        auto remoteWriterAlgorithm =
          vtkAlgorithm::SafeDownCast(remoteWriterHelper->GetClientSideObject());
        remoteWriterAlgorithm->SetInputDataObject(data[cc]);
        if (!this->Started)
        {
          // start needs input data, hence we do it here.
          vtkSMPropertyHelper(remoteWriterHelper, "State").Set(vtkRemoteWriterHelper::START);
          remoteWriterHelper->UpdateVTKObjects();
          remoteWriterHelper->UpdatePipeline();
        }
        vtkSMPropertyHelper(remoteWriterHelper, "State").Set(vtkRemoteWriterHelper::WRITE);
        remoteWriterHelper->UpdateVTKObjects();
        remoteWriterHelper->UpdatePipeline();
        remoteWriterAlgorithm->SetInputDataObject(nullptr);
        status &= (remoteWriterAlgorithm->GetErrorCode() == 0 &&
          remoteWriterAlgorithm->GetErrorCode() == vtkErrorCode::NoError);
      }
    }
    this->Started = true;
    return status;
  }

  bool SaveFinalize() override
  {
    if (this->Started)
    {
      for (int cc = 0; cc < 2; ++cc)
      {
        if (auto remoteWriterHelper = this->RemoteWriterHelpers[cc])
        {
          vtkSMPropertyHelper(remoteWriterHelper, "State").Set(vtkRemoteWriterHelper::END);
          remoteWriterHelper->UpdateVTKObjects();
          remoteWriterHelper->UpdatePipeline();
        }
      }
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

class SceneImageWriterImageSeries : public SceneImageWriter
{
  vtkSmartPointer<vtkSMSourceProxy> RemoteWriterHelper = nullptr;

public:
  static SceneImageWriterImageSeries* New();
  vtkTypeMacro(SceneImageWriterImageSeries, SceneImageWriter);

  /**
   * The suffix format to use to format the counter
   */
  vtkSetStringMacro(SuffixFormat);
  vtkGetStringMacro(SuffixFormat);

  /**
   * Set format proxy
   */
  void SetFormatProxy(vtkSMProxy* formatProxy, vtkTypeUInt32 location)
  {
    this->RemoteWriterHelper = this->GetRemoteWriterHelper(formatProxy, location);
  }

protected:
  SceneImageWriterImageSeries()
    : Counter(0)
    , SuffixFormat(nullptr)
  {
  }
  ~SceneImageWriterImageSeries() override { this->SetSuffixFormat(nullptr); }

  bool SaveInitialize(int startCount) override
  {
    this->Counter = startCount;
    auto path = vtksys::SystemTools::GetFilenamePath(this->FileName);
    auto prefix = vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);
    this->Prefix = path.empty() ? prefix : path + "/" + prefix;
    this->Extension = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
    return this->Superclass::SaveInitialize(startCount);
  }

  bool WriteFrameImage(
    double vtkNotUsed(time), vtkImageData* dataLeft, vtkImageData* dataRight) override
  {
    bool success = true;

    const auto remoteWriterHelper = this->RemoteWriterHelper;
    auto remoteWriterAlgorithm =
      vtkAlgorithm::SafeDownCast(remoteWriterHelper->GetClientSideObject());
    assert(dataLeft);
    assert(this->SuffixFormat);
    assert(remoteWriterAlgorithm);

    char buffer[1024];
    snprintf(buffer, 1024, this->SuffixFormat, this->Counter);

    std::ostringstream str;
    str << this->Prefix << buffer << this->Extension;

    const std::string filename = str.str();
    if (dataRight)
    {
      // write right image.
      const auto rFormat = vtkSMPropertyHelper(remoteWriterHelper, "Writer").GetAsProxy();
      vtkSMPropertyHelper(rFormat, "FileName")
        .Set(this->GetStereoFileName(filename, /*left=*/false).c_str());
      rFormat->UpdateVTKObjects();
      remoteWriterAlgorithm->SetInputDataObject(dataRight);
      vtkSMPropertyHelper(remoteWriterHelper, "State").Set(vtkRemoteWriterHelper::WRITE);
      remoteWriterHelper->UpdateVTKObjects();
      remoteWriterHelper->UpdatePipeline();
      success &= (remoteWriterAlgorithm->GetErrorCode() == vtkErrorCode::NoError);

      // write left image.
      const auto lFormat = vtkSMPropertyHelper(remoteWriterHelper, "Writer").GetAsProxy();
      vtkSMPropertyHelper(lFormat, "FileName")
        .Set(this->GetStereoFileName(filename, /*left=*/true).c_str());
      lFormat->UpdateVTKObjects();
      remoteWriterAlgorithm->SetInputDataObject(dataLeft);
      vtkSMPropertyHelper(remoteWriterHelper, "State").Set(vtkRemoteWriterHelper::WRITE);
      remoteWriterHelper->UpdateVTKObjects();
      remoteWriterHelper->UpdatePipeline();
      success &= (remoteWriterAlgorithm->GetErrorCode() == vtkErrorCode::NoError);
    }
    else
    {
      // write left image.
      const auto lFormat = vtkSMPropertyHelper(remoteWriterHelper, "Writer").GetAsProxy();
      vtkSMPropertyHelper(lFormat, "FileName").Set(filename.c_str());
      lFormat->UpdateVTKObjects();
      remoteWriterAlgorithm->SetInputDataObject(dataLeft);
      vtkSMPropertyHelper(remoteWriterHelper, "State").Set(vtkRemoteWriterHelper::WRITE);
      remoteWriterHelper->UpdateVTKObjects();
      remoteWriterHelper->UpdatePipeline();
      success &= (remoteWriterAlgorithm->GetErrorCode() == vtkErrorCode::NoError);
    }
    remoteWriterAlgorithm->SetInputDataObject(nullptr);

    success &= remoteWriterAlgorithm->GetErrorCode() == vtkErrorCode::NoError;
    this->Counter += success ? this->Stride : 0;
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
vtkSMSaveAnimationProxy::vtkSMSaveAnimationProxy() = default;

//----------------------------------------------------------------------------
vtkSMSaveAnimationProxy::~vtkSMSaveAnimationProxy() = default;

//----------------------------------------------------------------------------
bool vtkSMSaveAnimationProxy::EnforceSizeRestrictions(const char* filename)
{
  std::string ext = vtksys::SystemTools::GetFilenameLastExtension(filename ? filename : "");
  ext = vtksys::SystemTools::LowerCase(ext);
  int widthMultiple = 1;
  int heightMultiple = 1;
  if (ext == ".avi")
  {
    widthMultiple = 4;
    heightMultiple = 4;
  }
  else if (ext == ".mp4")
  {
    widthMultiple = 2;
    heightMultiple = 2;
  }

  vtkVector2i newsize;
  vtkSMPropertyHelper(this, "ImageResolution").Get(newsize.GetData(), 2);
  const vtkVector2i size(newsize);

  if (widthMultiple != 1)
  {
    if (newsize[0] % widthMultiple != 0)
    {
      newsize[0] -= (newsize[0] % widthMultiple);
    }
  }

  if (heightMultiple != 1)
  {
    if (newsize[1] % heightMultiple != 0)
    {
      newsize[1] -= (newsize[1] % heightMultiple);
    }
  }

  if (newsize != size)
  {
    vtkWarningMacro("The requested resolution '("
      << size[0] << ", " << size[1] << ")' has been changed to '(" << newsize[0] << ", "
      << newsize[1] << ")' to match format specification.");
    vtkSMPropertyHelper(this, "ImageResolution").Set(newsize.GetData(), 2);
    return true; // size changed.
  }

  // nothing  changed.
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMSaveAnimationProxy::WriteAnimation(const char* filename, vtkTypeUInt32 location)
{
  if (filename == nullptr)
  {
    return false;
  }

  if (location != vtkPVSession::CLIENT && location != vtkPVSession::DATA_SERVER &&
    location != vtkPVSession::DATA_SERVER_ROOT)
  {
    vtkErrorMacro("Location not supported: " << location);
    return false;
  }

  auto session = this->GetSession();
  if (session->GetProcessRoles() != vtkPVSession::CLIENT)
  {
    // implies that the current session is not a remote-session (since the
    // process is acting as more than just CLIENT). Simply set location to
    // CLIENT since CLIENT and DATA_SERVER_ROOT are the same process.
    location = vtkPVSession::CLIENT;
  }
  else if (location == vtkPVSession::DATA_SERVER)
  {
    location = vtkPVSession::DATA_SERVER_ROOT;
  }

  vtkSMViewLayoutProxy* layout = this->GetLayout();
  vtkSMViewProxy* view = this->GetView();

  // view and layout are mutually exclusive.
  assert(layout == nullptr || view == nullptr);
  if (layout == nullptr && view == nullptr)
  {
    vtkErrorMacro("Cannot WriteImage without a view or layout.");
    return false;
  }

  // Enforce any restrictions on the image size based on the file format
  this->EnforceSizeRestrictions(filename);

  SM_SCOPED_TRACE(SaveLayoutSizes)
    .arg(
      "proxy", view != nullptr ? static_cast<vtkSMProxy*>(view) : static_cast<vtkSMProxy*>(layout));

  SM_SCOPED_TRACE(SaveCameras)
    .arg(
      "proxy", view != nullptr ? static_cast<vtkSMProxy*>(view) : static_cast<vtkSMProxy*>(layout));

  SM_SCOPED_TRACE(SaveScreenshotOrAnimation)
    .arg("helper", this)
    .arg("filename", filename)
    .arg("view", view)
    .arg("layout", layout)
    .arg("mode_screenshot", 0)
    .arg("location", static_cast<int>(location));
  return this->WriteAnimationInternal(filename, location);
}

//----------------------------------------------------------------------------
bool vtkSMSaveAnimationProxy::WriteAnimationInternal(const char* filename, vtkTypeUInt32 location)
{
  auto formatProxy = this->GetFormatProxy(filename);
  if (!formatProxy)
  {
    vtkErrorMacro("Failed to determine format for '" << filename);
    return false;
  }

  if (!this->Prepare())
  {
    return false;
  }

  vtkSMProxy* sceneProxy = this->GetAnimationScene();

  // ideally, frame rate is directly set on the format proxy, but due to odd
  // interactions between frame rate and window, we need frame rate on `this`.
  // until we get around to cleaning that, I am letting this be.
  vtkSMPropertyHelper(formatProxy, "FrameRate", true)
    .Set(vtkSMPropertyHelper(this, "FrameRate").GetAsInt());
  formatProxy->UpdateVTKObjects();

  // check if we're writing 2-stereo video streams at the same time.
  vtkSmartPointer<vtkSMProxy> otherFormatProxy;

  // based on the format, we create an appropriate SceneImageWriter.
  vtkSmartPointer<vtkSMAnimationSceneWriter> writer;
  auto formatObj = formatProxy->GetClientSideObject();
  if (vtkImageWriter::SafeDownCast(formatObj))
  {
    vtkNew<vtkSMSaveAnimationProxyNS::SceneImageWriterImageSeries> realWriter;
    realWriter->SetSuffixFormat(vtkSMPropertyHelper(formatProxy, "SuffixFormat").GetAsString());
    realWriter->SetHelper(this);
    realWriter->SetFormatProxy(formatProxy, location);
    writer = realWriter;
  }
  else if (vtkGenericMovieWriter::SafeDownCast(formatObj))
  {
    vtkNew<vtkSMSaveAnimationProxyNS::SceneImageWriterMovie> realWriter;
    realWriter->SetHelper(this);
    realWriter->SetFormatProxy(0, formatProxy, location);

    // we need two movie writers when writing stereo videos
    if (vtkSMPropertyHelper(this, "StereoMode").GetAsInt() == VTK_STEREO_EMULATE)
    {
      auto pxm = this->GetSessionProxyManager();
      otherFormatProxy.TakeReference(
        pxm->NewProxy(formatProxy->GetXMLGroup(), formatProxy->GetXMLName()));
      otherFormatProxy->SetLocation(formatProxy->GetLocation());
      otherFormatProxy->Copy(formatProxy);
      otherFormatProxy->UpdateVTKObjects();

      realWriter->SetFormatProxy(1, otherFormatProxy, location);
    }
    writer = realWriter;
  }
  else
  {
    vtkErrorMacro("Unknown format type "
      << (formatObj ? formatObj->GetClassName() : "")
      << ". Currently, only vtkImageWriter or vtkGenericMovieWriter subclasses "
      << "are supported.");
    return false;
  }

  writer->SetAnimationScene(sceneProxy);
  writer->SetFileName(filename);
  writer->SetStride(vtkSMPropertyHelper(this, "FrameStride").GetAsInt());

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
      const int numFrames = vtkSMPropertyHelper(sceneProxy, "NumberOfFrames").GetAsInt();
      const double startTime = vtkSMPropertyHelper(sceneProxy, "StartTime").GetAsDouble();
      const double endTime = vtkSMPropertyHelper(sceneProxy, "EndTime").GetAsDouble();
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
      const int numTS = tsValuesHelper.GetNumberOfElements();
      frameWindow[0] = frameWindow[0] < 0 ? 0 : frameWindow[0];
      frameWindow[1] = frameWindow[1] >= numTS ? numTS - 1 : frameWindow[1];
      playbackTimeWindow[0] = tsValuesHelper.GetAsDouble(frameWindow[0]);
      playbackTimeWindow[1] = tsValuesHelper.GetAsDouble(frameWindow[1]);
    }
    break;
  }
  writer->SetStartFileCount(frameWindow[0]);
  writer->SetPlaybackTimeWindow(playbackTimeWindow);

  // register with progress handler so we monitor progress events.
  this->GetSession()->GetProgressHandler()->RegisterProgressEvent(
    writer.Get(), static_cast<int>(this->GetGlobalID()));
  this->GetSession()->PrepareProgress();
  const bool status = writer->Save();
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
  return nullptr;
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMSaveAnimationProxy::GetView()
{
  if (vtkSMPropertyHelper(this, "SaveAllViews").GetAsInt() == 0)
  {
    return vtkSMViewProxy::SafeDownCast(vtkSMPropertyHelper(this, "View").GetAsProxy());
  }
  return nullptr;
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
