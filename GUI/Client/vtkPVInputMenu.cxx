/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputMenu.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInputMenu.h"

#include "vtkArrayMap.txx"
#include "vtkDataSet.h"
#include "vtkSource.h"
#include "vtkPVApplication.h"
#include "vtkKWLabel.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVData.h"
#include "vtkPVInputProperty.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPart.h"
#include "vtkPVSource.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInputMenu);
vtkCxxRevisionMacro(vtkPVInputMenu, "1.63");


//----------------------------------------------------------------------------
vtkPVInputMenu::vtkPVInputMenu()
{
  this->InputName = NULL;
  this->Sources = NULL;
  this->CurrentValue = NULL;

  this->Label = vtkKWLabel::New();
  this->Menu = vtkKWOptionMenu::New();
}

//----------------------------------------------------------------------------
vtkPVInputMenu::~vtkPVInputMenu()
{
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
  if (label && label[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(label);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

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
  // Reset will initialze the menu.
  if ( ! currentFound)
    {
    this->SetCurrentValue(NULL);
    this->ModifiedCallback();
    }

  if (this->CurrentValue)
    {
    char* label = this->GetPVApplication()->GetTextRepresentation(
      this->CurrentValue);
    this->Menu->SetValue(label);
    delete[] label;
    }
  else
    {
    this->Menu->SetValue("");
    }

}

//----------------------------------------------------------------------------
int vtkPVInputMenu::AddEntry(vtkPVSource *pvs)
{
  if (pvs == this->PVSource || pvs == NULL)
    {
    return 0;
    }

  // Have to have the same number of parts as last input.
  if (this->CurrentValue)
    {
    if (pvs->GetNumberOfParts() != this->CurrentValue->GetNumberOfParts())
      {
      return 0;
      }
    }

  // Has to meet all requirments from XML filter description.
  vtkSMInputProperty* ip = this->GetInputProperty();
  if ( !ip )
    {
    return 0;
    }
  ip->RemoveAllUncheckedProxies();
  ip->AddUncheckedProxy(pvs->GetProxy());
  if (!ip->IsInDomains())
    {
    return 0;
    }

  char methodAndArgs[1024];
  sprintf(methodAndArgs, "MenuEntryCallback %s", pvs->GetTclName());

  char* label = this->GetPVApplication()->GetTextRepresentation(pvs);
  this->Menu->AddEntryWithCommand(label, this, methodAndArgs);
  delete[] label;
  
  // If there is not an input yet, default to the first in the list.
  // Primarily for glyph source input default.
  if (this->CurrentValue == NULL)
    {
    this->CurrentValue = pvs;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::MenuEntryCallback(vtkPVSource *pvs)
{
  if (pvs == this->CurrentValue)
    {
    return;
    }
  if ( this->CheckForLoop(pvs) )
    {
    vtkKWMessageDialog::PopupMessage(
        this->GetApplication(), this->GetPVApplication()->GetMainWindow(),
        "ParaView Error", 
        "This operation would result in a loop in the pipeline. "
        "Since loops in the pipeline can result in infinite loops, "
        "this operation is prohibited.",
        vtkKWMessageDialog::ErrorIcon);
    this->Menu->SetValue(this->CurrentValue->GetName());
    return;
    }
  this->CurrentValue = pvs;
  this->ModifiedCallback();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::Update()
{
  vtkSMInputProperty* ip = this->GetInputProperty();
  if (ip)
    {
    ip->RemoveAllUncheckedProxies();
    if (this->CurrentValue)
      {
      ip->AddUncheckedProxy(this->CurrentValue->GetProxy());
      }
    ip->UpdateDependentDomains();
    }
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::SetCurrentValue(vtkPVSource *pvs)
{
  if (pvs == this->CurrentValue)
    {
    return;
    }

  this->CurrentValue = pvs;
  if (this->GetApplication() == NULL)
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
int vtkPVInputMenu::CheckForLoop(vtkPVSource *pvs)
{
  if ( !pvs )
    {
    return 0;
    }
  vtkPVSource* source = this->GetPVSource();
  if ( pvs == source )
    {
    return 1;
    }
  int cc;
  int res = 0;
  for ( cc = 0; cc < pvs->GetNumberOfPVInputs(); cc ++ )
    {
    vtkPVSource* input = pvs->GetPVInput(cc);
    if ( input )
      {
      res += this->CheckForLoop(input);
      }
    }
  return res;
}

//----------------------------------------------------------------------------
// vtkPVSource handles this now.
void vtkPVInputMenu::SaveInBatchScript(ofstream*)
{
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::ModifiedCallback()
{
  this->vtkPVWidget::ModifiedCallback();
}

//----------------------------------------------------------------------------
// We should probably cache this value.
// compute it once when the InputName is set ...
int vtkPVInputMenu::GetPVInputIndex()
{
  int num, idx;

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource must be set before translation.");
    return 0;
    }
  num = this->PVSource->GetNumberOfInputProperties();
  for (idx = 0; idx < num; ++idx)
    {
    if (strcmp(this->InputName, 
               this->PVSource->GetInputProperty(idx)->GetName()) == 0)
      {
      return idx;
      }
    }

  vtkErrorMacro("Cound not find VTK input name: " << this->InputName);
  return 0;
}

//----------------------------------------------------------------------------
vtkSMInputProperty* vtkPVInputMenu::GetInputProperty()
{
  return vtkSMInputProperty::SafeDownCast(this->GetSMProperty());
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::AcceptInternal(vtkClientServerID)
{
  // Since this is done on PVSource and not vtk source,
  // We can ignore the sourceTclName and return after the first call.
  if (this->ModifiedFlag == 0)
    {
    return;
    }
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource not set.");
    return;
    }

  if (this->CurrentValue)
    {
    if (
      this->CurrentValue != this->PVSource->GetPVInput(this->GetPVInputIndex()))
      {
      this->Script("%s SetPVInput %s %d %s", 
                   this->PVSource->GetTclName(), 
                   this->InputName,
                   this->GetPVInputIndex(),
                   this->CurrentValue->GetTclName());
      // Turn visibility of ne input off.
      // We cannot put this in vtkPVSource::SetPVInput because
      // it is too early.
      if (this->PVSource->GetReplaceInput())
        {
        this->CurrentValue->SetVisibility(0);
        }
      }
    }
  else
    {
    this->Script("%s SetPVInput %s %d {}", 
                 this->InputName,
                 this->PVSource->GetTclName(), 
                 this->GetPVInputIndex());
    }

  // ??? used to be set to one ???
  this->ModifiedFlag=0;
}


//---------------------------------------------------------------------------
void vtkPVInputMenu::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  if (this->CurrentValue && this->CurrentValue->InitializeTrace(file))
    {
    *file << "$kw(" << this->GetTclName() << ") SetCurrentValue "
          << "$kw(" << this->CurrentValue->GetTclName() << ")\n";
    }
  else
    {
    *file << "$kw(" << this->GetTclName() << ") SetCurrentValue "
          << "{}\n";
    }
}



//----------------------------------------------------------------------------
void vtkPVInputMenu::ResetInternal()
{
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource not set.");
    return;
    }

  // The list of possible inputs could have changed.
  this->AddSources(this->Sources);

  // Set the current value.  The catch is here because GlyphSource is
  // initially NULL which causes an error.
  vtkPVSource* input = this->PVSource->GetPVInput(this->GetPVInputIndex());
  if (input)
    { // Use our default if there in no input set (glyph source).
    this->Script("%s SetCurrentValue %s", 
               this->GetTclName(), input->GetTclName());
    // Only turn off modified if we successfully set input.
    if (this->AcceptCalled)
      {
      this->ModifiedFlag = 0;
      }

    // Update any widgets that depend on this input menu.
    this->Update();
    }
}

//----------------------------------------------------------------------------
vtkPVInputMenu* vtkPVInputMenu::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVInputMenu::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVInputMenu* pvim = vtkPVInputMenu::SafeDownCast(clone);
  if (pvim)
    {
    pvim->SetLabel(this->Label->GetLabel());
    pvim->SetInputName(this->InputName);
    pvim->SetSources(this->GetSources());
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
const char* vtkPVInputMenu::GetLabel() 
{
  return this->Label->GetLabel();
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::SetSources(vtkPVSourceCollection *sources) 
{
  this->Sources = sources;
}

//----------------------------------------------------------------------------
vtkPVSourceCollection *vtkPVInputMenu::GetSources() 
{
  return this->Sources;
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::ClearEntries() 
{ 
  this->Menu->ClearEntries();
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Label);
  this->PropagateEnableState(this->Menu);
}

//----------------------------------------------------------------------------
int vtkPVInputMenu::GetNumberOfSources()
{
  if (!this->Sources)
    {
    return 0;
    }
  return this->Sources->GetNumberOfItems();
}

//----------------------------------------------------------------------------
vtkPVSource* vtkPVInputMenu::GetSource(int i)
{
  if ( i < 0 || i >= this->GetNumberOfSources() )
    {
    return 0;
    }
  return vtkPVSource::SafeDownCast(this->Sources->GetItemAsObject(i));
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputName: " << (this->InputName?this->InputName:"none") 
     << endl;
}
