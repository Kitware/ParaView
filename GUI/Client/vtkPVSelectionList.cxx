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
#include "vtkPVIndexWidgetProperty.h"
#include "vtkPVXMLElement.h"
#include "vtkStringList.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSelectionList);
vtkCxxRevisionMacro(vtkPVSelectionList, "1.41");

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
  
  this->DefaultValue = 0;
  
  this->Property = NULL;
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
  
  this->SetProperty(NULL);
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
  
  if ( this->Application && !this->BalloonHelpInitialized )
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

  this->SetCurrentValue(this->Property->GetIndex());
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

//----------------------------------------------------------------------------
void vtkPVSelectionList::AcceptInternal(vtkClientServerID sourceId)
{
  this->ModifiedFlag = 0;
  this->Property->SetIndex(this->CurrentValue);
  this->Property->SetVTKSourceID(sourceId);
  
  this->Property->AcceptInternal();
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
  this->SetCurrentValue(this->Property->GetIndex());
  
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
  if (this->Application)
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
  
  if (!this->AcceptCalled)
    {
    this->Property->SetIndex(value);
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::SelectCallback(const char *name, int value)
{
  this->CurrentValue = value;
  this->SetCurrentName(name);
  
//  this->Menu->SetButtonText(name);
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
    pvsl->SetDefaultValue(this->DefaultValue);
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

  const char* defaultValue = element->GetAttribute("default_value");
  if (defaultValue)
    {
    sscanf(defaultValue, "%d", &this->DefaultValue);
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
void vtkPVSelectionList::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVIndexWidgetProperty::SafeDownCast(prop);
  if (this->Property)
    {
    char *cmd = new char[strlen(this->VariableName)+4];
    sprintf(cmd, "Set%s", this->VariableName);
    this->Property->SetVTKCommand(cmd);
    this->Property->SetIndex(this->DefaultValue);
    delete [] cmd;
    }
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVSelectionList::GetProperty()
{
  return this->Property;
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVSelectionList::CreateAppropriateProperty()
{
  return vtkPVIndexWidgetProperty::New();
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
