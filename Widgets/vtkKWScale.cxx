/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWScale.cxx
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
#include "vtkKWScale.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkKWScale* vtkKWScale::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWScale");
  if(ret)
    {
    return (vtkKWScale*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWScale;
}




int vtkKWScaleCommand(ClientData cd, Tcl_Interp *interp,
		      int argc, char *argv[]);

vtkKWScale::vtkKWScale()
{
  this->CommandFunction = vtkKWScaleCommand;
  this->Command = NULL;
  this->StartCommand = NULL;
  this->EndCommand = NULL;
  this->Value = 0;
  this->Entry = NULL;
  this->ScaleLabel = NULL;
  this->ScaleWidget = vtkKWWidget::New();
  this->ScaleWidget->SetParent(this);
}

vtkKWScale::~vtkKWScale()
{
  if (this->Command)
    {
    delete [] this->Command;
    }
  if (this->StartCommand)
    {
    delete [] this->StartCommand;
    }
  if (this->EndCommand)
    {
    delete [] this->EndCommand;
    }
  if (this->Entry)
    {
    this->Entry->Delete();
    }
  if (this->ScaleLabel)
    {
    this->ScaleLabel->Delete();
    }
  this->ScaleWidget->Delete();
}

void vtkKWScale::SetValue(float s)
{
  vtkKWObject::Script(this->Application,"%s set %f",
		      this->ScaleWidget->GetWidgetName(),s);
  if (this->Entry)
    {
    this->Entry->SetValue(s,0);
    }
  this->Value = s;

  vtkKWObject::Script(this->Application, "update idletasks");
}


void vtkKWScale::Create(vtkKWApplication *app, char *args)
{
  const char *wname;

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Scale already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  vtkKWObject::Script(app,"frame %s",wname);
  this->ScaleWidget->Create(app,"scale","-orient horizontal -showvalue no");
  vtkKWObject::Script(this->Application,"%s configure %s",
		      this->ScaleWidget->GetWidgetName(),args);
  this->ScaleWidget->SetCommand("{%s ScaleValueChanged}",this->GetTclName());
  vtkKWObject::Script(app,"pack %s -side bottom -fill x -expand yes",
		      this->ScaleWidget->GetWidgetName());

}

void vtkKWScale::SetRange(float min, float max)
{
  vtkKWObject::Script(this->Application,"%s configure -from %f -to %f",
		      this->ScaleWidget->GetWidgetName(),min,max);
}


void vtkKWScale::DisplayEntry()
{
  if (this->Entry)
    {
    return;
    }
  this->Entry = vtkKWEntry::New();
  this->Entry->SetParent(this);
  this->Entry->Create(this->Application,"-width 10");
  vtkKWObject::Script(this->Application,
		      "bind %s <Return> {%s EntryValueChanged}",
		      this->Entry->GetWidgetName(), this->GetTclName());
  vtkKWObject::Script(this->Application,
		      "bind %s <ButtonPress> {%s InvokeStartCommand}",
		      this->ScaleWidget->GetWidgetName(), this->GetTclName());
  vtkKWObject::Script(this->Application,
		      "bind %s <ButtonRelease> {%s InvokeEndCommand}",
		      this->ScaleWidget->GetWidgetName(), this->GetTclName());
  vtkKWObject::Script(this->Application,"pack %s -side right",
		      this->Entry->GetWidgetName());
}

void vtkKWScale::DisplayLabel(const char *name)
{
  if (this->ScaleLabel)
    {
    return;
    }
  this->ScaleLabel = vtkKWWidget::New();
  this->ScaleLabel->SetParent(this);
  char temp[1024];
  sprintf(temp,"-text {%s}",name);
  this->ScaleLabel->Create(this->Application,"label",temp);
  vtkKWObject::Script(this->Application,"pack %s -side left",
		      this->ScaleLabel->GetWidgetName());
}

void vtkKWScale::EntryValueChanged()
{
  this->Value = this->Entry->GetValueAsFloat();
  vtkKWObject::Script(this->Application,"%s set %f",
		      this->ScaleWidget->GetWidgetName(), 
		      this->Value);
  vtkKWObject::Script(this->Application,"eval %s",this->Command);
}

void vtkKWScale::InvokeStartCommand()
{
  if ( this->StartCommand )
    {
    vtkKWObject::Script(this->Application,"eval %s",this->StartCommand);
    }
}

void vtkKWScale::InvokeEndCommand()
{
  if ( this->EndCommand )
    {
    vtkKWObject::Script(this->Application,"eval %s",this->EndCommand);
    }
}

void vtkKWScale::ScaleValueChanged(float num)
{
  this->Value = num;
  if (this->Entry)
    {
    this->Entry->SetValue(this->Value,0);
    }
  vtkKWObject::Script(this->Application,"eval %s",this->Command);
}

void vtkKWScale::SetCommand(char *format, ...)
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

void vtkKWScale::SetStartCommand(char *format, ...)
{
  static char event[16000];

  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  if (this->StartCommand)
    {
    delete [] this->StartCommand;
    }
  this->StartCommand = new char [strlen(event)+1];
  strcpy(this->StartCommand,event);
}

void vtkKWScale::SetEndCommand(char *format, ...)
{
  static char event[16000];

  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  if (this->EndCommand)
    {
    delete [] this->EndCommand;
    }
  this->EndCommand = new char [strlen(event)+1];
  strcpy(this->EndCommand,event);
}
