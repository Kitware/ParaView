// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVStereoCursorView
 * @brief Render View displaying a 3D Cursor
 *
 * The Stereo Cursor View is a Render View containing a 3D cursor, whose aim is to be displayed
 * in stereo like any actor in the render view.
 *
 * The 3D cursor is a widget that is created alongside the view. It is placed in the scene depending
 * on the position of the mouse: if it hovers an actor, the cursor is placed on its surface.
 * If not, the cursor is placed on the focal plane of the camera.
 * For now the cursor only work with surfacic data.
 *
 * @sa vtk3DCursorWidget vtk3DCursorRepresentation
 */

#ifndef vtkPVStereoCursorView_h
#define vtkPVStereoCursorView_h

#include "StereoCursorViewsModule.h" // For export macro
#include "vtkPVRenderView.h"

#include <memory> // For std::unique_ptr

class vtk3DCursorWidget;
class vtkRenderWindowInteractor;

class STEREOCURSORVIEWS_EXPORT vtkPVStereoCursorView : public vtkPVRenderView
{
public:
  static vtkPVStereoCursorView* New();
  vtkTypeMacro(vtkPVStereoCursorView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the size of the cursor (in pixels)
   */
  void SetCursorSize(int size);

  /**
   * Set the cursor shape.
   */
  void SetCursorShape(int cursorShape);

protected:
  vtkPVStereoCursorView();
  ~vtkPVStereoCursorView() override = default;

  /**
   * Set the interactor. Client applications must set the interactor to enable
   * interactivity. Note this method will also change the interactor styles set
   * on the interactor.
   */
  void SetupInteractor(vtkRenderWindowInteractor*) override;

private:
  vtkPVStereoCursorView(const vtkPVStereoCursorView&) = delete;
  void operator=(const vtkPVStereoCursorView&) = delete;

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif // vtkPVStereoCursorView_h
