/*=========================================================================

  Module:    vtkKWRenderWidgetCallbackCommand.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWRenderWidgetCallbackCommand.h"

#include "vtkKWRenderWidget.h"

//----------------------------------------------------------------------------
vtkKWRenderWidgetCallbackCommand::vtkKWRenderWidgetCallbackCommand()
{ 
  this->RenderWidget = NULL;
}

//----------------------------------------------------------------------------
vtkKWRenderWidgetCallbackCommand::~vtkKWRenderWidgetCallbackCommand()
{
  this->SetRenderWidget(NULL);
}

//----------------------------------------------------------------------------
void vtkKWRenderWidgetCallbackCommand::SetRenderWidget(vtkKWRenderWidget *arg)
{
  if (this->RenderWidget != arg)
    {
    if (this->RenderWidget)
      {
      this->RenderWidget->UnRegister(NULL);
      }
    this->RenderWidget = arg;
    if (arg)
      {
      arg->Register(NULL);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWRenderWidgetCallbackCommand::Execute(vtkObject *caller,
                                               unsigned long event, 
                                               void *calldata)
{  
  if (this->RenderWidget)
    {
    this->RenderWidget->ProcessEvent(caller, event, calldata);
    this->AbortFlagOn();
    }
}
