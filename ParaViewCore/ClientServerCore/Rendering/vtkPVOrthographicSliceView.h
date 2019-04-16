/*=========================================================================

  Program:   ParaView
  Module:    vtkPVOrthographicSliceView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVOrthographicSliceView
 * @brief view with 3 orthographic slice views and 1 3D view.
 *
 * vtkPVOrthographicSliceView extends vtkPVMultiSliceView to support showing a
 * quad-view with orthographic views along with the 3D view. Work with
 * vtkPVCompositeOrthographicSliceRepresentation and vtkGeometrySliceRepresentation,
 * this class create a 3 slices for any dataset shown in this view and shows
 * those slices in the orthographic views. The orthographic views themselves are
 * non-composited i.e. the data is simply cloned on all rendering processes
 * (hence we limit ourselves to showing slices alone).
 *
 * @par Interactions:
 * In the orthographic views, users can use the thumb-wheel to change the slice
 * plane (in which case the \c SliceIncrements are used to update the slice
 * position). Additionally, users can double click in any of the orthographic
 * views to move the slice position to that location.
 */

#ifndef vtkPVOrthographicSliceView_h
#define vtkPVOrthographicSliceView_h

#include "vtkPVMultiSliceView.h"

class vtkPVOrthographicSliceViewInteractorStyle;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVOrthographicSliceView : public vtkPVMultiSliceView
{
public:
  static vtkPVOrthographicSliceView* New();
  vtkTypeMacro(vtkPVOrthographicSliceView, vtkPVMultiSliceView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to ensure that the SlicePositionAxes3D doesn't get used when
   * determine view bounds.
   */
  void Update() override;

  enum
  {
    SAGITTAL_VIEW_RENDERER = vtkPVRenderView::NON_COMPOSITED_RENDERER + 1,
    AXIAL_VIEW_RENDERER,
    CORONAL_VIEW_RENDERER,
  };

  /**
   * Overridden to add support for new types of renderers.
   */
  vtkRenderer* GetRenderer(int rendererType = vtkPVRenderView::DEFAULT_RENDERER) override;

  virtual void ResetCamera();
  virtual void ResetCamera(double bounds[6]);
  void SetInteractionMode(int mode) override;
  void SetupInteractor(vtkRenderWindowInteractor*) override;

  //@{
  /**
   * Set the slice position.
   */
  void SetSlicePosition(double x, double y, double z);
  vtkGetVector3Macro(SlicePosition, double);
  //@}

  //@{
  /**
   * Set slice increments.
   */
  vtkSetVector3Macro(SliceIncrements, double);
  //@}

  //@{
  /**
   * Get/Set whether to show slice annotations.
   */
  vtkSetMacro(SliceAnnotationsVisibility, bool);
  vtkGetMacro(SliceAnnotationsVisibility, bool);
  //@}

  /**
   * To avoid confusion, we don't show the center axes at all in this view.
   */
  void SetCenterAxesVisibility(bool) override {}

  //*****************************************************************
  void SetBackground(double r, double g, double b) override;
  void SetBackground2(double r, double g, double b) override;
  void SetBackgroundTexture(vtkTexture* val) override;
  void SetGradientBackground(int val) override;
  void SetTexturedBackground(int val) override;

  /**
   * Overridden to scale the projection viewports appropriately.
   */
  void ScaleRendererViewports(const double viewport[4]) override;

protected:
  vtkPVOrthographicSliceView();
  ~vtkPVOrthographicSliceView() override;

  void AboutToRenderOnLocalProcess(bool interactive) override;
  void UpdateCenterAxes() override;

  //*****************************************************************
  // Forward to vtkPVOrthographicSliceView instances.
  void SetCenterOfRotation(double x, double y, double z) override;
  void SetRotationFactor(double factor) override;

  /**
   * Set the vtkPVGridAxes3DActor to use for the view.
   */
  void SetGridAxes3DActor(vtkPVGridAxes3DActor*) override;

  enum
  {
    SIDE_VIEW = 0,
    TOP_VIEW = 1,
    FRONT_VIEW = 2,

    YZ_PLANE = SIDE_VIEW,
    ZX_PLANE = TOP_VIEW,
    XY_PLANE = FRONT_VIEW,

    AXIAL_VIEW = TOP_VIEW,
    CORONAL_VIEW = FRONT_VIEW,
    SAGITTAL_VIEW = SIDE_VIEW,

    RIGHT_SIDE_VIEW = SIDE_VIEW
  };

  vtkNew<vtkRenderer> Renderers[3];
  vtkNew<vtkPVOrthographicSliceViewInteractorStyle> OrthographicInteractorStyle;
  vtkNew<vtkPVCenterAxesActor> SlicePositionAxes2D[3];
  vtkNew<vtkPVCenterAxesActor> SlicePositionAxes3D;
  vtkNew<vtkTextRepresentation> SliceAnnotations[3];
  vtkSmartPointer<vtkPVGridAxes3DActor> GridAxes3DActors[3];

  double SliceIncrements[3];
  double SlicePosition[3];
  bool SliceAnnotationsVisibility;

private:
  vtkPVOrthographicSliceView(const vtkPVOrthographicSliceView&) = delete;
  void operator=(const vtkPVOrthographicSliceView&) = delete;

  void OnMouseWheelForwardEvent();
  void OnMouseWheelBackwardEvent();
  void MoveSlicePosition(vtkRenderer* ren, double position[3]);

  unsigned long MouseWheelForwardEventId;
  unsigned long MouseWheelBackwardEventId;

  friend class vtkPVOrthographicSliceViewInteractorStyle;

  bool GridAxes3DActorsNeedShallowCopy;
  unsigned long GridAxes3DActorObserverId;
  void OnGridAxes3DActorModified();
};

#endif
