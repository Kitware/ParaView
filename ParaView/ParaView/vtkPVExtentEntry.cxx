/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVExtentEntry.cxx
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
#include "vtkPVExtentEntry.h"

#include "vtkArrayMap.txx"
#include "vtkKWEntry.h"
#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterface.h"
#include "vtkPVData.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkPVExtentEntry* vtkPVExtentEntry::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVExtentEntry");
  if (ret)
    {
    return (vtkPVExtentEntry*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVExtentEntry;
}

//---------------------------------------------------------------------------
vtkPVExtentEntry::vtkPVExtentEntry()
{
  this->LabelWidget = vtkKWLabel::New();
  this->LabelWidget->SetParent(this);
  this->EntryLabel = 0;
  this->XMinEntry = vtkKWEntry::New();
  this->XMaxEntry = vtkKWEntry::New();
  this->YMinEntry = vtkKWEntry::New();
  this->YMaxEntry = vtkKWEntry::New();
  this->ZMinEntry = vtkKWEntry::New();
  this->ZMaxEntry = vtkKWEntry::New();
}

//---------------------------------------------------------------------------
vtkPVExtentEntry::~vtkPVExtentEntry()
{
  this->XMinEntry->Delete();
  this->XMinEntry = NULL;
  this->XMaxEntry->Delete();
  this->XMaxEntry = NULL;
  this->YMinEntry->Delete();
  this->YMinEntry = NULL;
  this->YMaxEntry->Delete();
  this->YMaxEntry = NULL;
  this->ZMinEntry->Delete();
  this->ZMinEntry = NULL;
  this->ZMaxEntry->Delete();
  this->ZMaxEntry = NULL;

  this->LabelWidget->Delete();
  this->LabelWidget = NULL;
  this->SetEntryLabel(0);
}

void vtkPVExtentEntry::SetLabel(const char* label)
{
  this->SetEntryLabel(label);
  this->LabelWidget->SetLabel(label);
}

//---------------------------------------------------------------------------
void vtkPVExtentEntry::Create(vtkKWApplication *pvApp)
{
  const char* wname;
  
  if (this->Application)
    {
    vtkErrorMacro("VectorEntry already created");
    return;
    }
  
  // For getting the widget in a script.
  this->SetTraceName(this->EntryLabel);

  this->SetApplication(pvApp);

  // Initialize the extent of the VTK source. Normally, it
  // is set to -VTK_LARGE_FLOAT, VTK_LARGE_FLOAT...
  if (this->PVSource && this->PVSource->GetPVInput() && this->VariableName )
    {
    this->Script("eval %s Set%s [%s GetWholeExtent]",
		 this->PVSource->GetVTKSourceTclName(), 
		 this->GetVariableName(),
		 this->PVSource->GetPVInput()->GetVTKDataTclName());
    }

  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);

  // Now a label
  if (this->EntryLabel && this->EntryLabel[0] != '\0')
    {
    this->LabelWidget->Create(pvApp, "-width 18 -justify right");
    this->LabelWidget->SetLabel(this->EntryLabel);
    this->Script("pack %s -side left", this->LabelWidget->GetWidgetName());
    }
    
  // Now the entries
  this->XMinEntry->SetParent(this);
  this->XMinEntry->Create(pvApp, "-width 2");
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->XMinEntry->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill x -expand t",
               this->XMinEntry->GetWidgetName());

  this->XMaxEntry->SetParent(this);
  this->XMaxEntry->Create(pvApp, "-width 2");
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->XMaxEntry->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill x -expand t",
               this->XMaxEntry->GetWidgetName());

  this->YMinEntry->SetParent(this);
  this->YMinEntry->Create(pvApp, "-width 2");
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->YMinEntry->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill x -expand t",
               this->YMinEntry->GetWidgetName());

  this->YMaxEntry->SetParent(this);
  this->YMaxEntry->Create(pvApp, "-width 2");
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->YMaxEntry->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill x -expand t",
               this->YMaxEntry->GetWidgetName());

  this->ZMinEntry->SetParent(this);
  this->ZMinEntry->Create(pvApp, "-width 2");
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->ZMinEntry->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill x -expand t",
               this->ZMinEntry->GetWidgetName());

  this->ZMaxEntry->SetParent(this);
  this->ZMaxEntry->Create(pvApp, "-width 2");
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->ZMaxEntry->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill x -expand t",
               this->ZMaxEntry->GetWidgetName());
}


//---------------------------------------------------------------------------
void vtkPVExtentEntry::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  ofstream *traceFile = pvApp->GetTraceFile();
  int traceFlag = 0;

  if ( ! this->ModifiedFlag)
    {
    return;
    }

  // Start the trace entry and the accept command.
  if (traceFile && this->InitializeTrace())
    {
    traceFlag = 1;
    }

  if (traceFlag)
    {
    *traceFile << "$kw(" << this->GetTclName() << ") SetValue " 
               << this->XMinEntry->GetValueAsInt() << " "
               << this->XMaxEntry->GetValueAsInt() << " "
               << this->YMinEntry->GetValueAsInt() << " "
               << this->YMaxEntry->GetValueAsInt() << " "
               << this->ZMinEntry->GetValueAsInt() << " "
               << this->ZMaxEntry->GetValueAsInt() << endl;
    }

  pvApp->BroadcastScript("%s Set%s %d %d %d %d %d %d", this->ObjectTclName, this->VariableName,
          this->XMinEntry->GetValueAsInt(), this->XMaxEntry->GetValueAsInt(),
          this->YMinEntry->GetValueAsInt(), this->YMaxEntry->GetValueAsInt(),
          this->ZMinEntry->GetValueAsInt(), this->ZMaxEntry->GetValueAsInt());

  this->ModifiedFlag = 0;  
}

