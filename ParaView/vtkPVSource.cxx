/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSource.cxx
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
#include "vtkKWLabeledEntry.h"
#include "vtkKWMessageDialog.h"

#include "vtkPVSource.h"
#include "vtkPVApplication.h"
#include "vtkKWView.h"
#include "vtkKWScale.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPVSelectionList.h"
#include "vtkStringList.h"
#include "vtkCollection.h"
#include "vtkPVData.h"
#include "vtkPVSourceInterface.h"


int vtkPVSourceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSource::vtkPVSource()
{
  this->CommandFunction = vtkPVSourceCommand;
  this->Name = NULL;

  this->Initialized = 0;
  
  this->PVInputs = NULL;
  this->NumberOfPVInputs = 0;
  this->PVOutputs = NULL;
  this->NumberOfPVOutputs = 0;

  this->VTKSource = NULL;
  this->VTKSourceTclName = NULL;

  this->Properties = vtkKWWidget::New();
  
  this->NavigationFrame = vtkKWLabeledFrame::New();
  this->NavigationCanvas = vtkKWWidget::New();
  this->ParameterFrame = vtkKWLabeledFrame::New();
  this->AcceptButton = vtkKWPushButton::New();
  this->CancelButton = vtkKWPushButton::New();
  this->DeleteButton = vtkKWPushButton::New();
  
  this->Widgets = vtkKWWidgetCollection::New();
  this->LastSelectionList = NULL;
  
  this->AcceptCommands = vtkStringList::New();
  this->CancelCommands = vtkStringList::New();
  
  this->Interface = NULL;
}

//----------------------------------------------------------------------------
vtkPVSource::~vtkPVSource()
{
  int i;
  
  for (i = 0; i < this->NumberOfPVOutputs; i++)
    {
    if (this->PVOutputs[i])
      {
      this->PVOutputs[i]->UnRegister(this);
      this->PVOutputs[i] = NULL;
      }
    }
  
  if (this->PVOutputs)
    {
    delete [] this->PVOutputs;
    this->PVOutputs = 0;
    }
  
  this->NumberOfPVOutputs = 0;
  
  for (i = 0; i < this->NumberOfPVInputs; i++)
    {
    if (this->PVInputs[i])
      {
      this->PVInputs[i]->UnRegister(this);
      this->PVInputs[i] = NULL;
      }
    }

  if (this->PVInputs)
    {
    delete [] this->PVInputs;
    this->PVInputs = 0;
    }
  
  this->NumberOfPVInputs = 0;

  this->SetVTKSource(NULL, NULL);

  this->SetName(NULL);
  this->Properties->Delete();
  this->Properties = NULL;

  this->NavigationFrame->Delete();
  this->NavigationFrame = NULL;
  this->NavigationCanvas->Delete();
  this->NavigationCanvas = NULL;

  this->ParameterFrame->Delete();
  this->ParameterFrame = NULL;

  this->Widgets->Delete();
  this->Widgets = NULL;

  this->AcceptButton->Delete();
  this->AcceptButton = NULL;  
  
  this->CancelButton->Delete();
  this->CancelButton = NULL;  
  
  this->DeleteButton->Delete();
  this->DeleteButton = NULL;
  
  if (this->LastSelectionList)
    {
    this->LastSelectionList->UnRegister(this);
    this->LastSelectionList = NULL;
    }

  this->AcceptCommands->Delete();
  this->AcceptCommands = NULL;  
  this->CancelCommands->Delete();
  this->CancelCommands = NULL;

  this->SetInterface(NULL);
}

//----------------------------------------------------------------------------
vtkPVSource* vtkPVSource::New()
{
  return new vtkPVSource();
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void vtkPVSourceStartProgress(void *arg)
{
  //vtkPVSource *me = (vtkPVSource*)arg;
  //vtkSource *vtkSource = me->GetVTKSource();
  //static char str[200];
  
  //if (vtkSource && me->GetWindow())
  //  {
  //  sprintf(str, "Processing %s", vtkSource->GetClassName());
  //  me->GetWindow()->SetStatusText(str);
  //  }
}
//----------------------------------------------------------------------------
void vtkPVSourceReportProgress(void *arg)
{
  //vtkPVSource *me = (vtkPVSource*)arg;
  //vtkSource *vtkSource = me->GetVTKSource();

  //if (me->GetWindow())
  //  {
  //  me->GetWindow()->GetProgressGauge()->SetValue((int)(vtkSource->GetProgress() * 100));
  //  }
}
//----------------------------------------------------------------------------
void vtkPVSourceEndProgress(void *arg)
{
  //vtkPVSource *me = (vtkPVSource*)arg;
  
  //if (me->GetWindow())
  //  {
  //  me->GetWindow()->SetStatusText("");
  //  me->GetWindow()->GetProgressGauge()->SetValue(0);
  //  }
}

//----------------------------------------------------------------------------
// Tcl does the reference counting, so we are not going to put an 
// additional reference of the data.
void vtkPVSource::SetVTKSource(vtkSource *source, const char *tclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp == NULL)
    {
    vtkErrorMacro("Set the application before you set the VTKDataTclName.");
    return;
    }
  
  if (this->VTKSourceTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->VTKSourceTclName);
    delete [] this->VTKSourceTclName;
    this->VTKSourceTclName = NULL;
    this->VTKSource = NULL;
    }
  if (tclName)
    {
    this->VTKSourceTclName = new char[strlen(tclName) + 1];
    strcpy(this->VTKSourceTclName, tclName);
    this->VTKSource = source;
    // Set up the progress methods.
    //source->SetStartMethod(vtkPVSourceStartProgress, this);
    //source->SetProgressMethod(vtkPVSourceReportProgress, this);
    //source->SetEndMethod(vtkPVSourceEndProgress, this);
    }
}

