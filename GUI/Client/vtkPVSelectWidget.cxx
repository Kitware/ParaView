/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSelectWidget.h"

#include "vtkArrayMap.txx"
#include "vtkKWFrame.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVStringWidgetProperty.h"
#include "vtkPVWidgetProperty.h"
#include "vtkPVXMLElement.h"
#include "vtkStringList.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVSourceWidget.h"
#include "vtkPVWidgetCollection.h"
#include "vtkClientServerStream.h"
#include "vtkCollectionIterator.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"

#include <vtkstd/string>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSelectWidget);
vtkCxxRevisionMacro(vtkPVSelectWidget, "1.54");

int vtkPVSelectWidgetCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//-----------------------------------------------------------------------------
vtkPVSelectWidget::vtkPVSelectWidget()
{
  this->CommandFunction = vtkPVSelectWidgetCommand;

  this->LabeledFrame = vtkKWLabeledFrame::New();
  this->Menu = vtkKWOptionMenu::New();

  this->Labels = vtkStringList::New();
  this->Values = vtkStringList::New();
  this->Widgets = vtkPVWidgetCollection::New();

  this->CurrentIndex = -1;
  this->EntryLabel = 0;

  this->Property = NULL;

  this->ElementType = vtkPVSelectWidget::STRING;
}

