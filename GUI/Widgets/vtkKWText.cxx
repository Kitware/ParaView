/*=========================================================================

  Module:    vtkKWText.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWText.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWText );
vtkCxxRevisionMacro(vtkKWText, "1.23");

//----------------------------------------------------------------------------
vtkKWText::vtkKWText()
{
  this->ValueString = NULL;
}

//----------------------------------------------------------------------------
vtkKWText::~vtkKWText()
{
  this->SetValueString(NULL);
}

//----------------------------------------------------------------------------
char *vtkKWText::GetValue()
{
  if ( !this->IsCreated() )
    {
    return 0;
    }

  const char *val = 
    this->Script("%s get 1.0 {end -1 chars}", this->GetWidgetName());
  this->SetValueString(this->ConvertTclStringToInternalString(val));
  return this->GetValueString();
}

//----------------------------------------------------------------------------
void vtkKWText::SetValue(const char *s)
{
  if ( !this->IsCreated() )
    {
    return;
    }
  int enabled = this->Enabled;
  if ( !enabled )
    {
    this->SetEnabled(1);
    }
  this->Script("%s delete 1.0 end", this->GetWidgetName());
  if (s)
    {
    const char *str = this->ConvertInternalStringToTclString(s);
    this->Script("catch {%s insert 1.0 {%s}}", 
                 this->GetWidgetName(), str ? str : "");
    }
  if ( !enabled )
    {
    this->SetEnabled(0);
    }
}

//----------------------------------------------------------------------------
void vtkKWText::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "text", args))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWText::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetStateOption(this->Enabled);
}

//----------------------------------------------------------------------------
void vtkKWText::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "ValueString: " << (this->ValueString?this->ValueString:"(none)") << endl;
}