//----------------------------------------------------------------------------
// Need to avoid circular includes in header.
void vtkPVSource::SetInterface(vtkPVSourceInterface *pvsi)
{
  if (this->Interface == pvsi)
    {
    return;
    }
  this->Modified();

  // Get rid of old VTKInterface reference.
  if (this->Interface)
    {
    // Be extra careful of circular references. (not important here...)
    vtkPVSourceInterface *tmp = this->Interface;
    this->Interface = NULL;
    tmp->UnRegister(this);
    }
  if (pvsi)
    {
    this->Interface = pvsi;
    pvsi->Register(this);
    }
}

//----------------------------------------------------------------------------
vtkPVWindow* vtkPVSource::GetWindow()
{
  if (this->View == NULL || this->View->GetParentWindow() == NULL)
    {
    return NULL;
    }
  
  return vtkPVWindow::SafeDownCast(this->View->GetParentWindow());
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVSource::GetPVApplication()
{
  if (this->Application == NULL)
    {
    return NULL;
    }
  
  if (this->Application->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->Application);
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}

//----------------------------------------------------------------------------
void vtkPVSource::CreateProperties()
{
  const char *sourcePage;
  vtkPVApplication *app = this->GetPVApplication();

  // invoke super
  this->vtkKWComposite::CreateProperties();  

  // Set up the pages of the notebook.
  sourcePage = this->GetClassName();
  this->Notebook->AddPage(sourcePage);
  this->Properties->SetParent(this->Notebook->GetFrame(sourcePage));
  this->Properties->Create(this->Application,"frame","");
  this->Script("pack %s -pady 2 -fill x -expand yes",
               this->Properties->GetWidgetName());
  
  // Setup the source page of the notebook.
  this->NavigationFrame->SetParent(this->Properties);
  this->NavigationFrame->Create(this->Application);
  this->NavigationFrame->SetLabel("Navigation");
  this->Script("pack %s -fill x -expand t -side top", this->NavigationFrame->GetWidgetName());
  this->NavigationCanvas->SetParent(this->NavigationFrame->GetFrame());
  this->NavigationCanvas->Create(this->Application, "canvas", "-height 45 -bg white"); 
  this->Script("pack %s -fill x -expand t -side top", this->NavigationCanvas->GetWidgetName());

  this->ParameterFrame->SetParent(this->Properties);
  this->ParameterFrame->Create(this->Application);
  this->ParameterFrame->SetLabel("Parameters");
  this->Script("pack %s -fill x -expand t -side top", this->ParameterFrame->GetWidgetName());

  vtkKWWidget *frame = vtkKWWidget::New();
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());  
  
  this->AcceptButton->SetParent(frame);
  this->AcceptButton->Create(this->Application, "-text Accept");
  this->AcceptButton->SetCommand(this, "AcceptCallback");
  this->Script("pack %s -side left -fill x -expand t", 
	       this->AcceptButton->GetWidgetName());

  this->CancelButton->SetParent(frame);
  this->CancelButton->Create(this->Application, "-text Cancel");
  this->CancelButton->SetCommand(this, "CancelCallback");
  this->Script("pack %s -side left -fill x -expand t", 
	       this->CancelButton->GetWidgetName());

  this->DeleteButton->SetParent(frame);
  this->DeleteButton->Create(this->Application, "-text Delete");
  this->DeleteButton->SetCommand(this, "DeleteCallback");
  this->Script("pack %s -side left -fill x -expand t",
               this->DeleteButton->GetWidgetName());

  // Isolate events to this window untill accept or cancel is pressed.
  this->Script("grab set %s", this->ParameterFrame->GetWidgetName());
  
  this->UpdateProperties();
  
  this->UpdateParameterWidgets();
}

//----------------------------------------------------------------------------
void vtkPVSource::Select(vtkKWView *v)
{
  vtkPVData *data;
  
  // invoke super
  this->vtkKWComposite::Select(v); 
  
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());

  this->UpdateProperties();
  
  // This assumes that a source only has one output.
  data = this->GetNthPVOutput(0);
  if (data)
    {
    data->Select(v);
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::Deselect(vtkKWView *v)
{
  int idx;
  vtkPVData *data;

  // invoke super
  this->vtkKWComposite::Deselect(v); 
  
  this->Script("pack forget %s", this->Notebook->GetWidgetName());

  // Deselect all outputs.
  for (idx = 0; idx < this->NumberOfPVOutputs; ++idx)
    {
    data = this->GetNthPVOutput(idx);
    if (data)
      {
      data->Deselect(v);
      }
    }
}

//----------------------------------------------------------------------------
char* vtkPVSource::GetName()
{
  return this->Name;
}

//----------------------------------------------------------------------------
void vtkPVSource::SetName (const char* arg) 
{ 
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " 
                << this->Name << " to " << arg ); 
  if ( this->Name && arg && (!strcmp(this->Name,arg))) 
    { 
    return;
    } 
  if (this->Name) 
    { 
    delete [] this->Name; 
    } 
  if (arg) 
    { 
    this->Name = new char[strlen(arg)+1]; 
    strcpy(this->Name,arg); 
    } 
  else 
    { 
    this->Name = NULL;
    }
  this->Modified(); 
} 

//----------------------------------------------------------------------------
// We should really be dealing with the outputs.  Remove this method.
void vtkPVSource::SetVisibility(int v)
{
  int i;
  vtkPVData *ac;
  
  for (i = 0; i < this->NumberOfPVOutputs; ++i)
    {
    if (this->PVOutputs[i])
      {
      ac = this->PVOutputs[i];
      if (ac)
	{
	ac->SetVisibility(v);
	}
      }
    }
}

  
//----------------------------------------------------------------------------
int vtkPVSource::GetVisibility()
{
  vtkProp *p = this->GetProp();
  
  if (p == NULL)
    {
    return 0;
    }
  
  return p->GetVisibility();
}


