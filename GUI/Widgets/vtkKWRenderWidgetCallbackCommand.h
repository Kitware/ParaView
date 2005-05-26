/*=========================================================================

  Module:    vtkKWRenderWidgetCallbackCommand.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWRenderWidgetCallbackCommand - a progress callback
// .SECTION Description
// This class is specific to the render widgets and will call
// vtkKWRenderWidget::ProcessEvent

#ifndef __vtkKWRenderWidgetCallbackCommand_h
#define __vtkKWRenderWidgetCallbackCommand_h

#include "vtkCommand.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives

class vtkKWRenderWidget;

class KWWIDGETS_EXPORT vtkKWRenderWidgetCallbackCommand : public vtkCommand
{
public:
  static vtkKWRenderWidgetCallbackCommand *New() 
    { return new vtkKWRenderWidgetCallbackCommand; };
  
  void Execute(vtkObject *caller, unsigned long event, void *callData);

  void SetRenderWidget(vtkKWRenderWidget *RenderWidget);
  
protected:
  vtkKWRenderWidgetCallbackCommand();
  ~vtkKWRenderWidgetCallbackCommand();
  
  vtkKWRenderWidget  *RenderWidget;

private:
  vtkKWRenderWidgetCallbackCommand(const vtkKWRenderWidgetCallbackCommand&); // Not implemented
  void operator=(const vtkKWRenderWidgetCallbackCommand&); // Not implemented
};

#endif
