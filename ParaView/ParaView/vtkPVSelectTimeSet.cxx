/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSelectTimeSet.cxx
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
#include "vtkPVSelectTimeSet.h"

#include "vtkGenericEnSightReader.h"
#include "vtkDataArrayCollection.h"

#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWLabeledFrame.h"

#include "vtkPVApplication.h"
#include "vtkPVAnimationInterface.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSelectTimeSet);

//----------------------------------------------------------------------------
vtkPVSelectTimeSet::vtkPVSelectTimeSet()
{
  
  this->LabeledFrame = vtkKWLabeledFrame::New();
  this->LabeledFrame->SetParent(this);
  
  this->TimeLabel = vtkKWLabel::New();
  this->TimeLabel->SetParent(this->LabeledFrame->GetFrame());

  this->TreeFrame = vtkKWWidget::New();
  this->TreeFrame->SetParent(this->LabeledFrame->GetFrame());

  this->Tree = vtkKWWidget::New();
  this->Tree->SetParent(this->TreeFrame);

  this->TimeValue = 0.0;

  this->Reader = 0;

  this->FrameLabel = 0;
}

//----------------------------------------------------------------------------
vtkPVSelectTimeSet::~vtkPVSelectTimeSet()
{
  this->LabeledFrame->Delete();
  this->Tree->Delete();
  this->TreeFrame->Delete();
  this->TimeLabel->Delete();
  if (this->Reader)
    {
    this->Reader->Delete();
    }
  this->SetFrameLabel(0);
}

//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkPVSelectTimeSet, Reader, vtkGenericEnSightReader);

//----------------------------------------------------------------------------
void vtkPVSelectTimeSet::SetLabel(const char* label)
{
  this->SetFrameLabel(label);
  if (this->Application)
    {
    this->LabeledFrame->SetLabel(label);
    }
}

//----------------------------------------------------------------------------
const char* vtkPVSelectTimeSet::GetLabel()
{
  return this->GetFrameLabel();
}

//----------------------------------------------------------------------------
void vtkPVSelectTimeSet::Create(vtkKWApplication *pvApp)
{
  const char* wname;
  
  if (this->Application)
    {
    vtkErrorMacro("SelectTimeSet already created");
    return;
    }

  // For getting the widget in a script.
  this->SetTraceName("SelectTimeSet");
  
  this->SetApplication(pvApp);
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -relief flat -borderwidth 2", wname);

  this->LabeledFrame->Create(this->Application);
  if (this->FrameLabel)
    {
    this->LabeledFrame->SetLabel(this->FrameLabel);
    }
  this->TimeLabel->Create(this->Application, "");

  char label[32];
  sprintf(label, "Time value: %12.5e", 0.0);
  this->TimeLabel->SetLabel(label);
  this->Script("pack %s", this->TimeLabel->GetWidgetName());
  
  this->TreeFrame->Create(this->Application, "ScrolledWindow", 
                          "-relief sunken -borderwidth 2");

  this->Tree->Create(this->Application, "Tree", 
                     "-background white -borderwidth 0 -width 15 -padx 2 "
                     "-redraw 1 -relief flat -selectbackground red");
  this->Script("%s bindText <ButtonPress-1>  {%s SetTimeValueCallback}",
               this->Tree->GetWidgetName(), this->GetTclName());
  this->Script("%s setwidget %s", this->TreeFrame->GetWidgetName(),
               this->Tree->GetWidgetName());

  this->Script("pack %s -expand t -fill x", this->TreeFrame->GetWidgetName());

  this->Script("pack %s -side top -expand t -fill x", 
               this->LabeledFrame->GetWidgetName());

}

//----------------------------------------------------------------------------
void vtkPVSelectTimeSet::SetTimeValue(float time)
{
  if (this->TimeValue != time) 
    { 
    this->TimeValue = time; 
    
    char label[32];
    sprintf(label, "Time value: %12.5e", time);
    this->TimeLabel->SetLabel(label);
    this->Modified(); 
    } 
}

