/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractorStyle.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVInteractorStyle
 * @brief   interactive manipulation of the camera
 *
 * vtkPVInteractorStyle allows the user to interactively
 * manipulate the camera, the viewpoint of the scene.
 * The left button is for rotation; shift + left button is for rolling;
 * the right button is for panning; and shift + right button is for zooming.
 * This class fires vtkCommand::StartInteractionEvent and
 * vtkCommand::EndInteractionEvent to signal start and end of interaction.
*/

#ifndef vtkPVInteractorStyle_h
#define vtkPVInteractorStyle_h

#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class vtkCameraManipulator;
class vtkCollection;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVInteractorStyle
  : public vtkInteractorStyleTrackballCamera
{
public:
  static vtkPVInteractorStyle* New();
  vtkTypeMacro(vtkPVInteractorStyle, vtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  virtual void OnMouseMove() VTK_OVERRIDE;
  virtual void OnLeftButtonDown() VTK_OVERRIDE;
  virtual void OnLeftButtonUp() VTK_OVERRIDE;
  virtual void OnMiddleButtonDown() VTK_OVERRIDE;
  virtual void OnMiddleButtonUp() VTK_OVERRIDE;
  virtual void OnRightButtonDown() VTK_OVERRIDE;
  virtual void OnRightButtonUp() VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Unlike mouse events, these are forwarded to all camera manipulators
   * since we don't have a mechanism to activate a manipulator by key presses
   * currently.
   */
  virtual void OnKeyDown() VTK_OVERRIDE;
  virtual void OnKeyUp() VTK_OVERRIDE;
  //@}

  /**
   * Overrides superclass behaviors to only support the key codes that make
   * sense in a ParaView application.
   */
  virtual void OnChar() VTK_OVERRIDE;

  /**
   * Access to adding or removing manipulators.
   */
  void AddManipulator(vtkCameraManipulator* m);

  /**
   * Removes all manipulators.
   */
  void RemoveAllManipulators();

  //@{
  /**
   * Accessor for the collection of camera manipulators.
   */
  vtkGetObjectMacro(CameraManipulators, vtkCollection);
  //@}

  //@{
  /**
   * Propagates the center to the manipulators.
   * This simply sets an interal ivar.
   * It is propagated to a manipulator before the event
   * is sent to it.
   * Also changing the CenterOfRotation during interaction
   * i.e. after a button press but before a button up
   * has no effect until the next button press.
   */
  vtkSetVector3Macro(CenterOfRotation, double);
  vtkGetVector3Macro(CenterOfRotation, double);
  //@}

  //@{
  /**
   * Propagates the rotation factor to the manipulators.
   * This simply sets an interal ivar.
   * It is propagated to a manipulator before the event
   * is sent to it.
   * Also changing the RotationFactor during interaction
   * i.e. after a button press but before a button up
   * has no effect until the next button press.
   */
  vtkSetMacro(RotationFactor, double);
  vtkGetMacro(RotationFactor, double);
  //@}

  /**
   * Returns the chosen manipulator based on the modifiers.
   */
  virtual vtkCameraManipulator* FindManipulator(int button, int shift, int control);

  /**
   * Dolly the renderer's camera to a specific point
   */
  static void DollyToPosition(double fact, int* position, vtkRenderer* renderer);

  /**
   * Translate the renderer's camera
   */
  static void TranslateCamera(vtkRenderer* renderer, int toX, int toY, int fromX, int fromY);

  using vtkInteractorStyleTrackballCamera::Dolly;

protected:
  vtkPVInteractorStyle();
  ~vtkPVInteractorStyle();

  virtual void Dolly(double factor) VTK_OVERRIDE;

  vtkCameraManipulator* CurrentManipulator;
  double CenterOfRotation[3];
  double RotationFactor;

  // The CameraInteractors also store there button and modifier.
  vtkCollection* CameraManipulators;

  void OnButtonDown(int button, int shift, int control);
  void OnButtonUp(int button);
  void ResetLights();

  vtkPVInteractorStyle(const vtkPVInteractorStyle&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVInteractorStyle&) VTK_DELETE_FUNCTION;
};

#endif
