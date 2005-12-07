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

class KWWidgets_EXPORT vtkKWFrameWithScrollbar : public vtkKWCoreWidget
{
public:
  static vtkKWFrameWithScrollbar* New();
  vtkTypeRevisionMacro(vtkKWFrameWithScrollbar,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Get the internal widget (technically a Tk frame).
  vtkGetObjectMacro(Frame, vtkKWWidget);

  // Description:
  // Convenience method to set the width/height of a frame.
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

  vtkKWCoreWidget *Frame;
  vtkKWCoreWidget *ScrollableFrame;

private:
  vtkKWFrameWithScrollbar(const vtkKWFrameWithScrollbar&); // Not implemented
  void operator=(const vtkKWFrameWithScrollbar&); // Not implemented
};


#endif



