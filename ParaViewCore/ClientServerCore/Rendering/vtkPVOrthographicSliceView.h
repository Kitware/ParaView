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
 * @class   vtkPVOrthographicSliceView
 * View.
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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Initialize the view with an identifier. Unless noted otherwise, this method
   * must be called before calling any other methods on this class.
   * \note CallOnAllProcesses
   */
  virtual void Initialize(unsigned int id) VTK_OVERRIDE;

  /**
   * Overridden to ensure that the SlicePositionAxes3D doesn't get used when
   * determine view bounds.
   */
  virtual void Update() VTK_OVERRIDE;

  enum
  {
    SAGITTAL_VIEW_RENDERER = vtkPVRenderView::NON_COMPOSITED_RENDERER + 1,
    AXIAL_VIEW_RENDERER,
    CORONAL_VIEW_RENDERER,
  };

  /**
   * Overridden to add support for new types of renderers.
   */
  virtual vtkRenderer* GetRenderer(
    int rendererType = vtkPVRenderView::DEFAULT_RENDERER) VTK_OVERRIDE;

  virtual void ResetCamera();
  virtual void ResetCamera(double bounds[6]);
  virtual void SetInteractionMode(int mode) VTK_OVERRIDE;
  virtual void SetupInteractor(vtkRenderWindowInteractor*) VTK_OVERRIDE;

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
  virtual void SetCenterAxesVisibility(bool) VTK_OVERRIDE {}

  //*****************************************************************
  virtual void SetBackground(double r, double g, double b) VTK_OVERRIDE;
  virtual void SetBackground2(double r, double g, double b) VTK_OVERRIDE;
  virtual void SetBackgroundTexture(vtkTexture* val) VTK_OVERRIDE;
  virtual void SetGradientBackground(int val) VTK_OVERRIDE;
  virtual void SetTexturedBackground(int val) VTK_OVERRIDE;

protected:
  vtkPVOrthographicSliceView();
  ~vtkPVOrthographicSliceView();

  virtual void AboutToRenderOnLocalProcess(bool interactive) VTK_OVERRIDE;
  virtual void UpdateCenterAxes() VTK_OVERRIDE;

  //*****************************************************************
  // Forward to vtkPVOrthographicSliceView instances.
  virtual void SetCenterOfRotation(double x, double y, double z) VTK_OVERRIDE;
  virtual void SetRotationFactor(double factor) VTK_OVERRIDE;

  /**
   * Set the vtkPVGridAxes3DActor to use for the view.
   */
  virtual void SetGridAxes3DActor(vtkPVGridAxes3DActor*) VTK_OVERRIDE;

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
  vtkPVOrthographicSliceView(const vtkPVOrthographicSliceView&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVOrthographicSliceView&) VTK_DELETE_FUNCTION;

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