//-----------------------------------------------------------------------------
vtkPVSelectWidget::~vtkPVSelectWidget()
{
  this->LabeledFrame->Delete();
  this->LabeledFrame = NULL;
  this->Menu->Delete();
  this->Menu = NULL;
  this->Labels->Delete();
  this->Labels = NULL;
  this->Values->Delete();
  this->Values = NULL;
  this->Widgets->Delete();
  this->Widgets = NULL;
  this->SetEntryLabel(0);
  
  this->SetProperty(NULL);
}

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->LabeledFrame->SetParent(this);
  this->LabeledFrame->ShowHideFrameOn();
  this->LabeledFrame->Create(app, 0);
  if (this->EntryLabel)
    {
    this->LabeledFrame->SetLabel(this->EntryLabel);
    }
  this->Script("pack %s -side top -fill both -expand true", 
               this->LabeledFrame->GetWidgetName());

  vtkKWWidget *justifyFrame = vtkKWWidget::New();
  justifyFrame->SetParent(this->LabeledFrame->GetFrame());
  justifyFrame->Create(app, "frame", "");
  this->Script("pack %s -side top -fill x -expand true", 
               justifyFrame->GetWidgetName());

  this->Menu->SetParent(justifyFrame);
  this->Menu->Create(app, "");
  this->Script("pack %s -side left", this->Menu->GetWidgetName());

  justifyFrame->Delete();
  int len = this->Widgets->GetNumberOfItems();
  
  vtkPVWidget* widget;
  
  int i;
  for(i=0; i<len; i++)
    {
    widget = static_cast<vtkPVWidget*>(this->Widgets->GetItemAsObject(i));
    if (!widget->GetApplication())
      {
      widget->Create(this->GetApplication());
      }
    }

  len = this->Labels->GetLength();
  const char* label;
  for(i=0; i<len; i++)
    {
    label = this->Labels->GetString(i);
    this->Menu->AddEntryWithCommand(label, this, "MenuCallback");
    }
  if (len > 0 && this->CurrentIndex < 0)
    {
    this->Menu->SetValue(this->Labels->GetString(0));
    this->SetCurrentIndex(0);
    this->Property->SetString(this->GetVTKValue(0));
    }
}

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::SetLabel(const char* label) 
{
  // For getting the widget in a script.
  this->SetEntryLabel(label);
  if (label && label[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(label);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }

  if (this->GetApplication())
    {
    this->LabeledFrame->SetLabel(label);
    }
}
  
//-----------------------------------------------------------------------------
int vtkPVSelectWidget::GetModifiedFlag()
{
  if (this->ModifiedFlag)
    {
    return 1;
    }

  if (this->CurrentIndex >= 0)
    {
    vtkPVWidget *widget = (vtkPVWidget*)this->Widgets->GetItemAsObject(this->CurrentIndex);
    return widget->GetModifiedFlag();
    }
  return 0;
}
  
//-----------------------------------------------------------------------------
void vtkPVSelectWidget::SaveInBatchScript(ofstream *file)
{
  vtkPVWidget *widget = vtkPVWidget::SafeDownCast(
    this->Widgets->GetItemAsObject(this->CurrentIndex));
  if (!widget)
    {
    return;
    }

  widget->SaveInBatchScript(file);

  vtkSMInputProperty *ip = vtkSMInputProperty::SafeDownCast(
    this->GetSMProperty());
  vtkPVSourceWidget *sw = vtkPVSourceWidget::SafeDownCast(
    this->Widgets->GetItemAsObject(this->CurrentIndex));
  if (ip && sw)
    {
    vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);
    if (!sourceID.ID || !this->SMPropertyName)
      {
      vtkErrorMacro("Sanity check failed. " << this->GetClassName());
      return;
      }
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << this->SMPropertyName << "] RemoveAllProxies" << endl;
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << this->SMPropertyName << "] AddProxy $pvTemp"
          << sw->GetSourceProxy()->GetID(0);
    }
  else
    {
    if(this->ElementType == OBJECT)
      { 
      // TODO Fix this hack.
      vtkPVObjectWidget* ow = vtkPVObjectWidget::SafeDownCast(widget);
      if (strcmp(this->GetCurrentVTKValue(), "Output") == 0)
        {
        *file << "  [$pvTemp" << this->PVSource->GetVTKSourceID(0) 
              << " GetProperty " << this->VariableName
              << "] AddProxy ";
        if (ow)
          {
          vtkClientServerID id = ow->GetObjectByName(this->GetCurrentVTKValue());
          *file << "$pvTemp" << id.ID;
          }
        else
          {
          *file << "{}" ;
          }
        }
      else
        {
        *file << "  [$pvTemp" << this->PVSource->GetVTKSourceID(0) 
              <<  " GetProperty " << this->VariableName << "] AddProxy ";
        if (ow)
          {
          vtkClientServerID id = ow->GetObjectByName(this->GetCurrentVTKValue());
          *file << "$pvTemp" << id.ID;
          }
        else
          {
          *file << "{}" ;
          }
        }
      }
    else
      {
      *file << "  [$pvTemp" << this->PVSource->GetVTKSourceID(0) 
            <<  " GetProperty " << this->VariableName << "] SetElements1 ";
      *file << this->Property->GetString();
      }
    }
  *file << endl;
}

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::PostAccept()
{
  if (this->CurrentIndex >= 0)
    {
    vtkPVWidget *widget = (vtkPVWidget*)this->Widgets->GetItemAsObject(this->CurrentIndex);
    widget->PostAccept();
    }
 }

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::AcceptInternal(vtkClientServerID sourceId)
{
  // Command to update the UI.
  this->Property->SetStringType(this->ElementType);

  int modFlag = this->GetModifiedFlag();
  
  const char* value = this->GetCurrentVTKValue();
  if (!value)
    {
    return;
    }

  vtkSMInputProperty *ip = vtkSMInputProperty::SafeDownCast(
    this->GetSMProperty());
  vtkPVSourceWidget *sw = vtkPVSourceWidget::SafeDownCast(
    this->Widgets->GetItemAsObject(this->CurrentIndex));
  if (ip && sw)
    {
    ip->RemoveAllProxies();
    ip->AddProxy(sw->GetSourceProxy());
    }
  else
    {
    if(this->ElementType == OBJECT)
      { 
      vtkPVObjectWidget* ow = vtkPVObjectWidget::SafeDownCast(
        this->Widgets->GetItemAsObject(this->CurrentIndex));
      if (ow)
        {
        vtkClientServerID id = 
          ow->GetObjectByName(this->GetCurrentVTKValue());
        this->Property->SetObjectID(id);
        }
      else
        {
        vtkClientServerID id = { 0 };
        this->Property->SetObjectID(id);
        }
      }
    this->Property->SetString(value);
    this->Property->SetVTKSourceID(sourceId);
    this->Property->AcceptInternal();
    }
  
  if (this->CurrentIndex >= 0)
    {
    vtkPVWidget *widget = (vtkPVWidget*)this->Widgets->GetItemAsObject(this->CurrentIndex);
    if (ip)
      {
      widget->Accept();
      }
    else
      {
      widget->AcceptInternal(sourceId);
      }
    }

  this->ModifiedFlag = 0;
  if (ip && modFlag)
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

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file) )
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetCurrentValue {"
        << this->GetCurrentValue() << "}" << endl;

  if (this->CurrentIndex >= 0)
    {
    vtkPVWidget *widget = (vtkPVWidget*)this->Widgets->GetItemAsObject(this->CurrentIndex);
    widget->Trace(file);
    }
}

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::ResetInternal()
{
  int index=-1, i;
  int num = this->Values->GetNumberOfStrings();

  if (this->AcceptCalled)
    {
    if(this->ElementType == OBJECT)
      {
      vtkClientServerID csid = this->Property->GetObjectID();
      for (i = 0; i < num; ++i)
        {
        vtkPVObjectWidget *ow = vtkPVObjectWidget::SafeDownCast(
          this->Widgets->GetItemAsObject(i));
        if (ow)
          {
          vtkClientServerID id = 
            ow->GetObjectByName(this->GetVTKValue(i));
          if (id == csid)
            {
            index = i;
            break;
            }
          }
        }
      }
    else
      {
      const char* value;
      char* currentValue;
      currentValue = new char[strlen(this->Property->GetString())+1];
      strcpy(currentValue, this->Property->GetString());
      for (i = 0; i < num; ++i)
        {
        value = this->GetVTKValue(i);
        if (value && currentValue && strcmp(value, currentValue) == 0)
          {
          index = i;
          break;
          }
        }
      delete[] currentValue;
      }
    if ( index >= 0 )
      {
      this->Menu->SetValue(this->Labels->GetString(index));
      this->SetCurrentIndex(index);
      }
    this->ModifiedFlag = 0;
    }
  else
    { 
    // The value is not set. 
    // Keep the modified flag so that accept will set the default value.
    this->ModifiedFlag = 1;
    }

  if (this->CurrentIndex >= 0)
    {
    vtkPVWidget *widget = (vtkPVWidget*)(this->Widgets->GetItemAsObject(this->CurrentIndex));
    widget->ResetInternal();
    }
}

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::SetCurrentValue(const char *val)
{
  int idx;
  
  idx = this->FindIndex(val, this->Labels);
  if (idx < 0 || idx == this->CurrentIndex)
    {
    return;
    }
  this->Menu->SetValue(val);
  this->SetCurrentIndex(idx);  
}

