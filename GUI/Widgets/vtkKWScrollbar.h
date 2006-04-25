/*=========================================================================

  Module:    vtkKWScrollbar.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWScrollbar - a simple scrollbar
// .SECTION Description
// The core scrollbar


#ifndef __vtkKWScrollbar_h
#define __vtkKWScrollbar_h

#include "vtkKWCoreWidget.h"

class KWWidgets_EXPORT vtkKWScrollbar : public vtkKWCoreWidget
{
public:
  static vtkKWScrollbar* New();
  vtkTypeRevisionMacro(vtkKWScrollbar,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Set/Get the background color of the widget.
  virtual void GetBackgroundColor(double *r, double *g, double *b);
  virtual double* GetBackgroundColor();
  virtual void SetBackgroundColor(double r, double g, double b);
  virtual void SetBackgroundColor(double rgb[3])
    { this->SetBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the foreground color of the widget.
  virtual void GetForegroundColor(double *r, double *g, double *b);
  virtual double* GetForegroundColor();
  virtual void SetForegroundColor(double r, double g, double b);
  virtual void SetForegroundColor(double rgb[3])
    { this->SetForegroundColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the highlight thickness, a non-negative value indicating the
  // width of the highlight rectangle to draw around the outside of the
  // widget when it has the input focus.
  virtual void SetHighlightThickness(int);
  virtual int GetHighlightThickness();
  
  // Description:
  // Set/Get the active background color of the widget. An element
  // (a widget or portion of a widget) is active if the mouse cursor is
  // positioned over the element and pressing a mouse button will cause some
  // action to occur.
  virtual void GetActiveBackgroundColor(double *r, double *g, double *b);
  virtual double* GetActiveBackgroundColor();
  virtual void SetActiveBackgroundColor(double r, double g, double b);
  virtual void SetActiveBackgroundColor(double rgb[3])
    { this->SetActiveBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the border width, a non-negative value indicating the width of
  // the 3-D border to draw around the outside of the widget (if such a border
  // is being drawn; the Relief option typically determines this).
  virtual void SetBorderWidth(int);
  virtual int GetBorderWidth();
  
  // Description:
  // Set/Get the 3-D effect desired for the widget. 
  // The value indicates how the interior of the widget should appear
  // relative to its exterior. 
  // Valid constants can be found in vtkKWOptions::ReliefType.
  virtual void SetRelief(int);
  virtual int GetRelief();
  virtual void SetReliefToRaised();
  virtual void SetReliefToSunken();
  virtual void SetReliefToFlat();
  virtual void SetReliefToRidge();
  virtual void SetReliefToSolid();
  virtual void SetReliefToGroove();

  // Description:
  // Set/Get the orientation type.
  // For widgets that can lay themselves out with either a horizontal or
  // vertical orientation, such as scrollbars, this option specifies which 
  // orientation should be used. 
  // Valid constants can be found in vtkKWOptions::OrientationType.
  virtual void SetOrientation(int);
  virtual int GetOrientation();
  virtual void SetOrientationToHorizontal();
  virtual void SetOrientationToVertical();

  // Description:
  // Set/Get the trough color, i.e. the color to use for the rectangular
  // trough areas in widgets such as scrollbars and scales.
  // Ignored on Windows though (not supported by native widget)
  virtual void GetTroughColor(double *r, double *g, double *b);
  virtual double* GetTroughColor();
  virtual void SetTroughColor(double r, double g, double b);
  virtual void SetTroughColor(double rgb[3])
    { this->SetTroughColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked to change the view in the widget associated with
  // the scrollbar. 
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetCommand(vtkObject *object, const char *method);

protected:
  vtkKWScrollbar() {};
  ~vtkKWScrollbar() {};

private:
  vtkKWScrollbar(const vtkKWScrollbar&); // Not implemented
  void operator=(const vtkKWScrollbar&); // Not implemented
};

#endif
