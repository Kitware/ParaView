// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkTransferFunctionBoxItem_h
#define vtkTransferFunctionBoxItem_h

#include "vtkControlPointsItem.h"

#include "vtkRemotingViewsModule.h" // needed for export macro

// STL includes
#include <memory> // needed for unique_ptr

// Forward declarations
class vtkImageData;
class vtkTransferFunctionBoxItemInternals;
class vtkPVTransferFunction2DBox;

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
    BoxEditEvent,
    BoxSelectEvent,
    BoxDeleteEvent
  };

  /**
   * Returns the current box as [x0, y0, width, height].
   */
  const vtkRectd& GetBox();

  /**
   * Set position and width with respect to corner 0 (BOTTOM_LEFT).
   */
  void SetBox(double x, double y, double width, double height);

  /**
   * Get access to the texture of this box item
   */
  vtkSmartPointer<vtkImageData> GetTexture() const;

  /*
   * Override to rescale box corners when the valid bounds have changed.
   */
  void SetValidBounds(double x0, double x1, double y0, double y1) override;

  ///@{
  /**
   * Set/Get whether the box should be drawn selected.
   */
  vtkSetMacro(Selected, bool);
  vtkGetMacro(Selected, bool);
  vtkBooleanMacro(Selected, bool);
  ///@}

  ///@{
  /**
   * Set/Get the color to be used for this box.
   */
  virtual void SetBoxColor(double r, double g, double b, double a);
  virtual void SetBoxColor(const double c[4]) { this->SetBoxColor(c[0], c[1], c[2], c[3]); }
  double* GetBoxColor() VTK_SIZEHINT(4);
  virtual void GetBoxColor(double& r, double& g, double& b, double& a);
  virtual void GetBoxColor(double c[4]) { this->GetBoxColor(c[0], c[1], c[2], c[3]); }
  ///@}

  ///@{
  /**
   * Set/Get the internal transfer function control box.
   */
  void SetTransferFunctionBox(vtkPVTransferFunction2DBox* b);
  vtkPVTransferFunction2DBox* GetTransferFunctionBox();
  ///@}

  ///@{
  /**
   * Set/Get an ID used to reference this item.
   * This identifier is used by ParaView to update client/server proxy for the 2D transfer function.
   */
  vtkSetMacro(ID, int);
  vtkGetMacro(ID, int);
  ///@}

protected:
  vtkTransferFunctionBoxItem();
  ~vtkTransferFunctionBoxItem() override;

  vtkIdType AddPoint(double x, double y);
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
    BOTTOM_LEFT = 0,
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

  void emitEvent(unsigned long event, void* params = nullptr) override;

  void MovePoint(vtkIdType pointId, double deltaX, double deltaY);

  void DragBox(double deltaX, double deltaY);

  void DragCorner(vtkIdType cornerId, const double* delta);

  bool Paint(vtkContext2D* painter) override;

  /**
   * Returns true if the supplied x, y coordinate is within the bounds of
   * the box or any of the control points.
   */
  bool Hit(const vtkContextMouseEvent& mouse) override;

  ///@{
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
  ///@}

  /**
   * Highlight this box
   */
  virtual void SelectBox();

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
   * Update the internal representation box corners based on the transfer function box and the valid
   * bounds for the item.
   */
  void UpdateBoxPoints();

  /**
   * Predicate to check whether pointA crosses pointB in either axis after
   * displacing pontA by deltaA.
   */
  bool ArePointsCrossing(vtkIdType pointA, const double* deltaA, vtkIdType pointB);

  /**
   * Points move independently. In order to keep the box rigid when dragging it
   * outside of the chart edges it is first checked whether it stays within
   * bounds.
   */
  bool BoxIsWithinBounds(double deltaX, double deltaY);

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

  // Helper members
  std::unique_ptr<vtkTransferFunctionBoxItemInternals> Internals;

  bool Initialized = false;
  bool Selected = false;
  int ID = -1;
};

#endif // vtkTransferFunctionBoxItem_h
