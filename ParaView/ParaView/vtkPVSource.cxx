/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSource.cxx
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
#include "vtkPVSource.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWCompositeCollection.h"
#include "vtkKWRotateCameraInteractor.h"
#include "vtkSource.h"
#include "vtkPVData.h"
#include "vtkPVApplication.h"
#include "vtkKWView.h"
#include "vtkPVScale.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPVSelectionList.h"
#include "vtkStringList.h"
#include "vtkCollection.h"
#include "vtkPVData.h"
#include "vtkPVSourceInterface.h"
#include "vtkPVGlyph3D.h"
#include "vtkObjectFactory.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkPVInputMenu.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVArraySelection.h"
#include "vtkUnstructuredGridSource.h"
#include "vtkPVLabeledToggle.h"
#include "vtkPVFileEntry.h"
#include "vtkPVStringEntry.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVBoundsDisplay.h"
#include "vtkPVScalarRangeLabel.h"
#include "vtkKWEvent.h"
#include "vtkKWNotebook.h"
#include "vtkCallbackCommand.h"

int vtkPVSourceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSource::vtkPVSource()
{
  static int instanceCount = 0;
  
  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;
  
  this->CommandFunction = vtkPVSourceCommand;
  this->Name = NULL;

  this->Initialized = 0;
  
  this->Notebook = vtkKWNotebook::New();

  this->PVInputs = NULL;
  this->NumberOfPVInputs = 0;
  this->PVOutputs = NULL;
  this->NumberOfPVOutputs = 0;

  this->VTKSource = NULL;
  this->VTKSourceTclName = NULL;

  this->Properties = vtkKWWidget::New();
  
  this->ParameterFrame = vtkKWLabeledFrame::New();
  this->AcceptButton = vtkKWPushButton::New();
  this->ResetButton = vtkKWPushButton::New();
  this->DeleteButton = vtkKWPushButton::New();
  this->DisplayNameLabel = vtkKWLabel::New();
      
  this->Widgets = vtkKWWidgetCollection::New();
  this->LastSelectionList = NULL;
    
  this->Interface = NULL;

  this->ExtentTranslatorTclName = NULL;

  this->ReplaceInput = 1;

  this->InputMenu = NULL; 
  this->PropertiesParent = NULL;
  this->View = NULL;

  this->VisitedFlag = 0;
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
  
  this->RemoveAllPVInputs();

  this->SetVTKSource(NULL, NULL);

  this->SetName(NULL);

  this->Notebook->Delete();
  this->Notebook = NULL;

  this->Widgets->Delete();
  this->Widgets = NULL;

  this->AcceptButton->Delete();
  this->AcceptButton = NULL;  
  
  this->ResetButton->Delete();
  this->ResetButton = NULL;  
  
  this->DeleteButton->Delete();
  this->DeleteButton = NULL;
        
  this->DisplayNameLabel->Delete();
  this->DisplayNameLabel = NULL;
  
  this->ParameterFrame->Delete();
  this->ParameterFrame = NULL;

  this->Properties->Delete();
  this->Properties = NULL;
    
  if (this->LastSelectionList)
    {
    this->LastSelectionList->UnRegister(this);
    this->LastSelectionList = NULL;
    }

  this->SetInterface(NULL);

  if (this->ExtentTranslatorTclName)
    {
    this->GetPVApplication()->BroadcastScript("%s Delete", 
                                this->ExtentTranslatorTclName);
    this->SetExtentTranslatorTclName(NULL);
    }

  if (this->InputMenu)
    {
    this->InputMenu->UnRegister(this);
    this->InputMenu = NULL;
    }

  if (this->PropertiesParent)
    {
    this->PropertiesParent->UnRegister(this);
    this->PropertiesParent = NULL;
    }
  this->SetView(NULL);

}

//----------------------------------------------------------------------------
vtkPVSource* vtkPVSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVSource");
  if(ret)
    {
    return (vtkPVSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVSource;
}


//----------------------------------------------------------------------------
void vtkPVSource::SetPVInput(vtkPVData *pvd)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp == NULL)
    {
    vtkErrorMacro("No Application. Create the source before setting the input.");
    return;
    }

  this->SetNthPVInput(0, pvd);

  pvApp->BroadcastScript("%s SetInput %s", this->GetVTKSourceTclName(),
                         pvd->GetVTKDataTclName());
}

