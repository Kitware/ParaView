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

#include "vtkPVSource.h"
#include "vtkPVInputMenu.h"
#include "vtkPVData.h"
#include "vtkObjectFactory.h"
#include "vtkPVSourceCollection.h"
#include "vtkArrayMap.txx"
#include "vtkPVXMLElement.h"
#include "vtkPVWindow.h"

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
  this->VTKInputName = NULL;
  // Default name.
  this->SetVTKInputName("Input");
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
  this->SetVTKInputName(NULL);
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
void vtkPVInputMenu::AddSources(vtkPVSourceCollection *sources)
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
  this->Update();
}

//----------------------------------------------------------------------------
vtkPVData *vtkPVInputMenu::GetPVData()
{
  vtkPVSource *pvs = this->GetCurrentValue();

  if (pvs == NULL)
    {
    return NULL;
    }
  return pvs->GetPVOutput();
}

//----------------------------------------------------------------------------
vtkDataSet *vtkPVInputMenu::GetVTKData()
{
  vtkPVSource *pvs = this->GetCurrentValue();

  if (pvs == NULL)
    {
    return NULL;
    }
  return pvs->GetPVOutput()->GetVTKData();
}


//----------------------------------------------------------------------------
void vtkPVInputMenu::SaveInTclScript(ofstream *file)
{
  if (this->CurrentValue == NULL)
    {
    return;
    }
  *file << "\t";
  // This is a bit of a hack to get the input name.
  *file << this->PVSource->GetVTKSourceTclName() << " Set" 
        << this->VTKInputName << " ["
        << this->CurrentValue->GetVTKSourceTclName() << " GetOutput]\n";
}



//----------------------------------------------------------------------------
void vtkPVInputMenu::ModifiedCallback()
{
  this->vtkPVWidget::ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::Accept()
{
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource not set.");
    return;
    }

  if (this->CurrentValue)
    {
    this->Script("%s Set%s %s", this->PVSource->GetTclName(), this->InputName,
                 this->CurrentValue->GetPVOutput()->GetTclName());
    if (this->ModifiedFlag && this->CurrentValue->InitializeTrace())
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

vtkPVInputMenu* vtkPVInputMenu::ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVInputMenu::SafeDownCast(clone);
}

void vtkPVInputMenu::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVInputMenu* pvim = vtkPVInputMenu::SafeDownCast(clone);
  if (pvim)
    {
    pvim->SetLabel(this->Label->GetLabel());
    pvim->SetInputName(this->InputName);
    pvim->SetInputType(this->InputType);
    pvim->SetSources(this->GetSources());
    pvim->SetVTKInputName(this->VTKInputName);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVInputMenu.");
    }
}

//----------------------------------------------------------------------------
int vtkPVInputMenu::ReadXMLAttributes(vtkPVXMLElement* element,
                                      vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(!label)
    {
    vtkErrorMacro("No label attribute.");
    return 0;
    }
  this->Label->SetLabel(label);  
  
  // Setup the InputName.
  const char* input_name = element->GetAttribute("input_name");
  if(input_name)
    {
    this->SetInputName(input_name);
    }
  else
    {
    this->SetInputName("Input");
    }
  
  // Setup the InputType.
  const char* input_type = element->GetAttribute("input_type");
  if(!input_type)
    {
    vtkErrorMacro("No input_type attribute.");
    return 0;
    }
  this->SetInputType(input_type);
  
  vtkPVWindow* window = this->GetPVWindowFormParser(parser);
  const char* source_list = element->GetAttribute("source_list");
  if(source_list)
    {
    this->SetSources(window->GetSourceList(source_list));
    }
  else
    {
    this->SetSources(window->GetSourceList("Sources"));
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputName: " << (this->InputName?this->InputName:"none") 
     << endl;
  os << indent << "InputType: " << (this->InputType?this->InputType:"none") 
     << endl;
  os << indent << "VTKInputName: " 
     << (this->VTKInputName?this->VTKInputName:"none") << endl;
}
