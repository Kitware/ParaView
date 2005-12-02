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

class KWWIDGETS_EXPORT vtkKWFrame : public vtkKWCoreWidget
{
public:
  static vtkKWFrame* New();
  vtkTypeRevisionMacro(vtkKWFrame,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Convenience method to set the width/height of a frame.
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



