/*=========================================================================

  Module:    vtkKWFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWFrame - a simple frame
// .SECTION Description
// The core frame


#ifndef __vtkKWFrame_h
#define __vtkKWFrame_h

#include "vtkKWCoreWidget.h"

class KWWidgets_EXPORT vtkKWFrame : public vtkKWCoreWidget
{
public:
  static vtkKWFrame* New();
  vtkTypeRevisionMacro(vtkKWFrame,vtkKWCoreWidget);
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
  // Set/Get the highlight thickness, a non-negative value indicating the
  // width of the highlight rectangle to draw around the outside of the
  // widget when it has the input focus.
  virtual void SetHighlightThickness(int);
  virtual int GetHighlightThickness();
  
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
  // Set/Get the padding that will be applied around each widget (in pixels).
  // Specifies a non-negative value indicating how much extra space to request
  // for the widget in the X and Y-direction. When computing how large a
  // window it needs, the widget will add this amount to the width it would
  // normally need (as determined by the width of the things displayed
  // in the widget); if the geometry manager can satisfy this request, the 
  // widget will end up with extra internal space around what it displays 
  // inside. 
  virtual void SetPadX(int);
  virtual int GetPadX();
  virtual void SetPadY(int);
  virtual int GetPadY();

  // Description:
  // Request the width/height of a frame.
  // If width or height is less than or equal to zero then the frame will
  // not request any site at all, and we obey both the geometry of its
  // children and the space provided by its parent.
  // Supported only starting Tcl/Tk 8.3
  virtual void SetWidth(int);
  virtual int GetWidth();
  virtual void SetHeight(int);
  virtual int GetHeight();
  
protected:
  vtkKWFrame() {};
  ~vtkKWFrame() {};

private:
  vtkKWFrame(const vtkKWFrame&); // Not implemented
  void operator=(const vtkKWFrame&); // Not implemented
};


#endif



