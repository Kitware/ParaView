/*=========================================================================

  Program:   ParaView
  Module:    vtkTransferFunctionBoxItem.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkTransferFunctionBoxItem_h
#define vtkTransferFunctionBoxItem_h

#include "vtkControlPointsItem.h"

#include "vtkRemotingViewsModule.h" // needed for export macro

// Forward declarations
class vtkImageData;
class vtkPen;
class vtkPoints2D;

class VTKREMOTINGVIEWS_EXPORT vtkTransferFunctionBoxItem : public vtkControlPointsItem
{
public:
  static vtkTransferFunctionBoxItem* New();
  vtkTypeMacro(vtkTransferFunctionBoxItem, vtkControlPointsItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Events fires by this class (and subclasses).
  // \li BoxAddEvent is fired when this box item is added to a chart.
  // \li BoxEditEvent is fired when this box item is edited, either by dragging the whole box or
  // editing the corner points.
  enum
  {
    BoxAddEvent = vtkCommand::UserEvent + 100,
    BoxEditEvent
  };

  /**
   * Returns the curren box as [x0, y0, width, height].
   */
  const vtkRectd& GetBox();

  /**
   * Set position and width with respect to corner 0 (BOTTOM_LEFT).
   */
  void SetBox(const double x, const double y, const double width, const double height);

  /**
   * Get access to the texture of this box item
   */
  vtkSmartPointer<vtkImageData> GetTexture() const;

  /*
   * Override to rescale box corners when the valid bounds have changed.
   */
  virtual void SetValidBounds(double x0, double x1, double y0, double y1) override;

protected:
  vtkTransferFunctionBoxItem();
  ~vtkTransferFunctionBoxItem();

  vtkIdType AddPoint(const double x, const double y);
  vtkIdType AddPoint(double* pos) override;

  /**
   * Box corners are ordered as follows:
   *      3 ----- 2
   *      |       |
   *  (4) 0 ----- 1
   *
   * Point 0 is repeated for rendering purposes (vtkContext2D::DrawPoly
   * requires it to close the outline). This point is not registered with
   * vtkControlPointsItem.
   */
  enum BoxCorners
  {
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    TOP_RIGHT,
    TOP_LEFT,
    BOTTOM_LEFT_LOOP
  };

  /**
   * This method does nothing as this item has a fixed number of points (4).
   */
  vtkIdType RemovePoint(double* pos) override;

  vtkIdType GetNumberOfPoints() const override;

  void GetControlPoint(vtkIdType index, double* point) const override;

  vtkMTimeType GetControlPointsMTime() override;

  void SetControlPoint(vtkIdType index, double* point) override;

  void emitEvent(unsigned long event, void* params = 0) override;

  void MovePoint(const vtkIdType pointId, const double deltaX, const double deltaY);

  void DragBox(const double deltaX, const double deltaY);

  void DragCorner(const vtkIdType cornerId, const double* delta);

  bool Paint(vtkContext2D* painter) override;

  /**
   * Returns true if the supplied x, y coordinate is within the bounds of
   * the box or any of the control points.
   */
  bool Hit(const vtkContextMouseEvent& mouse) override;

  //@{
  /**
   * \brief Interaction overrides.
   * The box item can be dragged around the chart area by clicking within
   * the box and moving the cursor.  The size of the box can be manipulated by
   * clicking on the control points and moving them. No key events are currently
   * reimplemented.
   */
  bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) override;
  bool MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse) override;
  bool MouseDoubleClickEvent(const vtkContextMouseEvent& mouse) override;
  bool MouseMoveEvent(const vtkContextMouseEvent& mouse) override;
  bool KeyPressEvent(const vtkContextKeyEvent& key) override;
  bool KeyReleaseEvent(const vtkContextKeyEvent& key) override;
  //@}

  virtual void ComputeTexture();

private:
  /**
   * Custom method to clamp point positions to valid bounds (chart bounds).  A
   * custom method was required given that ControlPoints::ClampValidPos()
   * appears
   * to have bug where it does not not clamp to bounds[2,3].  The side effects
   * of
   * overriding that behavior are unclear so for now this custom method is used.
   */
  void ClampToValidPosition(double pos[2]);

  /**
   * Predicate to check whether pointA crosses pointB in either axis after
   * displacing pontA by deltaA.
   */
  bool ArePointsCrossing(const vtkIdType pointA, const double* deltaA, const vtkIdType pointB);

  /**
   * Points move independently. In order to keep the box rigid when dragging it
   * outside of the chart edges it is first checked whether it stays within
   * bounds.
   */
  bool BoxIsWithinBounds(const double deltaX, const double deltaY);

  // bool IsInitialized();
  // bool NeedsTextureUpdate();

  /**
   * Customized vtkControlPointsItem::FindPoint implementation for this Item.
   * vtkControlPointsItem::FindPoint stops searching for control points once the
   * (x-coord of the mouse click) < (current control point x-coord); points are
   * expected to be in ascending order with respect to x. In this Item, the
   * corners
   * of the box are ordered CCW.
   *
   * \sa vtkControlPointsItem::FindPoint
   */
  vtkIdType FindBoxPoint(double* _pos);

  vtkNew<vtkPoints2D> BoxPoints;
  const int NumPoints = 5;
  vtkRectd Box;

  vtkNew<vtkPen> Pen;
  vtkNew<vtkImageData> Texture;
  bool Initialized = false;
};

#endif // vtkTransferFunctionBoxItem_h
