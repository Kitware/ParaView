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

#include "vtkKWWidget.h"

class vtkKWRange;
class vtkKWApplication;

class VTK_EXPORT vtkKWExtent : public vtkKWWidget
{
public:
  static vtkKWExtent* New();
  vtkTypeRevisionMacro(vtkKWExtent,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set the Range of the Extent, this is the range of
  // acceptable values for the sliders. Specified as 
  // minx maxx miny maxy minz maxz
  virtual void SetExtentRange(double*);
  virtual void SetExtentRange(double, double, double, double, double, double);
  
  // Description:
  // Set/Get the Extent.
  vtkGetVector6Macro(Extent,double);
  virtual void SetExtent(double*);
  virtual void SetExtent(double, double, double, double, double, double);

  // Description:
  // Show/Hide part of the extent selectively (x, y, z).
  vtkGetMacro(ShowXExtent, int);
  vtkBooleanMacro(ShowXExtent, int);
  virtual void SetShowXExtent(int);
  vtkGetMacro(ShowYExtent, int);
  vtkBooleanMacro(ShowYExtent, int);
  virtual void SetShowYExtent(int);
  vtkGetMacro(ShowZExtent, int);
  vtkBooleanMacro(ShowZExtent, int);
  virtual void SetShowZExtent(int);

  // Description:
  // Handle the callback, this is called internally when one of the 
  // sliders has been moved.
  void ExtentSelected();

  // Description:
  // A method to set callback functions on objects.  The first argument is
  // the KWObject that will have the method called on it.  The second is the
  // name of the method to be called and any arguments in string form.
  // The calling is done via TCL wrappers for the KWObject.
  virtual void SetCommand(vtkKWObject* Object, const char *MethodAndArgString);

  // Description:
  // A convenience method to set the start and end method of all the
  // internal ranges.  
  virtual void SetStartCommand(
    vtkKWObject* Object, const char *MethodAndArgString);
  virtual void SetEndCommand(
    vtkKWObject* Object, const char *MethodAndArgString);

  // Description:
  // Convenience method to set whether the command should be called or not.
  // This just propagates SetDisableCommands to the internal ranges.
  virtual void SetDisableCommands(int);
  vtkBooleanMacro(DisableCommands, int);

  // Description:
  // Convenience method to set the ranges orientations and item positions.
  // This just propagates the same method to the internal ranges.
  virtual void SetOrientation(int);
  virtual void SetLabelPosition(int);
  virtual void SetEntriesPosition(int);
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
  vtkKWRange *GetXRange() { return this->XRange; };
  vtkKWRange *GetYRange() { return this->YRange; };
  vtkKWRange *GetZRange() { return this->ZRange; };

protected:
  vtkKWExtent();
  ~vtkKWExtent();

  char *Command;
  double Extent[6];

  vtkKWRange  *XRange;
  vtkKWRange  *YRange;
  vtkKWRange  *ZRange;

  int ShowXExtent;
  int ShowYExtent;
  int ShowZExtent;

  // Pack or repack the widget

  virtual void Pack();

private:
  vtkKWExtent(const vtkKWExtent&); // Not implemented
  void operator=(const vtkKWExtent&); // Not implemented
};

#endif