//----------------------------------------------------------------------------
void vtkPVSource::SetPVOutput(vtkPVData *pvd)
{
  this->SetNthPVOutput(0, pvd);
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
vtkPVWindow* vtkPVSource::GetPVWindow()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (pvApp == NULL)
    {
    return NULL;
    }
  
  return pvApp->GetMainWindow();
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
  char displayName[256];
  vtkPVApplication *app = this->GetPVApplication();
  

  // If the user has not set the properties parent.
  if (this->PropertiesParent == NULL)
    {
    vtkErrorMacro("PropertiesParent has not been set.");
    }

  this->Notebook->SetParent(this->PropertiesParent);
  this->Notebook->Create(this->Application,"");

  //this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
  //             this->Notebook->GetWidgetName());

  // Set up the pages of the notebook.
  this->Notebook->AddPage("Parameters");
  this->Notebook->AddPage("Display");
  //  this->Notebook->SetMinimumHeight(500);
  this->Properties->SetParent(this->Notebook->GetFrame("Parameters"));
  this->Properties->Create(this->Application,"frame","");
  this->Script("pack %s -pady 2 -fill x -expand yes",
               this->Properties->GetWidgetName());

  // For initializing the trace of the notebook.
  this->GetPropertiesParent()->SetTraceReferenceObject(this);
  this->GetPropertiesParent()->SetTraceReferenceCommand("GetPropertiesParent");

  // Setup the source page of the notebook.

  this->DisplayNameLabel->SetParent(this->Properties);
  if (this->Name != NULL)
    {
    sprintf(displayName, "Name: %s Type: %s", this->GetName(),
	    this->GetVTKSource()->GetClassName()+3);
    this->DisplayNameLabel->SetLabel(displayName);
    }
  else
    {
    this->DisplayNameLabel->SetLabel("Name: ");
    }
  this->DisplayNameLabel->Create(app, "");
  this->Script("pack %s", this->DisplayNameLabel->GetWidgetName());
  
  this->ParameterFrame->SetParent(this->Properties);
  this->ParameterFrame->Create(this->Application);
  this->ParameterFrame->SetLabel("Parameters");
  this->Script("pack %s -fill x -expand t -side top", this->ParameterFrame->GetWidgetName());

  vtkKWWidget *frame = vtkKWWidget::New();
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());  
  
  this->AcceptButton->SetParent(frame);
  this->AcceptButton->Create(this->Application, "-text Accept -background red1");
  this->AcceptButton->SetCommand(this, "PreAcceptCallback");
  this->AcceptButton->SetBalloonHelpString("Cause the current values in the user interface to take effect");
  this->Script("pack %s -side left -fill x -expand t", 
	       this->AcceptButton->GetWidgetName());

  this->ResetButton->SetParent(frame);
  this->ResetButton->Create(this->Application, "-text Reset");
  this->ResetButton->SetCommand(this, "ResetCallback");
  this->ResetButton->SetBalloonHelpString("Revert to the previous values in the user interface.  If no values have been set, remove the filter from the pipeline.");
  this->Script("pack %s -side left -fill x -expand t", 
	       this->ResetButton->GetWidgetName());

  this->DeleteButton->SetParent(frame);
  this->DeleteButton->Create(this->Application, "-text Delete");
  this->DeleteButton->SetCommand(this, "DeleteCallback");
  this->DeleteButton->SetBalloonHelpString("Remove this filter from the pipeline.  This can only be done if the filter is at the end of the pipeline.");
  this->Script("pack %s -side left -fill x -expand t",
               this->DeleteButton->GetWidgetName());

  frame->Delete();  
  
  // Isolate events to this window until accept or reset is pressed.
  this->Script("grab set %s", this->ParameterFrame->GetWidgetName());
  
  this->UpdateProperties();
  
  //this->UpdateParameterWidgets();
}


