/*=========================================================================

  Program:   ParaView
  Module:    vtkCameraManipulatorGUIHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCameraManipulatorGUIHelper
 * @brief   Helper class for Camera
 * Manipulators.
 *
 * This class is the interaface that defines API for a helper class
 * used by some specialized camera manipulators that needed
 * access to GUI. GUI implementations subclass this.
 * vtkPVInteractorStyle sets the helper on every manipulator,
 * if available so that the manipulator can use it.
 * @sa
 * vtkPVInteractorStyle
*/

#ifndef vtkCameraManipulatorGUIHelper_h
#define vtkCameraManipulatorGUIHelper_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkCameraManipulatorGUIHelper : public vtkObject
{
public:
  vtkTypeMacro(vtkCameraManipulatorGUIHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Called by the manipulator to update the GUI.
   * This typically involves calling processing pending
   * events on the GUI.
   */
  virtual void UpdateGUI() = 0;

  /**
   * Some interactors use the bounds of the active source.
   * The method returns 0 is no active source is present or
   * not supported by GUI, otherwise returns 1 and the bounds
   * are filled into the passed argument array.
   */
  virtual int GetActiveSourceBounds(double bounds[6]) = 0;

  //@{
  /**
   * Called to get/set the translation for the actor for the active
   * source in the active view. If applicable returns 1, otherwise
   * returns 0.
   */
  virtual int GetActiveActorTranslate(double translate[3]) = 0;
  virtual int SetActiveActorTranslate(double translate[3]) = 0;
  //@}

  //@{
  /**
   * Get the center of rotation. Returns 0 if not applicable.
   */
  virtual int GetCenterOfRotation(double center[3]) = 0;

protected:
  vtkCameraManipulatorGUIHelper();
  ~vtkCameraManipulatorGUIHelper() override;
  //@}

private:
  vtkCameraManipulatorGUIHelper(const vtkCameraManipulatorGUIHelper&) = delete;
  void operator=(const vtkCameraManipulatorGUIHelper&) = delete;
};

#endif
