// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVZSpaceView
 *
 * vtkPVZSpaceView extends vtkPVRenderView to render the scene using Crystal Eyes
 * stereo and interact with it through a zSpace device.
 * The zSpace device is composed of a high definition 3D display coupled with
 * an advanced mixed reality tracking system and angle-awareness sensor.
 *
 * This view is designed to work on full screen (or in a cave display).
 *
 * This only works on Windows as the zSpace SDK is only available on this OS.
 */

#ifndef vtkPVZSpaceView_h
#define vtkPVZSpaceView_h

#include "vtkDataObject.h" // for vtkDataObject enums
#include "vtkNew.h"        // for vtkNew
#include "vtkPVRenderView.h"
#include "vtkZSpaceViewModule.h" // for export macro

class vtkZSpaceInteractorStyle;
class vtkZSpaceRayActor;
class vtkZSpaceCamera;
class vtkZSpaceRenderer;

class VTKZSPACEVIEW_EXPORT vtkPVZSpaceView : public vtkPVRenderView
{
public:
  static vtkPVZSpaceView* New();
  vtkTypeMacro(vtkPVZSpaceView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Called when a ResetCameraEvent is fired. Call zSpaceSDKManager::CalculateFrustumFit
   * to update the viewer scale, near and far plane given by the zSpace SDK.
   */
  void ResetCamera();
  void ResetCamera(double bounds[6]);
  ///@}

  /**
   * Overriden to give to the Interactor the zSpaceInteractorStyle.
   */
  void SetInteractionMode(int mode) override;

  /**
   * Set the physical distance between eyes on the glasses
   * Delegate to zSpace SDK manager.
   * Default is 0.056f.
   */
  void SetInterPupillaryDistance(float);

  /**
   * If true, perform picking every frame to update the stylus ray length each frame.
   * Delegate to zSpaceInteractorStyle.
   */
  void SetInteractivePicking(bool);

  /**
   * Draw or not the ray stylus.
   * Delegate to zSpace renderer.
   * Default is true.
   */
  void SetDrawStylus(bool);

  ///@{
  /**
   * Select the field association used when picking.
   * Default is vtkDataObject::FIELD_ASSOCIATION_CELLS.
   */
  vtkSetClampMacro(PickingFieldAssociation, int, vtkDataObject::FIELD_ASSOCIATION_POINTS,
    vtkDataObject::FIELD_ASSOCIATION_CELLS);
  vtkGetMacro(PickingFieldAssociation, int);
  ///@}

  /**
   * All transformations applied to actors with the stylus can be
   * reset with this method.
   */
  void ResetAllUserTransforms();

protected:
  vtkPVZSpaceView();
  ~vtkPVZSpaceView() override;

  /**
   * Update zSpace SDK manager, the camera and render as usual.
   */
  void Render(bool interactive, bool skip_rendering) override;

  /**
   * Override ResetCameraClippingRange to disable automatic clipping range
   * calculations of the camera as it is done by the zSpace SDK.
   */
  void ResetCameraClippingRange() override{};

  /**
   * Overriden to set a vtkZSpaceRenderWindowInteractor instance as interactor.
   * The interactor in parameter is not used.
   */
  void SetupInteractor(vtkRenderWindowInteractor*) override;

private:
  vtkPVZSpaceView(const vtkPVZSpaceView&) = delete;
  void operator=(const vtkPVZSpaceView&) = delete;

  vtkNew<vtkZSpaceInteractorStyle> ZSpaceInteractorStyle;
  vtkNew<vtkZSpaceRayActor> StylusRayActor;
  vtkNew<vtkZSpaceCamera> ZSpaceCamera;
  vtkNew<vtkZSpaceRenderer> ZSpaceRenderer;

  // The field association used when picking with the ray
  int PickingFieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
};

#endif
