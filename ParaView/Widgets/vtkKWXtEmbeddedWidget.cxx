/*=========================================================================

  Module:    vtkKWXtEmbeddedWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWXtEmbeddedWidget.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWXtEmbeddedWidget);
vtkCxxRevisionMacro(vtkKWXtEmbeddedWidget, "1.9");

int vtkKWXtEmbeddedWidgetCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

vtkKWXtEmbeddedWidget::vtkKWXtEmbeddedWidget()
{
  this->WindowId = NULL;
}

vtkKWXtEmbeddedWidget::~vtkKWXtEmbeddedWidget()
{

}

void vtkKWXtEmbeddedWidget::Display()
{
  // map the window
  this->Script("wm deiconify %s",this->GetWidgetName());
}


void vtkKWXtEmbeddedWidget::Create(vtkKWApplication *app, char *args)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("Dialog already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  const char* wname = this->GetWidgetName();
  if(this->WindowId)
    {
    this->Script("toplevel %s %s -use 0x%x",wname,args, this->WindowId);
    }
  else
    {
    this->Script("toplevel %s %s",wname,args); 
    this->Script("wm withdraw %s",wname);
    }
}

//----------------------------------------------------------------------------
void vtkKWXtEmbeddedWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

