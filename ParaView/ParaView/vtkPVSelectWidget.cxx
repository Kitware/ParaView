/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectWidget.cxx
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
#include "vtkPVSelectWidget.h"
#include "vtkPVApplication.h"
#include "vtkStringList.h"
#include "vtkPVWidgetCollection.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkArrayMap.txx"
#include "vtkPVXMLElement.h"

int vtkPVSelectWidgetCommand(ClientData cd, Tcl_Interp *interp,
		     int argc, char *argv[]);


//----------------------------------------------------------------------------
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
  this->UseWidgetCommand = 0;
}

//----------------------------------------------------------------------------
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
}

//----------------------------------------------------------------------------
vtkPVSelectWidget* vtkPVSelectWidget::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVSelectWidget");
  if(ret)
    {
    return (vtkPVSelectWidget*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVSelectWidget;
}

//----------------------------------------------------------------------------
void vtkPVSelectWidget::Create(vtkKWApplication *app)
{
  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());

  this->LabeledFrame->SetParent(this);
  this->LabeledFrame->ShowHideFrameOn();
  this->LabeledFrame->Create(app);
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
  for(int i=0; i<len; i++)
    {
    widget = static_cast<vtkPVWidget*>(this->Widgets->GetItemAsObject(i));
    if (!widget->GetApplication())
      {
      widget->Create(this->Application);
      }
    }

  len = this->Labels->GetLength();
  char* label;
  for(int i=0; i<len; i++)
    {
    label = this->Labels->GetString(i);
    this->Menu->AddEntryWithCommand(label, this, "MenuCallback");
    }
  if (len > 0 && this->CurrentIndex < 0)
    {
    this->Menu->SetValue(this->Labels->GetString(0));
    this->SetCurrentIndex(0);
    }

}


//----------------------------------------------------------------------------
void vtkPVSelectWidget::SetLabel(const char* label) 
{
  // For getting the widget in a script.
  this->SetTraceName(label);
  this->SetEntryLabel(label);
  if (this->Application)
    {
    this->LabeledFrame->SetLabel(label);
    }
}
  
//----------------------------------------------------------------------------
void vtkPVSelectWidget::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ModifiedFlag)
    {  
    this->AddTraceEntry("$kw(%s) SetCurrentValue {%s}", this->GetTclName(), 
                         this->GetCurrentValue());
    }

  // Command to update the UI.
  if (this->GetCurrentVTKValue())
    {
    pvApp->BroadcastScript("%s Set%s {%s}",
			   this->ObjectTclName,
			   this->VariableName,
			   this->GetCurrentVTKValue()); 
    }

  if (this->CurrentIndex >= 0)
    {
    vtkPVWidget *pvw;
    pvw = (vtkPVWidget*)this->Widgets->GetItemAsObject(this->CurrentIndex);
    pvw->Accept();
    }

  this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVSelectWidget::Reset()
{
  int index=-1, i;
  int num = this->Values->GetNumberOfStrings();
  const char* value;
  const char* tmp;
  char* currentValue;

  this->Script("%s Get%s",this->ObjectTclName,this->VariableName);
  tmp = Tcl_GetStringResult(this->Application->GetMainInterp());
  if (tmp && strlen(tmp) > 0)
    {
    currentValue = new char[strlen(tmp)+1];
    strcpy(currentValue, tmp);
    for (i = 0; i < num; ++i)
      {
      value = this->GetVTKValue(i);
      if (value && currentValue && strcmp(value, currentValue) == 0)
	{
	index = i;
	break;
	}
      }
    if ( index >= 0 )
      {
      this->Menu->SetValue(this->Labels->GetString(index));
      this->SetCurrentIndex(index);
      }
    delete[] currentValue;
    }

  if (this->CurrentIndex >= 0)
    {
    vtkPVWidget *pvw;
    pvw = (vtkPVWidget*)(this->Widgets->GetItemAsObject(this->CurrentIndex));
    pvw->Reset();
    }

  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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
  if ( this->UseWidgetCommand )
    {
    vtkPVWidget* pvw = (vtkPVWidget*)(this->Widgets->GetItemAsObject(index));
    if (pvw)
      {
      if (res && res[0] != '\0')
	{
	this->Script("%s %s", pvw->GetTclName(), res);
	return Tcl_GetStringResult(this->Application->GetMainInterp());
	}
      else
	{
	return 0;
	}
      }
    }
  else
    {
    if (res && res[0] != '\0')
      {
      return this->Values->GetString(index);
      }
    else
      {
      return 0;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
const char* vtkPVSelectWidget::GetCurrentVTKValue()
{
  return this->GetVTKValue(this->CurrentIndex);
}

//----------------------------------------------------------------------------
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

  if (this->Application)
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
  sprintf(str, "GetPVWidget {%s}", labelVal);
  pvw->SetTraceReferenceCommand(str);
}

//----------------------------------------------------------------------------
vtkPVWidget *vtkPVSelectWidget::GetPVWidget(const char* label)
{
  int idx;

  idx = this->FindIndex(label, this->Labels);
  return (vtkPVWidget*)(this->Widgets->GetItemAsObject(idx));
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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
    this->Script("pack forget %s", pvw->GetWidgetName());
    }
  this->CurrentIndex = idx;
 
  // Pack the new widget.
  pvw = (vtkPVWidget*)(this->Widgets->GetItemAsObject(this->CurrentIndex));
  this->Script("pack %s -side top -fill both -expand t", pvw->GetWidgetName());
  pvw->Reset();

  this->ModifiedCallback();
}



//----------------------------------------------------------------------------
void vtkPVSelectWidget::SaveInTclScript(ofstream *file)
{
  const char* vtkValue;
  vtkPVWidget *pvw;
  pvw = (vtkPVWidget*)(this->Widgets->GetItemAsObject(this->CurrentIndex));
  pvw->SaveInTclScript(file);

  // The plane and sphere widgets could set this themselves. 
  // If they were on their own they would.
  
  vtkValue = this->GetCurrentVTKValue();
  *file << "\t" << this->ObjectTclName << " Set" << this->VariableName
        << " {" << vtkValue << "}" << endl;
}

vtkPVSelectWidget* vtkPVSelectWidget::ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVSelectWidget::SafeDownCast(clone);
}

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
    char* label;
    char* value;
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

void vtkPVSelectWidget::CopyProperties(vtkPVWidget* clone, 
				       vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVSelectWidget* pvse = vtkPVSelectWidget::SafeDownCast(clone);
  if (pvse)
    {
    pvse->SetLabel(this->EntryLabel);
    pvse->SetUseWidgetCommand(this->UseWidgetCommand);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVSelectWidget.");
    }
}

//----------------------------------------------------------------------------
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
  
  // Setup the UseWidgetCommand.
  if(!element->GetScalarAttribute("use_widget_command",
                                  &this->UseWidgetCommand))
    {
    this->UseWidgetCommand = 0;
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
