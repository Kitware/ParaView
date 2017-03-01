/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballZoomToMouse.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVTrackballZoomToMouse
 * @brief   Zooms camera with vertical mouse movement to mouse position.
 *
 * vtkPVTrackballZoomToMouse is a redifinition of a vtkPVTrackballZoom
 * allowing the user to zoom at the point projected under the mouse position.
*/

#ifndef vtkPVTrackballZoomToMouse_h
#define vtkPVTrackballZoomToMouse_h

#include "vtkPVTrackballZoom.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVTrackballZoomToMouse : public vtkPVTrackballZoom
{
public:
  static vtkPVTrackballZoomToMouse* New();
  vtkTypeMacro(vtkPVTrackballZoomToMouse, vtkPVTrackballZoom);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  virtual void OnMouseMove(
    int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) VTK_OVERRIDE;
  virtual void OnButtonDown(
    int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) VTK_OVERRIDE;
  //@}

protected:
  vtkPVTrackballZoomToMouse();
  ~vtkPVTrackballZoomToMouse();

  int ZoomPosition[2];

  vtkPVTrackballZoomToMouse(const vtkPVTrackballZoomToMouse&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVTrackballZoomToMouse&) VTK_DELETE_FUNCTION;
};

#endif
