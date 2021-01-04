/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkZSpaceInteractorStyle.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkZSpaceInteractorStyle
 *
 * vtkZSpaceInteractorStyle extends vtkInteractorStyle3D to override command methods.
 *
 * This class maps EventDataDevice3D device and input to an interaction state :
 *
 *  - LeftButton (LeftController + Trigger) maps to VTKIS_PICK. It asks vtkPVZSpaceView to use its
 *    own PVHardwareSelector to pick a cell or a point, depending on the value of
 * vtkPVZSpaceView::PickingFieldAssociation
 *    Then informations about picking is shown on the bottom left of the screen. A pick actor is
 * also shown to visualize
 *    the picked cell or picked point.
 *
 *  - MiddleButton (GenericTracker + Trigger) maps to VTKIS_POSITION_PROP. It allows the user to
 * grab the
 *    picked actor and move it with the stylus.
 *
 *  - RightButton (RightController + Trigger) allows to position the widgets that respond to
 *   this vtkEventDataDevice3D, such as vtkBoxWidget2, vtkHandleWidget, vtkImplicitPlaneWidget2 and
 *   vtkTensorWidget. It doesn't map to any VTKIS_XXX.
 *
 *  The move event will then call the method to position the current picked prop
 *  if the state is VTKIS_POSITION_PROP.
 */

#ifndef vtkZSpaceInteractorStyle_h
#define vtkZSpaceInteractorStyle_h

#include "vtkZSpaceViewModule.h" // For export macro

#include "vtkEventData.h" // for enums
#include "vtkInteractorStyle3D.h"
#include "vtkNew.h" // for ivars

#include <array>

class vtkCell;
class vtkPlane;
class vtkSelection;
class vtkDataSet;
class vtkTextActor;
class vtkZSpaceRayActor;
class vtkPVZSpaceView;

class VTKZSPACEVIEW_EXPORT vtkZSpaceInteractorStyle : public vtkInteractorStyle3D
{
public:
  static vtkZSpaceInteractorStyle* New();
  vtkTypeMacro(vtkZSpaceInteractorStyle, vtkInteractorStyle3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Override generic event bindings to call the corresponding action.
   */
  void OnButton3D(vtkEventData* edata) override;
  void OnMove3D(vtkEventData* edata) override;
  //@}

  //@{
  /**
   * Interaction mode entry points.
   */
  virtual void StartPick(vtkEventDataDevice3D*);
  virtual void EndPick(vtkEventDataDevice3D*);
  virtual void StartPositionProp(vtkEventDataDevice3D*);
  virtual void EndPositionProp(vtkEventDataDevice3D*);
  //@}

  //@{
  /**
   * Methods for interaction.
   */
  void ProbeData();
  virtual void PositionProp(vtkEventData*, double* lwpos = nullptr, double* lwori = nullptr);
  //@}

  //@{
  /**
   * Indicates if picking should be updated every frame. If so, the interaction
   * picker will try to pick a prop and ray will be updated accordingly.
   * Default is set to off.
   */
  vtkSetMacro(InteractivePicking, bool);
  vtkGetMacro(InteractivePicking, bool);
  vtkBooleanMacro(InteractivePicking, bool);
  //@}

  /**
   * Use FindPickedActor to update the InteractionProp.
   * Then update the ray length to the pick length if something is picked,
   * else to its max length.
   */
  void UpdateRay(vtkEventDataDevice3D*);

  /**
   * Map controller inputs to actions.
   * Actions are defined by a VTKIS_*STATE*, interaction entry points,
   * and the corresponding method for interaction.
   */
  void MapInputToAction(vtkEventDataDevice device, vtkEventDataDeviceInput input, int state);

  /**
   * Set the zSpaceRayActor that is used to draw the ray stylus.
   */
  vtkSetMacro(ZSpaceRayActor, vtkZSpaceRayActor*);

  /**
   * Set the zSpaceView to delegate the hardware select to its PVHardwareSelector.
   */
  vtkSetMacro(ZSpaceView, vtkPVZSpaceView*);

  /**
   * Set the ray maximum length. This is given by vtkPVZSpaceView depending
   * on the viewer scale.
   */
  vtkSetMacro(RayMaxLength, double);

protected:
  vtkZSpaceInteractorStyle();
  ~vtkZSpaceInteractorStyle() override;

  /**
   * Create the text to display information about the selection,
   * create the PickActor to draw the picked cell or point and add
   * it to the renderer.
   */
  void EndPickCallback(vtkSelection* sel);

  //@{
  /** Utility routines
  */
  void StartAction(int VTKIS_STATE, vtkEventDataDevice3D* edata);
  void EndAction(int VTKIS_STATE, vtkEventDataDevice3D* edata);
  //@}

  /**
   * Delegate the selection to the PVHardwareSelector of ZSpaceView. If
   * something is picked, this->InteractionProp contains the picked actor.
   */
  bool HardwareSelect();

  /**
   * inline function that returns by reference the state in InputMap defined by a device
   * and an input.
   */
  int& GetStateByReference(const vtkEventDataDevice& device, const vtkEventDataDeviceInput& input);

  /**
   * From the selection 'sel', find the corresponding dataset 'ds' and the point/cell id 'aid'.
   */
  bool FindDataSet(vtkSelection* sel, vtkSmartPointer<vtkDataSet>& ds, vtkIdType& aid);

  /**
   * Create a string that contains informations about the point or cell defined by the
   * index 'aid' in the dataset 'ds'.
   */
  std::string GetPickedText(vtkDataSet* ds, const vtkIdType& aid);

  /**
   * Create the PickActor to show the picked cell.
   */
  void CreatePickCell(vtkCell* cell);

  /**
   * Create the PickActor to show the picked point.
   */
  void CreatePickPoint(double* point);

  /**
   * Update the PickActor and the TextActor depending on
   * the PickedInteractionProp position and visibility.
   */
  void UpdatePickActor();

  /**
   * Remove the PickActor and the TextActor from the renderer.
   */
  void RemovePickActor();

  bool InteractivePicking = true;
  double RayMaxLength = 80.0;

  // Used to draw picked cells or points
  vtkNew<vtkActor> PickActor;
  // The text actor is linked to this prop
  vtkProp3D* PickedInteractionProp = nullptr;
  vtkNew<vtkTextActor> TextActor;

  // vtkEventDataNumberOfDevices and vtkEventDataNumberOfInputs are numbers defined in
  // vtkEventData.h. Clang format could see "vtkEventDataNumberOfDevices *" as a pointer
  // so disable formatting to make sure the space will stay here
  // clang-format off
  std::array<int, vtkEventDataNumberOfDevices * vtkEventDataNumberOfInputs> InputMap = {
    VTKIS_NONE
  };
  // clang-format on

  vtkZSpaceRayActor* ZSpaceRayActor;
  vtkPVZSpaceView* ZSpaceView;

private:
  vtkZSpaceInteractorStyle(const vtkZSpaceInteractorStyle&) = delete;
  void operator=(const vtkZSpaceInteractorStyle&) = delete;
};

#endif
