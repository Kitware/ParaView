/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballZoom.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVTrackballZoom
 * @brief   Zooms camera with vertical mouse movement.
 *
 * vtkPVTrackballZoom allows the user to interactively
 * manipulate the camera, the viewpoint of the scene.
 * Moving the mouse down zooms in. Up zooms out.
*/

#ifndef vtkPVTrackballZoom_h
#define vtkPVTrackballZoom_h

#include "vtkCameraManipulator.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVTrackballZoom : public vtkCameraManipulator
{
public:
  static vtkPVTrackballZoom* New();
  vtkTypeMacro(vtkPVTrackballZoom, vtkCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  virtual void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi);
  virtual void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi);
  virtual void OnButtonUp(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi);
  //@}

protected:
  vtkPVTrackballZoom();
  ~vtkPVTrackballZoom();

  double ZoomScale;

  vtkPVTrackballZoom(const vtkPVTrackballZoom&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVTrackballZoom&) VTK_DELETE_FUNCTION;
};

#endif
