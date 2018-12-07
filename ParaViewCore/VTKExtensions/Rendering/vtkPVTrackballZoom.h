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
 *
 * When zooming in Perpective projection, this manipulator by default, dollys
 * (rather than zooms) i.e. moves the camera further (or farther) in the view
 * direction. This is default behavior. You can change that to use zoom instead
 * i.e. change view angle on camera, by setting UseDollyForPerspectiveProjection
 * to off.
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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonUp(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  //@}

  /**
   * Set this to false (true by default) to not dolly in case of perspective
   * projection and use zoom i.e. change view angle, instead.
   */
  vtkSetMacro(UseDollyForPerspectiveProjection, bool);
  vtkGetMacro(UseDollyForPerspectiveProjection, bool);
  vtkBooleanMacro(UseDollyForPerspectiveProjection, bool);

protected:
  vtkPVTrackballZoom();
  ~vtkPVTrackballZoom() override;

  bool UseDollyForPerspectiveProjection;
  double ZoomScale;

  vtkPVTrackballZoom(const vtkPVTrackballZoom&) = delete;
  void operator=(const vtkPVTrackballZoom&) = delete;
};

#endif
