/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraIcon.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCameraIcon - A widget that provides the ability to 
// capture and retrieve the camera properties.
// .SECTION Description
// Widget can save the current camera properties and later retrieve them.

#ifndef __vtkPVCameraIcon_h
#define __vtkPVCameraIcon_h

#include "vtkKWLabel.h"

class vtkKWPushButton;
class vtkPVRenderView;
class vtkCamera;

class VTK_EXPORT vtkPVCameraIcon : public vtkKWLabel
{
public:
  static vtkPVCameraIcon* New();
  vtkTypeRevisionMacro(vtkPVCameraIcon, vtkKWLabel);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication *pvApp, const char *args);
  
  // Description:
  // Set the current render view.
  virtual void SetRenderView(vtkPVRenderView*);

  // Description:
  // Store the current camera from the render view.
  virtual void StoreCamera();

  // Description:
  // If the camera exists, restore the current camera to the render
  // view.
  virtual void RestoreCamera();

  //BTX
  // Description:
  // Get the stored camera as vtkCamera.
  vtkGetObjectMacro(Camera, vtkCamera);
  //ETX

protected:
  vtkPVCameraIcon();
  ~vtkPVCameraIcon();

  vtkPVRenderView* RenderView;
  vtkCamera* Camera;
  int Width;
  int Height;
  
private:
  vtkPVCameraIcon(const vtkPVCameraIcon&); // Not implemented
  void operator=(const vtkPVCameraIcon&); // Not implemented
};

#endif
