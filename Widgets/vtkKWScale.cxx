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
  this->Resolution = 1;
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
  this->Script("%s set %f", this->ScaleWidget->GetWidgetName(),s);
  if (this->Entry)
    {
    this->Entry->SetValue(s,0);
    }
  this->Value = s;

  this->Script( "update idletasks");
}


void vtkKWScale::SetResolution( float r )
{
  this->Resolution = r;
  
  if ( this->Application )
    {
    this->Script("%s configure -resolution %f",
                 this->ScaleWidget->GetWidgetName(), r);
    }
  
  this->Modified();
}

void vtkKWScale::Create(vtkKWApplication *app, const char *args)
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
  this->Script("frame %s",wname);
  this->ScaleWidget->Create(app,"scale","-orient horizontal -showvalue no");
  this->Script("%s configure %s",this->ScaleWidget->GetWidgetName(),args);
  this->Script("%s configure -resolution %f",
               this->ScaleWidget->GetWidgetName(),this->Resolution);
  this->ScaleWidget->SetCommand(this, "ScaleValueChanged");
  this->Script("pack %s -side bottom -fill x -expand yes",
               this->ScaleWidget->GetWidgetName());

}

void vtkKWScale::SetRange(float min, float max)
{
  this->Script("%s configure -from %f -to %f",
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
  this->Script("bind %s <Return> {%s EntryValueChanged}",
               this->Entry->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <ButtonPress> {%s InvokeStartCommand}",
               this->ScaleWidget->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <ButtonRelease> {%s InvokeEndCommand}",
               this->ScaleWidget->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side right", this->Entry->GetWidgetName());
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
  this->Script("pack %s -side left",
               this->ScaleLabel->GetWidgetName());
}

void vtkKWScale::EntryValueChanged()
{
  this->Value = this->Entry->GetValueAsFloat();
  this->Script("%s set %f", this->ScaleWidget->GetWidgetName(), this->Value);
  this->Script("eval %s",this->Command);
}

void vtkKWScale::InvokeStartCommand()
{
  if ( this->StartCommand )
    {
    this->Script("eval %s",this->StartCommand);
    }
}

void vtkKWScale::InvokeEndCommand()
{
  if ( this->EndCommand )
    {
    this->Script("eval %s",this->EndCommand);
    }
}

void vtkKWScale::ScaleValueChanged(float num)
{
  this->Value = num;
  if (this->Entry)
    {
    this->Entry->SetValue(this->Value,0);
    }
  if (this->Command)
    {
    this->Script("eval %s",this->Command);
    }
}


void vtkKWScale::SetStartCommand(vtkKWObject* Object, const char * MethodAndArgString)
{
  if (this->StartCommand)
    {
    delete [] this->StartCommand;
    }
  ostrstream command;
  command << Object->GetTclName() << " " << MethodAndArgString << ends;
  this->StartCommand = command.str();
}

void vtkKWScale::SetEndCommand(vtkKWObject* Object, const char * MethodAndArgString)
{
  if (this->EndCommand)
    {
    delete [] this->EndCommand;
    }
  ostrstream command;
  command << Object->GetTclName() << " " << MethodAndArgString << ends;
  this->EndCommand = command.str();
}


void vtkKWScale::SetCommand(vtkKWObject* CalledObject, const char *CommandString)
{
  if (this->Command)
    {
    delete [] this->Command;
    }
  ostrstream command;
  command << CalledObject->GetTclName() << " " << CommandString << ends;
  this->Command = command.str();
}
