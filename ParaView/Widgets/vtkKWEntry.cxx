/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWEntry.cxx
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
#include "vtkKWEntry.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWEntry );

vtkKWEntry::vtkKWEntry()
{
  this->ValueString = NULL;
  this->Width = -1;
}

vtkKWEntry::~vtkKWEntry()
{
  this->SetValueString(NULL);
}



char *vtkKWEntry::GetValue()
{
  this->Script("%s get", this->GetWidgetName());
  this->SetValueString( this->Application->GetMainInterp()->result );
  return this->GetValueString();
}

int vtkKWEntry::GetValueAsInt()
{
  return atoi(this->GetValue());
}

float vtkKWEntry::GetValueAsFloat()
{
  return atof(this->GetValue());
}

void vtkKWEntry::SetValue(const char *s)
{
  this->Script("%s delete 0 end", this->GetWidgetName());
  if (s)
    {
    this->Script("%s insert 0 {%s}", this->GetWidgetName(),s);
    }
}

void vtkKWEntry::SetValue(int i)
{
  this->Script("%s delete 0 end", this->GetWidgetName());
  this->Script("%s insert 0 %d", this->GetWidgetName(),i);
}

void vtkKWEntry::SetValue(float f, int size)
{
  char tmp[1024];
  
  this->Script("%s delete 0 end",this->GetWidgetName());
  sprintf(tmp,"%%s insert 0 %%.%df",size);
  this->Script(tmp,this->GetWidgetName(),f);
}

void vtkKWEntry::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Entry already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  if (this->Width > 0)
    {
    this->Script("entry %s -width %d -textvariable %sValue %s",
                 wname, this->Width, wname,args);
    }
  else
    {
    this->Script("entry %s -textvariable %sValue %s",wname,wname,args);
    }
}

void vtkKWEntry::SetWidth(int width)
{
  if (this->Width == width)
    {
    return;
    }

  this->Modified();
  this->Width = width;

  if (this->Application != NULL)
    {
    this->Script("%s configure -width %d", this->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
void vtkKWEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Width: " << this->GetWidth() << endl;
}
