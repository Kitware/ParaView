/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVMenuButton.cxx
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

#include "vtkPVMenuButton.h"
#include "vtkObjectFactory.h"

int vtkPVMenuButtonCommand(ClientData cd, Tcl_Interp *interp,
		      int argc, char *argv[]);

vtkPVMenuButton::vtkPVMenuButton()
{
  this->CommandFunction = vtkPVMenuButtonCommand;
  
  this->Menu = vtkKWMenu::New();
}

vtkPVMenuButton::~vtkPVMenuButton()
{
  this->Menu->Delete();
  this->Menu = NULL;
}


vtkPVMenuButton* vtkPVMenuButton::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVMenuButton");
  if(ret)
    {
    return (vtkPVMenuButton*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVMenuButton;
}

void vtkPVMenuButton::Create(vtkKWApplication *app, char *args)
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

void vtkPVMenuButton::SetButtonText(const char *text)
{
    this->Script("%s configure -text {%s}",
		 this->GetWidgetName(), text);
}

void vtkPVMenuButton::AddCommand(const char* label, vtkKWObject* Object,
				 const char* MethodAndArgString,
				 const char* help)
{
  this->Menu->AddCommand(label, Object, MethodAndArgString, help);
}

vtkKWMenu* vtkPVMenuButton::GetMenu()
{
  return this->Menu;
}