//----------------------------------------------------------------------------
void vtkPVSource::AcceptCallback()
{
  int i;
  vtkPVWindow *window;

  window = this->GetWindow();
  
  // Call the commands to set ivars from widget values.
  for (i = 0; i < this->AcceptCommands->GetLength(); ++i)
    {
    this->Script(this->AcceptCommands->GetString(i));
    }  
  
  // Initialize the output if necessary.
  if ( ! this->Initialized)
    { // This is the first 2time, initialize data.    
    vtkPVData *input;
    vtkPVData *ac;
    
    ac = this->GetPVOutput(0);
    window->GetMainView()->AddComposite(ac);
    ac->SetPropertiesParent(window->GetMainView()->GetActorParent());    
    ac->CreateProperties();
    ac->Initialize();
    // Make the last data invisible.
    input = this->GetNthPVInput(0);
    if (input)
      {
      input->SetVisibility(0);
      }
    window->GetMainView()->ResetCamera();

    // Set the current data of the window.
    window->SetCurrentPVData(this->GetNthPVOutput(0));
    
    // Remove the local grab
    this->Script("grab release %s", this->ParameterFrame->GetWidgetName());    
    this->Initialized = 1;
    }

  window->GetMainView()->SetSelectedComposite(this);  
  this->UpdateProperties();
  this->GetView()->Render();
  window->GetSourceList()->Update();
}

//----------------------------------------------------------------------------
void vtkPVSource::CancelCallback()
{
  vtkPVApplication *pvApp = (vtkPVApplication*)this->Application;
  
  if ( ! this->Initialized)
    { // Accept has not been called yet.  Delete the object.
    // What about the local grab?
    this->DeleteCallback();
    return;
    }

  this->UpdateParameterWidgets();
}

