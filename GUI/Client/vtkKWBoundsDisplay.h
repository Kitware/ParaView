/*=========================================================================

  Program:   ParaView
  Module:    vtkKWBoundsDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWBoundsDisplay - an entry with a label
// .SECTION Description
// The LabeledEntry creates an entry with a label in front of it; both are
// contained in a frame


#ifndef __vtkKWBoundsDisplay_h
#define __vtkKWBoundsDisplay_h

#include "vtkKWFrameLabeled.h"

class vtkKWApplication;
class vtkKWLabel;

class VTK_EXPORT vtkKWBoundsDisplay : public vtkKWFrameLabeled
{
public:
  static vtkKWBoundsDisplay* New();
  vtkTypeRevisionMacro(vtkKWBoundsDisplay, vtkKWFrameLabeled);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char* args);

  // Description:
  // Set the bounds to display.
  void SetBounds(double bounds[6]);
  void SetExtent(int ext[6]); 
  vtkGetVector6Macro(Bounds, double);

  // Description:
  // I want to use this widget to display an extent (int values).
  // This mode causes the extent to be printed as integers.
  // This flag is set to Bounds by default.
  // The mode is automatically set when the bounds or extent is set.
  void SetModeToExtent() {this->ExtentMode = 1; this->UpdateWidgets();}
  void SetModeToBounds() {this->ExtentMode = 0; this->UpdateWidgets();}

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWBoundsDisplay();
  ~vtkKWBoundsDisplay();

  void UpdateWidgets();

  vtkKWLabel *XRangeLabel;
  vtkKWLabel *YRangeLabel;
  vtkKWLabel *ZRangeLabel;

  double Bounds[6];
  int Extent[6];
  int ExtentMode;

  vtkKWBoundsDisplay(const vtkKWBoundsDisplay&); // Not implemented
  void operator=(const vtkKWBoundsDisplay&); // Not implemented
};


#endif
