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
#include "vtkKWLabeledEntry.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWCompositeCollection.h"
#include "vtkKWRotateCameraInteractor.h"

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
#include "vtkPVGlyph3D.h"
#include "vtkObjectFactory.h"

#include "vtkUnstructuredGridSource.h"

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
  this->InputMenuFrame = vtkKWWidget::New();
  this->InputMenuLabel = vtkKWLabel::New();
  this->InputMenu = vtkPVInputMenu::New();
  this->ScalarOperationFrame = vtkKWWidget::New();
  this->ScalarOperationLabel = vtkKWLabel::New();
  this->ScalarOperationMenu = vtkKWOptionMenu::New();
  this->VectorOperationFrame = vtkKWWidget::New();
  this->VectorOperationLabel = vtkKWLabel::New();
  this->VectorOperationMenu = vtkKWOptionMenu::New();
  this->DisplayNameLabel = vtkKWLabel::New();
  
  this->ChangeScalarsFilterTclName = NULL;
  this->DefaultScalarsName = NULL;
  this->DefaultVectorsName = NULL;
  
  this->ExtractPieceTclName = NULL;
  
  this->Widgets = vtkKWWidgetCollection::New();
  this->LastSelectionList = NULL;
  
  this->AcceptCommands = vtkStringList::New();
  this->ResetCommands = vtkStringList::New();
  
  this->Interface = NULL;

  this->ExtentTranslatorTclName = NULL;
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
  
  this->InputMenuLabel->Delete();
  this->InputMenuLabel = NULL;
  
  this->InputMenu->Delete();
  this->InputMenu = NULL;
  
  this->InputMenuFrame->Delete();
  this->InputMenuFrame = NULL;

  this->ScalarOperationLabel->Delete();
  this->ScalarOperationLabel = NULL;

  this->ScalarOperationMenu->Delete();
  this->ScalarOperationMenu = NULL;
  
  this->ScalarOperationFrame->Delete();
  this->ScalarOperationFrame = NULL;

  this->VectorOperationLabel->Delete();
  this->VectorOperationLabel = NULL;

  this->VectorOperationMenu->Delete();
  this->VectorOperationMenu = NULL;
  
  this->VectorOperationFrame->Delete();
  this->VectorOperationFrame = NULL;

  this->DisplayNameLabel->Delete();
  this->DisplayNameLabel = NULL;
  
  this->ParameterFrame->Delete();
  this->ParameterFrame = NULL;

  this->Properties->Delete();
  this->Properties = NULL;

  if (this->ChangeScalarsFilterTclName)
    {
    this->GetPVApplication()->BroadcastScript("%s Delete", 
                                this->ChangeScalarsFilterTclName);
    delete [] this->ChangeScalarsFilterTclName;
    this->ChangeScalarsFilterTclName = NULL;
    }
  if (this->DefaultScalarsName)
    {
    delete [] this->DefaultScalarsName;
    this->DefaultScalarsName = NULL;
    }
  if (this->DefaultVectorsName)
    {
    delete [] this->DefaultVectorsName;
    this->DefaultVectorsName = NULL;
    }
  
  if (this->ExtractPieceTclName)
    {
    this->GetPVApplication()->BroadcastScript("%s Delete",
                                              this->ExtractPieceTclName);
    delete [] this->ExtractPieceTclName;
    this->ExtractPieceTclName = NULL;
    }
  
  if (this->LastSelectionList)
    {
    this->LastSelectionList->UnRegister(this);
    this->LastSelectionList = NULL;
    }

  this->AcceptCommands->Delete();
  this->AcceptCommands = NULL;  
  this->ResetCommands->Delete();
  this->ResetCommands = NULL;

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
  char displayName[256];
  vtkPVApplication *app = this->GetPVApplication();
  
  // invoke super
  this->vtkKWComposite::CreateProperties();  

  // Set up the pages of the notebook.
  this->Notebook->AddPage("Parameters");
  this->Notebook->AddPage("Display");
  this->Notebook->SetMinimumHeight(500);
  this->Properties->SetParent(this->Notebook->GetFrame("Parameters"));
  this->Properties->Create(this->Application,"frame","");
  this->Script("pack %s -pady 2 -fill x -expand yes",
               this->Properties->GetWidgetName());
  
  // Setup the source page of the notebook.

  this->DisplayNameLabel->SetParent(this->Properties);
  sprintf(displayName, "Name: %s", this->VTKSourceTclName);
  this->DisplayNameLabel->Create(app, "");
  this->DisplayNameLabel->SetLabel(displayName);
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
  this->AcceptButton->SetCommand(this, "AcceptCallback");
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
  
  if (this->GetNumberOfPVInputs() > 0)
    {
    this->InputMenuFrame->SetParent(this->ParameterFrame->GetFrame());
    this->InputMenuFrame->Create(this->Application, "frame", "");
    this->Script("pack %s -side top -fill x -expand t",
                 this->InputMenuFrame->GetWidgetName());
    
    this->InputMenuLabel->SetParent(this->InputMenuFrame);
    this->InputMenuLabel->Create(this->Application, "");
    this->InputMenuLabel->SetLabel("Input: ");
    this->InputMenuLabel->SetBalloonHelpString("Choose which data set to use as input to this filter");
    
    this->InputMenu->SetParent(this->InputMenuFrame);
    this->InputMenu->Create(this->Application, "");
    this->InputMenu->SetBalloonHelpString("Choose which data set to use as input to this filter");
    this->Script("pack %s %s -side left -fill x",
                 this->InputMenuLabel->GetWidgetName(),
                 this->InputMenu->GetWidgetName());
    }

  this->ScalarOperationFrame->SetParent(this->ParameterFrame->GetFrame());
  this->ScalarOperationFrame->Create(this->Application, "frame", "");    
  this->Script("pack %s -side top -fill x -expand t",
               this->ScalarOperationFrame->GetWidgetName());

  this->ScalarOperationLabel->SetParent(this->ScalarOperationFrame);
  this->ScalarOperationLabel->Create(this->Application, "");
  this->ScalarOperationLabel->SetLabel("Point Scalars:");
  this->ScalarOperationLabel->SetBalloonHelpString("Select which array to use as point scalars for this filter");
  
  this->ScalarOperationMenu->SetParent(this->ScalarOperationFrame);
  this->ScalarOperationMenu->Create(this->Application, "");
  this->ScalarOperationMenu->SetBalloonHelpString("Select which array to use as point scalars for this filter");

  this->VectorOperationFrame->SetParent(this->ParameterFrame->GetFrame());
  this->VectorOperationFrame->Create(this->Application, "frame", "");    
  this->Script("pack %s -side top -fill x -expand t",
               this->VectorOperationFrame->GetWidgetName());

  this->VectorOperationLabel->SetParent(this->VectorOperationFrame);
  this->VectorOperationLabel->Create(this->Application, "");
  this->VectorOperationLabel->SetLabel("Point Vectors:");
  this->VectorOperationLabel->SetBalloonHelpString("Select which array to use as point vectors for this filter");
  
  this->VectorOperationMenu->SetParent(this->VectorOperationFrame);
  this->VectorOperationMenu->Create(this->Application, "");
  this->VectorOperationMenu->SetBalloonHelpString("Select which array to use as point vectors for this filter");
  
  // Isolate events to this window until accept or reset is pressed.
  this->Script("grab set %s", this->ParameterFrame->GetWidgetName());
  
  this->UpdateProperties();
  
  this->UpdateParameterWidgets();
}