//---------------------------------------------------------------------------
void vtkPVSource::DeleteCallback()
{
  vtkPVData *ac;
  vtkPVApplication *pvApp = (vtkPVApplication*)this->Application;
  vtkPVSource *prev;
  int i;
  
  if ( ! this->Initialized)
    {
    // Remove the local grab
    this->Script("grab release %s", this->ParameterFrame->GetWidgetName());    
    this->Initialized = 1;
    }
  
  for (i = 0; i < this->NumberOfPVOutputs; ++i)
    {
    if (this->PVOutputs[i] && 
	this->PVOutputs[i]->GetPVSourceUsers()->GetNumberOfItems() > 0)
      { // Button should be deactivated.
      vtkErrorMacro("An output is used.  We cannot delete this source.");
      return;
      }
    }
  
  // Remove this source from the inputs users collection.
  for (i = 0; i < this->GetNumberOfPVInputs(); i++)
    {
    if (this->PVInputs[i])
      {
      this->PVInputs[i]->RemovePVSourceFromUsers(this);
      }
    }
    
  // Look for a source to make current.
  prev = this->GetWindow()->GetPreviousPVSource();
  this->GetWindow()->SetCurrentPVSource(prev);
  if (prev)
    {
    prev->GetPVOutput(0)->VisibilityOn();
    prev->ShowProperties();
    }
  else
    {
    // Unpack the properties.  This is required if prev is NULL.
    this->Script("catch {eval pack forget [pack slaves %s]}",
		 this->View->GetPropertiesParent()->GetWidgetName());
    }
      
  // We need to remove this source from the Source List.    
  this->GetWindow()->GetSourceList()->GetSources()->RemoveItem(this);
  this->GetWindow()->GetSourceList()->Update();    

  // Remove all of the actors mappers. from the renderer.
  for (i = 0; i < this->NumberOfPVOutputs; ++i)
    {
    if (this->PVOutputs[i])
      {
      ac = this->GetPVOutput(i);
      this->GetWindow()->GetMainView()->RemoveComposite(ac);
      }
    }    
  
  // Remove all of the outputs
  for (i = 0; i < this->NumberOfPVOutputs; ++i)
    {
    if (this->PVOutputs[i])
      {
      this->PVOutputs[i]->UnRegister(this);
      this->PVOutputs[i] = NULL;
      }
    }
  
  this->GetView()->Render();
  // I hope this will delete this source.
  this->GetWindow()->GetMainView()->RemoveComposite(this);
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateParameterWidgets()
{
  int num, i;
  char *cmd;

  // Copy the ivars from the vtk object to the UI.
  num = this->CancelCommands->GetLength();
  for (i = 0; i < num; ++i)
    {
    cmd = this->CancelCommands->GetString(i);
    if (cmd)
      {
      this->Script(cmd);
      }
    } 
}


//----------------------------------------------------------------------------
void vtkPVSource::AcceptHelper(char *method, char *args)
{
}

//----------------------------------------------------------------------------
void vtkPVSource::AcceptHelper2(char *name, char *method, char *args)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  vtkDebugMacro("AcceptHelper2 " << name << ", " << method << ", " << args);

  pvApp->Script("%s %s %s", name, method, args);
  pvApp->BroadcastScript("%s %s %s", name,  method, args);
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateProperties()
{
  static char *font = "-adobe-helvetica-medium-r-normal-*-14-100-100-100-p-76-iso8859-1";
  char *result;
  int bbox[4], bboxOut[4];
  int xMid, yMid, y;
  char *tmp;
  vtkPVSource *source;
  vtkPVSourceCollection *outs, *moreOuts;
  vtkPVData *moreOut;
  int i;
  
  
  // --------------------------------------
  // Change the state of the delete button based on if there are any useres.
  // Only filters at the end of a pipeline can be deleted.
  if (this->GetPVOutput(0) &&
      this->GetPVOutput(0)->GetPVSourceUsers()->GetNumberOfItems() > 0)
      {
      this->Script("%s configure -state disabled",
                   this->DeleteButton->GetWidgetName());
      }
    else
      {
      this->Script("%s configure -state normal",
                   this->DeleteButton->GetWidgetName());
      }  
  
  // --------------------------------------
  // Now do the navigation canvas.
  // Clear the canvas
  this->Script("%s delete all",
               this->NavigationCanvas->GetWidgetName());

  // Put the inputs in the canvas.
  if (this->PVInputs)
    {
    y = 10;
    for (i = 0; i < this->NumberOfPVInputs; i++)
      {
      source = this->PVInputs[i]->GetPVSource();
      if (source)
        {
        // Draw the name of the assembly.
        this->Script(
          "%s create text %d %d -text {%s} -font %s -anchor w -tags x -fill blue",
          this->NavigationCanvas->GetWidgetName(), 20, y,
          source->GetName(), font);
        
        result = this->Application->GetMainInterp()->result;
        tmp = new char[strlen(result)+1];
        strcpy(tmp,result);
        this->Script("%s bind %s <ButtonPress-1> {%s SelectSource %s}",
                     this->NavigationCanvas->GetWidgetName(), tmp,
                     this->GetTclName(), source->GetTclName());
        
        // Get the bounding box for the name. We may need to highlight it.
        this->Script( "%s bbox %s",this->NavigationCanvas->GetWidgetName(),
                      tmp);
        delete [] tmp;
        tmp = NULL;
        result = this->Application->GetMainInterp()->result;
        sscanf(result, "%d %d %d %d", bbox, bbox+1, bbox+2, bbox+3);
        if (i == 0)
          {
          // only want to set xMid and yMid once
          yMid = (int)(0.5 * (bbox[1]+bbox[3]));
          xMid = (int)(0.5 * (bbox[2]+120));
          }
        
        // Draw a line from input to source.
        if (y == 10)
          {
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
                       this->NavigationCanvas->GetWidgetName(), bbox[2], yMid,
                       125, yMid);
          }
        else
          {
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
                       this->NavigationCanvas->GetWidgetName(), xMid, yMid,
                       xMid, yMid+15);
          yMid += 15;
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
                       this->NavigationCanvas->GetWidgetName(), bbox[2],
                       yMid, xMid, yMid);
          }
        
        if (source->GetPVInputs())
          {
          if (source->GetNthPVInput(0)->GetPVSource())
            {
            // Draw ellipsis indicating that this source has a source.
            this->Script("%s create line %d %d %d %d",
                         this->NavigationCanvas->GetWidgetName(), 6, yMid, 8,
                         yMid);
            this->Script("%s create line %d %d %d %d",
                         this->NavigationCanvas->GetWidgetName(), 10, yMid, 12,
                         yMid);
            this->Script("%s create line %d %d %d %d",
                         this->NavigationCanvas->GetWidgetName(), 14, yMid, 16,
                         yMid);
            }
          }
        }
      y += 15;
      }
    }

  // Draw the name of the assembly.
  this->Script(
    "%s create text %d %d -text {%s} -font %s -anchor w -tags x",
    this->NavigationCanvas->GetWidgetName(), 130, 10, this->GetName(), font);
  result = this->Application->GetMainInterp()->result;
  tmp = new char[strlen(result)+1];
  strcpy(tmp,result);
  // Get the bounding box for the name. We may need to highlight it.
  this->Script( "%s bbox %s",this->NavigationCanvas->GetWidgetName(), tmp);
  delete [] tmp;
  tmp = NULL;
  result = this->Application->GetMainInterp()->result;
  sscanf(result, "%d %d %d %d", bbox, bbox+1, bbox+2, bbox+3);
  yMid = (int)(0.5 * (bbox[1]+bbox[3]));
  xMid = (int)(0.5 * (bbox[2] + 245));

  // Put the outputs in the canvas.
  outs = NULL;
  if (this->PVOutputs)
    {
    y = 10;
    //  for (i = 0; i < this->NumberOfPVOutputs; i++)
    if (this->PVOutputs[0])
      {
      outs = this->PVOutputs[0]->GetPVSourceUsers();

      if (outs)
	{
	outs->InitTraversal();
	while ( (source = outs->GetNextPVSource()) )
	  {
	  // Draw the name of the assembly.
	  this->Script(
	    "%s create text %d %d -text {%s} -font %s -anchor w -tags x -fill blue",
	    this->NavigationCanvas->GetWidgetName(), 250, y,
	    source->GetName(), font);
	  
	  result = this->Application->GetMainInterp()->result;
	  tmp = new char[strlen(result)+1];
	  strcpy(tmp,result);
	  this->Script("%s bind %s <ButtonPress-1> {%s SelectSource %s}",
		       this->NavigationCanvas->GetWidgetName(), tmp,
		       this->GetTclName(), source->GetTclName());
	  // Get the bounding box for the name. We may need to highlight it.
	  this->Script( "%s bbox %s",this->NavigationCanvas->GetWidgetName(), tmp);
	  delete [] tmp;
	  tmp = NULL;
	  result = this->Application->GetMainInterp()->result;
	  sscanf(result, "%d %d %d %d", bboxOut, bboxOut+1, bboxOut+2, bboxOut+3);
	  
	  // Draw to output.
	  if (y == 10)
	    { // first is a special case (single line).
	    this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
			 this->NavigationCanvas->GetWidgetName(), bbox[2], yMid,
			 245, yMid);
	    }
	  else
	    {
	    this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
			 this->NavigationCanvas->GetWidgetName(), xMid, yMid, xMid,
			 yMid+15);
	    yMid += 15;
	    this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
			 this->NavigationCanvas->GetWidgetName(), xMid, yMid,
			 245, yMid);
	    }
	  if (moreOut = source->GetPVOutput(0))
	    {
	    if (moreOuts = moreOut->GetPVSourceUsers())
	      {
	      moreOuts->InitTraversal();
	      if (moreOuts->GetNextPVSource())
		{
		this->Script("%s create line %d %d %d %d",
			     this->NavigationCanvas->GetWidgetName(),
			     bboxOut[2]+10, yMid, bboxOut[2]+12, yMid);
		this->Script("%s create line %d %d %d %d",
			     this->NavigationCanvas->GetWidgetName(),
			     bboxOut[2]+14, yMid, bboxOut[2]+16, yMid);
		this->Script("%s create line %d %d %d %d",
			     this->NavigationCanvas->GetWidgetName(),
			     bboxOut[2]+18, yMid, bboxOut[2]+20, yMid);
		}
	      }
	    }
	  y += 15;
	  }
	}
      }
    }
}

