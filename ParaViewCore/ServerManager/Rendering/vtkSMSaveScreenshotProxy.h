/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSaveScreenshotProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMSaveScreenshotProxy
 * @brief proxy to help with saving screenshots for views
 *
 * vtkSMSaveScreenshotProxy is used to capture images from views or layouts.
 * It encapsulates the logic to generate screen shot of any arbitrary size
 * independent of the application window size.
 *
 * The properties on this proxy provide various options that user can configure
 * when saving images/screenshots. Once those properties are setup, one calls
 * `vtkSMSaveScreenshotProxy::WriteImage` or
 * `vtkSMSaveScreenshotProxy::CaptureImage`.
 *
 */

#ifndef vtkSMSaveScreenshotProxy_h
#define vtkSMSaveScreenshotProxy_h

#include "vtkPVServerManagerRenderingModule.h" // needed for exports
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.
#include "vtkVector.h"       // needed for vtkVector2i.

class vtkImageData;
class vtkSMViewLayoutProxy;
class vtkSMViewProxy;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMSaveScreenshotProxy : public vtkSMProxy
{
public:
  static vtkSMSaveScreenshotProxy* New();
  vtkTypeMacro(vtkSMSaveScreenshotProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Capture image. The properties for this proxy provide all the necessary
   * information to capture the image.
   */
  virtual bool WriteImage(const char* filename);

  /**
   * Capture the rendered image but doesn't save it out to any file.
   */
  virtual vtkSmartPointer<vtkImageData> CaptureImage();

  /**
   * Convenience method to update the panel visibility for properties that may
   * not be relevant if only 1 view is available.
   * @returns true if saving multiple views is feasible, otherwise false.
   */
  bool UpdateSaveAllViewsPanelVisibility();

  /**
   * This method can be used to capture an image for a view for a specific resolution
   * using default options. This is provided as a convenience method to quickly
   * capture images from a view.
   */
  static vtkSmartPointer<vtkImageData> CaptureImage(vtkSMViewProxy* view, const vtkVector2i& size);

  /**
   * This method can be used to capture an image for a layout for a specific resolution
   * using default options. This is provided as a convenience method to quickly
   * capture images from a layout.
   */
  static vtkSmartPointer<vtkImageData> CaptureImage(
    vtkSMViewLayoutProxy* view, const vtkVector2i& size);

  /**
   * Compute magnification factor and new size for target resolution.
   */
  static int ComputeMagnification(const vtkVector2i& targetSize, vtkVector2i& size);

protected:
  vtkSMSaveScreenshotProxy();
  ~vtkSMSaveScreenshotProxy();

  /**
   * Captures rendered image, but assumes that the `Prepare` has already been
   * called successfully.
   */
  virtual vtkSmartPointer<vtkImageData> CapturePreppedImage();

  /**
   * Prepares for saving an image. This will do any changes to view properties
   * necessary for saving appropriate image(s).
   */
  virtual bool Prepare();

  /**
   * This restores the state after the screenshot saving.
   */
  virtual bool Cleanup();

  vtkSMViewLayoutProxy* GetLayout();
  vtkSMViewProxy* GetView();

private:
  vtkSMSaveScreenshotProxy(const vtkSMSaveScreenshotProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMSaveScreenshotProxy&) VTK_DELETE_FUNCTION;

  /**
   * used to save/restore state for the view(s).
   */
  class vtkState;
  class vtkStateView;
  class vtkStateLayout;
  vtkState* State;
};

#endif