//---------------------------------------------------------------------------
void vtkPVExtentEntry::Reset()
{
  //int count = 0;

  if ( ! this->ModifiedFlag)
    {
    return;
    }

  this->Script("eval %s SetValue [%s Get%s]",
               this->GetTclName(), this->ObjectTclName, this->VariableName);

  this->ModifiedFlag = 0;
}


//---------------------------------------------------------------------------
void vtkPVExtentEntry::SetValue(int v0, int v1, int v2, 
                                int v3, int v4, int v5)
{
  this->XMinEntry->SetValue(v0);
  this->XMaxEntry->SetValue(v1);
  this->YMinEntry->SetValue(v2);
  this->YMaxEntry->SetValue(v3);
  this->ZMinEntry->SetValue(v4);
  this->ZMaxEntry->SetValue(v5);

  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVExtentEntry::AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                                 vtkPVAnimationInterface *ai)
{
  vtkKWMenu *cascadeMenu;
  char methodAndArgs[200];

  // Lets create a cascade menu to keep things neat.
  cascadeMenu = vtkKWMenu::New();
  cascadeMenu->SetParent(menu);
  cascadeMenu->Create(this->Application, "-tearoff 0");
  menu->AddCascade(this->GetTraceName(), cascadeMenu, 0,
		             "Choose a plane of the extent to animate.");  
  // X
  sprintf(methodAndArgs, "AnimationMenuCallback %s 0", ai->GetTclName());
  cascadeMenu->AddCommand("X Axis", this, methodAndArgs, 0, "");
  // Y
  sprintf(methodAndArgs, "AnimationMenuCallback %s 1", ai->GetTclName());
  cascadeMenu->AddCommand("Y Axis", this, methodAndArgs, 0, "");
  // Z
  sprintf(methodAndArgs, "AnimationMenuCallback %s 2", ai->GetTclName());
  cascadeMenu->AddCommand("Z Axis", this, methodAndArgs, 0, "");

  cascadeMenu->Delete();
  cascadeMenu = NULL;

  return;
}

//----------------------------------------------------------------------------
void vtkPVExtentEntry::AnimationMenuCallback(vtkPVAnimationInterface *ai,
                                             int mode)
{
  char script[500];
  int ext[6];

  // Get the whole extent to set up defaults.
  // Now I can imagine that we will need a more flexible way of getting 
  // the whole extent from sources (in the future.
  this->Script("[%s GetInput] GetWholeExtent", this->ObjectTclName);
  sscanf(this->Application->GetMainInterp()->result, "%d %d %d %d %d %d",
         ext, ext+1, ext+2, ext+3, ext+4, ext+5);

  if (mode == 0)
    {
    sprintf(script, "%s Set%s [expr int($pvTime)] [expr int($pvTime)] %d %d %d %d", 
            this->ObjectTclName,this->VariableName,ext[2],ext[3],ext[4],ext[5]);
    ai->SetLabelAndScript("X Axis", script);
    ai->SetTimeStart(ext[0]);
    ai->SetCurrentTime(ext[0]);
    ai->SetTimeEnd(ext[1]);
    ai->SetTimeStep(1.0);
    }
  else if (mode == 1)
    {
    sprintf(script, "%s Set%s %d %d [expr int($pvTime)] [expr int($pvTime)] %d %d", 
            this->ObjectTclName,this->VariableName,ext[0],ext[1],ext[4],ext[5]);
    ai->SetLabelAndScript("Y Axis", script);
    ai->SetTimeStart(ext[2]);
    ai->SetCurrentTime(ext[2]);
    ai->SetTimeEnd(ext[3]);
    ai->SetTimeStep(1.0);
    }
  else if (mode == 2)
    {
    sprintf(script, "%s Set%s %d %d %d %d [expr int($pvTime)] [expr int($pvTime)]", 
            this->ObjectTclName,this->VariableName,ext[0],ext[1],ext[2],ext[3]);
    ai->SetLabelAndScript("Z Axis", script);
    ai->SetTimeStart(ext[4]);
    ai->SetCurrentTime(ext[4]);
    ai->SetTimeEnd(ext[5]);
    ai->SetTimeStep(1.0);
    }
  else
    {
    vtkErrorMacro("Bad extent animation mode.");
    }
  ai->SetControlledWidget(this);
}

vtkPVExtentEntry* vtkPVExtentEntry::ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVExtentEntry::SafeDownCast(clone);
}

void vtkPVExtentEntry::CopyProperties(vtkPVWidget* clone, 
				      vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVExtentEntry* pvee = vtkPVExtentEntry::SafeDownCast(clone);
  if (pvee)
    {
    pvee->SetLabel(this->EntryLabel);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVExtentEntry.");
    }
}

//----------------------------------------------------------------------------
int vtkPVExtentEntry::ReadXMLAttributes(vtkPVXMLElement* element,
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
  
  return 1;
}
