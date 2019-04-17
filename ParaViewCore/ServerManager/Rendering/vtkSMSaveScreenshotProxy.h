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
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
   * Updates default property values for saving the given file.
   */
  virtual void UpdateDefaultsAndVisibilities(const char* filename);

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
   * Compute scale factors and new size for target resolution. This determines
   * integral scale factors (in X and Y) to get a box of size of \c targetSize from a
   * box of maximum size specified by \c size. If \c approximate is non-null,
   * then it is set to true when there no way to do that (e.g. one of the
   * components of the \c targetSize is prime and doesn't match \c size).
   *
   * On success, returns the scale factors and modifies \c size such that size *
   * scaleFactors == targetSize is possible. If not, size * scaleFactors <
   * targetSize and approximate if non-null, is set to true.
   *
   */
  static vtkVector2i GetScaleFactorsAndSize(
    const vtkVector2i& targetSize, vtkVector2i& size, bool* approximate = nullptr);

  /**
   * Compute a single magnification factor to reach \c targetSize using a box
   * that fits within \c size. This implementation is inaccurate and may not give
   * target resolution correctly. Hence `GetScaleFactorsAndSize` should be preferred.
   * This method is useful when the interest is in preserving the target aspect
   * ratio as closely as possible than reaching the target size.
   */
  static int ComputeMagnification(const vtkVector2i& targetSize, vtkVector2i& size);

  //@{
  /**
   * Convenience method to derive a QFileDialog friendly format string for
   * extensions supported by this proxy.
   */
  std::string GetFileFormatFilters();
  //@}

protected:
  vtkSMSaveScreenshotProxy();
  ~vtkSMSaveScreenshotProxy() override;

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

  /**
   * Select the format proxy to match the given extension. In otherwords, this
   * changes the "Format" proxy-property to have the writer proxy from the
   * domain that supports the given filename.
   */
  vtkSMProxy* GetFormatProxy(const std::string& filename);

  friend class pqCatalystExportReaction;  // access to GetView,FormatProxy
  friend class pqImmediateExportReaction; // access to GetView,FormatProxy
  friend class pqTemporalExportReaction;  // access to GetView,FormatProxy
private:
  vtkSMSaveScreenshotProxy(const vtkSMSaveScreenshotProxy&) = delete;
  void operator=(const vtkSMSaveScreenshotProxy&) = delete;

  /**
   * used to save/restore state for the view(s).
   */
  class vtkState;
  class vtkStateView;
  class vtkStateLayout;
  vtkState* State;
};

#endif