//----------------------------------------------------------------------------
// Why do we need this.  Isn't show properties and Raise handled by window?
void vtkPVSource::SelectSource(vtkPVSource *source)
{
  if (source)
    {
    this->GetWindow()->SetCurrentPVSource(source);
    source->ShowProperties();
    source->GetNotebook()->Raise(0);
    }
}

typedef vtkPVData *vtkPVDataPointer;
//---------------------------------------------------------------------------
void vtkPVSource::SetNumberOfPVInputs(int num)
{
  int idx;
  vtkPVDataPointer *inputs;

  // in case nothing has changed.
  if (num == this->NumberOfPVInputs)
    {
    return;
    }
  
  // Allocate new arrays.
  inputs = new vtkPVDataPointer[num];

  // Initialize with NULLs.
  for (idx = 0; idx < num; ++idx)
    {
    inputs[idx] = NULL;
    }

  // Copy old inputs
  for (idx = 0; idx < num && idx < this->NumberOfPVInputs; ++idx)
    {
    inputs[idx] = this->PVInputs[idx];
    }
  
  // delete the previous arrays
  if (this->PVInputs)
    {
    delete [] this->PVInputs;
    this->PVInputs = NULL;
    this->NumberOfPVInputs = 0;
    }
  
  // Set the new array
  this->PVInputs = inputs;
  
  this->NumberOfPVInputs = num;
  this->Modified();
}

//---------------------------------------------------------------------------
// In the future, this should consider the vtkPVSourceInterface.
void vtkPVSource::SetNthPVInput(int idx, vtkPVData *pvd)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (idx < 0)
    {
    vtkErrorMacro(<< "SetNthPVInput: " << idx << ", cannot set input. ");
    return;
    }
  
  // Expand array if necessary.
  if (idx >= this->NumberOfPVInputs)
    {
    this->SetNumberOfPVInputs(idx + 1);
    }
  
  // Does this change anything?  Yes, it keeps the object from being modified.
  if (pvd == this->PVInputs[idx])
    {
    return;
    }
  
  if (this->PVInputs[idx])
    {
    this->PVInputs[idx]->RemovePVSourceFromUsers(this);
    this->PVInputs[idx]->UnRegister(this);
    this->PVInputs[idx] = NULL;
    }
  
  if (pvd)
    {
    pvd->Register(this);
    pvd->AddPVSourceToUsers(this);
    this->PVInputs[idx] = pvd;
    }

  // Relay the change to the VTK objects.  
  // This is where we will need a SetCommand from the interface ...
  pvApp->BroadcastScript("%s SetInput %s", this->GetVTKSourceTclName(),
			 pvd->GetVTKDataTclName());
  
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkPVSource::RemoveAllPVInputs()
{
  if ( this->PVInputs )
    {
    for (int idx = 0; idx < this->NumberOfPVInputs; ++idx)
      {
      if ( this->PVInputs[idx] )
        {
        this->PVInputs[idx]->UnRegister(this);
        this->PVInputs[idx] = NULL;
        }
      }

    delete [] this->PVInputs;
    this->PVInputs = NULL;
    this->NumberOfPVInputs = 0;
    this->Modified();
    }
}

//---------------------------------------------------------------------------
vtkPVData *vtkPVSource::GetNthPVInput(int idx)
{
  if (idx >= this->NumberOfPVInputs)
    {
    return NULL;
    }
  
  return (vtkPVData *)(this->PVInputs[idx]);
}

//---------------------------------------------------------------------------
void vtkPVSource::SetNumberOfPVOutputs(int num)
{
  int idx;
  vtkPVDataPointer *outputs;

  // in case nothing has changed.
  if (num == this->NumberOfPVOutputs)
    {
    return;
    }
  
  // Allocate new arrays.
  outputs = new vtkPVDataPointer[num];

  // Initialize with NULLs.
  for (idx = 0; idx < num; ++idx)
    {
    outputs[idx] = NULL;
    }

  // Copy old outputs
  for (idx = 0; idx < num && idx < this->NumberOfPVOutputs; ++idx)
    {
    outputs[idx] = this->PVOutputs[idx];
    }
  
  // delete the previous arrays
  if (this->PVOutputs)
    {
    delete [] this->PVOutputs;
    this->PVOutputs = NULL;
    this->NumberOfPVOutputs = 0;
    }
  
  // Set the new array
  this->PVOutputs = outputs;
  
  this->NumberOfPVOutputs = num;
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkPVSource::SetNthPVOutput(int idx, vtkPVData *pvd)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (idx < 0)
    {
    vtkErrorMacro(<< "SetNthPVOutput: " << idx << ", cannot set output. ");
    return;
    }
  
  if (this->NumberOfPVOutputs <= idx)
    {
    this->SetNumberOfPVOutputs(idx+1);
    }
  
  // Does this change anything?  Yes, it keeps the object from being modified.
  if (pvd == this->PVOutputs[idx])
    {
    return;
    }
  
  if (this->PVOutputs[idx])
    {
    // Manage backward pointer.
    this->PVOutputs[idx]->SetPVSource(this);
    this->PVOutputs[idx]->UnRegister(this);
    this->PVOutputs[idx] = NULL;
    }
  
  if (pvd)
    {
    pvd->Register(this);
    this->PVOutputs[idx] = pvd;
    // Manage backward pointer.
    pvd->SetPVSource(this);
    }

  this->Modified();
}

