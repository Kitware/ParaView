/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectionList.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSelectionList.h"

#include "vtkArrayMap.txx"
#include "vtkKWLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkStringList.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSelectionList);
vtkCxxRevisionMacro(vtkPVSelectionList, "1.49");

int vtkPVSelectionListCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVSelectionList::vtkPVSelectionList()
{
  this->CommandFunction = vtkPVSelectionListCommand;

  this->CurrentValue = 0;
  this->CurrentName = NULL;
  
  this->Label = vtkKWLabel::New();
  this->Menu = vtkKWOptionMenu::New();

  this->Names = vtkStringList::New();

  this->OptionWidth = 0;
}

//----------------------------------------------------------------------------
vtkPVSelectionList::~vtkPVSelectionList()
{
  this->SetCurrentName(NULL);
  
  this->Label->Delete();
  this->Label = NULL;
  this->Menu->Delete();
  this->Menu = NULL;
  this->Names->Delete();
  this->Names = NULL;
}

void vtkPVSelectionList::SetBalloonHelpString(const char *str)
{

  // A little overkill.
  if (this->BalloonHelpString == NULL && str == NULL)
    {
    return;
    }

  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (str != this->BalloonHelpString)
    {
    // Normal string stuff.
    if (this->BalloonHelpString)
      {
      delete [] this->BalloonHelpString;
      this->BalloonHelpString = NULL;
      }
    if (str != NULL)
      {
      this->BalloonHelpString = new char[strlen(str)+1];
      strcpy(this->BalloonHelpString, str);
      }
    }
  
  if ( this->GetApplication() && !this->BalloonHelpInitialized )
    {
    this->Label->SetBalloonHelpString(this->BalloonHelpString);
    this->Menu->SetBalloonHelpString(this->BalloonHelpString);
    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::Disable()
{
  // TCL 8.2 does not allow to disable a label. Use the menu's
  // color to make it look like disabled.
  this->Script("%s configure -foreground [%s cget -disabledforeground]", 
               this->Label->GetWidgetName(),
               this->Menu->GetWidgetName());
  this->Script("%s configure -state disabled", this->Menu->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::Create(vtkKWApplication *app)
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
  if (this->OptionWidth > 0)
    {
    char arg[128];
    sprintf(arg, "-width %d", this->OptionWidth);
    this->Menu->Create(app, arg);
    }
  else
    {
    this->Menu->Create(app, "");
    }
  this->Script("pack %s -side left", this->Menu->GetWidgetName());

  char tmp[1024];
  int i, numItems = this->Names->GetLength();
  const char *name;
  for(i=0; i<numItems; i++)
    {
    name = this->Names->GetString(i);
    if (name)
      {
      sprintf(tmp, "SelectCallback {%s} %d", name, i);
      this->Menu->AddEntryWithCommand(name, this, tmp);
      }
    }
  name = this->Names->GetString(this->CurrentValue);
  if (name)
    {
    this->Menu->SetValue(name);
    }
  this->SetBalloonHelpString(this->BalloonHelpString);
}


//----------------------------------------------------------------------------
void vtkPVSelectionList::SetLabel(const char* label) 
{
  // For getting the widget in a script.
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
const char *vtkPVSelectionList::GetLabel()
{
  return this->Label->GetLabel();
}

//-----------------------------------------------------------------------------
void vtkPVSelectionList::SaveInBatchScript(ofstream *file)
{
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);
  if (sourceID.ID == 0 || !this->SMPropertyName)
    {
    vtkErrorMacro("Sanity check failed. "
                  << this->GetClassName());
    return;
    }
  
  
  *file << "  [$pvTemp" << sourceID <<  " GetProperty "
        << this->SMPropertyName << "] SetElements1 "
        << this->GetCurrentValue() << endl;
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::Accept()
{
  int modFlag = this->GetModifiedFlag();
  
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (ivp)
    {
    ivp->SetNumberOfElements(1);
    ivp->SetElement(0, this->CurrentValue);
    }
  else
    {
    vtkErrorMacro(
      "Could not find property of name: "
      << (this->GetSMPropertyName()?this->GetSMPropertyName():"(null)")
      << " for widget: " << this->GetTraceName());
    }
  
  this->ModifiedFlag = 0;

  
  // I put this after the accept internal, because
  // vtkPVGroupWidget inactivates and builds an input list ...
  // Putting this here simplifies subclasses AcceptInternal methods.
  if (modFlag)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    ofstream* file = pvApp->GetTraceFile();
    if (file)
      {
      this->Trace(file);
      }
    }

  this->AcceptCalled = 1;
}


//---------------------------------------------------------------------------
void vtkPVSelectionList::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetCurrentValue {"
        << this->GetCurrentValue() << "}" << endl;
}


//----------------------------------------------------------------------------
void vtkPVSelectionList::ResetInternal()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (ivp)
    {
    // Get the selected item.
    this->SetCurrentValue(ivp->GetElement(0));
    }
  
  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
}


//----------------------------------------------------------------------------
void vtkPVSelectionList::AddItem(const char *name, int value)
{
  char tmp[1024];
  
  // Save for internal use
  this->Names->SetString(value, name);

  // It should be possible to add items without creating
  // the widget. This is necessary for the prototypes.
  if (this->GetApplication())
    {
    sprintf(tmp, "SelectCallback {%s} %d", name, value);
    this->Menu->AddEntryWithCommand(name, this, tmp);
    
    if (value == this->CurrentValue)
      {
      this->Menu->SetValue(name);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::SetCurrentValue(int value)
{
  const char *name;

  if (this->CurrentValue == value)
    {
    return;
    }

  this->CurrentValue = value;
  name = this->Names->GetString(value);
  if (name)
    {
    this->Menu->SetValue(name);
    this->SelectCallback(name, value);
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::SelectCallback(const char *name, int value)
{
  if (this->CurrentValue == value)
    {
    return;
    }
  
  this->CurrentValue = value;
  this->SetCurrentName(name);
  
  this->ModifiedCallback();
  this->Update();
}

vtkPVSelectionList* vtkPVSelectionList::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVSelectionList::SafeDownCast(clone);
}

void vtkPVSelectionList::CopyProperties(vtkPVWidget* clone, 
                                        vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVSelectionList* pvsl = vtkPVSelectionList::SafeDownCast(clone);
  if (pvsl)
    {
    pvsl->SetOptionWidth(this->OptionWidth);
    pvsl->SetLabel(this->Label->GetLabel());
    int i, numItems = this->Names->GetLength();
    const char *name;
    for(i=0; i<numItems; i++)
      {
      name = this->Names->GetString(i);
      if (name)
        {
        pvsl->Names->SetString(i, name);
        }
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVSelectionList.");
    }
}

//----------------------------------------------------------------------------
int vtkPVSelectionList::ReadXMLAttributes(vtkPVXMLElement* element,
                                          vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Option menu width
  if(!element->GetScalarAttribute("option_width", &this->OptionWidth))
    {
    this->OptionWidth = 0;
    }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->Label->SetLabel(label);  
    }
  else
    {
    this->Label->SetLabel(this->VariableName);
    }

  // Extract the list of items.
  unsigned int i;
  for(i=0;i < element->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* item = element->GetNestedElement(i);
    if(strcmp(item->GetName(), "Item") != 0)
      {
      vtkErrorMacro("Found non-Item element in SelectionList.");
      return 0;
      }
    const char* itemName = item->GetAttribute("name");
    if(!itemName)
      {
      vtkErrorMacro("Item has no name.");
      return 0;
      }
    int itemValue;
    if(!item->GetScalarAttribute("value", &itemValue))
      {
      vtkErrorMacro("Item has no value.");
      return 0;
      }
    this->AddItem(itemName, itemValue);
    }

  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Label);
  this->PropagateEnableState(this->Menu);
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CurrentName: " << (this->CurrentName?this->CurrentName:"none") << endl;
  os << indent << "CurrentValue: " << this->CurrentValue << endl;
  os << indent << "OptionWidth: " << this->OptionWidth << endl;
}
