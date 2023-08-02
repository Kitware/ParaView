// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkPVVTKExtensionsInteractionStyleModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSINTERACTIONSTYLE_EXPORT vtkPVTrackballZoomToMouse
  : public vtkPVTrackballZoom
{
public:
  static vtkPVTrackballZoomToMouse* New();
  vtkTypeMacro(vtkPVTrackballZoomToMouse, vtkPVTrackballZoom);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  ///@}

protected:
  vtkPVTrackballZoomToMouse();
  ~vtkPVTrackballZoomToMouse() override;

  int ZoomPosition[2];

  vtkPVTrackballZoomToMouse(const vtkPVTrackballZoomToMouse&) = delete;
  void operator=(const vtkPVTrackballZoomToMouse&) = delete;
};

#endif