//-----------------------------------------------------------------------------
const char* vtkPVSelectWidget::GetCurrentValue()
{
  return this->Menu->GetValue();
}

const char* vtkPVSelectWidget::GetVTKValue(int index)
{
  if (index < 0)
    {
    return 0;
    }
  
  const char* res = this->Values->GetString(index);
  if (res)
    {
    return res;
    }
  return 0;
}

//-----------------------------------------------------------------------------
const char* vtkPVSelectWidget::GetCurrentVTKValue()
{
  return this->GetVTKValue(this->CurrentIndex);
}

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::AddItem(const char* labelVal, vtkPVWidget *pvw, 
                                const char* vtkVal)
{
  char str[512];

  this->Labels->AddString(labelVal);
  this->Widgets->AddItem(pvw);

  if (vtkVal)
    {
    this->Values->AddString(vtkVal);
    }
  else
    {
    this->Values->AddString("");
    }

  if (this->GetApplication())
    {
    this->Menu->AddEntryWithCommand(labelVal, this, "MenuCallback");
    if (this->CurrentIndex < 0)
      {
      this->Menu->SetValue(labelVal);
      this->SetCurrentIndex(0);
      }
    }

  pvw->SetTraceReferenceObject(this);
  pvw->SetTraceName(labelVal);
  this->SetTraceNameState(vtkPVWidget::UserInitialized);
  sprintf(str, "GetPVWidget {%s}", labelVal);
  pvw->SetTraceReferenceCommand(str);
}

