/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSaveAnimationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkPVAnimationModule.h" //needed for exports
#include "vtkSMSaveScreenshotProxy.h"
namespace vtkSMSaveAnimationProxyNS
{
class SceneGrabber;
}

class vtkPVXMLElement;

class VTKPVANIMATION_EXPORT vtkSMSaveAnimationProxy : public vtkSMSaveScreenshotProxy
{
public:
  static vtkSMSaveAnimationProxy* New();
  vtkTypeMacro(vtkSMSaveAnimationProxy, vtkSMSaveScreenshotProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Save animation as images/video. The properties on this proxy provide all
   * the necessary information to save the animation.
   */
  virtual bool WriteAnimation(const char* filename);

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
  virtual bool WriteAnimationLocally(const char* filename);

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

  friend class vtkSMSaveAnimationProxyNS::SceneGrabber;

  vtkSmartPointer<vtkPVXMLElement> SceneState;
};

#endif
