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
#include "vtkKWOptionMenu.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkString.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWOptionMenu );
vtkCxxRevisionMacro(vtkKWOptionMenu, "1.19");

//-----------------------------------------------------------------------------
vtkKWOptionMenu::vtkKWOptionMenu()
{
  this->CurrentValue = NULL;
  this->Menu = vtkKWWidget::New();
  this->Menu->SetParent(this);
}

//-----------------------------------------------------------------------------
vtkKWOptionMenu::~vtkKWOptionMenu()
{
  if (this->CurrentValue)
    {
    delete [] this->CurrentValue;
    this->CurrentValue = NULL;
    }
  this->Menu->Delete();
}

//-----------------------------------------------------------------------------
const char *vtkKWOptionMenu::GetValue()
{
  if (this->CurrentValue)
    {
    delete [] this->CurrentValue;
    this->CurrentValue = 0;
    }
  if ( this->Application )
    {
    this->Script("set %sValue",this->GetWidgetName());
    this->CurrentValue = vtkString::Duplicate(
      this->Application->GetMainInterp()->result);
    }
  return this->CurrentValue;  
}

//-----------------------------------------------------------------------------
void vtkKWOptionMenu::SetValue(const char *s)
{
  if (this->Application && s)
    {
    this->Script("set %sValue {%s}", this->GetWidgetName(),s);
    }
}

//-----------------------------------------------------------------------------
void vtkKWOptionMenu::SetCurrentEntry(const char *name)
{ 
  this->SetValue(name);
}
 
//-----------------------------------------------------------------------------
void vtkKWOptionMenu::AddEntry(const char *name)
{
  if (this->Application)
    {
    this->Script("%s add radiobutton -label {%s} -variable %sValue",
                 this->Menu->GetWidgetName(), name, 
                 this->GetWidgetName());
    }
}

//-----------------------------------------------------------------------------
void vtkKWOptionMenu::AddEntryWithCommand(const char *name, 
                                          const char *obj, 
                                          const char *method,
                                          const char *options)
{
  this->Script(
    "%s add radiobutton -label {%s} -variable %sValue -command {%s %s} %s",
    this->Menu->GetWidgetName(), name, 
    this->GetWidgetName(), obj, method, (options ? options : ""));
}

//-----------------------------------------------------------------------------
void vtkKWOptionMenu::AddEntryWithCommand(const char *name, 
                                          vtkKWObject *obj, 
                                          const char *methodAndArgs,
                                          const char *options)
{
  this->AddEntryWithCommand(name, obj->GetTclName(), methodAndArgs, options);
}

//-----------------------------------------------------------------------------
void vtkKWOptionMenu::AddImageEntryWithCommand(const char *imageName, 
                                               const char *obj, 
                                               const char *method,
                                               const char *options)
{
  this->Script(
    "%s add radiobutton -image %s -selectimage %s -variable %sValue -command {%s %s} %s",
    this->Menu->GetWidgetName(), imageName, imageName,
    this->GetWidgetName(), obj, method, (options ? options : ""));
}

//-----------------------------------------------------------------------------
void vtkKWOptionMenu::AddImageEntryWithCommand(const char *imageName, 
                                               vtkKWObject *obj, 
                                               const char *methodAndArgs,
                                               const char *options)
{
  this->AddEntryWithCommand(imageName, 
                            obj->GetTclName(), 
                            methodAndArgs,
                            options);
}

//-----------------------------------------------------------------------------
void vtkKWOptionMenu::DeleteEntry(const char* name)
{ 
  this->Script("%s delete {%s}", this->Menu->GetWidgetName(), name);
}

//-----------------------------------------------------------------------------
void vtkKWOptionMenu::DeleteEntry(int index)
{
  this->Script("%s delete %d", this->Menu->GetWidgetName(), index);
}

//-----------------------------------------------------------------------------
int vtkKWOptionMenu::HasEntry(const char *name)
{
  return (this->IsCreated() &&
          !this->Application->EvaluateBooleanExpression(
            "catch {%s index {%s}}",
            this->Menu->GetWidgetName(), name)) ? 1 : 0;
}

//-----------------------------------------------------------------------------
void vtkKWOptionMenu::ClearEntries()
{
  this->Script("%s delete 0 end", this->Menu->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkKWOptionMenu::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;

  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("OptionMenu already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  
  this->Script("menubutton %s -textvariable %sValue -indicatoron 1 -menu %s -relief raised -bd 2 -highlightthickness 0 -anchor c -direction flush %s", wname, wname, this->Menu->GetWidgetName(), (args?args:""));
  this->Menu->Create(app,"menu","-tearoff 0");

  // Update enable state

  this->UpdateEnableState();
}

//-----------------------------------------------------------------------------
void vtkKWOptionMenu::IndicatorOn()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("%s config -indicatoron 1", this->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkKWOptionMenu::IndicatorOff()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("%s config -indicatoron 0", this->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkKWOptionMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Menu: " << this->Menu << endl;
}
