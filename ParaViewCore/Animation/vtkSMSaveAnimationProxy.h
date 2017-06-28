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
 * vtkSMSaveAnimationProxy provides static methods such as
 * `SupportsDisconnectAndSave`, `SupportsAVI`, and `SupportsOGV` that
 * applications can use to determine support for specific functionality in the
 * current session/application.
 */

#ifndef vtkSMSaveAnimationProxy_h
#define vtkSMSaveAnimationProxy_h

#include "vtkPVAnimationModule.h" //needed for exports
#include "vtkSMSaveScreenshotProxy.h"
namespace vtkSMSaveAnimationProxyNS
{
class SceneImageWriter;
}

class vtkPVXMLElement;

class VTKPVANIMATION_EXPORT vtkSMSaveAnimationProxy : public vtkSMSaveScreenshotProxy
{
public:
  static vtkSMSaveAnimationProxy* New();
  vtkTypeMacro(vtkSMSaveAnimationProxy, vtkSMSaveScreenshotProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Save animation as images/video. The properties on this proxy provide all
   * the necessary information to save the animation.
   */
  virtual bool WriteAnimation(const char* filename);

  /**
   * Returns true if the session can support disconnecting and saving
   * animations.
   */
  static bool SupportsDisconnectAndSave(vtkSMSession* session);

  /**
   * Returns true if the session supports AVI file writing.
   */
  static bool SupportsAVI(vtkSMSession* session, bool remote = false);

  /**
   * Returns true if the session supports OGV file writing.
   */
  static bool SupportsOGV(vtkSMSession* session, bool remote = false);

protected:
  vtkSMSaveAnimationProxy();
  ~vtkSMSaveAnimationProxy();

  /**
   * Write animation on local process.
   */
  virtual bool WriteAnimationLocally(const char* filename);

  /**
   * Write animation on server after disconnecting from it.
   */
  virtual bool DisconnectAndWriteAnimation(const char* filename);

  /**
   * Prepares for saving animation.
   */
  bool Prepare() VTK_OVERRIDE;

  /**
   * This restores the state after saving the animation.
   */
  bool Cleanup() VTK_OVERRIDE;

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
  vtkSMSaveAnimationProxy(const vtkSMSaveAnimationProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMSaveAnimationProxy&) VTK_DELETE_FUNCTION;

  friend class vtkSMSaveAnimationProxyNS::SceneImageWriter;

  vtkSmartPointer<vtkPVXMLElement> SceneState;
};

#endif
