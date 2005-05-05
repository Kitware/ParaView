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
#include "vtkPVDisplayGUI.h"
#include "vtkPVInputProperty.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPart.h"
#include "vtkPVSource.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVTraceHelper.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInputMenu);
vtkCxxRevisionMacro(vtkPVInputMenu, "1.72");


//----------------------------------------------------------------------------
vtkPVInputMenu::vtkPVInputMenu()
{
  this->InputName = NULL;
  this->Sources = NULL;
  this->CurrentValue = NULL;

  this->Label = vtkKWLabel::New();
  this->Menu = vtkKWOptionMenu::New();

  this->InitializeWithCurrent = 1;
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
  this->Label->SetText(label);
  if (label && label[0] &&
      (this->GetTraceHelper()->GetObjectNameState() == 
       vtkPVTraceHelper::ObjectNameStateUninitialized ||
       this->GetTraceHelper()->GetObjectNameState() == 
       vtkPVTraceHelper::ObjectNameStateDefault) )
    {
    this->GetTraceHelper()->SetObjectName(label);
    this->GetTraceHelper()->SetObjectNameState(
      vtkPVTraceHelper::ObjectNameStateSelfInitialized);
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

  this->DeleteAllEntries();
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
  ip->RemoveAllUncheckedProxies();

  char methodAndArgs[1024];
  sprintf(methodAndArgs, "MenuEntryCallback %s", pvs->GetTclName());

  char* label = this->GetPVApplication()->GetTextRepresentation(pvs);
  this->Menu->AddEntryWithCommand(label, this, methodAndArgs);
  delete[] label;
  
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
    char* label = this->GetPVApplication()->GetTextRepresentation(pvs);
    this->Menu->SetValue(label);
    delete[] label;
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
void vtkPVInputMenu::Accept()
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
                 this->PVSource->GetTclName(), 
                 this->InputName,
                 this->GetPVInputIndex());
    }

  this->Superclass::Accept();
}


//---------------------------------------------------------------------------
void vtkPVInputMenu::Trace(ofstream *file)
{
  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }

  if (this->CurrentValue && 
      this->CurrentValue->GetTraceHelper()->Initialize(file))
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
void vtkPVInputMenu::Initialize()
{
  // If there is not an input yet, default to the current source
  // or the first one in the list.
  if (this->CurrentValue == NULL)
    {
    if (this->InitializeWithCurrent)
      {
      this->CurrentValue = 
        this->GetPVSource()->GetPVWindow()->GetCurrentPVSource();
      }
    else
      {
      this->Sources->InitTraversal();
      vtkPVSource* pvs = vtkPVSource::SafeDownCast(
        this->Sources->GetNextItemAsObject());
      if (pvs)
        {
        this->CurrentValue = pvs;
        }
      }
    this->PVSource->SetPVInput(
      this->InputName, this->GetPVInputIndex(), this->CurrentValue);
    }

  // The list of possible inputs could have changed.
  this->AddSources(this->Sources);

  // Update any widgets that depend on this input menu.
  this->Update();
}

//----------------------------------------------------------------------------
vtkPVSource* vtkPVInputMenu::GetLastAcceptedValue()
{
  return this->PVSource->GetPVInput(this->GetPVInputIndex());
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::Select()
{
  // The list of possible inputs could have changed.
  this->AddSources(this->Sources);
}

//----------------------------------------------------------------------------
void vtkPVInputMenu::ResetInternal()
{
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource not set.");
    return;
    }

  vtkPVSource* input = this->PVSource->GetPVInput(this->GetPVInputIndex());
  if (input)
    {
    this->Script("%s SetCurrentValue %s", 
                 this->GetTclName(), 
                 input->GetTclName());

    // Update any widgets that depend on this input menu.
    // SetCurrentValue already has a call to this->Update
    // so this call is redundant and also erroneous when
    // input == this->CurrentValue, (as it leads to ModifiedCallback
    // which leads to setting the Accept button green in  a Reset method!
    // this->Update();
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
    pvim->SetLabel(this->Label->GetText());
    pvim->SetInputName(this->InputName);
    pvim->SetSources(this->GetSources());
    pvim->InitializeWithCurrent = this->InitializeWithCurrent;
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
  this->Label->SetText(label);  
  
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

  if(!element->GetScalarAttribute("initialize_with_current", 
                                  &this->InitializeWithCurrent))
    {
    this->InitializeWithCurrent = 1;
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
  return this->Label->GetText();
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
void vtkPVInputMenu::DeleteAllEntries() 
{ 
  this->Menu->DeleteAllEntries();
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