//-----------------------------------------------------------------------------
vtkPVWidget *vtkPVSelectWidget::GetPVWidget(const char* label)
{
  int idx;

  idx = this->FindIndex(label, this->Labels);
  return (vtkPVWidget*)(this->Widgets->GetItemAsObject(idx));
}

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::MenuCallback()
{
  int idx;

  idx = this->FindIndex(this->Menu->GetValue(), this->Labels);
  if (idx < 0 )
    {
    vtkErrorMacro("Could not find value.");
    return;
    }

  this->SetCurrentIndex(idx);
}

//-----------------------------------------------------------------------------
int vtkPVSelectWidget::FindIndex(const char* str, vtkStringList *list)
{
  int idx, num;

  if (str == NULL)
    {
    vtkErrorMacro("Null value.");
    return -1;
    }

  num = list->GetNumberOfStrings();
  for (idx = 0; idx < num; ++idx)
    {
    if (strcmp(str, list->GetString(idx)) == 0)
      {
      return idx;
      }
    }
   
  vtkErrorMacro("Could not find value " << str);
  return -1;
}

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::AddAnimationScriptsToMenu(vtkKWMenu *menu, vtkPVAnimationInterfaceEntry *ai)
{
  vtkPVWidget* pv = this->GetPVWidget(this->GetCurrentValue());
  if ( pv )
    {
    pv->AddAnimationScriptsToMenu(menu, ai);
    }
}

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::SetCurrentIndex(int idx)
{
  vtkPVWidget *pvw;
  
  if (this->CurrentIndex == idx)
    {
    return;
    }

  // Unpack the old widget.
  if (this->CurrentIndex >= 0)
    {
    pvw = (vtkPVWidget*)(this->Widgets->GetItemAsObject(this->CurrentIndex));
    pvw->Deselect();
    this->Script("pack forget %s", pvw->GetWidgetName());
    }
  this->CurrentIndex = idx;
 
  // Pack the new widget.
  pvw = (vtkPVWidget*)(this->Widgets->GetItemAsObject(this->CurrentIndex));
  this->Script("pack %s -side top -fill both -expand t", pvw->GetWidgetName());
  pvw->Reset();
  pvw->Select();

  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
vtkPVSelectWidget* vtkPVSelectWidget::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVSelectWidget::SafeDownCast(clone);
}

//-----------------------------------------------------------------------------
vtkPVWidget* vtkPVSelectWidget::ClonePrototypeInternal(vtkPVSource* pvSource,
                                vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* pvWidget = 0;
  // Check if a clone of this widget has already been created
  if ( map->GetItem(this, pvWidget) != VTK_OK )
    {
    // If not, create one and add it to the map
    pvWidget = this->NewInstance();
    map->SetItem(this, pvWidget);
    // Now copy all the properties
    this->CopyProperties(pvWidget, pvSource, map);

    vtkPVSelectWidget* pvSelect = vtkPVSelectWidget::SafeDownCast(pvWidget);
    if (!pvSelect)
      {
      vtkErrorMacro("Internal error. Could not downcast pointer.");
      pvWidget->Delete();
      return 0;
      }
    
    // Now clone all the children
    int len = this->Labels->GetLength();
    const char* label;
    const char* value;
    vtkPVWidget* widget;
    vtkPVWidget* clone;
    for(int i=0; i<len; i++)
      {
      label = this->Labels->GetString(i);
      value = this->Values->GetString(i);
      widget = static_cast<vtkPVWidget*>(this->Widgets->GetItemAsObject(i));
      clone = widget->ClonePrototype(pvSource, map);
      clone->SetParent(pvSelect->GetFrame());
      pvSelect->AddItem(label, clone, value);
      clone->Delete();
      }
    }
  else
    {
    // Increment the reference count. This is necessary
    // to make the behavior same whether a widget is created
    // or returned from the map. Always call Delete() after
    // cloning.
    pvWidget->Register(this);
    }

  // note pvSelect == pvWidget
  return pvWidget;
}

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::CopyProperties(vtkPVWidget* clone, 
                                       vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVSelectWidget* pvse = vtkPVSelectWidget::SafeDownCast(clone);
  if (pvse)
    {
    pvse->SetLabel(this->EntryLabel);
    pvse->ElementType = this->ElementType;
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVSelectWidget.");
    }
}

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::Select()
{
  vtkPVWidget *pvw = (vtkPVWidget*)(this->Widgets->GetItemAsObject(this->CurrentIndex));
  if (pvw)
    {
    pvw->Select();
    }
}

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::Deselect()
{
  vtkPVWidget *pvw = (vtkPVWidget*)(this->Widgets->GetItemAsObject(this->CurrentIndex));
  if (pvw)
    {
    pvw->Deselect();
    }
}