//---------------------------------------------------------------------------
vtkPVData *vtkPVSource::GetNthPVOutput(int idx)
{
  if (idx >= this->NumberOfPVOutputs)
    {
    return NULL;
    }
  
  return (vtkPVData *)(this->PVOutputs[idx]);
}

//----------------------------------------------------------------------------
void vtkPVSource::Save(ofstream *file)
{
}



//----------------------------------------------------------------------------
vtkKWCheckButton *vtkPVSource::AddLabeledToggle(char *label, char *setCmd, char *getCmd, 
                                                vtkKWObject *o)
{
  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // A frame to hold the other widgets.
  vtkKWWidget *frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {
    vtkKWLabel *labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "-width 18 -justify right");
    labelWidget->SetLabel(label);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  // Now the check button
  vtkKWCheckButton *check = vtkKWCheckButton::New();
  this->Widgets->AddItem(check);
  check->SetParent(frame);
  check->Create(this->Application, "");
  this->Script("pack %s -side left", check->GetWidgetName());

  // Command to update the UI.
  this->CancelCommands->AddString("%s SetState [%s %s]",
          check->GetTclName(), tclName, getCmd); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s [%s GetState]",
          this->GetTclName(), tclName, setCmd, check->GetTclName()); 

  this->Script("pack %s -side left", check->GetWidgetName());

  frame->Delete();
  check->Delete();

  // Although it has been deleted, it did not destruct.
  // We need to change this into a PVWidget.
  return check;
}  
//----------------------------------------------------------------------------
vtkKWEntry *vtkPVSource::AddFileEntry(char *label, char *setCmd, char *getCmd,
                                      char *ext, vtkKWObject *o)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry *entry;
  vtkKWPushButton *browseButton;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {  
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "-width 18 -justify right");
    labelWidget->SetLabel(label);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  entry = vtkKWEntry::New();
  this->Widgets->AddItem(entry);
  entry->SetParent(frame);
  entry->Create(this->Application, "");
  this->Script("pack %s -side left -fill x -expand t", entry->GetWidgetName());


  browseButton = vtkKWPushButton::New();
  this->Widgets->AddItem(browseButton);
  browseButton->SetParent(frame);
  browseButton->Create(this->Application, "");
  browseButton->SetLabel("Browse");
  this->Script("pack %s -side left", browseButton->GetWidgetName());
  if (ext)
    {
    char str[1000];
    sprintf(str, "SetValue [tk_getOpenFile -filetypes {{{} {.%s}}}]", ext);
    browseButton->SetCommand(entry, str);
    }
  else
    {
    browseButton->SetCommand(entry, "SetValue [tk_getOpenFile]");
    }
  browseButton->Delete();
  browseButton = NULL;

  // Command to update the UI.
  this->CancelCommands->AddString("%s SetValue [%s %s]",
             entry->GetTclName(), tclName, getCmd); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s [%s GetValue]",
             this->GetTclName(), tclName, setCmd, entry->GetTclName());

  frame->Delete();
  entry->Delete();

  // Although it has been deleted, it did not destruct.
  // We need to change this into a PVWidget.
  return entry;
}

//----------------------------------------------------------------------------
vtkKWEntry *vtkPVSource::AddStringEntry(char *label, char *setCmd, char *getCmd,
                                        vtkKWObject *o)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry *entry;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {  
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "-width 18 -justify right");
    labelWidget->SetLabel(label);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  entry = vtkKWEntry::New();
  this->Widgets->AddItem(entry);
  entry->SetParent(frame);
  entry->Create(this->Application, "");
  this->Script("pack %s -side left -fill x -expand t", entry->GetWidgetName());

  // Command to update the UI.
  this->CancelCommands->AddString("%s SetValue [%s %s]",
             entry->GetTclName(), tclName, getCmd); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s [list [%s GetValue]]",
             this->GetTclName(), tclName, setCmd, entry->GetTclName());

  frame->Delete();
  entry->Delete();

  // Although it has been deleted, it did not destruct.
  // We need to change this into a PVWidget.
  return entry;
}

//----------------------------------------------------------------------------
vtkKWEntry *vtkPVSource::AddLabeledEntry(char *label, char *setCmd, char *getCmd,
                                         vtkKWObject *o)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry *entry;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {  
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "-width 18 -justify right");
    labelWidget->SetLabel(label);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  entry = vtkKWEntry::New();
  this->Widgets->AddItem(entry);
  entry->SetParent(frame);
  entry->Create(this->Application, "");
  this->Script("pack %s -side left -fill x -expand t", entry->GetWidgetName());

  // Command to update the UI.
  this->CancelCommands->AddString("%s SetValue [%s %s]",
             entry->GetTclName(), tclName, getCmd); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s [%s GetValue]",
             this->GetTclName(), tclName, setCmd, entry->GetTclName());

  frame->Delete();
  entry->Delete();

  // Although it has been deleted, it did not destruct.
  // We need to change this into a PVWidget.
  return entry;
}

//----------------------------------------------------------------------------
void vtkPVSource::AddVector2Entry(char *label, char *l1, char *l2,
                                  char *setCmd, char *getCmd, vtkKWObject *o)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry *minEntry, *maxEntry;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {  
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "-width 18 -justify right");
    labelWidget->SetLabel(label);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  // Min
  if (l1 && l1[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l1);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  minEntry = vtkKWEntry::New();
  this->Widgets->AddItem(minEntry);
  minEntry->SetParent(frame);
  minEntry->Create(this->Application, "-width 2");
  this->Script("pack %s -side left -fill x -expand t", minEntry->GetWidgetName());

  // Max
  if (l2 && l2[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l2);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }  
  maxEntry = vtkKWEntry::New();
  this->Widgets->AddItem(maxEntry);
  maxEntry->SetParent(frame);
  maxEntry->Create(this->Application, "-width 2");
  this->Script("pack %s -side left -fill x -expand t", maxEntry->GetWidgetName());

  // Command to update the UI.
  this->CancelCommands->AddString(
    "%s SetValue [lindex [%s %s] 0]", minEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddString(
    "%s SetValue [lindex [%s %s] 1]", maxEntry->GetTclName(), tclName, getCmd);
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s \"[%s GetValue] [%s GetValue]\"",
                        this->GetTclName(), tclName, setCmd, minEntry->GetTclName(),
                        maxEntry->GetTclName());

  frame->Delete();
  minEntry->Delete();
  maxEntry->Delete();
}

