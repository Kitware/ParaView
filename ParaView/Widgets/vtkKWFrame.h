/*=========================================================================

  Module:    vtkKWFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWFrame - a frame with a scroll bar
// .SECTION Description
// The ScrollableFrame creates a frame with an attached scrollbar


#ifndef __vtkKWFrame_h
#define __vtkKWFrame_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWFrame : public vtkKWWidget
{
public:
  static vtkKWFrame* New();
  vtkTypeRevisionMacro(vtkKWFrame,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char* args);

  // Description:
  // Get the vtkKWWidget for the internal frame.
  vtkKWWidget *GetFrame() {return this->Frame;};

  // Description:
  // By default this is a simple frame. BY turning Scrollable on it becomes
  // a scrolled frame. This must be set prior to creation.
  vtkSetMacro(Scrollable,int);
  vtkGetMacro(Scrollable,int);
  vtkBooleanMacro(Scrollable,int);

  // Description:
  // Convenience method to set the width/height of a frame.
  // Supported only starting Tcl/Tk 8.3
  virtual void SetWidth(int);
  virtual void SetHeight(int);
  
protected:
  vtkKWFrame();
  ~vtkKWFrame();

  vtkKWWidget *Frame;
  vtkKWWidget *ScrollWindow;
  vtkKWWidget *ScrollFrame;

  char* FrameId;
  int Scrollable;
  
private:
  vtkKWFrame(const vtkKWFrame&); // Not implemented
  void operator=(const vtkKWFrame&); // Not implemented
};


#endif



