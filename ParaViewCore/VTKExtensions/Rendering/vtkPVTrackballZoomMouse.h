/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballZoomMouse.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTrackballZoomMouse - Zooms camera with vertical mouse movement to mouse position.
// .SECTION Description
// vtkPVTrackballZoomMouse is a redifinition of a vtkPVTrackballZoom
// allowing the user to zoom on mouse position

#ifndef vtkPVTrackballZoomMouse_h
#define vtkPVTrackballZoomMouse_h

#include "vtkPVTrackballZoom.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVTrackballZoomMouse : public vtkPVTrackballZoom
{
public:
  static vtkPVTrackballZoomMouse *New();
  vtkTypeMacro(vtkPVTrackballZoomMouse, vtkPVTrackballZoom);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnMouseMove(int x, int y, vtkRenderer *ren,
                           vtkRenderWindowInteractor *rwi);
  virtual void OnButtonDown(int x, int y, vtkRenderer *ren,
                            vtkRenderWindowInteractor *rwi);

protected:
  vtkPVTrackballZoomMouse();
  ~vtkPVTrackballZoomMouse();

  int ZoomPosition[2];

  vtkPVTrackballZoomMouse(const vtkPVTrackballZoomMouse&); // Not implemented
  void operator=(const vtkPVTrackballZoomMouse&); // Not implemented
};

#endif
