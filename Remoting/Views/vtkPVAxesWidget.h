/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVAxesWidget
 * @brief   A widget to manipulate vtkPVAxesWidget.
 *
 *
 * This widget creates and manages its own vtkPVAxesActor. To use this widget,
 * make sure you call SetParentRenderer and SetInteractor (if interactivity is
 * needed). Use `SetEnabled` to enable/disable interactivity and `SetVisibility`
 * to show/hide the axes.
 *
 * @note This is an old class that uses old style for create widgets. Please
 * don't use it as a reference for creating similar elements.
*/

#ifndef vtkPVAxesWidget_h
#define vtkPVAxesWidget_h

#include "vtkInteractorObserver.h"
#include "vtkRemotingViewsModule.h" // needed for export macro

class vtkActor2D;
class vtkKWApplication;
class vtkPolyData;
class vtkPVAxesActor;
class vtkRenderer;

class VTKREMOTINGVIEWS_EXPORT vtkPVAxesWidget : public vtkInteractorObserver
{
public:
  static vtkPVAxesWidget* New();
  vtkTypeMacro(vtkPVAxesWidget, vtkInteractorObserver);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Layer number to use for the internal renderer created by vtkPVAxesWidget.
   */
  static const int RendererLayer = 1;

  //@{
  /**
   * Set/get the axes actor to be displayed in this 3D widget.
   */
  void SetAxesActor(vtkPVAxesActor* actor);
  vtkGetObjectMacro(AxesActor, vtkPVAxesActor);
  //@}

  //@{
  /**
   * Set the renderer this 3D widget will be contained in.
   */
  void SetParentRenderer(vtkRenderer* ren);
  vtkRenderer* GetParentRenderer();
  //@}

  /**
   * Overridden to add interaction observers.
   */
  void SetInteractor(vtkRenderWindowInteractor* iren) override;

  /**
   * Get the renderer.
   */
  vtkGetObjectMacro(Renderer, vtkRenderer);

  /**
   * Overridden to update this->Enabled and hide outline when disabled.
   * Use this method to enable/disable interactivity.
   */
  void SetEnabled(int) override;

  //@{
  /**
   * Get/Set the visibility. Note if visibility is off, Enabled state is ignored
   * and assumed off.
   */
  void SetVisibility(bool val);
  bool GetVisibility();
  //@}

  //@{
  /**
   * Set/get the color of the outline of this widget.  The outline is visible
   * when (in interactive mode) the cursor is over this 3D widget.
   */
  void SetOutlineColor(double r, double g, double b);
  double* GetOutlineColor();
  //@}

  //@{
  /**
   * Set/get the color of the axis labels of this widget.
   */
  void SetAxisLabelColor(double r, double g, double b);
  double* GetAxisLabelColor();
  //@}

  //@{
  /**
   * Set/get the viewport to position/size this 3D widget.
   */
  void SetViewport(double minX, double minY, double maxX, double maxY);
  double* GetViewport();
  //@}

protected:
  vtkPVAxesWidget();
  ~vtkPVAxesWidget() override;

  vtkRenderer* Renderer;
  vtkRenderer* ParentRenderer;

  vtkPVAxesActor* AxesActor;
  vtkPolyData* Outline;
  vtkActor2D* OutlineActor;

  static void ProcessEvents(
    vtkObject* object, unsigned long event, void* clientdata, void* calldata);

  /**
   * Callback to keep the camera for the axes actor up to date with the
   * camera in the parent renderer
   */
  void UpdateCameraFromParentRenderer();

  int MouseCursorState;
  int Moving;
  int StartPosition[2];

  void UpdateCursorIcon();
  void SetMouseCursor(int cursorState);

  enum AxesWidgetState
  {
    Outside = 0,
    Inside,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
  };

  void OnButtonPress();
  void OnMouseMove();
  void OnButtonRelease();
  void MoveWidget();
  void ResizeTopLeft();
  void ResizeTopRight();
  void ResizeBottomLeft();
  void ResizeBottomRight();
  void SquareRenderer();

  unsigned long StartEventObserverId;

private:
  vtkPVAxesWidget(const vtkPVAxesWidget&) = delete;
  void operator=(const vtkPVAxesWidget&) = delete;
};

#endif