//----------------------------------------------------------------------------
void vtkPVSource::Select()
{
  vtkPVData *data;
  
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->PropertiesParent->GetWidgetName());
  this->Script("pack %s -side top -fill x",
               this->GetPVRenderView()->GetNavigationFrame()->GetWidgetName());
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());

  this->UpdateProperties();
  // This may best be merged with the UpdateProperties call but ...
  // We make the call here to update the input menu, 
  // which is now just another pvWidget.
  this->UpdateParameterWidgets();
  
  // This assumes that a source only has one output.
  data = this->GetPVOutput();
  if (data)
    {
    // Update the Display page.
    data->UpdateProperties();
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::Deselect()
{
  vtkPVData *data;

  this->Script("pack forget %s", this->Notebook->GetWidgetName());

  data = this->GetPVOutput();
  if (data)
    {
    data->SetScalarBarVisibility(0);
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
  
  // Make sure the label is upto date.
  if (this->Name != NULL)
    {
    char displayName[512];
    sprintf(displayName, "Name: %s", this->GetName());
    this->DisplayNameLabel->SetLabel(displayName);
    }
  else
    {
    this->DisplayNameLabel->SetLabel("Name: ");
    }   
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
void vtkPVSource::PreAcceptCallback()
{
  this->Script("%s configure -cursor watch", this->GetPVWindow()->GetWidgetName());
  this->Script("after idle {%s AcceptCallback}", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVSource::AcceptCallback()
{
  this->Accept(0);
  this->GetPVApplication()->AddTraceEntry("$kw(%s) AcceptCallback",
                                          this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVSource::Accept(int hideFlag)
{
  vtkPVWindow *window;

  this->Script("update");

  window = this->GetPVWindow();

  this->SetAcceptButtonColorToWhite();
  
  // We need to pass the parameters from the UI to the VTK objects before
  // we check whether to insert ExtractPieces.  Otherwise, we'll get errors
  // about unspecified file names, etc., when ExecuteInformation is called on
  // the VTK source.  (The vtkPLOT3DReader is a good example of this.)
  this->UpdateVTKSourceParameters();
  
  // This adds an extract filter only when the MaximumNumberOfPieces is 1.
  // This is only the case the first time the accept is called.
  if (this->GetNthPVOutput(0))
    {
    this->GetNthPVOutput(0)->InsertExtractPiecesIfNecessary();
    }

  // Initialize the output if necessary.
  if ( ! this->Initialized)
    { // This is the first time, initialize data.    
    vtkPVData *input;
    vtkPVData *ac;
    
    ac = this->GetPVOutput(0);
    if (ac == NULL)
      { // I suppose we should try and delete the source.
      vtkErrorMacro("Could not get output.");
      this->DeleteCallback();    
      return;
      }
    
    window->GetMainView()->AddPVData(ac);
    ac->CreateProperties();
    ac->Initialize();
    // Make the last data invisible.
    input = this->GetPVInput();
    if (input)
      {
      if (this->ReplaceInput)
        {
        input->SetVisibility(0);
        }
      }
    // The best test I could come up with to only reset
    // the camera when the first source is created.
    if (window->GetSources()->GetNumberOfItems() == 1)
      {
      float bds[6];
      ac->GetBounds(bds);
      window->GetRotateCameraInteractor()->SetCenter(
                    0.5*(bds[0]+bds[1]), 
                    0.5*(bds[2]+bds[3]),
                    0.5*(bds[4]+bds[5]));
      window->GetMainView()->ResetCamera();
      }

    // Set the current data of the window.
    if ( ! hideFlag)
      {
      window->SetCurrentPVData(this->GetNthPVOutput(0));
      }
    else
      {
      this->SetVisibility(0);
      }

    // Remove the local grab
    this->Script("grab release %s", this->ParameterFrame->GetWidgetName());    
    this->Initialized = 1;
    }

  window->GetMenuProperties()->CheckRadioButton(
                                  window->GetMenuProperties(), "Radio", 2);
  this->UpdateProperties();
  this->GetPVRenderView()->EventuallyRender();

  // Update the selection menu.
  window->UpdateSelectMenu();
  
  // Regenerate the data property page in case something has changed.
  if (this->NumberOfPVOutputs > 0)
    {
    this->GetNthPVOutput(0)->UpdateProperties();
    }

  this->Script("update");  

  window->EnableMenus();

#ifdef _WIN32
  this->Script("%s configure -cursor arrow", window->GetWidgetName());
#else
  this->Script("%s configure -cursor left_ptr", window->GetWidgetName());
#endif  

}

//----------------------------------------------------------------------------
void vtkPVSource::ResetCallback()
{
  if ( ! this->Initialized)
    { // Accept has not been called yet.  Delete the object.
    // What about the local grab?
    this->DeleteCallback();
    return;
    }

  this->UpdateParameterWidgets();
  this->Script("update");

  this->SetAcceptButtonColorToWhite();
}

//---------------------------------------------------------------------------
void vtkPVSource::DeleteCallback()
{
  int i;
  vtkPVData *ac;
  vtkPVSource *prev = NULL;
  vtkPVWindow *window = this->GetPVWindow();

  // Just in case cursor was left in a funny state.
#ifdef _WIN32
  this->Script("%s configure -cursor arrow", window->GetWidgetName());
#else
  this->Script("%s configure -cursor left_ptr", window->GetWidgetName());
#endif  

  if ( ! this->Initialized)
    {
    // Remove the local grab
    this->Script("grab release %s", this->ParameterFrame->GetWidgetName()); 
    this->Script("update");   
    this->Initialized = 1;
    }
  
  for (i = 0; i < this->NumberOfPVOutputs; ++i)
    {
    if (this->PVOutputs[i] && 
        this->PVOutputs[i]->GetNumberOfPVConsumers() > 0)
      { // Button should be deactivated.
      vtkErrorMacro("An output is used.  We cannot delete this source.");
      return;
      }
    }
  
  // Save this action in the trace file.
  this->GetPVApplication()->AddTraceEntry("$kw(%s) DeleteCallback",
                                          this->GetTclName());

  // Get the input so we can make it visible and make it current.
  if (this->GetNumberOfPVInputs() > 0)
    {
    prev = this->PVInputs[0]->GetPVSource();
    // Just a sanity check
    if (prev == NULL)
      {
      vtkErrorMacro("Expecting an input but none found.");
      }
    else
      {
      prev->SetVisibility(1);
      }
    }

  // Remove this source from the inputs users collection.
  for (i = 0; i < this->GetNumberOfPVInputs(); i++)
    {
    if (this->PVInputs[i])
      {
      this->PVInputs[i]->RemovePVConsumer(this);
      }
    }
    
  // Look for a source to make current.
  if (prev == NULL)
    {
    prev = this->GetPVWindow()->GetPreviousPVSource();
    }
  this->GetPVWindow()->SetCurrentPVSource(prev);
  if (prev == NULL)
    {
    // Unpack the properties.  This is required if prev is NULL.
    this->Script("catch {eval pack forget [pack slaves %s]}",
		 this->PropertiesParent->GetWidgetName());
    }
  else
    {
    //prev->GetPVOutput(0)->VisibilityOn();
    //prev->ShowProperties();
    }
        
  // Remove all of the actors mappers. from the renderer.
  for (i = 0; i < this->NumberOfPVOutputs; ++i)
    {
    if (this->PVOutputs[i])
      {
      ac = this->GetPVOutput(i);
      this->GetPVRenderView()->RemovePVData(ac);
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
  
  this->GetPVRenderView()->EventuallyRender();

  this->GetPVWindow()->EnableMenus();
  
  // This should delete this source.
  this->GetPVWindow()->RemovePVSource(this);
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateParameterWidgets()
{
  int i;
  vtkKWWidget *widget;
  vtkPVWidget *pvw;
  
  this->Widgets->InitTraversal();
  for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
    {
    widget = this->Widgets->GetNextKWWidget();
    pvw = vtkPVWidget::SafeDownCast(widget);
    if (pvw)
      {
      pvw->Reset();
      }
    }
}


//----------------------------------------------------------------------------
// This should be apart of AcceptCallback.
void vtkPVSource::UpdateVTKSourceParameters()
{
  int i;
  vtkKWWidget *widget;
  vtkPVWidget *pvw;
  
  this->Widgets->InitTraversal();
  for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
    {
    widget = this->Widgets->GetNextKWWidget();
    pvw = vtkPVWidget::SafeDownCast(widget);

    // We should be calling Accept on the widgets regardless of whether they
    // were modified.
    if (pvw)
      {
      pvw->Accept();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateProperties()
{
  //int num, idx;
  //vtkPVData *input;
  
  // --------------------------------------
  // Change the state of the delete button based on if there are any users.
  // Only filters at the end of a pipeline can be deleted.
  if (this->GetPVOutput(0) &&
      this->GetPVOutput(0)->GetNumberOfPVConsumers() > 0)
      {
      this->Script("%s configure -state disabled",
                   this->DeleteButton->GetWidgetName());
      }
    else
      {
      this->Script("%s configure -state normal",
                   this->DeleteButton->GetWidgetName());
      }
  
  this->GetPVRenderView()->UpdateNavigationWindow(this);
  
  // I do not know why the inputs have to be updated.
  // I am changing it to output as an experiment.
  // The output might have been already updated elsewhere.
  if (this->GetPVOutput())
    {
    // Update the VTK data.
    this->GetPVOutput()->Update();
    }
}

void vtkPVSource::SetPropertiesParent(vtkKWWidget *parent)
{
  if (this->PropertiesParent == parent)
    {
    return;
    }
  if (this->PropertiesParent)
    {
    vtkErrorMacro("Cannot reparent properties.");
    return;
    }
  this->PropertiesParent = parent;
  parent->Register(this);
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
    this->PVInputs[idx]->SetVisibility(1);
    this->PVInputs[idx]->RemovePVConsumer(this);
    this->PVInputs[idx]->UnRegister(this);
    this->PVInputs[idx] = NULL;
    }
  
  if (pvd)
    {
    if (this->ReplaceInput)
      {
      pvd->SetVisibility(0);
      }
    pvd->Register(this);
    pvd->AddPVConsumer(this);
    this->PVInputs[idx] = pvd;
    }

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
void vtkPVSource::SaveInTclScript(ofstream *file)
{
  int i, numWidgets;
  vtkPVSourceInterface *pvsInterface = NULL;
  vtkPVWidget *widget;

  // Detect special sources we do not handle yet.
  if (this->VTKSource == NULL)
    {
    return;
    }

  // This should not be needed, but We can check anyway.
  if (this->VisitedFlag == 2)
    {
    return;
    }

  // Special condition to detect loops in the pipeline.
  // Visited 1 means that source is in stack, 2 means that
  // the tcl script contains the source.
  if (this->VisitedFlag == 1)
    { // This source is already in in the stack, but there is a loop.
    *file << "\n" << this->VTKSource->GetClassName() << " "
          << this->VTKSourceTclName << "\n";
    this->VisitedFlag = 2;
    return;
    }

  // This is the recursive part.
  this->VisitedFlag = 1;
  // Loop through all of the inputs
  for (i = 0; i < this->NumberOfPVInputs; ++i)
    {
    if (this->PVInputs[i] && this->PVInputs[i]->GetPVSource()->GetVisitedFlag() != 2)
      {
      this->PVInputs[i]->GetPVSource()->SaveInTclScript(file);
      }
    }
  
  // Save the object in the script.
  if (this->VisitedFlag != 2)
    {
    *file << "\n" << this->VTKSource->GetClassName() << " "
          << this->VTKSourceTclName << "\n";
    this->VisitedFlag = 2;
    }

  // Let the PVWidgets set up the object.
  numWidgets = this->Widgets->GetNumberOfItems();
  for (i = 0; i < numWidgets; i++)
    {
    widget = vtkPVWidget::SafeDownCast(this->Widgets->GetItemAsObject(i));
    if (widget)
      {
      widget->SaveInTclScript(file);
      }
    }

  // Add the mapper, actor, scalar bar actor ...
  this->GetPVOutput()->SaveInTclScript(file);
}


//----------------------------------------------------------------------------
void vtkPVSource::AddPVWidget(vtkPVWidget *pvw)
{
  char str[512];
  this->Widgets->AddItem(pvw);  

  if (pvw->GetTraceName() == NULL)
    {
    vtkWarningMacro("TraceName not set.");
    return;
    }

  pvw->SetTraceReferenceObject(this);
  sprintf(str, "GetPVWidget {%s}", pvw->GetTraceName());
  pvw->SetTraceReferenceCommand(str);
}



//----------------------------------------------------------------------------
vtkPVInputMenu *vtkPVSource::AddInputMenu(char *label, char *inputName, char *inputType,
                                          char *help, vtkCollection *sources)
{
  vtkPVInputMenu *inputMenu;

  inputMenu = vtkPVInputMenu::New();
  inputMenu->SetPVSource(this);
  inputMenu->SetCurrentValue(this->GetPVWindow()->GetCurrentPVSource());
  inputMenu->SetSources(sources);
  inputMenu->SetParent(this->ParameterFrame->GetFrame());
  inputMenu->SetLabel(label);
  inputMenu->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  inputMenu->Create(this->Application);
  inputMenu->SetInputName(inputName);
  inputMenu->SetInputType(inputType);
  inputMenu->SetBalloonHelpString(help);

  this->Script("pack %s -side top -fill x -expand t",
               inputMenu->GetWidgetName());
  this->AddPVWidget(inputMenu);

  if (this->InputMenu == NULL)
    {
    this->InputMenu = inputMenu;
    inputMenu->Register(this);
    }

  inputMenu->Delete();
  return inputMenu;
}

//----------------------------------------------------------------------------
vtkPVBoundsDisplay* vtkPVSource::AddBoundsDisplay(vtkPVInputMenu *inputMenu)
{
  vtkPVBoundsDisplay* boundsDisplay;

  if (inputMenu == NULL)
    {
    vtkErrorMacro("Bounds display must have an associated input menu.");
    return NULL;
    }

  // Format the label
  boundsDisplay = vtkPVBoundsDisplay::New();
  boundsDisplay->SetParent(this->GetParameterFrame()->GetFrame());
  boundsDisplay->Create(this->Application);
  // This assumes there will be no more than one bounds display per source.
  boundsDisplay->SetTraceName("BoundsDisplay");
  // Here we assume the data set name is "Input".  Generalize it when needed.
  boundsDisplay->GetWidget()->SetLabel("Input Bounds");
  boundsDisplay->SetInputMenu(inputMenu);
  inputMenu->AddDependant(boundsDisplay);
  this->Script("pack %s -side top -fill x",
               boundsDisplay->GetWidgetName());
  this->AddPVWidget(boundsDisplay);
  boundsDisplay->Delete();
  return boundsDisplay;
}

//----------------------------------------------------------------------------
vtkPVScalarRangeLabel* vtkPVSource::AddScalarRangeLabel(vtkPVArrayMenu *arrayMenu)
{
  vtkPVScalarRangeLabel* rangeLabel;

  if (arrayMenu == NULL)
    {
    vtkErrorMacro("Range label must have an associated array menu.");
    return NULL;
    }

  rangeLabel = vtkPVScalarRangeLabel::New();
  rangeLabel->SetArrayMenu(arrayMenu);
  arrayMenu->AddDependant(rangeLabel);
  rangeLabel->SetParent(this->GetParameterFrame()->GetFrame());
  rangeLabel->Create(this->Application);
  // This assumes there will be no more than one range label per source.
  rangeLabel->SetTraceName("ScalarRangeLabel");
  this->Script("pack %s", rangeLabel->GetWidgetName());
  this->AddPVWidget(rangeLabel);
  rangeLabel->Delete();
  return rangeLabel;
}


//----------------------------------------------------------------------------
vtkPVArrayMenu *vtkPVSource::AddArrayMenu(const char *label, 
                                          int attributeType,
                                          int numComponenets,
                                          const char *help)
{
  vtkPVArrayMenu *arrayMenu;

  if (this->InputMenu == NULL)
    {
    vtkErrorMacro("Could not find the input menu.");
    return NULL;
    }

  arrayMenu = vtkPVArrayMenu::New();
  arrayMenu->SetNumberOfComponents(numComponenets);
  arrayMenu->SetInputName("Input");
  arrayMenu->SetAttributeType(attributeType);
  arrayMenu->SetObjectTclName(this->GetVTKSourceTclName());

  arrayMenu->SetParent(this->ParameterFrame->GetFrame());
  arrayMenu->SetLabel(label);
  arrayMenu->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  arrayMenu->Create(this->Application);
  arrayMenu->SetBalloonHelpString(help);

  // Set up the dependancy so that the array menu updates when the input changes.
  this->InputMenu->AddDependant(arrayMenu);
  arrayMenu->SetInputMenu(this->InputMenu);  

  this->Script("pack %s -side top -fill x -expand t",
               arrayMenu->GetWidgetName());
  this->AddPVWidget(arrayMenu);
  arrayMenu->Delete();
  return arrayMenu;
}

//----------------------------------------------------------------------------
vtkPVArraySelection *vtkPVSource::AddArraySelection(const char *attributeName, 
                                                    const char *help)
{
  vtkPVArraySelection *pvw;
  char traceName[200];

  pvw = vtkPVArraySelection::New();
  pvw->SetAttributeName(attributeName);
  pvw->SetVTKReaderTclName(this->GetVTKSourceTclName());
  sprintf(traceName, "%sArraySelection", attributeName);
  pvw->SetTraceName(traceName);

  pvw->SetParent(this->ParameterFrame->GetFrame());
  pvw->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  pvw->Create(this->Application);
  pvw->SetBalloonHelpString(help);

  this->Script("pack %s -side top -fill x -expand t",
               pvw->GetWidgetName());
  this->AddPVWidget(pvw);
  pvw->Delete();
  return pvw;
}

//----------------------------------------------------------------------------
vtkPVLabeledToggle *vtkPVSource::AddLabeledToggle(char *label, char *varName, 
                                                  char* help)
{
  vtkPVLabeledToggle *toggle = vtkPVLabeledToggle::New();

  toggle->SetParent(this->ParameterFrame->GetFrame());
  toggle->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  toggle->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  toggle->SetLabel(label);
  toggle->Create(this->Application, help);

  this->Script("pack %s -fill x -expand t", toggle->GetWidgetName());
  
  this->AddPVWidget(toggle);
  toggle->Delete();

  // Although it has been deleted, it did not destruct.
  return toggle;
}

//----------------------------------------------------------------------------
vtkPVFileEntry *vtkPVSource::AddFileEntry(char *label, char *varName,
                                          char *ext, char *help)
{
  vtkPVFileEntry *entry;

  entry = vtkPVFileEntry::New();
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  entry->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  entry->Create(this->Application, label, ext, help);

  this->Script("pack %s -fill x -expand t", entry->GetWidgetName());

  this->AddPVWidget(entry);
  entry->Delete();

  // Although it has been deleted, it did not destruct.
  return entry;
}

//----------------------------------------------------------------------------
vtkPVStringEntry *vtkPVSource::AddStringEntry(char *label, char *varName, 
                                              char *help)
{
  vtkPVStringEntry *entry;

  entry = vtkPVStringEntry::New();
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  entry->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  entry->Create(this->Application, label, help);

  this->Script("pack %s -fill x -expand t", entry->GetWidgetName());

  this->AddPVWidget(entry);
  entry->Delete();

  // Although it has been deleted, it did not destruct.
  return entry;
}

//----------------------------------------------------------------------------
vtkPVVectorEntry *vtkPVSource::AddLabeledEntry(char *label, char *varName, 
                                               char* help)
{
  vtkPVVectorEntry *entry;

  entry = vtkPVVectorEntry::New();
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  entry->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  entry->Create(this->Application, label, 1, NULL, help);
  this->Script("pack %s -fill x -expand t", entry->GetWidgetName());

  this->AddPVWidget(entry);
  entry->Delete();

  // Although it has been deleted, it did not destruct.
  return entry;
}

//----------------------------------------------------------------------------
vtkPVVectorEntry* vtkPVSource::AddVector2Entry(char *label, char *l1, char *l2,
                                               char *varName, char *help)
{
  vtkPVVectorEntry *entry;
  char* subLabels[2];
  int i;
  
  subLabels[0] = new char[strlen(l1) + 1];
  strcpy(subLabels[0], l1);
  subLabels[1] = new char[strlen(l2) + 1];
  strcpy(subLabels[1], l2);
  entry = vtkPVVectorEntry::New();
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  entry->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  entry->Create(this->Application, label, 2, subLabels, help);

  this->Script("pack %s -fill x -expand t", entry->GetWidgetName());
  this->AddPVWidget(entry);
  entry->Delete();
  
  for (i = 0; i < 2; i++)
    {
    delete [] subLabels[i];
    }

  return entry;
}

//----------------------------------------------------------------------------
vtkPVVectorEntry* vtkPVSource::AddVector3Entry(char *label, char *l1, char *l2,
                                               char *l3, char *varName, 
                                               char* help)
{
  vtkPVVectorEntry *entry;
  char* subLabels[3];
  int i;
  
  subLabels[0] = new char[strlen(l1) + 1];
  strcpy(subLabels[0], l1);
  subLabels[1] = new char[strlen(l2) + 1];
  strcpy(subLabels[1], l2);
  subLabels[2] = new char[strlen(l3) + 1];
  strcpy(subLabels[2], l3);
  entry = vtkPVVectorEntry::New();
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  entry->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  entry->Create(this->Application, label, 3, subLabels, help);

  this->Script("pack %s -fill x -expand t", entry->GetWidgetName());
  this->AddPVWidget(entry);
  entry->Delete();
  
  for (i = 0; i < 3; i++)
    {
    delete [] subLabels[i];
    }
  
  return entry;
}


//----------------------------------------------------------------------------
vtkPVVectorEntry* vtkPVSource::AddVector4Entry(char *label, char *l1, char *l2,
                                               char *l3, char *l4,
                                               char *varName, char* help)
{
  vtkPVVectorEntry* entry;
  char* subLabels[4];
  int i;

  subLabels[0] = new char[strlen(l1) + 1];
  strcpy(subLabels[0], l1);
  subLabels[1] = new char[strlen(l2) + 1];
  strcpy(subLabels[1], l2);
  subLabels[2] = new char[strlen(l3) + 1];
  strcpy(subLabels[2], l3);
  subLabels[3] = new char[strlen(l4) + 1];
  strcpy(subLabels[3], l4);

  entry = vtkPVVectorEntry::New();
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  entry->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  entry->Create(this->Application, label, 4, subLabels, help);

  this->Script("pack %s -fill x -expand t", entry->GetWidgetName());
  this->AddPVWidget(entry);
  entry->Delete();
  
  for (i = 0; i < 4; i++)
    {
    delete [] subLabels[i];
    }
  
  return entry;
}

//----------------------------------------------------------------------------
// It might make sense here to store the labels in an array 
// so that a loop can be used to create the widgets.
vtkPVVectorEntry* vtkPVSource::AddVector6Entry(char *label, char *l1, char *l2,
                                               char *l3, char *l4, char *l5,
                                               char *l6, char *varName, 
                                               char *help)

{
  vtkPVVectorEntry *entry;
  char* subLabels[6];
  int i;

  subLabels[0] = new char[strlen(l1) + 1];
  strcpy(subLabels[0], l1);
  subLabels[1] = new char[strlen(l2) + 1];
  strcpy(subLabels[1], l2);
  subLabels[2] = new char[strlen(l3) + 1];
  strcpy(subLabels[2], l3);
  subLabels[3] = new char[strlen(l4) + 1];
  strcpy(subLabels[3], l4);
  subLabels[4] = new char[strlen(l5) + 1];
  strcpy(subLabels[4], l5);
  subLabels[5] = new char[strlen(l6) + 1];
  strcpy(subLabels[5], l6);

  entry = vtkPVVectorEntry::New();
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  entry->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  entry->Create(this->Application, label, 6, subLabels, help);

  this->Script("pack %s -fill x -expand t", entry->GetWidgetName());
  this->AddPVWidget(entry);
  entry->Delete();
  
  for (i = 0; i < 6; i++)
    {
    delete [] subLabels[i];
    }
  
  return entry;
}


//----------------------------------------------------------------------------
vtkPVScale *vtkPVSource::AddScale(char *label, char *varName,
                                  float min, float max, float resolution,
                                  char* help)
{
  vtkPVScale *scale;

  scale = vtkPVScale::New();
  scale->SetParent(this->ParameterFrame->GetFrame());
  scale->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  scale->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  scale->Create(this->Application, label, min, max, resolution, help);

  this->Script("pack %s -fill x -expand t", scale->GetWidgetName());

  this->AddPVWidget(scale);
  scale->Delete();

  // Although it has been deleted, it did not destruct.
  return scale;
}

//----------------------------------------------------------------------------
vtkPVSelectionList *vtkPVSource::AddModeList(char *label, char *varName, 
                                             char *help)
{
  vtkPVSelectionList *sl = vtkPVSelectionList::New();  
  sl->SetParent(this->ParameterFrame->GetFrame());
  sl->SetLabel(label);
  sl->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  sl->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  sl->Create(this->Application);

  if (help)
    {
    sl->SetBalloonHelpString(help);
    }
  this->Script("pack %s -fill x -expand t", sl->GetWidgetName());    

  // Save this selection list so the user can add items to it.
  if (this->LastSelectionList)
    {
    this->LastSelectionList->UnRegister(this);
    }
  sl->Register(this);
  this->LastSelectionList = sl;

  this->AddPVWidget(sl);
  sl->Delete();

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

//----------------------------------------------------------------------------
void vtkPVSource::SetAcceptButtonColorToRed()
{
  this->Script("%s configure -background red1",
               this->AcceptButton->GetWidgetName());
}

void vtkPVSource::SetAcceptButtonColorToWhite()
{
#ifdef _WIN32
  this->Script("%s configure -background SystemButtonFace",
               this->AcceptButton->GetWidgetName());
#else
  this->Script("%s configure -background #d9d9d9",
               this->AcceptButton->GetWidgetName());
#endif
}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVSource::GetPVWidget(const char *name)
{
  vtkObject *o;
  vtkPVWidget *pvw;
  this->Widgets->InitTraversal();

  while ( (o = this->Widgets->GetNextItemAsObject()) )
    {
    pvw = vtkPVWidget::SafeDownCast(o);
    if (pvw && pvw->GetTraceName() && strcmp(pvw->GetTraceName(), name) == 0)
      {
      return pvw;
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVSource::GetPVRenderView()
{
  return vtkPVRenderView::SafeDownCast(this->View);
}
