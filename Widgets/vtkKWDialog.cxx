/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWDialog.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWDialog.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkKWDialog* vtkKWDialog::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWDialog");
  if(ret)
    {
    return (vtkKWDialog*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWDialog;
}




int vtkKWDialogCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

vtkKWDialog::vtkKWDialog()
{
  this->CommandFunction = vtkKWDialogCommand;
  this->Command = NULL;
  this->Done = 1;
}

vtkKWDialog::~vtkKWDialog()
{
  if (this->Command)
    {
    delete [] this->Command;
    }
}

int vtkKWDialog::Invoke()
{
  this->Done = 0;

  // map the window
  vtkKWObject::Script(this->Application,
    "wm deiconify %s",this->GetWidgetName());
  vtkKWObject::Script(this->Application,
                      "focus %s",this->GetWidgetName());
  // do a grab
  // wait for the end
  while (!this->Done)
    {
    Tcl_DoOneEvent(0);    
    }
  return (this->Done-1);
}

void vtkKWDialog::Display()
{
  this->Done = 0;

  // map the window
  vtkKWObject::Script(this->Application,
                      "wm deiconify %s",this->GetWidgetName());
  vtkKWObject::Script(this->Application,
                      "focus %s",this->GetWidgetName());
}

void vtkKWDialog::Cancel()
{
  vtkKWObject::Script(this->Application,
		      "wm withdraw %s",this->GetWidgetName());
  this->Done = 1;  
  if (this->Command && strlen(this->Command) > 0)
    {
    vtkKWObject::Script(this->Application,"eval %s",this->Command);
    }
}

void vtkKWDialog::OK()
{
  vtkKWObject::Script(this->Application,
		      "wm withdraw %s",this->GetWidgetName());
  this->Done = 2;  
  if (this->Command && strlen(this->Command) > 0)
    {
    vtkKWObject::Script(this->Application,"eval %s",this->Command);
    }
}


void vtkKWDialog::Create(vtkKWApplication *app, char *args)
{
  const char *wname;

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Dialog already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  vtkKWObject::Script(app,"toplevel %s %s",wname,args);
  vtkKWObject::Script(app,"wm title %s \"Kitware Dialog\"",wname);
  vtkKWObject::Script(app,"wm iconname %s \"Dialog\"",wname);
  vtkKWObject::Script(app,
		      "wm protocol %s WM_DELETE_WINDOW {%s Cancel}",
		      wname, this->GetTclName());
  vtkKWObject::Script(app,"wm withdraw %s",wname);
}

void vtkKWDialog::SetCommand(char *format, ...)
{
  static char event[16000];

  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  if (this->Command)
    {
    delete [] this->Command;
    }
  this->Command = new char [strlen(event)+1];
  strcpy(this->Command,event);
}