void vtkPVSource::UpdateScalarsMenu()
{
  int i, defaultSet = 0;
  vtkFieldData *fd;
  const char *arrayName;

  if (this->GetNumberOfPVInputs() == 0)
    {
    return;
    }
  
  // Set up some logic to set the default array if this is the first pass.
  // Retain previous value if not.
  arrayName = this->ScalarOperationMenu->GetValue();
  if (arrayName && arrayName[0] != '\0')
    {
    defaultSet = 1;
    } 

  fd = this->GetNthPVInput(0)->GetVTKData()->GetPointData()->GetFieldData();
  
  if (fd)
    {
    this->ScalarOperationMenu->ClearEntries();
    for (i = 0; i < fd->GetNumberOfArrays(); i++)
      {
      if (fd->GetArray(i)->GetNumberOfComponents() == 1)
        {
        this->ScalarOperationMenu->AddEntryWithCommand(fd->GetArrayName(i),
                                                       this, "ChangeScalars");
        if (!defaultSet)
          {
          arrayName = fd->GetArrayName(i);
          if (arrayName && arrayName[0] != '\0')
            {
            defaultSet = 1;
            } 
          }
        }
      }
    }
  if (defaultSet)
    {
    this->ScalarOperationMenu->SetValue(arrayName);
    }
  this->UpdateScalars();
}

void vtkPVSource::UpdateVectorsMenu()
{
  int i, defaultSet = 0;
  vtkFieldData *fd;
  const char *arrayName;

  if (this->GetNumberOfPVInputs() == 0)
    {
    return;
    }
  
  // Set up some logic to set the default array if this is the first pass.
  // Retain previous value if not.
  arrayName = this->VectorOperationMenu->GetValue();
  if (arrayName && arrayName[0] != '\0')
    {
    defaultSet = 1;
    } 

  fd = this->GetNthPVInput(0)->GetVTKData()->GetPointData()->GetFieldData();
  
  if (fd)
    {
    this->VectorOperationMenu->ClearEntries();
    for (i = 0; i < fd->GetNumberOfArrays(); i++)
      {
      if (fd->GetArray(i)->GetNumberOfComponents() == 3)
        {
        this->VectorOperationMenu->AddEntryWithCommand(fd->GetArrayName(i),
                                                       this, "ChangeVectors");
        if (!defaultSet)
          {
          arrayName = fd->GetArrayName(i);
          if (arrayName && arrayName[0] != '\0')
            {
            defaultSet = 1;
            } 
          }
        }
      }
    }
  if (defaultSet)
    {
    this->VectorOperationMenu->SetValue(arrayName);
    }
  this->UpdateVectors();
}

