/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWMenuButton.cxx
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

#include "vtkKWMenuButton.h"
#include "vtkObjectFactory.h"

int vtkKWMenuButtonCommand(ClientData cd, Tcl_Interp *interp,
		      int argc, char *argv[]);

vtkKWMenuButton::vtkKWMenuButton()
{
  this->CommandFunction = vtkKWMenuButtonCommand;
  
  this->Menu = vtkKWMenu::New();
}

vtkKWMenuButton::~vtkKWMenuButton()
{
  this->Menu->Delete();
  this->Menu = NULL;
}


vtkKWMenuButton* vtkKWMenuButton::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWMenuButton");
  if(ret)
    {
    return (vtkKWMenuButton*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWMenuButton;
}

void vtkKWMenuButton::Create(vtkKWApplication *app, char *args)
{ 
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Menu already created");
    return;
    }
  this->SetApplication(app);

  this->Menu->SetParent(this);
  this->Script("menubutton %s -menu %s -relief raised -bd 2", 
	       this->GetWidgetName(), this->Menu->GetWidgetName());

  this->Menu->Create(app, "");  
  
}

void vtkKWMenuButton::SetButtonText(const char *text)
{
    this->Script("%s configure -text {%s}",
		 this->GetWidgetName(), text);
}

void vtkKWMenuButton::AddCommand(const char* label, vtkKWObject* Object,
				 const char* MethodAndArgString,
				 const char* help)
{
  this->Menu->AddCommand(label, Object, MethodAndArgString, help);
}

vtkKWMenu* vtkKWMenuButton::GetMenu()
{
  return this->Menu;
}
