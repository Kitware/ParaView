/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGlyph3D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/

#include "vtkPVGlyph3D.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPVPolyData.h"
#include "vtkPVActorComposite.h"
#include "vtkPVConeSource.h"
#include "vtkPVAssignment.h"

int vtkPVGlyph3DCommand(ClientData cd, Tcl_Interp *interp,
			int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVGlyph3D::vtkPVGlyph3D()
{
  this->CommandFunction = vtkPVGlyph3DCommand;
  
  this->GlyphSourceButton = vtkKWPushButton::New();
  this->GlyphSourceButton->SetParent(this->Properties);
  this->ScaleFactorEntry = vtkKWLabeledEntry::New();
  this->ScaleFactorEntry->SetParent(this->Properties);
  this->SourceButton = vtkKWPushButton::New();
  this->SourceButton->SetParent(this->Properties);
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);
  
  this->Glyph = vtkGlyph3D::New();

  
  this->Widgets = vtkKWWidgetCollection::New();
  
  this->NumberOfAcceptCommands = 0;
  this->AcceptCommandArrayLength = 0;
  this->AcceptCommands = NULL;

}

//----------------------------------------------------------------------------
vtkPVGlyph3D::~vtkPVGlyph3D()
{
  this->GlyphSourceButton->Delete();
  this->GlyphSourceButton = NULL;
  this->ScaleFactorEntry->Delete();
  this->ScaleFactorEntry = NULL;
  this->SourceButton->Delete();
  this->SourceButton = NULL;
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->Glyph->Delete();
  this->Glyph = NULL;

  this->Widgets->Delete();
  this->Widgets = NULL;

  this->DeleteAcceptCommands();
}

//----------------------------------------------------------------------------
vtkPVGlyph3D* vtkPVGlyph3D::New()
{
  return new vtkPVGlyph3D();
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::CreateProperties()
{
  // must set the application
  this->vtkPVSource::CreateProperties();
  
  this->GlyphSourceButton->Create(this->Application, "-text GetGlyphSource");
  this->GlyphSourceButton->SetCommand(this, "ShowGlyphSource");
  
  this->ScaleFactorEntry->Create(this->Application);
  this->ScaleFactorEntry->SetValue(1, 2);
  this->ScaleFactorEntry->SetLabel("Scale Factor:");
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "ScaleFactorChanged");
  this->SourceButton->Create(this->Application, "-text GetSource");
  this->SourceButton->SetCommand(this, "GetSource");
  
  this->Script("pack %s %s %s %s",
	       this->SourceButton->GetWidgetName(),
	       this->GlyphSourceButton->GetWidgetName(),
	       this->Accept->GetWidgetName(),
	       this->ScaleFactorEntry->GetWidgetName());

  
  
  // Try to create a widget without instance variables.
  vtkKWLabeledEntry *entry = vtkKWLabeledEntry::New();
  this->Widgets->AddItem(entry);
  entry->SetParent(this->Properties);
  entry->Create(this->Application);
  entry->SetLabel("Scale Factor:");
  // Initialize (value 1, with size 2).
  entry->SetValue(1.0, 2);
  this->Script("pack %s", entry->GetWidgetName());
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects are going to have to have the same Tcl name!
  this->AddAcceptCommand("%s AcceptHelper SetScaleFactor [%s GetValueAsFloat]",
                          this->GetTclName(), entry->GetTclName()); 
  entry->Delete();
  

}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::AcceptHelper(char *method, char *args)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  pvApp->Script("[%s GetPolyDataSource] %s %s", this->GetTclName(),
                method, args);
  pvApp->BroadcastScript("[%s GetPolyDataSource] %s %s", this->GetTclName(),
                         method, args);
}


//----------------------------------------------------------------------------
void vtkPVGlyph3D::SetInput(vtkPVData *pvData)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetInput %s", this->GetTclName(),
			   pvData->GetTclName());
    }  
  
  this->GetGlyph()->SetInput(pvData->GetData());
  this->Input = pvData;
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::SetSource(vtkPVPolyData *pvData)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetSource %s", this->GetTclName(),
			   pvData->GetTclName());
    }  
  
  this->GetGlyph()->SetSource(pvData->GetPolyData());
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::SetOutput(vtkPVPolyData *pvd)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutput %s", this->GetTclName(),
			   pvd->GetTclName());
    }  
  
  this->SetPVData(pvd);  
  pvd->SetData(this->Glyph->GetOutput());
}

