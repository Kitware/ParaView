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
#include "vtkUnstructuredGridSource.h"
#include "vtkPVArraySelection.h"
#include "vtkPVLabeledToggle.h"
#include "vtkPVFileEntry.h"
#include "vtkPVStringEntry.h"
#include "vtkPVVectorEntry.h"

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
  char displayName[256];
  vtkPVApplication *app = this->GetPVApplication();
  
  // invoke super
  this->vtkKWComposite::CreateProperties();  

  // Set up the pages of the notebook.
  this->Notebook->AddPage("Parameters");
  this->Notebook->AddPage("Display");
  //  this->Notebook->SetMinimumHeight(500);
  this->Properties->SetParent(this->Notebook->GetFrame("Parameters"));
  this->Properties->Create(this->Application,"frame","");
  this->Script("pack %s -pady 2 -fill x -expand yes",
               this->Properties->GetWidgetName());
  
  // Setup the source page of the notebook.

  this->DisplayNameLabel->SetParent(this->Properties);
  if (this->Name != NULL)
    {
    sprintf(displayName, "Name: %s", this->GetName());
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
void vtkPVSource::Select(vtkKWView *v)
{
  vtkPVData *data;
  
  // invoke super
  this->vtkKWComposite::Select(v); 
  
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->View->GetPropertiesParent()->GetWidgetName());
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
void vtkPVSource::PreAcceptCallback()
{
  this->Script("%s configure -cursor watch", this->GetPVWindow()->GetWidgetName());
  this->Script("after idle {%s AcceptCallback}", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVSource::AcceptCallback()
{
  this->Accept();

  this->GetPVApplication()->AddTraceEntry("$pv(%s) AcceptCallback",
                                          this->GetTclName());
}

void vtkPVSource::Accept()
{
  vtkPVWindow *window;

  this->Script("update");

  window = this->GetPVWindow();

#ifdef _WIN32
  this->Script("%s configure -background SystemButtonFace",
               this->AcceptButton->GetWidgetName());
#else
  this->Script("%s configure -background #d9d9d9",
               this->AcceptButton->GetWidgetName());
#endif
  
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
    
    window->GetMainView()->AddComposite(ac);
    ac->CreateProperties();
    ac->Initialize();
    // Make the last data invisible.
    input = this->GetPVInput();
    if (input)
      {
      if (this->ReplaceInput)
        {
        input->SetVisibility(0);
        input->GetVisibilityCheck()->SetState(0);
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
    window->SetCurrentPVData(this->GetNthPVOutput(0));
    
    // Remove the local grab
    this->Script("grab release %s", this->ParameterFrame->GetWidgetName());    
    this->Initialized = 1;
    }

  window->GetMainView()->SetSelectedComposite(this);
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

#ifdef _WIN32
  this->Script("%s configure -background SystemButtonFace",
               this->AcceptButton->GetWidgetName());
#else
  this->Script("%s configure -background #d9d9d9",
               this->AcceptButton->GetWidgetName());
#endif
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
  this->GetPVApplication()->AddTraceEntry("$pv(%s) DeleteCallback",
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
      prev->VisibilityOn();
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
		 this->View->GetPropertiesParent()->GetWidgetName());
    }
  else
    {
    //prev->GetPVOutput(0)->VisibilityOn();
    //prev->ShowProperties();
    }
      
  // We need to remove this source from the SelectMenu
  this->GetPVWindow()->GetSources()->RemoveItem(this);
  this->GetPVWindow()->UpdateSelectMenu();
  
  // Remove all of the actors mappers. from the renderer.
  for (i = 0; i < this->NumberOfPVOutputs; ++i)
    {
    if (this->PVOutputs[i])
      {
      ac = this->GetPVOutput(i);
      this->GetPVWindow()->GetMainView()->RemoveComposite(ac);
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
  this->GetPVWindow()->GetMainView()->RemoveComposite(this);
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
    if (pvw && pvw->GetModifiedFlag())
      {
      if ( ! pvw->GetTraceVariableInitialized())
        {
        this->GetPVApplication()->AddTraceEntry(
                         "set pv(%s) [$pv(%s) GetPVWidget {%s}]",
                         pvw->GetTclName(), this->GetTclName(),
                         pvw->GetName());
        pvw->SetTraceVariableInitialized(1);
        }
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
  
  this->GetPVWindow()->GetMainView()->UpdateNavigationWindow(this);
  
  // Make sure all the inputs are up to date.
  //num = this->GetNumberOfPVInputs();
  //for (idx = 0; idx < num; ++idx)
  //  {
  //  input = this->GetNthPVInput(idx);
  //  input->Update();
  //  }

  // I do not know why the inputs have to be updated.
  // I am changing it to output as an experiment.
  // The output might have been already updated elsewhere.
  if (this->GetPVOutput(0))
    {
    this->GetPVOutput(0)->Update();
    }

  //this->UpdateScalarsMenu();
  //this->UpdateVectorsMenu();

  if (this->Interface)
    {
    if (this->Interface->GetDefaultScalars())
      {
      // ....
      }
    if (this->Interface->GetDefaultScalars())
      {
      // ....
      }
    }
}
  
//----------------------------------------------------------------------------
// Why do we need this.  Isn't show properties and Raise handled by window?
void vtkPVSource::SelectSource(vtkPVSource *source)
{
  if (source)
    {
    this->GetPVWindow()->SetCurrentPVSource(source);
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
      pvd->GetVisibilityCheck()->SetState(0);
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
  char tclName[256];
  char sourceTclName[256];
  char* tempName;
  char* extension;
  int pos, i, numWidgets;
  vtkPVSourceInterface *pvsInterface = NULL;
  vtkPVWidget *widget;
  
  if (this->GetPVInput())
    {
    pvsInterface = this->GetPVInput()->GetPVSource()->GetInterface();
    }
  
  if (this->VTKSource)
    {
    *file << this->VTKSource->GetClassName() << " "
          << this->VTKSourceTclName << "\n";
    sprintf(tclName, this->VTKSourceTclName);
    }
  else if (strcmp(this->GetInterface()->GetSourceClassName(),
                  "vtkGenericEnSightReader") == 0)
    {
    extension = strrchr(this->Name, '_');
    pos = extension - this->Name;
    strncpy(tclName, this->Name, pos);
    tclName[pos] = '\0';
    this->Interface->SaveInTclScript(file, tclName);
    this->GetPVOutput(0)->SaveInTclScript(file, tclName);
    return;
    }
  else if (strcmp(this->GetInterface()->GetSourceClassName(),
                  "vtkPDataSetReader") == 0)
    {
    *file << "vtkPDataSetReader " << this->Name << "\n";
    sprintf(tclName, this->Name);
    }
  
  if (this->NumberOfPVInputs > 0)
    {
    *file << "\t" << tclName << " SetInput [";
    if (strcmp(this->GetPVInput()->GetPVSource()->GetInterface()->
               GetSourceClassName(), "vtkGenericEnSightReader") == 0)
      {
      char *charFound;
      int pos;
      char *dataName = new char[strlen(this->GetPVInput()->GetVTKDataTclName()) + 1];
      strcpy(dataName, this->GetPVInput()->GetVTKDataTclName());
      
      charFound = strrchr(dataName, 't');
      tempName = strtok(dataName, "O");
      *file << dataName << " GetOutput ";
      pos = charFound - dataName + 1;
      *file << dataName+pos << "]\n";
      delete [] dataName;
      }
    else if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                                    "vtkPDataSetReader") == 0)
      {
      char *dataName = new char[strlen(this->GetPVInput()->GetVTKDataTclName()) + 1];
      strcpy(dataName, this->GetPVInput()->GetVTKDataTclName());
      
      sprintf(sourceTclName, "DataSetReader");
      tempName = strtok(dataName, "O");
      strcat(sourceTclName, tempName+7);
      *file << sourceTclName << " GetOutput]\n";
      delete [] dataName;
      }
    else
      {
      *file << this->GetPVInput()->GetPVSource()->GetVTKSourceTclName()
            << " GetOutput]\n";
      }
    }

  if (this->Interface)
    {
    this->Interface->SaveInTclScript(file, tclName);
    }

  numWidgets = this->Widgets->GetNumberOfItems();
  for (i = 0; i < numWidgets; i++)
    {
    widget = vtkPVWidget::SafeDownCast(this->Widgets->GetItemAsObject(i));
    if (widget && (widget->IsA("vtkPVLabeledToggle") ||
                   widget->IsA("vtkPVFileEntry") ||
                   widget->IsA("vtkPVScale") ||
                   widget->IsA("vtkPVSelectionList") ||
                   widget->IsA("vtkPVStringEntry") ||
                   widget->IsA("vtkPVVectorEntry")) )
      {
      *file << "\t";
      widget->SaveInTclScript(file, tclName);
      }
    }
  
  *file << "\n";

  this->GetPVOutput(0)->SaveInTclScript(file, tclName);
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
  inputMenu->SetModifiedCommand(this->GetTclName(), "ChangeAcceptButtonColor");
  inputMenu->Create(this->Application);
  inputMenu->SetInputName(inputName);
  inputMenu->SetInputType(inputType);
  inputMenu->SetBalloonHelpString(help);
  this->Script("pack %s -side top -fill x -expand t",
               inputMenu->GetWidgetName());
  this->Widgets->AddItem(inputMenu);
  inputMenu->Delete();
  return inputMenu;
}

//----------------------------------------------------------------------------
vtkPVArrayMenu *vtkPVSource::AddArrayMenu(const char *label, 
                                          int attributeType,
                                          int numComponenets,
                                          const char *help)
{
  vtkPVArrayMenu *arrayMenu;

  arrayMenu = vtkPVArrayMenu::New();
  arrayMenu->SetDataSetCommand(this->GetVTKSourceTclName(), "GetInput");
  arrayMenu->SetNumberOfComponents(numComponenets);
  arrayMenu->SetInputName("Input");
  arrayMenu->SetAttributeType(attributeType);
  arrayMenu->SetObjectTclName(this->GetVTKSourceTclName());

  arrayMenu->SetParent(this->ParameterFrame->GetFrame());
  arrayMenu->SetLabel(label);
  arrayMenu->SetModifiedCommand(this->GetTclName(), "ChangeAcceptButtonColor");
  arrayMenu->Create(this->Application);
  arrayMenu->SetBalloonHelpString(help);
  this->Script("pack %s -side top -fill x -expand t",
               arrayMenu->GetWidgetName());
  this->Widgets->AddItem(arrayMenu);
  arrayMenu->Delete();
  return arrayMenu;
}


//----------------------------------------------------------------------------
vtkPVLabeledToggle *vtkPVSource::AddLabeledToggle(char *label, char *varName, 
                                                  char* help)
{
  vtkPVLabeledToggle *toggle = vtkPVLabeledToggle::New();
  this->Widgets->AddItem(toggle);
  toggle->SetParent(this->ParameterFrame->GetFrame());
  toggle->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  toggle->SetModifiedCommand(this->GetTclName(), "ChangeAcceptButtonColor");
  toggle->SetLabel(label);
  toggle->Create(this->Application, help);

  this->Script("pack %s -fill x -expand t", toggle->GetWidgetName());
  
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
  this->Widgets->AddItem(entry);
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  entry->SetModifiedCommand(this->GetTclName(), "ChangeAcceptButtonColor");
  entry->Create(this->Application, label, ext, help);

  this->Script("pack %s -fill x -expand t", entry->GetWidgetName());

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
  this->Widgets->AddItem(entry);
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  entry->SetModifiedCommand(this->GetTclName(), "ChangeAcceptButtonColor");
  entry->Create(this->Application, label, help);

  this->Script("pack %s -fill x -expand t", entry->GetWidgetName());

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
  this->Widgets->AddItem(entry);
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  entry->SetModifiedCommand(this->GetTclName(), "ChangeAcceptButtonColor");
  entry->Create(this->Application, label, 1, NULL, help);
  this->Script("pack %s -fill x -expand t", entry->GetWidgetName());

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
  this->Widgets->AddItem(entry);
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  entry->SetModifiedCommand(this->GetTclName(), "ChangeAcceptButtonColor");
  entry->Create(this->Application, label, 2, subLabels, help);

  this->Script("pack %s -fill x -expand t", entry->GetWidgetName());
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
  this->Widgets->AddItem(entry);
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  entry->SetModifiedCommand(this->GetTclName(), "ChangeAcceptButtonColor");
  entry->Create(this->Application, label, 3, subLabels, help);

  this->Script("pack %s -fill x -expand t", entry->GetWidgetName());
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
  this->Widgets->AddItem(entry);
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  entry->SetModifiedCommand(this->GetTclName(), "ChangeAcceptButtonColor");
  entry->Create(this->Application, label, 4, subLabels, help);

  this->Script("pack %s -fill x -expand t", entry->GetWidgetName());
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
  this->Widgets->AddItem(entry);
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  entry->SetModifiedCommand(this->GetTclName(), "ChangeAcceptButtonColor");
  entry->Create(this->Application, label, 6, subLabels, help);

  this->Script("pack %s -fill x -expand t", entry->GetWidgetName());
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
  this->Widgets->AddItem(scale);
  scale->SetParent(this->ParameterFrame->GetFrame());
  scale->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  scale->SetModifiedCommand(this->GetTclName(), "ChangeAcceptButtonColor");
  scale->Create(this->Application, label, min, max, resolution, help);

  this->Script("pack %s -fill x -expand t", scale->GetWidgetName());

  scale->Delete();

  // Although it has been deleted, it did not destruct.
  return scale;
}

//----------------------------------------------------------------------------
vtkPVSelectionList *vtkPVSource::AddModeList(char *label, char *varName, 
                                             char *help)
{
  vtkPVSelectionList *sl = vtkPVSelectionList::New();  
  this->Widgets->AddItem(sl);
  sl->SetParent(this->ParameterFrame->GetFrame());
  sl->SetLabel(label);
  sl->SetObjectVariable(this->GetVTKSourceTclName(), varName);
  sl->SetModifiedCommand(this->GetTclName(), "ChangeAcceptButtonColor");
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
void vtkPVSource::ChangeAcceptButtonColor()
{
  this->Script("%s configure -background red1",
               this->AcceptButton->GetWidgetName());
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
    if (pvw && strcmp(pvw->GetName(), name) == 0)
      {
      return pvw;
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVSource::GetPVRenderView()
{
  return vtkPVRenderView::SafeDownCast(this->GetView());
}
