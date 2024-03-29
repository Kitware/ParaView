// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkSMSaveAnimationProxy
 * @brief proxy to save animation to images/video.
 *
 * vtkSMSaveAnimationProxy is a proxy that helps with saving animation to images
 * or video. The properties on this proxy provide various options that user can
 * configure when saving animations. Once those properties are setup, one
 * calls vtkSMSaveAnimationProxy::WriteAnimation` to save out the animation.
 *
 */

#ifndef vtkSMSaveAnimationProxy_h
#define vtkSMSaveAnimationProxy_h

#include "vtkRemotingAnimationModule.h" //needed for exports
#include "vtkSMSaveScreenshotProxy.h"
namespace vtkSMSaveAnimationProxyNS
{
class Friendship;
}

class vtkPVXMLElement;

class VTKREMOTINGANIMATION_EXPORT vtkSMSaveAnimationProxy : public vtkSMSaveScreenshotProxy
{
public:
  static vtkSMSaveAnimationProxy* New();
  vtkTypeMacro(vtkSMSaveAnimationProxy, vtkSMSaveScreenshotProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Save animation as images/video. The properties on this proxy provide all
   * the necessary information to save the animation.
   */
  virtual bool WriteAnimation(const char* filename, vtkTypeUInt32 location = vtkPVSession::CLIENT);

  /**
   * Overridden to update visibility state of "FrameRate" property.
   */
  void UpdateDefaultsAndVisibilities(const char* filename) override;

protected:
  vtkSMSaveAnimationProxy();
  ~vtkSMSaveAnimationProxy() override;

  /**
   * Write animation on local process.
   */
  virtual bool WriteAnimationInternal(
    const char* filename, vtkTypeUInt32 location = vtkPVSession::CLIENT);

  /**
   * Prepares for saving animation.
   */
  bool Prepare() override;

  /**
   * This restores the state after saving the animation.
   */
  bool Cleanup() override;

  /**
   * Change "ImageResolution" property as needed for the file format requested.
   * @returns true if changed, false otherwise.
   */
  virtual bool EnforceSizeRestrictions(const char* filename);

  vtkSMViewLayoutProxy* GetLayout();
  vtkSMViewProxy* GetView();
  vtkSMSaveScreenshotProxy* GetScreenshotHelper();
  vtkSMProxy* GetAnimationScene();

private:
  vtkSMSaveAnimationProxy(const vtkSMSaveAnimationProxy&) = delete;
  void operator=(const vtkSMSaveAnimationProxy&) = delete;

  friend class vtkSMSaveAnimationProxyNS::Friendship;

  vtkSmartPointer<vtkPVXMLElement> SceneState;
};

#endif
