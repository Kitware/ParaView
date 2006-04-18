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

class KWWidgets_EXPORT vtkKWExtent : public vtkKWCompositeWidget
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
  virtual void SetExtentRange(const double extent[6]);
  virtual void SetExtentRange(double, double, double, double, double, double);
  virtual double* GetExtentRange();
  virtual void GetExtentRange(
    double&, double&, double&, double&, double&, double&);
  virtual void GetExtentRange(double extent[6]);
  
  // Description:
  // Set/Get the Extent.
  vtkGetVector6Macro(Extent,double);
  virtual void SetExtent(const double extent[6]);
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
  // Specifies commands to associate with the widget. 
  // 'Command' is invoked when the widget value is changing (i.e. during
  // user interaction).
  // 'StartCommand' is invoked at the beginning of a user interaction with
  // the widget (when a mouse button is pressed over the widget for example).
  // 'EndCommand' is invoked at the end of the user interaction with the 
  // widget (when the mouse button is released for example).
  // The need for a 'Command', 'StartCommand' and 'EndCommand' can be
  // explained as follows: 'EndCommand' can be used to be notified about any
  // changes made to this widget *after* the corresponding user interaction has
  // been performed (say, after releasing the mouse button that was dragging
  // a slider, or after clicking on a checkbutton). 'Command' can be set
  // *additionally* to be notified about the intermediate changes that
  // occur *during* the corresponding user interaction (say, *while* dragging
  // a slider). While setting 'EndCommand' is enough to be notified about
  // any changes, setting 'Command' is an application-specific choice that
  // is likely to depend on how fast you want (or can) answer to rapid changes
  // occuring during a user interaction, if any. 'StartCommand' is rarely
  // used but provides an opportunity for the application to modify its
  // state and prepare itself for user-interaction; in that case, the
  // 'EndCommand' is usually set in a symmetric fashion to set the application
  // back to its previous state.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - the current extent: int, int, int, int, int, int (if the Resolution of
  //   all the visible ranges are integer); double, double, double, double, 
  //   double, double otherwise.
  //   Note: the 'int' signature is for convenience, so that the command can
  //   be set to a callback accepting 'int'. In doubt, implement the callback
  //   using a 'double' signature that will accept both 'int' and 'double'.
  virtual void SetCommand(vtkObject *object, const char *method);
  virtual void SetStartCommand(vtkObject *object, const char *method);
  virtual void SetEndCommand(vtkObject *object, const char *method);

  // Description:
  // Set/Get whether the above commands should be called or not.
  virtual void SetDisableCommands(int);
  vtkBooleanMacro(DisableCommands, int);

  // Description:
  // Set the ranges orientations and item positions.
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

  // Description:
  // Callbacks. Internal, do not use.
  virtual void RangeCommandCallback(double r0, double r1);
  virtual void RangeStartCommandCallback(double r0, double r1);
  virtual void RangeEndCommandCallback(double r0, double r1);

protected:
  vtkKWExtent();
  ~vtkKWExtent();

  char *Command;
  char *StartCommand;
  char *EndCommand;

  virtual void InvokeExtentCommand(
    const char *command, 
    double x0, double x1, double y0, double y1, double z0, double z1);
  virtual void InvokeCommand(
    double x0, double x1, double y0, double y1, double z0, double z1);
  virtual void InvokeStartCommand(
    double x0, double x1, double y0, double y1, double z0, double z1);
  virtual void InvokeEndCommand(
    double x0, double x1, double y0, double y1, double z0, double z1);

  double Extent[6];

  vtkKWRange  *Range[3];

  int ExtentVisibility[3];

  // Pack or repack the widget

  virtual void Pack();

private:

  // Temporary var for wrapping purposes

  double ExtentRangeTemp[6];

  vtkKWExtent(const vtkKWExtent&); // Not implemented
  void operator=(const vtkKWExtent&); // Not implemented
};

#endif

