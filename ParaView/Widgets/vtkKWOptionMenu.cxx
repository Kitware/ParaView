/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWOptionMenu.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWOptionMenu );




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

void vtkKWOptionMenu::SetValue(const char *s)
{
  if (s)
    {
    this->Script("set %sValue {%s}", this->GetWidgetName(),s);
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
 
void vtkKWOptionMenu::Create(vtkKWApplication *app, const char *args)
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


void vtkKWOptionMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Menu: " << this->Menu << endl;
}
