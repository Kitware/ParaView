/*=========================================================================

  Module:    vtkKWCoreWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWCoreWidget - a core widget.
// .SECTION Description
// A superclass for all core widgets, i.e. C++ wrappers around simple
// Tk widgets.

#ifndef __vtkKWCoreWidget_h
#define __vtkKWCoreWidget_h

#include "vtkKWWidget.h"

class KWWIDGETS_EXPORT vtkKWCoreWidget : public vtkKWWidget
{
public:
  static vtkKWCoreWidget* New();
  vtkTypeRevisionMacro(vtkKWCoreWidget, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

protected:
  vtkKWCoreWidget() {};
  ~vtkKWCoreWidget() {};

private:
  vtkKWCoreWidget(const vtkKWCoreWidget&); // Not implemented
  void operator=(const vtkKWCoreWidget&); // Not implemented
};


#endif



