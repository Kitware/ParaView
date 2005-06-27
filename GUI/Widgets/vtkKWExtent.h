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

class vtkKWRange;
class vtkKWApplication;

class KWWIDGETS_EXPORT vtkKWExtent : public vtkKWCompositeWidget
{
public:
  static vtkKWExtent* New();
  vtkTypeRevisionMacro(vtkKWExtent,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

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
  // Show/Hide part of the extent selectively (x, y, z).
  virtual void SetShowExtent(int index, int arg);
  vtkBooleanMacro(ShowXExtent, int);
  virtual int GetShowXExtent(int arg) { return this->ShowExtent[0]; };
  virtual void SetShowXExtent(int arg) { this->SetShowExtent(0, arg); };
  vtkBooleanMacro(ShowYExtent, int);
  virtual int GetShowYExtent(int arg) { return this->ShowExtent[1]; };
  virtual void SetShowYExtent(int arg) { this->SetShowExtent(1, arg); };
  vtkBooleanMacro(ShowZExtent, int);
  virtual int GetShowZExtent(int arg) { return this->ShowExtent[2]; };
  virtual void SetShowZExtent(int arg) { this->SetShowExtent(2, arg); };

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
  virtual void SetLabelPosition(int);
  virtual void SetEntry1Position(int);
  virtual void SetEntry2Position(int);
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

  int ShowExtent[3];

  // Pack or repack the widget

  virtual void Pack();

private:
  vtkKWExtent(const vtkKWExtent&); // Not implemented
  void operator=(const vtkKWExtent&); // Not implemented
};

#endif

