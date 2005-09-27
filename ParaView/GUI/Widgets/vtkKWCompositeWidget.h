/*=========================================================================

  Module:    vtkKWCompositeWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWCompositeWidget - a composite widget.
// .SECTION Description
// A superclass for all composite widgets, i.e. widgets made of
// an assembly of sub-widgets.
// This superclass provides the container for the sub-widgets.
// Right now, it can be safely assumed to be a frame (similar to a
// vtkKWFrame).

#ifndef __vtkKWCompositeWidget_h
#define __vtkKWCompositeWidget_h

#include "vtkKWFrame.h"

class KWWIDGETS_EXPORT vtkKWCompositeWidget : public vtkKWFrame
{
public:
  static vtkKWCompositeWidget* New();
  vtkTypeRevisionMacro(vtkKWCompositeWidget, vtkKWFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

protected:
  vtkKWCompositeWidget() {};
  ~vtkKWCompositeWidget() {};

private:
  vtkKWCompositeWidget(const vtkKWCompositeWidget&); // Not implemented
  void operator=(const vtkKWCompositeWidget&); // Not implemented
};


#endif



