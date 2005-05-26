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

#include "vtkKWWidget.h"

class KWWIDGETS_EXPORT vtkKWFrame : public vtkKWWidget
{
public:
  static vtkKWFrame* New();
  vtkTypeRevisionMacro(vtkKWFrame,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a the widget
  virtual void Create(vtkKWApplication *app, const char* args);

  // Description:
  // Convenience method to set the width/height of a frame.
  // Supported only starting Tcl/Tk 8.3
  virtual void SetWidth(int);
  virtual void SetHeight(int);
  
protected:
  vtkKWFrame() {};
  ~vtkKWFrame() {};

private:
  vtkKWFrame(const vtkKWFrame&); // Not implemented
  void operator=(const vtkKWFrame&); // Not implemented
};


#endif