//----------------------------------------------------------------------------
void vtkPVSource::AddVector3Entry(char *label, char *l1, char *l2, char *l3,
				  char *setCmd, char *getCmd, vtkKWObject *o)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry *xEntry, *yEntry, *zEntry;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "-width 18 -justify right");
    labelWidget->SetLabel(label);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  // X
  if (l1 && l1[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l1);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  xEntry = vtkKWEntry::New();
  this->Widgets->AddItem(xEntry);
  xEntry->SetParent(frame);
  xEntry->Create(this->Application, "-width 2");
  this->Script("pack %s -side left -fill x -expand t", xEntry->GetWidgetName());

  // Y
  if (l2 && l2[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l2);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  yEntry = vtkKWEntry::New();
  this->Widgets->AddItem(yEntry);
  yEntry->SetParent(frame);
  yEntry->Create(this->Application, "-width 2");
  this->Script("pack %s -side left -fill x -expand t", yEntry->GetWidgetName());

  // Z
  if (l3 && l3[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l3);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  zEntry = vtkKWEntry::New();
  this->Widgets->AddItem(zEntry);
  zEntry->SetParent(frame);
  zEntry->Create(this->Application, "-width 2");
  this->Script("pack %s -side left -fill x -expand t", zEntry->GetWidgetName());

  // Command to update the UI.
  this->CancelCommands->AddString("%s SetValue [lindex [%s %s] 0]", 
             xEntry->GetTclName(), tclName, getCmd); 
  this->CancelCommands->AddString("%s SetValue [lindex [%s %s] 1]", 
             yEntry->GetTclName(), tclName, getCmd); 
  this->CancelCommands->AddString("%s SetValue [lindex [%s %s] 2]", 
             zEntry->GetTclName(), tclName, getCmd); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s \"[%s GetValue] [%s GetValue] [%s GetValue]\"",
                        this->GetTclName(), tclName, setCmd, xEntry->GetTclName(),
                        yEntry->GetTclName(), zEntry->GetTclName());

  frame->Delete();
  xEntry->Delete();
  yEntry->Delete();
  zEntry->Delete();
}


//----------------------------------------------------------------------------
void vtkPVSource::AddVector4Entry(char *label, char *l1, char *l2, char *l3,
                                  char *l4, char *setCmd, char *getCmd,
                                  vtkKWObject *o)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry *xEntry, *yEntry, *zEntry, *wEntry;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "-width 18 -justify right");
    labelWidget->SetLabel(label);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  // X
  if (l1 && l1[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l1);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  xEntry = vtkKWEntry::New();
  this->Widgets->AddItem(xEntry);
  xEntry->SetParent(frame);
  xEntry->Create(this->Application, "-width 2");
  this->Script("pack %s -side left -fill x -expand t", xEntry->GetWidgetName());

  // Y
  if (l2 && l2[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l2);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  yEntry = vtkKWEntry::New();
  this->Widgets->AddItem(yEntry);
  yEntry->SetParent(frame);
  yEntry->Create(this->Application, "-width 2");
  this->Script("pack %s -side left -fill x -expand t", yEntry->GetWidgetName());

  // Z
  if (l3 && l3[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l3);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  zEntry = vtkKWEntry::New();
  this->Widgets->AddItem(zEntry);
  zEntry->SetParent(frame);
  zEntry->Create(this->Application, "-width 2");
  this->Script("pack %s -side left -fill x -expand t", zEntry->GetWidgetName());

  // W
  if (l4 && l4[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l4);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  wEntry = vtkKWEntry::New();
  this->Widgets->AddItem(wEntry);
  wEntry->SetParent(frame);
  wEntry->Create(this->Application, "-width 2");
  this->Script("pack %s -side left -fill x -expand t", wEntry->GetWidgetName());

  // Command to update the UI.
  this->CancelCommands->AddString(
    "%s SetValue [lindex [%s %s] 0]", xEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddString(
    "%s SetValue [lindex [%s %s] 1]", yEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddString(
    "%s SetValue [lindex [%s %s] 2]", zEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddString(
    "%s SetValue [lindex [%s %s] 3]", wEntry->GetTclName(), tclName, getCmd);
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s \"[%s GetValue] [%s GetValue] [%s GetValue] [%s GetValue]\"",
                        this->GetTclName(), tclName, setCmd, xEntry->GetTclName(),
                        yEntry->GetTclName(), zEntry->GetTclName(), wEntry->GetTclName());

  frame->Delete();
  xEntry->Delete();
  yEntry->Delete();
  zEntry->Delete();
  wEntry->Delete();
}

//----------------------------------------------------------------------------
// It might make sence here to store the labels in an array 
// so that a loop can be used to create the widgets.
void vtkPVSource::AddVector6Entry(char *label, char *l1, char *l2, char *l3,
                                  char *l4, char *l5, char *l6,
                                  char *setCmd, char *getCmd, vtkKWObject *o)

{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry  *uEntry, *vEntry, *wEntry, *xEntry, *yEntry, *zEntry;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "-width 18 -justify right");
    labelWidget->SetLabel(label);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  // U
  if (l1 && l1[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l1);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  uEntry = vtkKWEntry::New();
  this->Widgets->AddItem(uEntry);
  uEntry->SetParent(frame);
  uEntry->Create(this->Application, "-width 2");
  this->Script("pack %s -side left -fill x -expand t", uEntry->GetWidgetName());

  // V
  if (l2 && l2[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l2);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  vEntry = vtkKWEntry::New();
  this->Widgets->AddItem(vEntry);
  vEntry->SetParent(frame);
  vEntry->Create(this->Application, "-width 2");
  this->Script("pack %s -side left -fill x -expand t", vEntry->GetWidgetName());

  // W
  if (l3 && l3[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l3);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  wEntry = vtkKWEntry::New();
  this->Widgets->AddItem(wEntry);
  wEntry->SetParent(frame);
  wEntry->Create(this->Application, "-width 2");
  this->Script("pack %s -side left -fill x -expand t", wEntry->GetWidgetName());

  // X
  if (l4 && l4[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l4);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  xEntry = vtkKWEntry::New();
  this->Widgets->AddItem(xEntry);
  xEntry->SetParent(frame);
  xEntry->Create(this->Application, "-width 2");
  this->Script("pack %s -side left -fill x -expand t", xEntry->GetWidgetName());

  // Y
  if (l5 && l5[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l5);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  yEntry = vtkKWEntry::New();
  this->Widgets->AddItem(yEntry);
  yEntry->SetParent(frame);
  yEntry->Create(this->Application, "-width 2");
  this->Script("pack %s -side left -fill x -expand t", yEntry->GetWidgetName());

  // Z
  if (l6 && l6[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l6);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  zEntry = vtkKWEntry::New();
  this->Widgets->AddItem(zEntry);
  zEntry->SetParent(frame);
  zEntry->Create(this->Application, "-width 2");
  this->Script("pack %s -side left -fill x -expand t", zEntry->GetWidgetName());

  // Command to update the UI.
  this->CancelCommands->AddString(
    "%s SetValue [lindex [%s %s] 0]",uEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddString(
    "%s SetValue [lindex [%s %s] 1]",vEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddString(
    "%s SetValue [lindex [%s %s] 2]",wEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddString(
    "%s SetValue [lindex [%s %s] 3]",xEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddString(
    "%s SetValue [lindex [%s %s] 4]",yEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddString(
    "%s SetValue [lindex [%s %s] 5]",zEntry->GetTclName(), tclName, getCmd);
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s \"[%s GetValue] [%s GetValue] [%s GetValue] [%s GetValue] [%s GetValue] [%s GetValue]\"",
  	 this->GetTclName(), tclName, setCmd, uEntry->GetTclName(), 
         vEntry->GetTclName(), wEntry->GetTclName(),
         xEntry->GetTclName(), yEntry->GetTclName(), zEntry->GetTclName());

  frame->Delete();
  uEntry->Delete();
  vEntry->Delete();
  wEntry->Delete();
  xEntry->Delete();
  yEntry->Delete();
  zEntry->Delete();
}


//----------------------------------------------------------------------------
vtkKWScale *vtkPVSource::AddScale(char *label, char *setCmd, char *getCmd,
                                  float min, float max, float resolution,
                                  vtkKWObject *o)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWScale *slider;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  labelWidget = vtkKWLabel::New();
  this->Widgets->AddItem(labelWidget);
  labelWidget->SetParent(frame);
  labelWidget->Create(this->Application, "-width 18 -justify right");
  labelWidget->SetLabel(label);
  this->Script("pack %s -side left", labelWidget->GetWidgetName());

  slider = vtkKWScale::New();
  this->Widgets->AddItem(slider);
  slider->SetParent(frame);
  slider->Create(this->Application, "-showvalue 1");
  slider->SetRange(min, max);
  slider->SetResolution(resolution);
  this->Script("pack %s -side left -fill x -expand t", slider->GetWidgetName());

  // Command to update the UI.
  this->CancelCommands->AddString("%s SetValue [%s %s]",
                  slider->GetTclName(), tclName, getCmd); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s [%s GetValue]",
                  this->GetTclName(), tclName, setCmd, slider->GetTclName());

  frame->Delete();
  labelWidget->Delete();
  slider->Delete();

  // Although it has been deleted, it did not destruct.
  // We need to change this into a PVWidget.
  return slider;
}

//----------------------------------------------------------------------------
vtkPVSelectionList *vtkPVSource::AddModeList(char *label, char *setCmd, char *getCmd,
                                             vtkKWObject *o)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  labelWidget = vtkKWLabel::New();
  this->Widgets->AddItem(labelWidget);
  labelWidget->SetParent(frame);
  labelWidget->Create(this->Application, "-width 18 -justify right");
  labelWidget->SetLabel(label);
  this->Script("pack %s -side left", labelWidget->GetWidgetName());

  vtkPVSelectionList *sl = vtkPVSelectionList::New();  
  this->Widgets->AddItem(sl);
  sl->SetParent(frame);
  sl->Create(this->Application);  
  this->Script("pack %s -fill x -expand t", sl->GetWidgetName());
    
  // Command to update the UI.
  this->CancelCommands->AddString("%s SetCurrentValue [%s %s]",
                   sl->GetTclName(), tclName, getCmd); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s [%s GetCurrentValue]",
                   this->GetTclName(), tclName, setCmd, sl->GetTclName());
    
  // Save this selection list so the user can add items to it.
  if (this->LastSelectionList)
    {
    this->LastSelectionList->UnRegister(this);
    }
  sl->Register(this);
  this->LastSelectionList = sl;

  sl->Delete();
  labelWidget->Delete();
  frame->Delete();

  // Although it has been deleted, it did not destruct.
  // We need to change this into a PVWidget.
  return sl;
}

//----------------------------------------------------------------------------
void vtkPVSource::AddModeListItem(char *name, int value)
{
  if (this->LastSelectionList == NULL)
    {
    vtkErrorMacro("No selection list exists yet.");
    return;
    }
  this->LastSelectionList->AddItem(name, value);
}