//-----------------------------------------------------------------------------
int vtkPVSelectWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                         vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->SetLabel(label);  
    }
  else
    {
    this->SetLabel(this->VariableName);
    }
  
  const char* type = element->GetAttribute("type");
  if (type)
    {
    if(!strcmp(type, "int"))
      {
      this->ElementType = INT;
      }
    else if(!strcmp(type, "float"))
      {
      this->ElementType = FLOAT;
      }
    else if(!strcmp(type, "string"))
      {
      this->ElementType = STRING;
      }
    else if(!strcmp(type, "object"))
      {
      this->ElementType = OBJECT;
      }
    }
  else
    {
    vtkErrorMacro("Required element is missing: type");
    return 0;
    }
  
  // Extract the list of items.
  unsigned int i;
  for(i=0;i < element->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* item = element->GetNestedElement(i);
    if(strcmp(item->GetName(), "Item") != 0)
      {
      vtkErrorMacro("Found non-Item element in SelectWidget.");
      return 0;
      }
    else if(item->GetNumberOfNestedElements() != 1)
      {
      vtkErrorMacro("Item element doesn't have exactly 1 widget.");
      return 0;
      }
    const char* itemLabel = item->GetAttribute("label");
    const char* itemValue = item->GetAttribute("value");
    if(!itemLabel)
      {
      vtkErrorMacro("Item has no label.");
      return 0;
      }
    vtkPVXMLElement* we = item->GetNestedElement(0);
    vtkPVWidget* widget = this->GetPVWidgetFromParser(we, parser);
    this->AddItem(itemLabel, widget, itemValue);
    widget->Delete();
    }
  
  return 1;
}

//-----------------------------------------------------------------------------
vtkKWWidget *vtkPVSelectWidget::GetFrame() 
{
  return this->LabeledFrame->GetFrame();
}

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::SetProperty(vtkPVWidgetProperty *prop)
{
  if (this->Property == prop)
    {
    return;
    }
  
  if (this->Property)
    {
    this->Property->UnRegister(this);
    }
  this->Property = vtkPVStringWidgetProperty::SafeDownCast(prop);
  if (this->Property)
    {
    this->Property->Register(this);
    if (this->VariableName)
      {
      char *cmd = new char[strlen(this->VariableName)+4];
      sprintf(cmd, "Set%s", this->VariableName);
      this->Property->SetVTKCommand(cmd);
      delete [] cmd;
      }
    }
}

//-----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVSelectWidget::CreateAppropriateProperty()
{
  return vtkPVStringWidgetProperty::New();
}

//----------------------------------------------------------------------------
void vtkPVSelectWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->LabeledFrame);
  this->PropagateEnableState(this->Menu);

  vtkCollectionIterator* it = this->Widgets->NewIterator();
  for ( it->InitTraversal();
    !it->IsDoneWithTraversal();
    it->GoToNextItem() )
    {
    vtkPVWidget* widget = vtkPVWidget::SafeDownCast(it->GetObject());
    if (widget)
      {
      widget->SetEnabled(this->Enabled);
      }
    }
  it->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVSelectWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ElementType: " << this->ElementType << "\n";
}