void vtkPVSource::PackScalarsMenu()
{
  this->UpdateScalarsMenu();
  this->Script("pack %s %s -side left -fill x",
               this->ScalarOperationLabel->GetWidgetName(),
               this->ScalarOperationMenu->GetWidgetName());
}

void vtkPVSource::PackVectorsMenu()
{
  this->UpdateVectorsMenu();
  this->Script("pack %s %s -side left -fill x",
               this->VectorOperationLabel->GetWidgetName(),
               this->VectorOperationMenu->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVSource::ChangeScalars()
{
  this->ChangeAcceptButtonColor();
  this->UpdateScalars();
}

//----------------------------------------------------------------------------
void vtkPVSource::ChangeVectors()
{
  this->ChangeAcceptButtonColor();
  this->UpdateVectors();
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateScalars()
{
  char *newScalars = this->ScalarOperationMenu->GetValue();
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (strcmp(newScalars, "") == 0)
    {
    return;
    }
  
  if (this->DefaultScalarsName)
    {
    if (strcmp(newScalars, this->DefaultScalarsName) == 0)
      {
      return;
      }
    }

  if (this->DefaultScalarsName)
    {
    delete [] this->DefaultScalarsName;
    this->DefaultScalarsName = NULL;
    }
  this->SetDefaultScalarsName(newScalars);

  if (!this->ChangeScalarsFilterTclName)
    {
    char tclName[256];
    sprintf(tclName, "ChangeScalars%d", this->InstanceCount);
    this->SetChangeScalarsFilterTclName(tclName);
    // I don't know why we are not using "MakeTclObject".
    pvApp->BroadcastScript("vtkFieldDataToAttributeDataFilter %s",
                          this->ChangeScalarsFilterTclName);
    pvApp->BroadcastScript("%s SetInput [%s GetInput]",
                           this->ChangeScalarsFilterTclName,
                           this->VTKSourceTclName);
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]",
                           this->VTKSourceTclName,
                           this->ChangeScalarsFilterTclName);
    }
  pvApp->BroadcastScript("%s SetInputFieldToPointDataField",
                         this->ChangeScalarsFilterTclName);
  pvApp->BroadcastScript("%s SetOutputAttributeDataToPointData",
                         this->ChangeScalarsFilterTclName);
  pvApp->BroadcastScript("%s SetScalarComponent 0 %s 0",
                         this->ChangeScalarsFilterTclName,
                         this->DefaultScalarsName);
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateVectors()
{
  char *newVectors = this->VectorOperationMenu->GetValue();
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (strcmp(newVectors, "") == 0)
    {
    return;
    }
  
  if (this->DefaultVectorsName)
    {
    if (strcmp(newVectors, this->DefaultVectorsName) == 0)
      {
      return;
      }
    }

  if (this->DefaultVectorsName)
    {
    delete [] this->DefaultVectorsName;
    this->DefaultVectorsName = NULL;
    }
  this->SetDefaultVectorsName(newVectors);

  if (!this->ChangeScalarsFilterTclName)
    {
    char tclName[256];
    sprintf(tclName, "ChangeScalars%d", this->InstanceCount);
    this->SetChangeScalarsFilterTclName(tclName);
    // I don't know why we are not using "MakeTclObject".
    pvApp->BroadcastScript("vtkFieldDataToAttributeDataFilter %s",
                          this->ChangeScalarsFilterTclName);
    pvApp->BroadcastScript("%s SetInput [%s GetInput]",
                           this->ChangeScalarsFilterTclName,
                           this->VTKSourceTclName);
    pvApp->BroadcastScript("%s SetInput [%s GetOutput]",
                           this->VTKSourceTclName,
                           this->ChangeScalarsFilterTclName);
    }
  pvApp->BroadcastScript("%s SetInputFieldToPointDataField",
                         this->ChangeScalarsFilterTclName);
  pvApp->BroadcastScript("%s SetOutputAttributeDataToPointData",
                         this->ChangeScalarsFilterTclName);
  pvApp->BroadcastScript("%s SetVectorComponent 0 %s 0",
                         this->ChangeScalarsFilterTclName,
                         this->DefaultVectorsName);
  pvApp->BroadcastScript("%s SetVectorComponent 1 %s 1",
                         this->ChangeScalarsFilterTclName,
                         this->DefaultVectorsName);
  pvApp->BroadcastScript("%s SetVectorComponent 2 %s 2",
                         this->ChangeScalarsFilterTclName,
                         this->DefaultVectorsName);
}

//----------------------------------------------------------------------------
void vtkPVSource::CreateInputList(const char *inputType)
{
  if (this->NumberOfPVInputs == 0)
    {
    return;
    }  
  
  this->InputMenu->SetInputType(inputType);

  this->UpdateInputList();
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateInputList()
{
  char* inputType = this->InputMenu->GetInputType();

  if (inputType == NULL || this->NumberOfPVInputs == 0)
    {
    return;
    }
  
  int i;
  vtkKWCompositeCollection *sources = this->GetWindow()->GetSources();
  vtkPVSource *currentSource;
  char *tclName;
  char methodAndArgs[256];
  
  this->InputMenu->ClearEntries();
  for (i = 0; i < sources->GetNumberOfItems(); i++)
    {
    currentSource = (vtkPVSource*)sources->GetItemAsObject(i);
    if (currentSource->GetNthPVOutput(0))
      {
      if (currentSource->GetNthPVOutput(0)->GetVTKData()->IsA(inputType))
        {
        tclName = currentSource->GetNthPVOutput(0)->GetVTKDataTclName();
        sprintf(methodAndArgs, "ChangeInput %s",
                currentSource->GetNthPVOutput(0)->GetTclName());
        this->InputMenu->AddEntryWithCommand(tclName, this,
                                             methodAndArgs);
        }
      }
    }
  this->InputMenu->SetValue(this->GetNthPVInput(0)->GetVTKDataTclName());
}

//----------------------------------------------------------------------------
void vtkPVSource::ChangeInput(const char *inputTclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->ChangeAcceptButtonColor();
  
  pvApp->Script("%s SetNthPVInput 0 %s", this->GetTclName(), inputTclName);
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
  char methodAndArg[256];
  int numSources;
  vtkPVSource *source;
  int numMenus;
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  window = this->GetWindow();

  this->Script("%s configure -cursor watch", window->GetWidgetName());
  this->Script("update");
  
#ifdef _WIN32
  this->Script("%s configure -background SystemButtonFace",
               this->AcceptButton->GetWidgetName());
#else
  this->Script("%s configure -background #d9d9d9",
               this->AcceptButton->GetWidgetName());
#endif
  
  // Call the commands to set ivars from widget values.
  for (i = 0; i < this->AcceptCommands->GetLength(); ++i)
    {
    this->Script(this->AcceptCommands->GetString(i));
    }  
  
  // Initialize the output if necessary.
  if ( ! this->Initialized)
    { // This is the first time, initialize data.    
    vtkPVData *input;
    vtkPVData *ac;
    
    ac = this->GetPVOutput(0);
    window->GetMainView()->AddComposite(ac);
    ac->CreateProperties();
    ac->Initialize();
    // Make the last data invisible.
    input = this->GetNthPVInput(0);
    if (input)
      {
      input->SetVisibility(0);
      input->GetVisibilityCheck()->SetState(0);
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
  window->GetSelectMenu()->DeleteAllMenuItems();
  numSources = window->GetSources()->GetNumberOfItems();
  
  for (i = 0; i < numSources; i++)
    {
    source = (vtkPVSource*)window->GetSources()->GetItemAsObject(i);
    sprintf(methodAndArg, "SetCurrentPVSource %s", source->GetTclName());
    window->GetSelectMenu()->AddCommand(source->GetName(), window,
                                        methodAndArg);
    }
  
  // Regenerate the data property page in case something has changed.
  if (this->NumberOfPVOutputs > 0)
    {
    this->GetNthPVOutput(0)->UpdateProperties();
    }

  this->Script("update");
  
#ifdef _WIN32
  this->Script("%s configure -cursor arrow", window->GetWidgetName());
#else
  this->Script("%s configure -cursor left_ptr", window->GetWidgetName());
#endif


  this->Script("%s index end", window->GetMenu()->GetWidgetName());
  numMenus = atoi(pvApp->GetMainInterp()->result);
  
  for (i = 0; i <= numMenus; i++)
    {
    this->Script("%s entryconfigure %d -state normal",
                 window->GetMenu()->GetWidgetName(), i);
    }
  this->Script("%s configure -state normal",
               window->GetCalculatorButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetThresholdButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetContourButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetGlyphButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetProbeButton()->GetWidgetName());
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
  vtkPVData *ac;
  vtkPVSource *prev;
  int i, numMenus;
  int numSources;
  char methodAndArg[256];
  vtkPVSource *source;
  vtkPVWindow *window = this->GetWindow();
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if ( ! this->Initialized)
    {
    // Remove the local grab
    this->Script("grab release %s", this->ParameterFrame->GetWidgetName());    
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
  
  // Remove this source from the inputs users collection.
  for (i = 0; i < this->GetNumberOfPVInputs(); i++)
    {
    if (this->PVInputs[i])
      {
      this->PVInputs[i]->RemovePVConsumer(this);
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
      
  // We need to remove this source from the SelectMenu
  this->GetWindow()->GetSources()->RemoveItem(this);
  this->GetWindow()->GetSelectMenu()->DeleteAllMenuItems();
  numSources = this->GetWindow()->GetSources()->GetNumberOfItems();
  for (i = 0; i < numSources; i++)
    {
    source = (vtkPVSource*)this->GetWindow()->GetSources()->GetItemAsObject(i);
    sprintf(methodAndArg, "SetCurrentPVSource %s", source->GetTclName());
    this->GetWindow()->GetSelectMenu()->AddCommand(source->GetName(),
                                                   this->GetWindow(),
                                                   methodAndArg);
    }
  
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
  
  this->GetPVRenderView()->EventuallyRender();

  this->Script("%s index end", window->GetMenu()->GetWidgetName());
  numMenus = atoi(pvApp->GetMainInterp()->result);
  
  for (i = 0; i <= numMenus; i++)
    {
    this->Script("%s entryconfigure %d -state normal",
                 window->GetMenu()->GetWidgetName(), i);
    }
  this->Script("%s configure -state normal",
               window->GetCalculatorButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetThresholdButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetContourButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetGlyphButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetProbeButton()->GetWidgetName());

  // This should delete this source.
  this->GetWindow()->GetMainView()->RemoveComposite(this);
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateParameterWidgets()
{
  int num, i;
  char *cmd;

  // Copy the ivars from the vtk object to the UI.
  num = this->ResetCommands->GetLength();
  for (i = 0; i < num; ++i)
    {
    cmd = this->ResetCommands->GetString(i);
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
  int num, idx;
  vtkPVData *input;
  
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
  
  this->GetWindow()->GetMainView()->UpdateNavigationWindow(this);
  
  // Make sure all the inputs are up to date.
  num = this->GetNumberOfPVInputs();
  for (idx = 0; idx < num; ++idx)
    {
    input = this->GetNthPVInput(idx);
    input->Update();
    }
  this->UpdateScalarsMenu();
  this->UpdateVectorsMenu();
}
  
//----------------------------------------------------------------------------
// Why do we need this.  Isn't show properties and Raise handled by window?
void vtkPVSource::SelectSource(vtkPVSource *source)
{
  if (source)
    {
    this->GetWindow()->SetCurrentPVSource(source);
    source->UpdateInputList();
    if (source->IsA("vtkPVGlyph3D"))
      {
      ((vtkPVGlyph3D*)source)->UpdateSourceMenu();
      }
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
    this->PVInputs[idx]->RemovePVConsumer(this);
    this->PVInputs[idx]->UnRegister(this);
    this->PVInputs[idx] = NULL;
    }
  
  if (pvd)
    {
    pvd->Register(this);
    pvd->AddPVConsumer(this);
    this->PVInputs[idx] = pvd;
    }


  // Relay the change to the VTK objects.  
  // This is where we will need a SetCommand from the interface ...
  if (this->ChangeScalarsFilterTclName && idx == 0)
    {
    pvApp->BroadcastScript("%s Delete",
                           this->ChangeScalarsFilterTclName);
    }
  else
    {
    char tclName[256];
    sprintf(tclName, "ChangeScalars%d", this->InstanceCount);
    this->SetChangeScalarsFilterTclName(tclName);
    }

  // I don't know why we are not using "MakeTclObject".
  pvApp->BroadcastScript("vtkFieldDataToAttributeDataFilter %s",
                        this->ChangeScalarsFilterTclName);
  pvApp->BroadcastScript("%s SetInput %s",
                         this->ChangeScalarsFilterTclName,
                         pvd->GetVTKDataTclName());
  pvApp->BroadcastScript("%s SetInput [%s GetOutput]",
                         this->VTKSourceTclName,
                         this->ChangeScalarsFilterTclName);
  
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
  int pos;
  char *charFound;
  char *dataName;
  
  if (this->ChangeScalarsFilterTclName)
    {
    *file << "vtkFieldDataToAttributeDataFilter "
          << this->ChangeScalarsFilterTclName << "\n\t"
          << this->ChangeScalarsFilterTclName << " SetInput [";
    if (strcmp(this->GetNthPVInput(0)->GetPVSource()->GetInterface()->
               GetSourceClassName(), "vtkGenericEnSightReader") == 0)
      {
      dataName = this->GetNthPVInput(0)->GetVTKDataTclName();
      charFound = strrchr(dataName, 't');
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput ";
      pos = charFound - dataName + 1;
      *file << dataName+pos << "]\n\t";
      }
    else if (strcmp(this->GetNthPVInput(0)->GetPVSource()->GetInterface()->
                    GetSourceClassName(), "vtkDataSetReader") == 0)
      {
      dataName = this->GetNthPVInput(0)->GetVTKDataTclName();      
      tempName = strtok(dataName, "O");
      *file << tempName << " GetOutput]\n\t";
      }
    else
      {
      *file << this->GetNthPVInput(0)->GetPVSource()->GetVTKSourceTclName()
            << " GetOutput]\n\t";
      }
    *file << this->ChangeScalarsFilterTclName
          << " SetInputFieldToPointDataField\n\t";
    *file << this->ChangeScalarsFilterTclName
          << " SetOutputAttributeDataToPointData\n\t";
    if (this->DefaultScalarsName)
      {
      *file << this->ChangeScalarsFilterTclName << " SetScalarComponent 0 "
            << this->DefaultScalarsName << " 0\n\t";
      }
    if (this->DefaultVectorsName)
      {
      *file << this->ChangeScalarsFilterTclName << " SetVectorComponent 0 "
            << this->DefaultVectorsName << " 0\n\t";
      *file << this->ChangeScalarsFilterTclName << " SetVectorComponent 1 "
            << this->DefaultVectorsName << " 1\n\t";
      *file << this->ChangeScalarsFilterTclName << " SetVectorComponent 2 "
            << this->DefaultVectorsName << " 2\n\t";
      }
    *file << "\n";
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
                  "vtkDataSetReader") == 0)
    {
    *file << "vtkDataSetReader " << this->Name << "\n";
    sprintf(tclName, this->Name);
    }
  
  if (this->NumberOfPVInputs > 0 && !this->ChangeScalarsFilterTclName)
    {
    *file << "\t" << tclName << " SetInput [";
    if (strcmp(this->GetNthPVInput(0)->GetPVSource()->GetInterface()->
               GetSourceClassName(), "vtkGenericEnSightReader") == 0)
      {
      char *charFound;
      int pos;
      char *dataName = this->GetNthPVInput(0)->GetVTKDataTclName();
      
      charFound = strrchr(dataName, 't');
      tempName = strtok(dataName, "O");
      *file << dataName << " GetOutput ";
      pos = charFound - dataName + 1;
      *file << dataName+pos << "]\n";
      }
    else if (strcmp(this->GetNthPVInput(0)->GetPVSource()->GetInterface()->
                    GetSourceClassName(), "vtkDataSetReader") == 0)
      {
      sprintf(sourceTclName, "DataSetReader");
      tempName = strtok(this->GetNthPVInput(0)->GetVTKDataTclName(), "O");
      strcat(sourceTclName, tempName+7);
      *file << sourceTclName << " GetOutput]\n";
      }
    else
      {
      *file << this->GetNthPVInput(0)->GetPVSource()->GetVTKSourceTclName()
            << " GetOutput]\n";
      }
    }
  else if (this->ChangeScalarsFilterTclName)
    {
    *file << "\t" << this->VTKSourceTclName << " SetInput ["
          << this->ChangeScalarsFilterTclName << " GetOutput]\n";
    }
  
  if (this->Interface)
    {
    this->Interface->SaveInTclScript(file, tclName);
    }
  
  *file << "\n";

  this->GetPVOutput(0)->SaveInTclScript(file, tclName);
}



//----------------------------------------------------------------------------
vtkKWCheckButton *vtkPVSource::AddLabeledToggle(char *label, char *setCmd,
                                                char *getCmd, char* help,
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
    if (help)
      {
      labelWidget->SetBalloonHelpString(help);
      }
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  // Now the check button
  vtkKWCheckButton *check = vtkKWCheckButton::New();
  this->Widgets->AddItem(check);
  check->SetParent(frame);
  check->Create(this->Application, "");
  check->SetCommand(this, "ChangeAcceptButtonColor");
  if (help)
    {
    check->SetBalloonHelpString(help);
    }
  this->Script("pack %s -side left", check->GetWidgetName());

  // Command to update the UI.
  this->ResetCommands->AddString("%s SetState [%s %s]",
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
                                      char *ext, char *help, vtkKWObject *o)
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
    if (help)
      {
      labelWidget->SetBalloonHelpString(help);
      }
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  entry = vtkKWEntry::New();
  this->Widgets->AddItem(entry);
  entry->SetParent(frame);
  entry->Create(this->Application, "");

  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               entry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    entry->SetBalloonHelpString(help);
    }
  this->Script("pack %s -side left -fill x -expand t", entry->GetWidgetName());


  browseButton = vtkKWPushButton::New();
  this->Widgets->AddItem(browseButton);
  browseButton->SetParent(frame);
  browseButton->Create(this->Application, "");
  browseButton->SetLabel("Browse");
  if (help)
    {
    browseButton->SetBalloonHelpString(help);
    }
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
  this->ResetCommands->AddString("%s SetValue [%s %s]",
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
vtkKWEntry *vtkPVSource::AddStringEntry(char *label, char *setCmd,
                                        char *getCmd, char *help,
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
    if (help)
      {
      labelWidget->SetBalloonHelpString(help);
      }
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  entry = vtkKWEntry::New();
  this->Widgets->AddItem(entry);
  entry->SetParent(frame);
  entry->Create(this->Application, "");
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               entry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    entry->SetBalloonHelpString(help);
    }
  this->Script("pack %s -side left -fill x -expand t", entry->GetWidgetName());

  // Command to update the UI.
  this->ResetCommands->AddString("%s SetValue [%s %s]",
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
vtkKWEntry *vtkPVSource::AddLabeledEntry(char *label, char *setCmd,
                                         char *getCmd, char* help,
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
    if (help)
      {
      labelWidget->SetBalloonHelpString(help);
      }
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  entry = vtkKWEntry::New();
  this->Widgets->AddItem(entry);
  entry->SetParent(frame);
  entry->Create(this->Application, "");
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               entry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    entry->SetBalloonHelpString(help);
    }
  this->Script("pack %s -side left -fill x -expand t", entry->GetWidgetName());

  // Command to update the UI.
  this->ResetCommands->AddString("%s SetValue [%s %s]",
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
                                  char *setCmd, char *getCmd, char *help,
                                  vtkKWObject *o)
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
    if (help)
      {
      labelWidget->SetBalloonHelpString(help);
      }
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
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               minEntry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    minEntry->SetBalloonHelpString(help);
    }
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
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               maxEntry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    maxEntry->SetBalloonHelpString(help);
    }
  this->Script("pack %s -side left -fill x -expand t", maxEntry->GetWidgetName());

  // Command to update the UI.
  this->ResetCommands->AddString("%s SetValue [lindex [%s %s] 0]",
                                 minEntry->GetTclName(), tclName, getCmd);
  this->ResetCommands->AddString("%s SetValue [lindex [%s %s] 1]",
                                 maxEntry->GetTclName(), tclName, getCmd);
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
				  char *setCmd, char *getCmd, char* help,
                                  vtkKWObject *o)
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
    if (help)
      {
      labelWidget->SetBalloonHelpString(help);
      }
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
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               xEntry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    xEntry->SetBalloonHelpString(help);
    }
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
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               yEntry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    yEntry->SetBalloonHelpString(help);
    }
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
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               zEntry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    zEntry->SetBalloonHelpString(help);
    }
  this->Script("pack %s -side left -fill x -expand t", zEntry->GetWidgetName());

  // Command to update the UI.
  this->ResetCommands->AddString("%s SetValue [lindex [%s %s] 0]", 
                                 xEntry->GetTclName(), tclName, getCmd); 
  this->ResetCommands->AddString("%s SetValue [lindex [%s %s] 1]", 
                                 yEntry->GetTclName(), tclName, getCmd); 
  this->ResetCommands->AddString("%s SetValue [lindex [%s %s] 2]", 
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
                                  char* help, vtkKWObject *o)
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
    if (help)
      {
      labelWidget->SetBalloonHelpString(help);
      }
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
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               xEntry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    xEntry->SetBalloonHelpString(help);
    }
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
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               yEntry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    yEntry->SetBalloonHelpString(help);
    }
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
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               zEntry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    zEntry->SetBalloonHelpString(help);
    }
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
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               wEntry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    wEntry->SetBalloonHelpString(help);
    }
  this->Script("pack %s -side left -fill x -expand t", wEntry->GetWidgetName());

  // Command to update the UI.
  this->ResetCommands->AddString(
    "%s SetValue [lindex [%s %s] 0]", xEntry->GetTclName(), tclName, getCmd);
  this->ResetCommands->AddString(
    "%s SetValue [lindex [%s %s] 1]", yEntry->GetTclName(), tclName, getCmd);
  this->ResetCommands->AddString(
    "%s SetValue [lindex [%s %s] 2]", zEntry->GetTclName(), tclName, getCmd);
  this->ResetCommands->AddString(
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
                                  char *setCmd, char *getCmd, char *help,
                                  vtkKWObject *o)

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
    if (help)
      {
      labelWidget->SetBalloonHelpString(help);
      }
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
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               uEntry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    uEntry->SetBalloonHelpString(help);
    }
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
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               vEntry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    vEntry->SetBalloonHelpString(help);
    }
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
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               wEntry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    wEntry->SetBalloonHelpString(help);
    }
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
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               xEntry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    xEntry->SetBalloonHelpString(help);
    }
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
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               yEntry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    yEntry->SetBalloonHelpString(help);
    }
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
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               zEntry->GetWidgetName(), this->GetTclName());
  if (help)
    {
    zEntry->SetBalloonHelpString(help);
    }
  this->Script("pack %s -side left -fill x -expand t", zEntry->GetWidgetName());

  // Command to update the UI.
  this->ResetCommands->AddString(
    "%s SetValue [lindex [%s %s] 0]",uEntry->GetTclName(), tclName, getCmd);
  this->ResetCommands->AddString(
    "%s SetValue [lindex [%s %s] 1]",vEntry->GetTclName(), tclName, getCmd);
  this->ResetCommands->AddString(
    "%s SetValue [lindex [%s %s] 2]",wEntry->GetTclName(), tclName, getCmd);
  this->ResetCommands->AddString(
    "%s SetValue [lindex [%s %s] 3]",xEntry->GetTclName(), tclName, getCmd);
  this->ResetCommands->AddString(
    "%s SetValue [lindex [%s %s] 4]",yEntry->GetTclName(), tclName, getCmd);
  this->ResetCommands->AddString(
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
                                  char* help, vtkKWObject *o)
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
  if (help)
    {
    labelWidget->SetBalloonHelpString(help);
    }
  this->Script("pack %s -side left", labelWidget->GetWidgetName());

  slider = vtkKWScale::New();
  this->Widgets->AddItem(slider);
  slider->SetParent(frame);
  slider->Create(this->Application, "-showvalue 1");
  slider->SetCommand(this, "ChangeAcceptButtonColor");
  slider->SetRange(min, max);
  slider->SetResolution(resolution);
  if (help)
    {
    slider->SetBalloonHelpString(help);
    }
  this->Script("pack %s -side left -fill x -expand t", slider->GetWidgetName());

  // Command to update the UI.
  this->ResetCommands->AddString("%s SetValue [%s %s]",
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
vtkPVSelectionList *vtkPVSource::AddModeList(char *label, char *setCmd,
                                             char *getCmd, char *help,
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
  if (help)
    {
    labelWidget->SetBalloonHelpString(help);
    }
  this->Script("pack %s -side left", labelWidget->GetWidgetName());

  vtkPVSelectionList *sl = vtkPVSelectionList::New();  
  this->Widgets->AddItem(sl);
  sl->SetParent(frame);
  sl->Create(this->Application);
  sl->SetCommand(this, "ChangeAcceptButtonColor");
  if (help)
    {
    sl->SetBalloonHelpString(help);
    }
  this->Script("pack %s -fill x -expand t", sl->GetWidgetName());
    
  // Command to update the UI.
  this->ResetCommands->AddString("%s SetCurrentValue [%s %s]",
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

void vtkPVSource::ChangeAcceptButtonColor()
{
  this->Script("%s configure -background red1",
               this->AcceptButton->GetWidgetName());
}

// This is necessary because -xscrollcommand passes 2 arguments to the
// method it calls, and entry widgets don't have a -command.
void vtkPVSource::EntryChanged(float vtkNotUsed(f1), float vtkNotUsed(f2))
{
  this->ChangeAcceptButtonColor();
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVSource::GetPVRenderView()
{
  return vtkPVRenderView::SafeDownCast(this->GetView());
}

//----------------------------------------------------------------------------
void vtkPVSource::ExtractPieces()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int numProcs, i;
  char tclPieceNum[256];
  char tclName[256];

  this->Script("%s UpdateInformation",
    this->GetPVOutput(0)->GetVTKDataTclName());
  this->Script("set numPieces [%s GetMaximumNumberOfPieces]",
    this->GetPVOutput(0)->GetVTKDataTclName());

  if ( atoi(pvApp->GetMainInterp()->result) != 1)
    {
    return;
    }
  
  sprintf(tclName, "ExtractPiece%d", this->InstanceCount);
  this->SetExtractPieceTclName(tclName);
  
  if (this->GetPVOutput(0)->GetVTKData()->IsA("vtkPolyData"))
    {
    pvApp->MakeTclObject("vtkExtractPolyDataPiece",
                         this->ExtractPieceTclName);
    }
  else if (this->GetPVOutput(0)->GetVTKData()->IsA("vtkUnstructuredGrid"))
    {
    pvApp->MakeTclObject("vtkExtractUnstructuredGridPiece",
                         this->ExtractPieceTclName);
    }
  
  pvApp->BroadcastScript("%s SetInput %s",
                         this->ExtractPieceTclName,
                         this->GetPVOutput(0)->GetVTKDataTclName());
  numProcs = pvApp->GetController()->GetNumberOfProcesses();
  
  pvApp->BroadcastScript("[%s GetOutput] SetUpdateNumberOfPieces %d",
                         this->ExtractPieceTclName, numProcs);

  for (i = 1; i < numProcs; i++)
    {
    sprintf(tclPieceNum, "[%s GetOutput] SetUpdatePiece %d",
            this->ExtractPieceTclName, i);    
    }

  this->Script("[%s GetOutput] SetUpdatePiece 0",
               this->ExtractPieceTclName);
  pvApp->BroadcastScript("%s ReleaseDataFlagOn", this->ExtractPieceTclName);
}