//----------------------------------------------------------------------------
void vtkPVSelectTimeSet::SetTimeValueCallback(const char* item)
{
  if ( strncmp(item, "timeset", strlen("timeset")) == 0 )
    {
    this->Script("if [%s itemcget %s -open] "
                 "{%s closetree %s} else {%s opentree %s}", 
                 this->Tree->GetWidgetName(), item,
                 this->Tree->GetWidgetName(), item,
                 this->Tree->GetWidgetName(), item);
    return;
    }

  this->Script("%s selection set %s", this->Tree->GetWidgetName(),
               item);
  this->Script("%s itemcget %s -data", this->Tree->GetWidgetName(),
               item);
  const char* result = this->Application->GetMainInterp()->result;
  if (result[0] == '\0' || !this->Reader)
    {
    return;
    }

  int index[2];
  sscanf(result, "%d %d", &(index[0]), &(index[1]));

  vtkDataArrayCollection* timeSets = this->Reader->GetTimeSets();

  this->SetTimeValue(timeSets->GetItem(index[0])->GetTuple1(index[1]));
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVSelectTimeSet::AddRootNode(const char* name, const char* text)
{
  if (!this->Application)
    {
    return;
    }
  this->Script("%s insert end root %s -text {%s}", this->Tree->GetWidgetName(),
               name, text);
}

//----------------------------------------------------------------------------
void vtkPVSelectTimeSet::AddChildNode(const char* parent, const char* name, 
                                      const char* text, const char* data)
{
  if (!this->Application)
    {
    return;
    }
  this->Script("%s insert end %s %s -text {%s} -data %s", 
               this->Tree->GetWidgetName(), parent, name, text, data);
}


//----------------------------------------------------------------------------
void vtkPVSelectTimeSet::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ModifiedFlag)
    {
    this->Script("%s selection get", this->Tree->GetWidgetName());
    this->AddTraceEntry("$kw(%s) SetTimeValueCallback {%s}", 
                        this->GetTclName(), 
                        this->Application->GetMainInterp()->result);
    }

  pvApp->BroadcastScript("%s SetTimeValue {%12.5e}",
                         this->ObjectTclName, this->GetTimeValue());

  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVSelectTimeSet::Reset()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }

  // Command to update the UI.
  if (!this->Tree)
    {
    return;
    }

  this->Script("%s delete [%s nodes root]", this->Tree->GetWidgetName(),
               this->Tree->GetWidgetName());
  
  if (!this->Reader)
    {
    return;
    }

  vtkDataArrayCollection* timeSets = this->Reader->GetTimeSets();
  if (!timeSets)
    {
    vtkErrorMacro("The reader does not contain any time sets.");
    return;
    }

  int timeSetId=0;
  char timeSetName[32];
  char timeSetText[32];

  char timeValueName[32];
  char timeValueText[32];
  char indices[32];

  float actualTimeValue = this->Reader->GetTimeValue();
  int matchFound = 0;

  timeSets->InitTraversal();
  vtkDataArray* da;
  while( (da=timeSets->GetNextItem()) )
    {
    timeSetId++;
    sprintf(timeSetName,"timeset%d", timeSetId);
    sprintf(timeSetText,"Time Set %d", timeSetId); 
    this->AddRootNode(timeSetName, timeSetText);

    vtkIdType tuple;
    for(tuple=0; tuple<da->GetNumberOfTuples(); tuple++)
      {
      float timeValue = da->GetTuple1(tuple);
      sprintf(timeValueName, "time%d_%-12.5e", timeSetId, timeValue);
      sprintf(timeValueText, "%-12.5e", timeValue);
      ostrstream str;
      str << "{" << timeSetId-1 << " " << tuple << "}" << ends;
      sprintf(indices, "%s", str.str());
      str.rdbuf()->freeze(0);
      this->AddChildNode(timeSetName, timeValueName, timeValueText, indices);
      if (actualTimeValue == timeValue && !matchFound)
        {
        matchFound=1;
        this->Script("%s selection set %s", this->Tree->GetWidgetName(),
                     timeValueName);
        }
      }
    if (timeSetId == 1)
      {
      this->Script("%s opentree %s", this->Tree->GetWidgetName(), 
                   timeSetName);
      }
    }

  this->SetTimeValue(actualTimeValue);
  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVSelectTimeSet::AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                                   vtkPVAnimationInterface *ai)
{
  char methodAndArgs[500];

  sprintf(methodAndArgs, "SetLabelAndScript {%s} {%s SetTimeValue $pvTime}", 
          this->GetTraceName(), this->PVSource->GetVTKSourceTclName());

  menu->AddCommand(this->GetTraceName(), ai, methodAndArgs, 0, "");
}

//----------------------------------------------------------------------------
vtkPVSelectTimeSet* vtkPVSelectTimeSet::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVSelectTimeSet::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVSelectTimeSet::CopyProperties(vtkPVWidget* clone, 
                                      vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVSelectTimeSet* pvts = vtkPVSelectTimeSet::SafeDownCast(clone);
  if (pvts)
    {
    pvts->SetLabel(this->FrameLabel);
    }
  else 
    {
    vtkErrorMacro(
      "Internal error. Could not downcast clone to PVSelectTimeSet.");
    }
}

//----------------------------------------------------------------------------
int vtkPVSelectTimeSet::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->SetLabel(label);
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVSelectTimeSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TimeValue: " << this->TimeValue << endl;
  os << indent << "LabeledFrame: " << this->LabeledFrame << endl;
  os << indent << "Reader: " << this->Reader << endl;
}
