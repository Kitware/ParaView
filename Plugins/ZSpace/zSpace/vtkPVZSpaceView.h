/*=========================================================================

  Program:   ParaView
  Module:    vtkPVZSpaceView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 *
 * This class holds a vtkZSpaceSDKManager object to communicate with the zSpace SDK :
 * -  get the camera view and projection matrix from the zSpace sdk and give it to the
 *    actual camera
 * -  handle interactions by getting stylus state from zSpace sdk and invoking event for the zSpace
 * interactor style.
 *    Notice that the interactor style responds to a Button3DEvent or Move3DEvent, so
 *    the EventDataDevice3D device, input and action need to be set for each action.
 *    Here is the list of associations ( in the format Device + Input + Action ) :
 *      - MiddleButton of the stylus : GenericTracker  + Trigger + Press/Release
 *      - RightButton of the stylus  : RightController + Trigger + Press/Release
 *      - LeftButton of the stylus   : LeftController  + Trigger + Press/Release
 *
 *    Please refer to vtkZSpaceInteractorStyle to know what are the possible interactions.
 *
 * Notice that this PVView stores the current/last world event position and orientation
 * to simulate the RenderWindowInteractor3D behavior, as ParaView doesn't support
 * a RenderWindowInteractor3D for instance. This should be removed when this feature is supported.
 */

#ifndef vtkPVZSpaceView_h
#define vtkPVZSpaceView_h

#include "vtkDataObject.h"
#include "vtkNew.h" // for vtkNew
#include "vtkPVRenderView.h"
#include "vtkZSpaceViewModule.h" // for export macro

class vtkZSpaceInteractorStyle;
class vtkZSpaceSDKManager;
class vtkZSpaceRayActor;
class vtkProp3D;
class vtkEventDataDevice3D;
class vtkZSpaceCamera;

class VTKZSPACEVIEW_EXPORT vtkPVZSpaceView : public vtkPVRenderView
{
public:
  static vtkPVZSpaceView* New();
  vtkTypeMacro(vtkPVZSpaceView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Called when a ResetCameraEvent is fired. Call zSpaceSDKManager::CalculateFit
   * to update the viewer scale, near and far plane given by the zSpace SDK.
   */
  void ResetCamera();
  void ResetCamera(double bounds[6]);
  //@}

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
   * If true, perform picking every frame
   * Delegate to zSpaceInteractorStyle.
   * Default is true.
   */
  void SetInteractivePicking(bool);

  /**
   * Draw or not the ray stylus
   * Delegate to zSpace renderer.
   * Default is true.
   */
  void SetDrawStylus(bool);

  /**
   * Perform an hardware picking with a ray defined by the ZSpaceSDKManager
   * ray transform. Configure the camera to be at the origin of the
   * ray, looking in the direction of the ray, and pick the center of
   * the viewport.
   * Restore previous camera settings at the end.
   */
  void SelectWithRay(vtkProp3D* prop = nullptr);

  //@{
  /**
   * Select the field association used when picking.
   * Default is vtkDataObject::FIELD_ASSOCIATION_CELLS.
   */
  vtkSetClampMacro(PickingFieldAssociation, int, vtkDataObject::FIELD_ASSOCIATION_POINTS,
    vtkDataObject::FIELD_ASSOCIATION_CELLS);
  vtkGetMacro(PickingFieldAssociation, int);
  //@}

  //@{
  /**
   * Get/Set the current world event position and orientation.
   * This is needed to simulate the RenderWindowInteractor3D behavior.
   */
  vtkGetVector3Macro(WorldEventPosition, double);
  vtkSetVector3Macro(WorldEventPosition, double);
  vtkGetVector4Macro(WorldEventOrientation, double);
  vtkSetVector4Macro(WorldEventOrientation, double);
  //@}

  //@{
  /**
   * Get/Set the last world event position and orientation.
   * This is needed to simulate the RenderWindowInteractor3D behavior.
   */
  vtkGetVector3Macro(LastWorldEventPosition, double);
  vtkSetVector3Macro(LastWorldEventPosition, double);
  vtkGetVector4Macro(LastWorldEventOrientation, double);
  vtkSetVector4Macro(LastWorldEventOrientation, double);
  //@}

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
   * Give to the zSpace SDK the scene bounds to let it choose the best
   * viewer scale, near and far clip. The camera position is set with the
   * zSpace advised position.
   */
  void CalculateFit(double* bounds);

  /**
   * Override ResetCameraClippingRange to disable automatic clipping range
   * calculations of the camera as it is done by the zSpace SDK.
   */
  void ResetCameraClippingRange() override{};

  /**
   * Update WorldEventPosition and WorldEventOrientation, then
   * call event functions depending on the zSpace buttons states.
   */
  void HandleInteractions();

  //@{
  /**
   * LeftButton event function (invoke Button3DEvent)
   * Initiate a clip : choose a clipping plane origin
   * and normal with the stylus.
   */
  void OnLeftButtonDown(vtkEventDataDevice3D*);
  void OnLeftButtonUp(vtkEventDataDevice3D*);
  //@}

  //@{
  /**
   * MiddleButton event function (invoke Button3DEvent)
   * Allows to position a prop with the stylus.
   */
  void OnMiddleButtonDown(vtkEventDataDevice3D*);
  void OnMiddleButtonUp(vtkEventDataDevice3D*);
  //@}

  //@{
  /**
   * LeftButton event function (invoke Button3DEvent)
   * Perform an hardware picking with the stylus
   * and show picked data if ShowPickedData is true.
   */
  void OnRightButtonDown(vtkEventDataDevice3D*);
  void OnRightButtonUp(vtkEventDataDevice3D*);
  //@}

  /**
   * Invoke a Move3DEvent
   * Called at each step.
   */
  void OnStylusMove();

private:
  vtkPVZSpaceView(const vtkPVZSpaceView&) = delete;
  void operator=(const vtkPVZSpaceView&) = delete;

  friend class vtkPVZSpaceViewInteractorStyle;

  vtkNew<vtkZSpaceSDKManager> ZSpaceSDKManager;
  vtkNew<vtkZSpaceInteractorStyle> ZSpaceInteractorStyle;
  vtkNew<vtkZSpaceRayActor> StylusRayActor;
  vtkNew<vtkZSpaceCamera> ZSpaceCamera;

  bool DrawStylus = true;

  // The field association used when picking with the ray
  int PickingFieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;

  double WorldEventPosition[3] = {};
  double WorldEventOrientation[4] = {};

  double LastWorldEventPosition[3] = {};
  double LastWorldEventOrientation[4] = {};
};

#endif
