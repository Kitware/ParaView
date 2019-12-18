/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiSliceContextItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMultiSliceContextItem
*/

#ifndef vtkMultiSliceContextItem_h
#define vtkMultiSliceContextItem_h

#include "vtkCommand.h" // needed for vtkCommand::UserEvent.
#include "vtkContextItem.h"
#include "vtkRemotingViewsModule.h" // needed for export macro

class vtkAxis;

class VTKREMOTINGVIEWS_EXPORT vtkMultiSliceContextItem : public vtkContextItem
{
public:
  static vtkMultiSliceContextItem* New();
  vtkTypeMacro(vtkMultiSliceContextItem, vtkContextItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Paint the texture into a rectangle defined by the bounds. If
   * MaskAboveCurve is true and a shape has been provided by a subclass, it
   * draws the texture into the shape
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Return the Axis on which that ContextItem is based.
   * In order to configure that item, just configure the Axis itself.
   * (Range + Position)
   */
  vtkAxis* GetAxis();

  /**
   * The active size define the number of pixel that are going to be used for
   * the slider handle.
   */
  void SetActiveSize(int size);

  /**
   * The margin used on the side of the Axis.
   */
  void SetEdgeMargin(int margin);

  /**
   * Returns the active slice index. Active slice is only valid when the slice
   * is being updated between mouse-press/release events.
   */
  int GetActiveSliceIndex();

  /**
   * Return true if the supplied x, y coordinate is inside the item.
   */
  bool Hit(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button down event
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button release event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  bool MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button double click event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  bool MouseDoubleClickEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse move event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  bool MouseMoveEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Get access to the data model. Return a pointer array to the different
   * visible slices
   */
  const double* GetVisibleSlices(int& nbSlices) const;

  /**
   * Returns access to slices.
   */
  const double* GetSlices(int& nbSlices) const;

  /**
   * Allow user to programmatically update the data model. Note, this does not
   * fire any of the slice modification/addition/deletion events.
   */
  void SetSlices(double* values, bool* visibility, int numberOfSlices);

  /**
   * Return the slice position for a given index
   */
  double GetSliceValue(int sliceIndex);

  /**
   * Return the number of slices
   */
  int GetNumberOfSlices();

  // Events files when slices are modified by interaction.
  enum
  {
    AddSliceEvent = vtkCommand::UserEvent + 1,
    RemoveSliceEvent = vtkCommand::UserEvent + 2,
    ModifySliceEvent = vtkCommand::UserEvent + 3
  };

protected:
  double ScreenToRange(float position);
  double ComputeEpsilon(int numberOfPixel = 5);
  void forceRender();

  vtkMultiSliceContextItem();
  ~vtkMultiSliceContextItem() override;

private:
  vtkMultiSliceContextItem(const vtkMultiSliceContextItem&) = delete;
  void operator=(const vtkMultiSliceContextItem&) = delete;

  struct vtkInternal;
  vtkInternal* Internal;
};

#endif
