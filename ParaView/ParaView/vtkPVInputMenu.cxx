/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputMenu.cxx
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

#include "vtkPVInputMenu.h"
#include "vtkPVData.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkPVInputMenu* vtkPVInputMenu::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVInputMenu");
  if(ret)
    {
    return (vtkPVInputMenu*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVInputMenu;
}

//----------------------------------------------------------------------------
vtkPVInputMenu::vtkPVInputMenu()
{
  this->InputType = NULL;
  this->InputName = NULL;
  this->Sources = NULL;
  this->CurrentValue = NULL;

  this->Label = vtkKWLabel::New();
  this->Menu = vtkKWOptionMenu::New();
}

//----------------------------------------------------------------------------
vtkPVInputMenu::~vtkPVInputMenu()
{
  if (this->InputType)
    {
    delete [] this->InputType;
    this->InputType = NULL;
    }
  this->SetInputName(NULL);
  this->Sources = NULL;

  this->Label->Delete();
  this->Label = NULL;
  this->Menu->Delete();
  this->Menu = NULL;
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::SetLabel(const char* label)
{
  this->Label->SetLabel(label);
  this->SetTraceName(label);
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::Create(vtkKWApplication *app)
{
  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    return;
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());

  this->Label->SetParent(this);
  this->Label->Create(app, "-width 18 -justify right");
  this->Script("pack %s -side left", this->Label->GetWidgetName());

  this->Menu->SetParent(this);
  this->Menu->Create(app, "");
  this->Script("pack %s -side left", this->Menu->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::AddSources(vtkCollection *sources)
{
  vtkObject *o;
  vtkPVSource *source;
  int currentFound = 0;
  
  if (sources == NULL)
    {
    return;
    }

  this->ClearEntries();
  sources->InitTraversal();
  while ( (o = sources->GetNextItemAsObject()) )
    {
    source = vtkPVSource::SafeDownCast(o);
    if (this->AddEntry(source) && source == this->CurrentValue)
      {
      currentFound = 1;
      }
    }
  // Input should be initialized in VTK and reset will initialze the menu.
  if ( ! currentFound)
    {
    // Set the default source.
    sources->InitTraversal();
    this->SetCurrentValue((vtkPVSource*)(sources->GetNextItemAsObject()));
    this->ModifiedCallback();
    }

  if (this->CurrentValue)
    {
    this->Menu->SetValue(this->CurrentValue->GetName());
    }
  else
    {
    this->Menu->SetValue("");
    }

}

//----------------------------------------------------------------------------
int vtkPVInputMenu::AddEntry(vtkPVSource *pvs)
{
  if (pvs == this->PVSource)
    {
    return 0;
    }

  if (pvs == NULL || pvs->GetPVOutput() == NULL)
    {
    return 0;
    }

  if (this->InputType == NULL || 
      ! pvs->GetPVOutput()->GetVTKData()->IsA(this->InputType))
    {
    return 0;
    }

  char methodAndArgs[1024];
  sprintf(methodAndArgs, "MenuEntryCallback %s", pvs->GetTclName());

  this->Menu->AddEntryWithCommand(pvs->GetName(), 
                                  this, methodAndArgs);
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::MenuEntryCallback(vtkPVSource *pvs)
{
  if (pvs == this->CurrentValue)
    {
    return;
    }
  this->CurrentValue = pvs;
  this->ModifiedCallback();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::SetCurrentValue(vtkPVSource *pvs)
{
  if (pvs == this->CurrentValue)
    {
    return;
    }
  this->CurrentValue = pvs;
  if (this->Application == NULL)
    {
    return;
    }
  if (pvs)
    {
    this->Menu->SetValue(pvs->GetName());
    }
  else
    {
    this->Menu->SetValue("");
    }
  this->ModifiedCallback();
}



//----------------------------------------------------------------------------
void vtkPVInputMenu::ModifiedCallback()
{
  this->vtkPVWidget::ModifiedCallback();
}



//----------------------------------------------------------------------------
void vtkPVInputMenu::Accept()
{
  // Why does the widget have to be modified in order to execute this method???
  
//  if ( ! this->ModifiedFlag)
//    {
//    return;
//    }

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource not set.");
    return;
    }

  if (this->CurrentValue)
    {
    this->Script("%s Set%s %s", this->PVSource->GetTclName(), this->InputName,
                 this->CurrentValue->GetPVOutput()->GetTclName());
    if (this->ModifiedFlag)
      {
      this->AddTraceEntry("$kw(%s) SetCurrentValue $kw(%s)", 
                           this->GetTclName(), 
                           this->CurrentValue->GetTclName());
      }
    }
  else
    {
    this->Script("%s Set%s {}", this->PVSource->GetTclName(), this->InputName);
    if (this->ModifiedFlag)
      {
      this->AddTraceEntry("$kw(%s) SetCurrentValue {}", 
                           this->GetTclName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::Reset()
{
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource not set.");
    return;
    }

  // The list of possible inputs could have changed.
  this->AddSources(this->Sources);

  // Set the current value.
  // The catch is here because GlyphSource is initially NULL which causes an error.
  this->Script("catch {%s SetCurrentValue [[%s Get%s] GetPVSource]}", 
               this->GetTclName(), 
               this->PVSource->GetTclName(), this->InputName);    

  // Only turn modified off if the SetCurrentValue was successful.
  if (this->GetIntegerResult(this->Application) == 0)
    {
    this->ModifiedFlag = 0;
    }
  else
    {
    this->ModifiedFlag = 1;
    }
}
