/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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

#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkKWMenuButton );
vtkCxxRevisionMacro(vtkKWMenuButton, "1.15");

int vtkKWMenuButtonCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWMenuButton::vtkKWMenuButton()
{
  this->CommandFunction = vtkKWMenuButtonCommand;
  
  this->Menu = vtkKWMenu::New();
}

//----------------------------------------------------------------------------
vtkKWMenuButton::~vtkKWMenuButton()
{
  this->Menu->Delete();
  this->Menu = NULL;
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::Create(vtkKWApplication *app, char *args)
{ 
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("Menu already created");
    return;
    }
  this->SetApplication(app);

  this->Menu->SetParent(this);
  this->Script("menubutton %s -menu %s -indicatoron 1 -relief raised -bd 2 -direction flush %s", 
               this->GetWidgetName(), this->Menu->GetWidgetName(), (args ? args : ""));

  // Should the args be passed through?

  this->Menu->Create(app, "");  

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::SetButtonText(const char *text)
{
    this->Script("%s configure -text {%s}",
                 this->GetWidgetName(), text);
}

//----------------------------------------------------------------------------
const char* vtkKWMenuButton::GetButtonText()
{
    return this->Script("%s cget -text",
                 this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::AddCommand(const char* label, vtkKWObject* Object,
                                 const char* MethodAndArgString,
                                 const char* help)
{
  this->Menu->AddCommand(label, Object, MethodAndArgString, help);
}

//----------------------------------------------------------------------------
vtkKWMenu* vtkKWMenuButton::GetMenu()
{
  return this->Menu;
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::IndicatorOn()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("%s config -indicatoron 1", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::IndicatorOff()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("%s config -indicatoron 0", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->Menu)
    {
    this->Menu->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

