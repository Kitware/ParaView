/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraManipulatorGUIHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCameraManipulatorGUIHelper - implementation for ParaView gui.
// .SECTION Description
// vtkCameraManipulatorGUIHelper implementation for ParaView Tk GUI.

#ifndef __vtkPVCameraManipulatorGUIHelper_h
#define __vtkPVCameraManipulatorGUIHelper_h

#include "vtkCameraManipulatorGUIHelper.h"

class vtkPVApplication;

class VTK_EXPORT vtkPVCameraManipulatorGUIHelper:
  public vtkCameraManipulatorGUIHelper
{
public:
  static vtkPVCameraManipulatorGUIHelper* New();
  vtkTypeRevisionMacro(vtkPVCameraManipulatorGUIHelper,
    vtkCameraManipulatorGUIHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called by the manipulator to update the GUI.
  // This typically involves calling processing pending
  // events on the GUI.
  virtual void UpdateGUI();

  // Description:
  // Some interactors use the bounds of the active source.
  // The method returns 0 is no active source is present or
  // not supported by GUI, otherwise returns 1 and the bounds 
  // are filled into the passed argument array.
  virtual int GetActiveSourceBounds(double bounds[6]);

  // Description:
  // Called to get/set the translation for the actor for the active
  // source in the active view. If applicable returns 1, otherwise
  // returns 0.
  virtual int GetActiveActorTranslate(double translate[3]);
  virtual int SetActiveActorTranslate(double translate[3]);

  // Description:
  // Get the center of rotation. Returns 0 if not applicable.
  virtual int GetCenterOfRotation(double center[3]);

  // Description:
  // Get/Set the PVApplication.
  vtkGetObjectMacro(PVApplication, vtkPVApplication);
  void SetPVApplication(vtkPVApplication* app);
protected:
  vtkPVCameraManipulatorGUIHelper();
  ~vtkPVCameraManipulatorGUIHelper();

  vtkPVApplication* PVApplication;
private:
  vtkPVCameraManipulatorGUIHelper(const vtkPVCameraManipulatorGUIHelper&); /// Not implemented.
  void operator=(const vtkPVCameraManipulatorGUIHelper&); // Not implemented.
};


#endif

