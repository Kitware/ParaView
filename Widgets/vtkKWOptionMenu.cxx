/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWOptionMenu.cxx
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
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkKWOptionMenu* vtkKWOptionMenu::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWOptionMenu");
  if(ret)
    {
    return (vtkKWOptionMenu*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWOptionMenu;
}




vtkKWOptionMenu::vtkKWOptionMenu()
{
  this->CurrentValue = NULL;
  this->Menu = vtkKWWidget::New();
  this->Menu->SetParent(this);
}

vtkKWOptionMenu::~vtkKWOptionMenu()
{
  if (this->CurrentValue)
    {
    delete [] this->CurrentValue;
    this->CurrentValue = NULL;
    }
  this->Menu->Delete();
}


char *vtkKWOptionMenu::GetValue()
{
  this->Script("set %sValue",this->GetWidgetName());
  
  if (this->CurrentValue)
    {
    delete [] this->CurrentValue;
    }
  this->CurrentValue = 
    strcpy(new char[strlen(this->Application->GetMainInterp()->result)+1], 
	   this->Application->GetMainInterp()->result);
  return this->CurrentValue;  
}

void vtkKWOptionMenu::SetValue(char *s)
{
  if (s)
    {
    this->Script("set %sValue %s", this->GetWidgetName(),s);
    }
}

void vtkKWOptionMenu::AddEntry(const char *name)
{
  this->Script("%s add radiobutton -label {%s} -variable %sValue",
               this->Menu->GetWidgetName(), name, 
               this->GetWidgetName());
}

void vtkKWOptionMenu::AddEntryWithCommand(const char *name, const char *obj, 
					  const char *method)
{
  this->Script(
    "%s add radiobutton -label {%s} -variable %sValue -command {%s %s}",
    this->Menu->GetWidgetName(), name, 
    this->GetWidgetName(), obj, method);
}

void vtkKWOptionMenu::AddEntryWithCommand(const char *name, vtkKWObject *obj, 
					  const char *methodAndArgs)
{
  this->Script(
    "%s add radiobutton -label {%s} -variable %sValue -command {%s %s}",
    this->Menu->GetWidgetName(), name, 
    this->GetWidgetName(), obj->GetTclName(), methodAndArgs);
}

void vtkKWOptionMenu::DeleteEntry(const char* name)
{ 
  this->Script(
    "%s  delete {%s}",
    this->Menu->GetWidgetName(), name);
}


void vtkKWOptionMenu::DeleteEntry(int index)
{
  this->Script(
    "%s  delete %d",
    this->Menu->GetWidgetName(), index);

}


void vtkKWOptionMenu::ClearEntries()
{
  this->Script("%s delete 0 end", this->Menu->GetWidgetName());
}

void vtkKWOptionMenu::SetCurrentEntry(const char *name)
{ 
  this->Script("set %sValue {%s}",this->GetWidgetName(), name);
}
 
void vtkKWOptionMenu::Create(vtkKWApplication *app, char *args)
{
  const char *wname;

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("OptionMenu already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  
  this->Script("menubutton %s -textvariable %sValue -indicatoron 1 -menu %s -relief raised -bd 2 -highlightthickness 2 -anchor c -direction flush %s", wname, wname, this->Menu->GetWidgetName(), args);
  this->Menu->Create(app,"menu","-tearoff 0");
}


