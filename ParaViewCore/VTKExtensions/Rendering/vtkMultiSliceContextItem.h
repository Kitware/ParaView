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
// .NAME vtkMultiSliceContextItem

#ifndef vtkMultiSliceContextItem_h
#define vtkMultiSliceContextItem_h

#include "vtkCommand.h" // needed for vtkCommand::UserEvent.
#include "vtkContextItem.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class vtkAxis;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkMultiSliceContextItem: public vtkContextItem
{
public:
  static vtkMultiSliceContextItem* New();
  vtkTypeMacro(vtkMultiSliceContextItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Paint the texture into a rectangle defined by the bounds. If
  // MaskAboveCurve is true and a shape has been provided by a subclass, it
  // draws the texture into the shape
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Return the Axis on which that ContextItem is based.
  // In order to configure that item, just configure the Axis itself.
  // (Range + Position)
  vtkAxis* GetAxis();

  // Description:
  // The active size define the number of pixel that are going to be used for
  // the slider handle.
  void SetActiveSize(int size);

  // Description:
  // The margin used on the side of the Axis.
  void SetEdgeMargin(int margin);

  // Description:
  // Returns the active slice index. Active slice is only valid when the slice
  // is being updated between mouse-press/release events.
  int GetActiveSliceIndex();

  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button down event
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button release event.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button double click event.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseDoubleClickEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse move event.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Get access to the data model. Return a pointer array to the differents
  // visible slices
  const double* GetVisibleSlices(int &nbSlices) const;

  // Description:
  // Returns access to slices.
  const double* GetSlices(int &nbSlices) const;

  // Description:
  // Allow user to programatically update the data model. Note, this does not
  // fire any of the slice modification/addition/deletion events.
  void SetSlices(double* values, bool* visibility, int numberOfSlices);

  // Description:
  // Return the slice position for a given index
  double GetSliceValue(int sliceIndex);

  // Description:
  // Return the number of slices
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
  virtual ~vtkMultiSliceContextItem();

private:
  vtkMultiSliceContextItem(const vtkMultiSliceContextItem &) VTK_DELETE_FUNCTION;
  void operator=(const vtkMultiSliceContextItem &) VTK_DELETE_FUNCTION;

  struct vtkInternal;
  vtkInternal* Internal;

};

#endif
