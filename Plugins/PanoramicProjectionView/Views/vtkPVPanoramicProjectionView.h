/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPanoramicProjectionView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