//----------------------------------------------------------------------------
vtkPVPolyData *vtkPVGlyph3D::GetOutput()
{
  return vtkPVPolyData::SafeDownCast(this->Output);
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::SetGlyphSource(vtkPVSource *comp)
{
  this->GlyphSource = comp;
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::SetScaleModeToDataScalingOff()  
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetScaleModeToDataScalingOff", 
			   this->GetTclName());
    }
  
  this->GetGlyph()->SetScaleModeToDataScalingOff();
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::SetScaleFactor(float factor)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetScaleFactor %f", this->GetTclName(),
			   factor);
    }  
  
  this->GetGlyph()->SetScaleFactor(factor);
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::ShowGlyphSource()
{
  vtkPVWindow *window = 
		vtkPVWindow::SafeDownCast(this->GetView()->GetParentWindow());
  
  this->GetPVData()->GetActorComposite()->VisibilityOff();
  window->SetCurrentSource(this->GlyphSource);
  this->GlyphSource->GetPVData()->GetActorComposite()->VisibilityOn();
  this->GlyphSource->GetView()->Render();
  window->GetSourceList()->Update();
  
  window->GetMainView()->ResetCamera();
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void StartGlyphProgress(void *arg)
{
  vtkPVGlyph3D *me = (vtkPVGlyph3D*)arg;
  me->GetWindow()->SetStatusText("Processing Glyph");
}

//----------------------------------------------------------------------------
void GlyphProgress(void *arg)
{
  vtkPVGlyph3D *me = (vtkPVGlyph3D*)arg;
  me->GetWindow()->GetProgressGauge()->SetValue((int)(me->GetGlyph()->GetProgress() * 100));
}

//----------------------------------------------------------------------------
void EndGlyphProgress(void *arg)
{
  vtkPVGlyph3D *me = (vtkPVGlyph3D*)arg;
  me->GetWindow()->SetStatusText("");
  me->GetWindow()->GetProgressGauge()->SetValue(0);
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::ScaleFactorChanged()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPolyData *pvd;
  vtkPVAssignment *a;
  vtkPVWindow *window = this->GetWindow();
  vtkPVActorComposite *ac;
  
  this->SetScaleFactor(this->ScaleFactorEntry->GetValueAsFloat());
  
  if (this->GetPVData() == NULL)
    { // This is the first time.  Create the data.
    vtkPVConeSource *cone;
    vtkPVPolyData *coneOut;
    vtkPVAssignment *coneAssignment;

    this->GetGlyph()->SetStartMethod(StartGlyphProgress, this);
    this->GetGlyph()->SetProgressMethod(GlyphProgress, this);
    this->GetGlyph()->SetEndMethod(EndGlyphProgress, this);
    
    // Here we are creating our own glyph source.
    // In the future this may be set in the UI.
    cone = vtkPVConeSource::New();
    cone->Clone(pvApp);
    cone->SetName("glyph cone");
    this->SetGlyphSource(cone);
    window->GetMainView()->AddComposite(cone);
    window->SetCurrentSource(cone);
    // Accept
    coneOut = vtkPVPolyData::New();
    coneOut->Clone(pvApp);
    coneAssignment = vtkPVAssignment::New();
    coneAssignment->Clone(pvApp);
    coneOut->SetAssignment(coneAssignment);
    cone->SetOutput(coneOut);
    cone->CreateDataPage();
    window->GetMainView()->AddComposite(coneOut->GetActorComposite());
    coneOut->GetActorComposite()->VisibilityOff();
    cone->Delete();
    cone = NULL;
    this->SetSource(coneOut);
    coneOut->Delete();
    coneOut = NULL;
    coneAssignment->Delete();
    coneAssignment = NULL;

    // Create our own output.
    pvd = vtkPVPolyData::New();
    pvd->Clone(pvApp);
    this->SetOutput(pvd);
    a = this->GetInput()->GetAssignment();
    pvd->SetAssignment(a);
    this->GetInput()->GetActorComposite()->VisibilityOff();
    this->CreateDataPage();
    ac = this->GetPVData()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);
    window->GetSourceList()->Update();
    }
  
  window->GetMainView()->SetSelectedComposite(this);
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::GetSource()
{
  this->GetPVData()->GetActorComposite()->VisibilityOff();
  this->GetWindow()->GetMainView()->
    SetSelectedComposite(this->GetInput()->GetPVSource());
  this->GetInput()->GetActorComposite()->VisibilityOn();
  this->GetView()->Render();
  this->GetWindow()->GetMainView()->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::AddAcceptCommand(const char *format, ...)
{
  static char event[16000];

  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  // Check to see if we need to extent to array of commands.
  if (this->AcceptCommandArrayLength <= this->NumberOfAcceptCommands)
    { // Yes.
    int i;
    // Allocate a new array
    this->AcceptCommandArrayLength += 20;
    char **tmp = new char* [this->AcceptCommandArrayLength];
    // Copy array elements.
    for (i = 0; i < this->NumberOfAcceptCommands; ++i)
      {
      tmp[i] = this->AcceptCommands[i];
      }
    // Delete the old array.
    if (this->AcceptCommands)
      {
      delete [] this->AcceptCommands;
      this->AcceptCommands = NULL;
      }
    // Set the new array.
    this->AcceptCommands = tmp;
    tmp = NULL;
    }

  // Allocate the string for and set the new command.
  this->AcceptCommands[this->NumberOfAcceptCommands] 
              = new char[strlen(event) + 2];
  strcpy(this->AcceptCommands[this->NumberOfAcceptCommands], event);
  this->NumberOfAcceptCommands += 1;
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::DeleteAcceptCommands()
{
  int i;

  for (i = 0; i < this->NumberOfAcceptCommands; ++i)
    {
    delete [] this->AcceptCommands[i];
    this->AcceptCommands[i] = NULL;
    }
  delete [] this->AcceptCommands;
  this->AcceptCommands = NULL;
  this->NumberOfAcceptCommands = 0;
  this->AcceptCommandArrayLength = 0;
}
