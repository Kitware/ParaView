/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWMenuButton.cxx
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

#include "vtkKWMenuButton.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkKWMenuButton );

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


void vtkKWMenuButton::Create(vtkKWApplication *app, char* /*args*/)
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

  // Should the args be passed through?
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
