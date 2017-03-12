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
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVServerInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLElement.h"
#include "vtkSMAnimationSceneImageWriter.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMSessionClient.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"

#include <sstream>

namespace vtkSMSaveAnimationProxyNS
{
class SceneImageWriter : public vtkSMAnimationSceneImageWriter
{
  vtkWeakPointer<vtkSMSaveAnimationProxy> Helper;

public:
  static SceneImageWriter* New();
  vtkTypeMacro(SceneImageWriter, vtkSMAnimationSceneImageWriter);

  void SetHelper(vtkSMSaveAnimationProxy* helper) { this->Helper = helper; }

protected:
  SceneImageWriter() {}
  ~SceneImageWriter() {}

  vtkSmartPointer<vtkImageData> CaptureFrame() VTK_OVERRIDE
  {
    return this->Helper ? this->Helper->CapturePreppedImage() : vtkSmartPointer<vtkImageData>();
  }

private:
  SceneImageWriter(const SceneImageWriter&) VTK_DELETE_FUNCTION;
  void operator=(const SceneImageWriter&) VTK_DELETE_FUNCTION;
};

vtkStandardNewMacro(SceneImageWriter);
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
bool vtkSMSaveAnimationProxy::SupportsDisconnectAndSave(vtkSMSession* session)
{
  // disconnect-n-save is only possible for client-server connections.
  if ((vtkSMSessionClient::SafeDownCast(session) != NULL) && (!session->IsMultiClients()))
  {
    // let's also confirm that the server supports rendering.
    vtkNew<vtkPVDisplayInformation> dinfo;
    session->GatherInformation(vtkPVSession::RENDER_SERVER, dinfo.Get(), 0);
    return (dinfo->GetCanOpenDisplay() && dinfo->GetSupportsOpenGL());
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMSaveAnimationProxy::SupportsAVI(vtkSMSession* session, bool remote)
{
  vtkSmartPointer<vtkPVServerInformation> info;
  if (remote)
  {
    info = session->GetServerInformation();
  }
  else
  {
    // vtkPVServerInformation initialize AVI/OGG support in constructor for the
    // local process.
    info = vtkSmartPointer<vtkPVServerInformation>::New();
  }

  return info->GetAVISupport() != 0;
}

//----------------------------------------------------------------------------
bool vtkSMSaveAnimationProxy::SupportsOGV(vtkSMSession* session, bool remote)
{
  vtkSmartPointer<vtkPVServerInformation> info;
  if (remote)
  {
    info = session->GetServerInformation();
  }
  else
  {
    // vtkPVServerInformation initialize AVI/OGG support in constructor for the
    // local process.
    info = vtkSmartPointer<vtkPVServerInformation>::New();
  }

  return info->GetOGVSupport() != 0;
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

  SM_SCOPED_TRACE(SaveCameras)
    .arg("proxy", view != NULL ? static_cast<vtkSMProxy*>(view) : static_cast<vtkSMProxy*>(layout));

  SM_SCOPED_TRACE(SaveScreenshotOrAnimation)
    .arg("helper", this)
    .arg("filename", filename)
    .arg("view", view)
    .arg("layout", layout)
    .arg("mode_screenshot", 0);

  vtkSMSession* session = view ? view->GetSession() : layout->GetSession();
  if (this->SupportsDisconnectAndSave(session) &&
    vtkSMPropertyHelper(this, "DisconnectAndSave").GetAsInt() == 1)
  {
    return this->DisconnectAndWriteAnimation(filename);
  }
  else
  {
    return this->WriteAnimationLocally(filename);
  }
}

//----------------------------------------------------------------------------
bool vtkSMSaveAnimationProxy::WriteAnimationLocally(const char* filename)
{
  if (!this->Prepare())
  {
    return false;
  }

  vtkSMProxy* sceneProxy = this->GetAnimationScene();

  vtkNew<vtkSMSaveAnimationProxyNS::SceneImageWriter> imageWriter;
  imageWriter->SetAnimationScene(sceneProxy);
  imageWriter->SetFrameRate(vtkSMPropertyHelper(this, "FrameRate").GetAsInt());
  imageWriter->SetQuality(vtkSMPropertyHelper(this, "ImageQuality").GetAsInt());
  imageWriter->SetFileName(filename);
  imageWriter->SetHelper(this);

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
  imageWriter->SetStartFileCount(frameWindow[0]);
  imageWriter->SetPlaybackTimeWindow(playbackTimeWindow);

  // TODO: handle progress.
  bool status = imageWriter->Save();

  this->Cleanup();
  return status;
}

//----------------------------------------------------------------------------
bool vtkSMSaveAnimationProxy::DisconnectAndWriteAnimation(const char* filename)
{
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();

  // Register ourselves with PXM.
  std::string pname = pxm->RegisterProxy("misc", this);

  // Save proxy manager state.
  vtkSmartPointer<vtkPVXMLElement> pxmState =
    vtkSmartPointer<vtkPVXMLElement>::Take(pxm->SaveXMLState());
  std::ostringstream pxmStateStream;
  pxmState->PrintXML(pxmStateStream, vtkIndent());

  pxm->UnRegisterProxy("misc", pname.c_str(), this);

  // Create server-side AnimationPlayer proxy.
  vtkSmartPointer<vtkSMProxy> proxy =
    vtkSmartPointer<vtkSMProxy>::Take(pxm->NewProxy("remote_player", "AnimationPlayer"));
  vtkSMPropertyHelper(proxy, "SessionProxyManagerState").Set(pxmStateStream.str().c_str());
  vtkSMPropertyHelper(proxy, "FileName").Set(filename);
  proxy->UpdateVTKObjects();
  proxy->InvokeCommand("Activate");
  return true;
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
    vtkSMPropertyHelper(scene, "NumberOfFrames")
      .Set(frameRate * vtkSMPropertyHelper(scene, "Duration").GetAsInt());
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
