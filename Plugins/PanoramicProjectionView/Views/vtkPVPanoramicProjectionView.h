// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVPanoramicProjectionView
 *
 * vtkPVPanoramicProjectionView extends vtkPVRenderView in order to take benefit
 * from the panoramic projection pass.
 */

#ifndef vtkPVPanoramicProjectionView_h
#define vtkPVPanoramicProjectionView_h

#include "vtkNew.h" // for vtkNew
#include "vtkPVRenderView.h"
#include "vtkPanoramicProjectionViewsModule.h" // for export macro

class vtkPanoramicProjectionPass;

class VTKPANORAMICPROJECTIONVIEWS_EXPORT vtkPVPanoramicProjectionView : public vtkPVRenderView
{
public:
  static vtkPVPanoramicProjectionView* New();
  vtkTypeMacro(vtkPVPanoramicProjectionView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the projection type
   */
  void SetProjectionType(int type);

  /**
   * Set cubemap resolution used to render (offscreen) all directions
   */
  void SetCubeResolution(int resolution);

  /**
   * Set the angle of projection
   */
  void SetAngle(double angle);

  /**
   * Use hardware interpolation of the cubemap texture during projection. In case of
   * low resolution of the cubemap, it can be useful to enable it. The drawback is that there
   * can be visible artifacts when enabled with some OpenGL implementations.
   */
  void SetCubemapInterpolation(bool interpolate);

  /**
   * FXAA is not supported yet in this view
   * This method has no effect
   */
  void SetUseFXAA(bool) override{};

protected:
  vtkPVPanoramicProjectionView();
  ~vtkPVPanoramicProjectionView() override = default;

private:
  vtkPVPanoramicProjectionView(const vtkPVPanoramicProjectionView&) = delete;
  void operator=(const vtkPVPanoramicProjectionView&) = delete;

  vtkNew<vtkPanoramicProjectionPass> ProjectionPass;
};

#endif
