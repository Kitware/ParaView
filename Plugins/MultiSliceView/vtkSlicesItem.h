/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSlicesItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkSlicesItem

#ifndef __vtkScalarsToColorsItem_h
#define __vtkScalarsToColorsItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkContextItem.h"
#include "vtkRect.h"

class vtkAxis;

class vtkSlicesItem: public vtkContextItem
{
public:
  static vtkSlicesItem* New();
  vtkTypeMacro(vtkSlicesItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Paint the texture into a rectangle defined by the bounds. If
  // MaskAboveCurve is true and a shape has been provided by a subclass, it
  // draws the texture into the shape
  virtual bool Paint(vtkContext2D *painter);

  vtkAxis* GetAxis() { return this->Axis; }

//BTX
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

protected:
  double ScreenToRange(float position);
  double ComputeEpsilon(int numberOfPixel = 5);
  void forceRender();

  vtkSlicesItem();
  virtual ~vtkSlicesItem();
  vtkAxis* Axis;
  vtkRectf ActiveArea;
  int ActiveSliceIndex;

private:
  vtkSlicesItem(const vtkSlicesItem &); // Not implemented.
  void operator=(const vtkSlicesItem &);   // Not implemented.

  struct vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif
