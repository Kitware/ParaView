/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVVectorEntry.cxx
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
#include "vtkPVVectorEntry.h"
#include "vtkObjectFactory.h"
#include "vtkKWEntry.h"

//----------------------------------------------------------------------------
vtkPVVectorEntry* vtkPVVectorEntry::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVVectorEntry");
  if (ret)
    {
    return (vtkPVVectorEntry*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVVectorEntry;
}

vtkPVVectorEntry::vtkPVVectorEntry()
{
  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this);
  this->Entries = vtkKWWidgetCollection::New();
  this->SubLabels = vtkKWWidgetCollection::New();

  this->PVSource = NULL;
}

vtkPVVectorEntry::~vtkPVVectorEntry()
{
  this->Entries->Delete();
  this->Entries = NULL;
  this->SubLabels->Delete();
  this->SubLabels = NULL;
  this->Label->Delete();
  this->Label = NULL;
}

void vtkPVVectorEntry::Create(vtkKWApplication *pvApp, char *label,
                              int vectorLength, char **subLabels,
                              char *setCmd, char *getCmd,
                              char *help, const char *tclName)
{
  const char* wname;
  int i;
  vtkKWEntry* entry;
  vtkKWLabel* subLabel;
  char acceptCmd[1024];
  
  if (this->Application)
    {
    vtkErrorMacro("VectorEntry already created");
    return;
    }
  if ( ! this->PVSource)
    {
    vtkErrorMacro("PVSource must be set before calling Create");
    return;
    }
  
  // For getting the widget in a script.
  this->SetName(label);

  this->SetApplication(pvApp);
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);
  
  // Now a label
  if (label && label[0] != '\0')
    {
    this->Label->Create(pvApp, "-width 18 -justify right");
    this->Label->SetLabel(label);
    if (help)
      {
      this->Label->SetBalloonHelpString(help);
      }
    this->Script("pack %s -side left", this->Label->GetWidgetName());
    }
  
  //sprintf(acceptCmd, "%s AcceptHelper2 %s %s \"",
  //        this->PVSource->GetTclName(), tclName, setCmd);
  sprintf(acceptCmd, "eval %s %s ", tclName, setCmd);
  
  // Now the sublabels and entries
  for (i = 0; i < vectorLength; i++)
    {
    if (subLabels && subLabels[i] && subLabels[i][0] != '\0')
      {
      subLabel = vtkKWLabel::New();
      subLabel->SetParent(this);
      subLabel->Create(pvApp, "");
      subLabel->SetLabel(subLabels[i]);
      this->Script("pack %s -side left", subLabel->GetWidgetName());
      this->SubLabels->AddItem(subLabel);
      subLabel->Delete();
      }
    
    entry = vtkKWEntry::New();
    entry->SetParent(this);
    entry->Create(pvApp, "-width 2");
    this->Script("%s configure -xscrollcommand {%s XScrollCallback}",
                 entry->GetWidgetName(), this->GetTclName());
    if (help)
      { 
      entry->SetBalloonHelpString(help);
      }
    this->Script("pack %s -side left -fill x -expand t",
                 entry->GetWidgetName());
    this->Entries->AddItem(entry);
    
    this->ResetCommands->AddString("%s SetValue [lindex [%s %s] %d]",
                                   entry->GetTclName(), tclName, getCmd, i);
    strcat(acceptCmd, "[");
    strcat(acceptCmd, entry->GetTclName());
    strcat(acceptCmd, " GetValue]");
    if (i < vectorLength-1)
      {
      strcat(acceptCmd, " ");
      }
    entry->Delete();
    }
  
  strcat(acceptCmd, "");
  this->AcceptCommands->AddString(acceptCmd);
}

void vtkPVVectorEntry::XScrollCallback(float x, float y)
{
  this->ModifiedCallback();
}

void vtkPVVectorEntry::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  ofstream *traceFile = pvApp->GetTraceFile();

  if (this->ModifiedFlag && this->PVSource && traceFile)
    {
    vtkKWEntry *entry;
    int num, idx;
  
    if ( ! this->TraceInitialized)
      {
      pvApp->AddTraceEntry("set pv(%s) [$pv(%s) GetPVWidget {%s}]",
                           this->GetTclName(), this->PVSource->GetTclName(),
                           this->Name);
      this->TraceInitialized = 1;
      }

    *traceFile << "$pv(" << this->GetTclName() << ") SetValue";
    num = this->Entries->GetNumberOfItems();
    for (idx = 0; idx < num; ++idx)
      {
      entry = this->GetEntry(idx);
      *traceFile << " " << entry->GetValue();
      }
    *traceFile << endl;
    }

  this->vtkPVWidget::Accept();
}

vtkKWLabel* vtkPVVectorEntry::GetSubLabel(int idx)
{
  if (idx > this->SubLabels->GetNumberOfItems())
    {
    return NULL;
    }
  return ((vtkKWLabel*)this->SubLabels->GetItemAsObject(idx));
}

vtkKWEntry* vtkPVVectorEntry::GetEntry(int idx)
{
  if (idx > this->Entries->GetNumberOfItems())
    {
    return NULL;
    }
  return ((vtkKWEntry*)this->Entries->GetItemAsObject(idx));
}


void vtkPVVectorEntry::SetValue(char** values, int num)
{
  int idx;
  vtkKWEntry *entry;

  if (num != this->Entries->GetNumberOfItems())
    {
    vtkErrorMacro("Componenet mismatch.");
    return;
    }
  for (idx = 0; idx < num; ++idx)
    {
    entry = this->GetEntry(idx);
    entry->SetValue(values[idx]);
    }
  this->ModifiedCallback();
}


void vtkPVVectorEntry::SetValue(char *v0)
{
  char* vals[1];
  vals[0] = v0;
  this->SetValue(vals, 1);
}

void vtkPVVectorEntry::SetValue(char *v0, char *v1)
{
  char* vals[2];
  vals[0] = v0;
  vals[1] = v1;
  this->SetValue(vals, 2);
}

void vtkPVVectorEntry::SetValue(char *v0, char *v1, char *v2)
{
  char* vals[3];
  vals[0] = v0;
  vals[1] = v1;
  vals[2] = v2;
  this->SetValue(vals, 3);
}

void vtkPVVectorEntry::SetValue(char *v0, char *v1, char *v2, char *v3)
{
  char* vals[4];
  vals[0] = v0;
  vals[1] = v1;
  vals[2] = v2;
  vals[3] = v3;
  this->SetValue(vals, 4);
}

void vtkPVVectorEntry::SetValue(char *v0, char *v1, char *v2, char *v3, char *v4)
{
  char* vals[5];
  vals[0] = v0;
  vals[1] = v1;
  vals[2] = v2;
  vals[3] = v3;
  vals[4] = v4;
  this->SetValue(vals, 5);
}

void vtkPVVectorEntry::SetValue(char *v0, char *v1, char *v2, 
                                char *v3, char *v4, char *v5)
{
  char* vals[6];
  vals[0] = v0;
  vals[1] = v1;
  vals[2] = v2;
  vals[3] = v3;
  vals[4] = v4;
  vals[5] = v5;
  this->SetValue(vals, 6);
}

