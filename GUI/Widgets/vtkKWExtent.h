/*=========================================================================

  Module:    vtkKWExtent.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWExtent - six sliders defining a (xmin,xmax,ymin,ymax,zmin,zmax) extent
// .SECTION Description
// vtkKWExtent is a widget containing six sliders which represent the
// xmin, xmax, ymin, ymax, zmin, zmax extent of a volume. It is a 
// convinience object and has logic to keep the min values less than
// or equal to the max values.

#ifndef __vtkKWExtent_h
#define __vtkKWExtent_h

#include "vtkKWCompositeWidget.h"

#include "vtkKWRange.h" // Needed for some constants

class KWWIDGETS_EXPORT vtkKWExtent : public vtkKWCompositeWidget
{
public:
  static vtkKWExtent* New();
  vtkTypeRevisionMacro(vtkKWExtent,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Set the Range of the Extent, this is the range of
  // acceptable values for the sliders. Specified as 
  // minx maxx miny maxy minz maxz
  virtual void SetExtentRange(double*);
  virtual void SetExtentRange(double, double, double, double, double, double);
  virtual double* GetExtentRange();
  virtual void GetExtentRange(
    double&, double&, double&, double&, double&, double&);
  virtual void GetExtentRange(double*);
  
  // Description:
  // Set/Get the Extent.
  vtkGetVector6Macro(Extent,double);
  virtual void SetExtent(double*);
  virtual void SetExtent(double, double, double, double, double, double);

  // Description:
  // Set/Get the visibility of the extent selectively (x, y, z).
  virtual void SetExtentVisibility(int index, int arg);
  vtkBooleanMacro(XExtentVisibility, int);
  virtual int GetXExtentVisibility() { return this->ExtentVisibility[0]; };
  virtual void SetXExtentVisibility(int arg) 
    { this->SetExtentVisibility(0, arg); };
  vtkBooleanMacro(YExtentVisibility, int);
  virtual int GetYExtentVisibility() { return this->ExtentVisibility[1]; };
  virtual void SetYExtentVisibility(int arg) 
    { this->SetExtentVisibility(1, arg); };
  vtkBooleanMacro(ZExtentVisibility, int);
  virtual int GetZExtentVisibility() { return this->ExtentVisibility[2]; };
  virtual void SetZExtentVisibility(int arg) 
    { this->SetExtentVisibility(2, arg); };

  // Description:
  // Handle the callback, this is called internally when one of the 
  // sliders has been moved.
  void ExtentChangedCallback();

  // Description:
  // A method to set callback functions on objects.  The first argument is
  // the KWObject that will have the method called on it.  The second is the
  // name of the method to be called and any arguments in string form.
  // The calling is done via TCL wrappers for the KWObject.
  virtual void SetCommand(vtkObject *obj, const char *method);

  // Description:
  // A convenience method to set the start and end method of all the
  // internal ranges.  
  virtual void SetStartCommand(vtkObject *obj, const char *method);
  virtual void SetEndCommand(vtkObject *obj, const char *method);

  // Description:
  // Convenience method to set whether the command should be called or not.
  // This just propagates SetDisableCommands to the internal ranges.
  virtual void SetDisableCommands(int);
  vtkBooleanMacro(DisableCommands, int);

  // Description:
  // Convenience method to set the ranges orientations and item positions.
  // This just propagates the same method to the internal ranges.
  virtual void SetOrientation(int);
  virtual void SetOrientationToHorizontal()
    { this->SetOrientation(vtkKWRange::OrientationHorizontal); };
  virtual void SetOrientationToVertical() 
    { this->SetOrientation(vtkKWRange::OrientationVertical); };
  virtual void SetLabelPosition(int);
  virtual void SetLabelPositionToDefault()
    { this->SetLabelPosition(vtkKWWidgetWithLabel::LabelPositionDefault); };
  virtual void SetLabelPositionToTop()
    { this->SetLabelPosition(vtkKWWidgetWithLabel::LabelPositionTop); };
  virtual void SetLabelPositionToBottom()
    { this->SetLabelPosition(vtkKWWidgetWithLabel::LabelPositionBottom); };
  virtual void SetLabelPositionToLeft()
    { this->SetLabelPosition(vtkKWWidgetWithLabel::LabelPositionLeft); };
  virtual void SetLabelPositionToRight()
    { this->SetLabelPosition(vtkKWWidgetWithLabel::LabelPositionRight); };
  virtual void SetEntry1Position(int);
  virtual void SetEntry1PositionToDefault()
    { this->SetEntry1Position(vtkKWRange::EntryPositionDefault); };
  virtual void SetEntry1PositionToTop()
    { this->SetEntry1Position(vtkKWRange::EntryPositionTop); };
  virtual void SetEntry1PositionToBottom()
    { this->SetEntry1Position(vtkKWRange::EntryPositionBottom); };
  virtual void SetEntry1PositionToLeft()
    { this->SetEntry1Position(vtkKWRange::EntryPositionLeft); };
  virtual void SetEntry1PositionToRight()
    { this->SetEntry1Position(vtkKWRange::EntryPositionRight); };
  virtual void SetEntry2Position(int);
  virtual void SetEntry2PositionToDefault()
    { this->SetEntry2Position(vtkKWRange::EntryPositionDefault); };
  virtual void SetEntry2PositionToTop()
    { this->SetEntry2Position(vtkKWRange::EntryPositionTop); };
  virtual void SetEntry2PositionToBottom()
    { this->SetEntry2Position(vtkKWRange::EntryPositionBottom); };
  virtual void SetEntry2PositionToLeft()
    { this->SetEntry2Position(vtkKWRange::EntryPositionLeft); };
  virtual void SetEntry2PositionToRight()
    { this->SetEntry2Position(vtkKWRange::EntryPositionRight); };
  virtual void SetThickness(int);
  virtual void SetInternalThickness(double);
  virtual void SetRequestedLength(int);
  virtual void SetSliderSize(int);
  virtual void SetSliderCanPush(int);
  vtkBooleanMacro(SliderCanPush, int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Access the internal vtkKWRange's.
  vtkKWRange* GetXRange() { return this->Range[0]; };
  vtkKWRange* GetYRange() { return this->Range[1]; };
  vtkKWRange* GetZRange() { return this->Range[2]; };
  vtkKWRange* GetRange(int index);

protected:
  vtkKWExtent();
  ~vtkKWExtent();

  char *Command;
  double Extent[6];

  vtkKWRange  *Range[3];

  int ExtentVisibility[3];

  // Pack or repack the widget

  virtual void Pack();

private:
  vtkKWExtent(const vtkKWExtent&); // Not implemented
  void operator=(const vtkKWExtent&); // Not implemented
};

#endif

