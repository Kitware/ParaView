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
#include "vtkStringList.h"
#include "vtkCollection.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"

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
  this->Widgets = vtkCollection::New();

  this->CurrentIndex = -1;
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
int vtkPVSelectWidget::Create(vtkKWApplication *app)
{
  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    return 0;
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());

  this->LabeledFrame->SetParent(this);
  this->LabeledFrame->ShowHideFrameOn();
  this->LabeledFrame->Create(app);
  this->Script("pack %s -side left", this->LabeledFrame->GetWidgetName());

  this->Menu->SetParent(this->LabeledFrame->GetFrame());
  this->Menu->Create(app, "");
  this->Script("pack %s -side left", this->Menu->GetWidgetName());

  return 1;
}


//----------------------------------------------------------------------------
void vtkPVSelectWidget::SetLabel(const char* label) 
{
  // For getting the widget in a script.
  this->SetTraceName(label);
  this->LabeledFrame->SetLabel(label);
}
  
//----------------------------------------------------------------------------
void vtkPVSelectWidget::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ModifiedFlag)
    {  
    this->AddTraceEntry("$kw(%s) SetCurrentValue {%d}", this->GetTclName(), 
                         this->GetCurrentValue());
    }

  // Command to update the UI.
  pvApp->BroadcastScript("%s Set%s {%s}",
                         this->ObjectTclName,
                         this->VariableName,
                         this->GetCurrentVTKValue()); 

  if (this->CurrentIndex >= 0)
    {
    vtkPVWidget *pvw;
    pvw = (vtkPVWidget*)(this->Widgets->GetItemAsObject(this->CurrentIndex));
    pvw->Accept();
    }

  this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVSelectWidget::Reset()
{

  this->Script("%s SetCurrentVTKValue [%s Get%s]",
               this->GetTclName(),
               this->ObjectTclName,
               this->VariableName);

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

//----------------------------------------------------------------------------
void vtkPVSelectWidget::SetCurrentVTKValue(const char *val)
{
  int idx;
  
  idx = this->FindIndex(val, this->Values);
  if (idx < 0 || idx == this->CurrentIndex)
    {
    return;
    }

  this->Menu->SetValue(this->Labels->GetString(idx));
  this->SetCurrentIndex(idx);
}

//----------------------------------------------------------------------------
const char* vtkPVSelectWidget::GetCurrentVTKValue()
{
  if (this->CurrentIndex < 0)
    {
    return NULL;
    }
  return this->Values->GetString(this->CurrentIndex);
}

//----------------------------------------------------------------------------
void vtkPVSelectWidget::AddItem(const char* labelVal, vtkPVWidget *pvw, 
                                const char* vtkVal)
{
  this->Labels->AddString(labelVal);
  this->Values->AddString(vtkVal);
  this->Widgets->AddItem(pvw);

  // So we get the modified callbacks of these widgets.
  pvw->AddDependant(this);

  this->Menu->AddEntryWithCommand(labelVal, this, "MenuCallback");
  if (this->Application && this->Menu->GetValue() == NULL)
    {
    this->Menu->SetValue(labelVal);
    this->SetCurrentIndex(0);
    }
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
  // Pack the new widget.
  pvw = (vtkPVWidget*)(this->Widgets->GetItemAsObject(this->CurrentIndex));
  this->Script("pack %s -side top -fill both -expand t", pvw->GetWidgetName());

  this->CurrentIndex = idx;

  this->ModifiedCallback();
}
