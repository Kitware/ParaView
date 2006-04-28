/*=========================================================================

  Module:    vtkKWFrameWithScrollbar.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWFrameWithScrollbar - a frame with a scroll bar
// .SECTION Description
// It creates a frame with an attached scrollbar


#ifndef __vtkKWFrameWithScrollbar_h
#define __vtkKWFrameWithScrollbar_h

#include "vtkKWCoreWidget.h"

class vtkKWFrame;

class KWWidgets_EXPORT vtkKWFrameWithScrollbar : public vtkKWCoreWidget
{
public:
  static vtkKWFrameWithScrollbar* New();
  vtkTypeRevisionMacro(vtkKWFrameWithScrollbar,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the internal widget.
  vtkGetObjectMacro(Frame, vtkKWFrame);

  // Description:
  // Set/Get the background color of the widget.
  virtual void GetBackgroundColor(double *r, double *g, double *b);
  virtual double* GetBackgroundColor();
  virtual void SetBackgroundColor(double r, double g, double b);
  virtual void SetBackgroundColor(double rgb[3])
    { this->SetBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
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
  // Set/Get the width/height of a frame.
  virtual void SetWidth(int);
  virtual int GetWidth();
  virtual void SetHeight(int);
  virtual int GetHeight();
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
protected:
  vtkKWFrameWithScrollbar();
  ~vtkKWFrameWithScrollbar();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  vtkKWFrame *Frame;
  vtkKWCoreWidget *ScrollableFrame;

private:
  vtkKWFrameWithScrollbar(const vtkKWFrameWithScrollbar&); // Not implemented
  void operator=(const vtkKWFrameWithScrollbar&); // Not implemented
};

#endif



