/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWMenu.cxx
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
#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"


//------------------------------------------------------------------------------
vtkKWMenu* vtkKWMenu::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWMenu");
  if(ret)
    {
    return (vtkKWMenu*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWMenu;
}




int vtkKWMenuCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

vtkKWMenu::vtkKWMenu()
{
}

vtkKWMenu::~vtkKWMenu()
{
}

void vtkKWMenu::Create(vtkKWApplication* app, const char* args)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Menu already created");
    return;
    }
  this->SetApplication(app);
  this->Script("menu %s %s", this->GetWidgetName(), args);
}

void vtkKWMenu::AddGeneric(const char* addtype, const char* label, vtkKWObject* Object,
			   const char* MethodAndArgString, const char* extra)
{
  ostrstream str;
  str << this->GetWidgetName() << " add " 
      << addtype << " -label {" << label
      << "} -command {" << Object->GetTclName() 
      << " " << MethodAndArgString << "} " ;
  if(extra)
    {
    str << extra;
    }
  str << ends;
  
  this->Application->SimpleScript(str.str());
  delete [] str.str();
}



void vtkKWMenu::InsertGeneric(int position, const char* addtype, const char* label, 
			      vtkKWObject* Object,
			      const char* MethodAndArgString, const char* extra)
{
  ostrstream str;
  str << this->GetWidgetName() << " insert " << position << " " 
      << addtype << " -label {" << label
      << "} -command {" << Object->GetTclName() 
      << " " << MethodAndArgString << "} ";
  if(extra)
    {
    str << extra;
    }
  str << ends;

  this->Application->SimpleScript(str.str());
  delete [] str.str();
}

void vtkKWMenu::AddCascade(const char* label, vtkKWMenu* menu, int underline)
{
  ostrstream str;
  str << this->GetWidgetName() << " add cascade -label \"" << label << "\" -menu " 
      << menu->GetWidgetName() << " -underline " << underline << ends;
  this->Application->SimpleScript(str.str());
  delete [] str.str();
}



void  vtkKWMenu::InsertCascade(int position, 
			       const char* label, 
			       vtkKWMenu* menu, 
			       int underline)
{
  ostrstream str;
  
  str << this->GetWidgetName() << " insert " << position 
      << " cascade -label \"" << label << "\" -menu " 
      << menu->GetWidgetName() << " -underline " << underline << ends;
  this->Application->SimpleScript(str.str());
  delete [] str.str();
}

void  vtkKWMenu::AddCheckButton(const char* label, vtkKWObject* Object, 
				const char* MethodAndArgString )
{ 
  static int count = 0;
  ostrstream str;
  str << "-variable " << this->GetWidgetName() << "TempVar" << count++ << ends;
  this->AddGeneric("checkbutton", label, Object, MethodAndArgString, str.str());
  delete [] str.str();
}


void vtkKWMenu::InsertCheckButton(int position, 
				  const char* label, vtkKWObject* Object, 
				  const char* MethodAndArgString )
{ 
  static int count = 0;
  ostrstream str;
  str << "-variable " << this->GetWidgetName() << count++ << "TempVar " << ends;
  this->InsertGeneric(position, "checkbutton", label, Object, 
		      MethodAndArgString, str.str());
  delete [] str.str();
}


void  vtkKWMenu::AddCommand(const char* label, vtkKWObject* Object,
							const char* MethodAndArgString )
{
  this->AddGeneric("command", label, Object, 
		   MethodAndArgString, NULL);
}

void vtkKWMenu::InsertCommand(int position, const char* label, vtkKWObject* Object,
			     const char* MethodAndArgString )
{
  this->InsertGeneric(position, "command", label, Object,
		      MethodAndArgString, NULL);
}

char* vtkKWMenu::CreateRadioButtonVariable(vtkKWObject* Object, 
                                           const char* varname)
{
  ostrstream str;
  str << Object->GetTclName() << varname << ends;
  return str.str();
}

void vtkKWMenu::AddRadioButton(int value, const char* label, const char* buttonVar, 
			       vtkKWObject* Object, 
			       const char* MethodAndArgString)
{
  ostrstream str;
  str << "-value " << value << " -variable " << buttonVar << ends;
  this->AddGeneric("radiobutton", label, Object,
		   MethodAndArgString, str.str());
  delete [] str.str();
}


void vtkKWMenu::InsertRadioButton(int position, int value, const char* label, 
                                  const char* buttonVar, 
				  vtkKWObject* Object, 
				  const char* MethodAndArgString)
{
  ostrstream str;
  str << "-value " << value << " -variable " << buttonVar << ends;
  this->InsertGeneric(position, "radiobutton", label, Object,
		      MethodAndArgString, str.str());
  delete [] str.str();
}

void vtkKWMenu::Invoke(int position)
{
  this->Script("%s invoke %d", this->GetWidgetName(), position);
}

void vtkKWMenu::DeleteMenuItem(int position)
{
  this->Script("catch {%s delete %d}", this->GetWidgetName(), position);
}

void vtkKWMenu::DeleteMenuItem(const char* menuitem)
{
  this->Script("catch {%s delete {%s}}", this->GetWidgetName(), menuitem);
}

int vtkKWMenu::GetIndex(const char* menuname)
{
  this->Script("%s index {%s}", this->GetWidgetName(), menuname);
  return vtkKWObject::GetIntegerResult(this->Application);
}

void vtkKWMenu::AddSeparator()
{
  this->Script( "%s add separator", this->GetWidgetName());
}
void vtkKWMenu::InsertSeparator(int position)
{
  this->Script( "%s insert %d separator", this->GetWidgetName(), position);
}


