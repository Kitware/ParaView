// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVCameraManipulator
 * @brief   Abstraction of style away from button.
 *
 * vtkPVCameraManipulator is a superclass foractions inside an
 * interactor style and associated with a single button. An example
 * might be rubber-band bounding-box zoom. This abstraction allows a
 * camera manipulator to be assigned to any button.  This super class
 * might become a subclass of vtkInteractorObserver in the future.
 */

#ifndef vtkPVCameraManipulator_h
#define vtkPVCameraManipulator_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsInteractionStyleModule.h" // needed for export macro

class vtkCameraManipulatorGUIHelper;
class vtkRenderer;
class vtkRenderWindowInteractor;

class VTKPVVTKEXTENSIONSINTERACTIONSTYLE_EXPORT vtkPVCameraManipulator : public vtkObject
{
public:
  static vtkPVCameraManipulator* New();
  vtkTypeMacro(vtkPVCameraManipulator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  virtual void StartInteraction();
  virtual void EndInteraction();
  ///@}

  virtual void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren);
  virtual void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren);
  virtual void OnButtonUp(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren);

  ///@{
  /**
   * These methods are called on all registered manipulators, not just the
   * active one. Hence, these should just be used to record state and not
   * perform any interactions.
   */
  virtual void OnKeyUp(vtkRenderWindowInteractor* iren);
  virtual void OnKeyDown(vtkRenderWindowInteractor* iren);
  ///@}

  ///@{
  /**
   * These settings determine which button and modifiers the
   * manipulator responds to. Button can be either 1 (left), 2
   * (middle), and 3 right.
   */
  vtkSetMacro(Button, int);
  vtkGetMacro(Button, int);
  vtkSetMacro(Shift, int);
  vtkGetMacro(Shift, int);
  vtkBooleanMacro(Shift, int);
  vtkSetMacro(Control, int);
  vtkGetMacro(Control, int);
  vtkBooleanMacro(Control, int);
  ///@}

  ///@{
  /**
   * For setting the center of rotation.
   */
  vtkSetVector3Macro(Center, double);
  vtkGetVector3Macro(Center, double);
  ///@}

  ///@{
  /**
   * Set and get the rotation factor.
   */
  vtkSetMacro(RotationFactor, double);
  vtkGetMacro(RotationFactor, double);
  ///@}

  ///@{
  /**
   * Set and get the manipulator name.
   */
  vtkSetStringMacro(ManipulatorName);
  vtkGetStringMacro(ManipulatorName);
  ///@}

  ///@{
  /**
   * Get/Set the GUI helper.
   */
  void SetGUIHelper(vtkCameraManipulatorGUIHelper*);
  vtkGetObjectMacro(GUIHelper, vtkCameraManipulatorGUIHelper);

protected:
  vtkPVCameraManipulator();
  ~vtkPVCameraManipulator() override;
  ///@}

  char* ManipulatorName;

  int Button;
  int Shift;
  int Control;

  double Center[3];
  double RotationFactor;
  double DisplayCenter[2];
  void ComputeDisplayCenter(vtkRenderer* ren);

  vtkCameraManipulatorGUIHelper* GUIHelper;

private:
  vtkPVCameraManipulator(const vtkPVCameraManipulator&) = delete;
  void operator=(const vtkPVCameraManipulator&) = delete;
};

#endif
