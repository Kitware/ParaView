/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWCheckButton.cxx
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
#include "vtkKWCheckButton.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWCheckButton );
vtkCxxRevisionMacro(vtkKWCheckButton, "1.18");

//------------------------------------------------------------------------------
vtkKWCheckButton::vtkKWCheckButton() 
{
  this->IndicatorOn = 1;
  this->MyText = 0;
  this->VariableName = NULL;
}

//------------------------------------------------------------------------------
vtkKWCheckButton::~vtkKWCheckButton() 
{
  this->SetMyText(0);
  this->SetVariableName(0);
}

//------------------------------------------------------------------------------
void vtkKWCheckButton::SetVariableName(const char* _arg)
{
  if (this->VariableName == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->VariableName && _arg && (!strcmp(this->VariableName, _arg))) 
    { 
    return;
    }

  int has_old_state = 0, old_state = 0;
  if (this->VariableName) 
    { 
    if (_arg)
      {
      has_old_state = 1;
      old_state = this->GetState();
      }
    delete [] this->VariableName; 
    }

  if (_arg)
    {
    this->VariableName = new char[strlen(_arg)+1];
    strcpy(this->VariableName,_arg);
    }
   else
    {
    this->VariableName = NULL;
    }

  this->Modified();
  
  if (this->IsCreated() && this->VariableName)
    {
    this->Script("%s configure -variable {%s}", 
                 this->GetWidgetName(), this->VariableName);
    if (has_old_state)
      {
      this->SetState(old_state);
      }
    }
} 

//------------------------------------------------------------------------------
void vtkKWCheckButton::SetIndicator(int ind)
{
  if (ind != this->IndicatorOn)
    {
    this->IndicatorOn = ind;
    this->Modified();
    if (this->IsCreated())
      {
      this->Script("%s configure -indicatoron %d", 
                   this->GetWidgetName(), (ind ? 1 : 0));
      }
    }
  this->SetMyText(0);
}

//------------------------------------------------------------------------------
void vtkKWCheckButton::SetText(const char* txt)
{
  this->SetMyText(txt);
  
  if (this->IsCreated())
    {
    if (this->MyText)
      {
      this->Script("%s configure -text {%s}", 
                   this->GetWidgetName(), this->MyText);
      }
    }
}

//------------------------------------------------------------------------------
const char* vtkKWCheckButton::GetText()
{
  return this->MyText;
}

//------------------------------------------------------------------------------
int vtkKWCheckButton::GetState()
{
  if (this->IsCreated())
    {
    this->Script("expr {${%s}} == {[%s cget -onvalue]}",
                 this->VariableName, this->GetWidgetName());
    return vtkKWObject::GetIntegerResult(this->Application);
    }
  return 0;
}

//------------------------------------------------------------------------------
void vtkKWCheckButton::SetState(int s)
{
  if (this->IsCreated())
    {
    if (s)
      {
      this->Script("%s select",this->GetWidgetName());
      }
    else
      {
      this->Script("%s deselect",this->GetWidgetName());
      }
    }
}

//------------------------------------------------------------------------------
void vtkKWCheckButton::Create(vtkKWApplication *app, const char *args)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("CheckButton already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  const char *wname = this->GetWidgetName();

  this->Script("checkbutton %s", wname);
  this->Configure();
  this->Script("%s configure %s", wname, (args?args:""));
}

//------------------------------------------------------------------------------
void vtkKWCheckButton::Configure()
{
  const char *wname = this->GetWidgetName();

  this->Script("%s configure -indicatoron %d",
               wname, (this->IndicatorOn ? 1 : 0));

  if (this->MyText)
    {
    this->Script("%s configure -text {%s}", wname, this->MyText);
    }

  // Set the variable name if not set already
  if (!this->VariableName)
    {
    char *vname = new char [strlen(wname) + 5 + 1];
    sprintf(vname, "%sValue", wname);
    this->SetVariableName(vname);
    delete [] vname;
    }
  else
    {
    this->Script("%s configure -variable {%s}", wname, this->VariableName);
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "VariableName: " 
     << (this->VariableName ? this->VariableName : "None" );
}
