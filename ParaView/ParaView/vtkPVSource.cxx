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

#include "vtkArrayMap.txx"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkDataSet.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWImageLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWLabeledLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWNotebook.h"
#include "vtkKWPushButton.h"
#include "vtkKWSerializer.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVPart.h"
#include "vtkPVInputProperty.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVWidgetCollection.h"
#include "vtkPVWindow.h"
#include "vtkSource.h"
#include "vtkString.h"
#include "vtkRenderer.h"
#include "vtkArrayMap.txx"
#include "vtkStringList.h"
#include "vtkCollection.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSource);
vtkCxxRevisionMacro(vtkPVSource, "1.280");

int vtkPVSourceCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

vtkCxxSetObjectMacro(vtkPVSource, View, vtkKWView);

//----------------------------------------------------------------------------
vtkPVSource::vtkPVSource()
{
  this->CommandFunction = vtkPVSourceCommand;

  this->PVParts = vtkCollection::New();
  this->DataInformation = vtkPVDataInformation::New();
  this->DataInformationValid = 0;

  // Number of instances cloned from this prototype
  this->PrototypeInstanceCount = 0;

  this->Name = 0;
  this->Label = 0;
  this->ModuleName = 0;
  this->MenuName = 0;
  this->ShortHelp = 0;
  this->LongHelp  = 0;

  // Initialize the data only after  Accept is invoked for the first time.
  // This variable is used to determine that.
  this->Initialized = 0;

  // The notebook which holds Parameters, Display and Information pages.
  this->Notebook = vtkKWNotebook::New();
  this->Notebook->AlwaysShowTabsOn();

  this->PVInputs = NULL;
  this->NumberOfPVInputs = 0;
  this->PVOutput = NULL;

  this->NumberOfPVConsumers = 0;
  this->PVConsumers = 0;

  // The underlying VTK object. This will change. PVSource will
  // support multiple VTK sources/filters.
  this->VTKSources = vtkCollection::New();
  this->VTKSourceTclNames = vtkStringList::New();

  // The frame which contains the parameters related to the data source
  // and the Accept/Reset/Delete buttons.
  this->Parameters = vtkKWWidget::New();
  
  this->ParameterFrame = vtkKWFrame::New();
  this->ButtonFrame = vtkKWWidget::New();
  this->MainParameterFrame = vtkKWWidget::New();
  this->AcceptButton = vtkKWPushButton::New();
  this->ResetButton = vtkKWPushButton::New();
  this->DeleteButton = vtkKWPushButton::New();

  this->DescriptionFrame = vtkKWWidget::New();
  this->NameLabel = vtkKWLabeledLabel::New();
  this->TypeLabel = vtkKWLabeledLabel::New();
  this->LongHelpLabel = vtkKWLabeledLabel::New();
  this->LabelEntry = vtkKWLabeledEntry::New();
      
  this->Widgets = vtkPVWidgetCollection::New();
    
  this->ReplaceInput = 1;

  this->ParametersParent = NULL;
  this->View = NULL;

  this->VisitedFlag = 0;

  this->SourceClassName = 0;

  this->RequiredNumberOfInputParts = -1;
  this->VTKMultipleInputsFlag = 0;
  this->InputProperties = vtkCollection::New();

  this->IsPermanent = 0;

  this->HideDisplayPage = 0;
  this->HideParametersPage = 0;
  this->HideInformationPage = 0;
  
  this->AcceptCallbackFlag = 0;

  this->SourceGrabbed = 0;

  this->ToolbarModule = 0;

  this->Prototype = 0;

  this->LODResolution = 50;
}

//----------------------------------------------------------------------------
vtkPVSource::~vtkPVSource()
{
  this->SetPVOutput(NULL);  
  this->RemoveAllPVInputs();

  this->PVParts->Delete();
  this->PVParts = NULL;
  this->DataInformation->Delete();
  this->DataInformation = NULL;

  if (this->PVConsumers)
    {
    delete [] this->PVConsumers;
    this->PVConsumers = NULL;
    this->NumberOfPVConsumers = 0;
    }

  // We need to delete the Tcl object too.  This call does it.
  this->RemoveAllVTKSources();
  this->VTKSources->Delete();
  this->VTKSourceTclNames->Delete();

  // Do not use SetName() or SetLabel() here. These make
  // the navigation window update when it should not.
  delete[] this->Name;
  delete[] this->Label;

  this->SetMenuName(0);
  this->SetShortHelp(0);
  this->SetLongHelp(0);

  // This is necessary in order to make the parent frame release it's
  // reference to the widgets. Otherwise, the widgets get deleted only
  // when the parent (usually the parameters notebook page) is deleted.
  this->Notebook->SetParent(0);
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

  this->DescriptionFrame->Delete();
  this->DescriptionFrame = NULL;

  this->NameLabel->Delete();
  this->NameLabel = NULL;

  this->TypeLabel->Delete();
  this->TypeLabel = NULL;

  this->LongHelpLabel->Delete();
  this->LongHelpLabel = NULL;

  this->LabelEntry->Delete();
  this->LabelEntry = NULL;

  this->ParameterFrame->Delete();
  this->ParameterFrame = NULL;

  this->MainParameterFrame->Delete();
  this->MainParameterFrame = NULL;

  this->ButtonFrame->Delete();
  this->ButtonFrame = NULL;

  this->Parameters->Delete();
  this->Parameters = NULL;
    
  if (this->ParametersParent)
    {
    this->ParametersParent->UnRegister(this);
    this->ParametersParent = NULL;
    }
  this->SetView(NULL);

  this->SetSourceClassName(0);
 
  this->InputProperties->Delete();
  this->InputProperties = NULL;

  this->SetModuleName(0);
}


//----------------------------------------------------------------------------
void vtkPVSource::SetPVInput(int idx, vtkPVSource *pvs)
{
  int partIdx, numParts;
  vtkPVPart *part;
  vtkPVApplication *pvApp = this->GetPVApplication();
  char *sourceTclName;
  const char* inputName;

  if (pvApp == NULL)
    {
    vtkErrorMacro(
      "No Application. Create the source before setting the input.");
    return;
    }
  // Handle visibility of old and new input.
  if (this->ReplaceInput)
    {
    vtkPVSource *oldInput = this->GetNthPVInput(idx);
    if (oldInput)
      {
      oldInput->SetVisibility(1);
      this->GetPVRenderView()->EventuallyRender();
      }
    }

  // Set the paraview reference to the new input.
  this->SetNthPVInput(idx, pvs);
  if (pvs == NULL)
    {
    return;
    }

  // Set the VTK reference to the new input.
  vtkPVInputProperty* ip = this->GetInputProperty(idx);
  if (ip)
    {
    inputName = ip->GetName();
    }
  else
    {
    inputName = "Input";
    }
  numParts = pvs->GetNumberOfPVParts();
  for (partIdx = 0; partIdx < numParts; ++partIdx)
    {
    part = pvs->GetPVPart(partIdx);
    if (this->VTKMultipleInputsFlag)
      { // Only one source takes all parts as input.
      sourceTclName = this->GetVTKSourceTclName(0);
      
      if (part->GetVTKDataTclName() == NULL || sourceTclName == NULL)
        { // Sanity check.
        vtkErrorMacro("Missing tcl name.");
        }
      else
        {
        pvApp->BroadcastScript("%s Add%s %s", sourceTclName, inputName,
                               part->GetVTKDataTclName());
        }      
      }
    else
      { // One source for each part.
      sourceTclName = this->GetVTKSourceTclName(partIdx);
      if (part->GetVTKDataTclName() == NULL || sourceTclName == NULL)
        {
        vtkErrorMacro("Source data mismatch.");
        }
      else
        {
        pvApp->BroadcastScript("%s Set%s %s", sourceTclName, inputName,
                               part->GetVTKDataTclName());
        }
      }
    }

  this->GetPVRenderView()->UpdateNavigationWindow(this, 0);
}



//----------------------------------------------------------------------------
void vtkPVSource::AddPVConsumer(vtkPVSource *c)
{
  // make sure it isn't already there
  if (this->IsPVConsumer(c))
    {
    return;
    }
  // add it to the list, reallocate memory
  vtkPVSource **tmp = this->PVConsumers;
  this->NumberOfPVConsumers++;
  this->PVConsumers = new vtkPVSource* [this->NumberOfPVConsumers];
  for (int i = 0; i < (this->NumberOfPVConsumers-1); i++)
    {
    this->PVConsumers[i] = tmp[i];
    }
  this->PVConsumers[this->NumberOfPVConsumers-1] = c;
  // free old memory
  delete [] tmp;
}

//----------------------------------------------------------------------------
void vtkPVSource::RemovePVConsumer(vtkPVSource *c)
{
  // make sure it is already there
  if (!this->IsPVConsumer(c))
    {
    return;
    }
  // remove it from the list, reallocate memory
  vtkPVSource **tmp = this->PVConsumers;
  this->NumberOfPVConsumers--;
  this->PVConsumers = new vtkPVSource* [this->NumberOfPVConsumers];
  int cnt = 0;
  int i;
  for (i = 0; i <= this->NumberOfPVConsumers; i++)
    {
    if (tmp[i] != c)
      {
      this->PVConsumers[cnt] = tmp[i];
      cnt++;
      }
    }
  // free old memory
  delete [] tmp;
}

//----------------------------------------------------------------------------
int vtkPVSource::IsPVConsumer(vtkPVSource *c)
{
  int i;
  for (i = 0; i < this->NumberOfPVConsumers; i++)
    {
    if (this->PVConsumers[i] == c)
      {
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkPVSource *vtkPVSource::GetPVConsumer(int i)
{
  if (i >= this->NumberOfPVConsumers)
    {
    return 0;
    }
  return this->PVConsumers[i];
}





//----------------------------------------------------------------------------
int vtkPVSource::GetNumberOfPVParts()
{
  return this->PVParts->GetNumberOfItems();
}

//----------------------------------------------------------------------------
void vtkPVSource::SetPVPart(vtkPVPart* part)
{
  this->PVParts->AddItem(part);
}

//----------------------------------------------------------------------------
void vtkPVSource::AddPVPart(vtkPVPart* part)
{
  this->PVParts->AddItem(part);
}

//----------------------------------------------------------------------------
vtkPVPart* vtkPVSource::GetPVPart(int idx)
{
  vtkPVPart *part;

  part = static_cast<vtkPVPart*>(this->PVParts->GetItemAsObject(idx));
  return part;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkPVSource::GetDataInformation()
{
  if (this->DataInformationValid == 0)
    {
    this->GatherDataInformation();
    }
  return this->DataInformation;
}

//----------------------------------------------------------------------------
void vtkPVSource::InvalidateDataInformation()
{
  this->DataInformationValid = 0;
}

//----------------------------------------------------------------------------
void vtkPVSource::GatherDataInformation()
{
  vtkPVPart *part;

  this->DataInformation->Initialize();
  this->PVParts->InitTraversal();
  while ( ( part = (vtkPVPart*)(this->PVParts->GetNextItemAsObject())) )
    {
    part->GatherDataInformation();
    this->DataInformation->AddInformation(part->GetDataInformation());
    }
  this->DataInformationValid = 1;
  // This will cause a recursive call, but the Vvlid flag will terminate it.
  this->GetPVOutput()->UpdateProperties();
}

//----------------------------------------------------------------------------
void vtkPVSource::SetVTKData(vtkDataSet *data, const char *name)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  vtkPVPart *part = vtkPVPart::New();
  part->SetPVApplication(pvApp);
  part->SetVTKData(data, name);
  pvApp->BroadcastScript("%s SetNumberOfDivisions %d %d %d", 
                         part->GetLODDeciTclName(), this->LODResolution,
                         this->LODResolution, this->LODResolution); 

  pvApp->GetProcessModule()->InitializePVPartPartition(part);
  this->PVParts->AddItem(part);
  part->Delete();
}



//----------------------------------------------------------------------------
void vtkPVSource::ForceUpdate(vtkPVApplication* pvApp)
{
  vtkPVPart *part;

  this->PVParts->InitTraversal();
  while ( (part = (vtkPVPart*)(this->PVParts->GetNextItemAsObject())) )
    {
    part->ForceUpdate(pvApp);
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::Update()
{
  vtkPVPart *part;

  this->PVParts->InitTraversal();
  while ( (part = (vtkPVPart*)(this->PVParts->GetNextItemAsObject())) )
    {
    part->Update();
    }
  
  // Now done as a callback.
  //this->GatherDataInformation();
}

  
//----------------------------------------------------------------------------
void vtkPVSource::SetVisibilityInternal(int v)
{
  vtkPVPart *part;
  int idx, num;

  if (this->GetPVOutput())
    {
    this->GetPVOutput()->SetVisibilityCheckState(v);
    }

  num = this->GetNumberOfPVParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVPart(idx);
    part->SetVisibility(v);
    }
}


//----------------------------------------------------------------------------
// Functions to update the progress bar
void vtkPVSourceStartProgress(void* vtkNotUsed(arg))
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
void vtkPVSourceReportProgress(void* vtkNotUsed(arg))
{
  //vtkPVSource *me = (vtkPVSource*)arg;
  //vtkSource *vtkSource = me->GetVTKSource();

  //if (me->GetWindow())
  //  {
  //  me->GetWindow()->GetProgressGauge()->SetValue((int)(vtkSource->GetProgress() * 100));
  //  }
}
//----------------------------------------------------------------------------
void vtkPVSourceEndProgress(void* vtkNotUsed(arg))
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
void vtkPVSource::AddVTKSource(vtkSource *source, const char *tclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp == NULL)
    {
    return;
    }
  
  if (tclName == NULL || source == NULL)
    {
    vtkErrorMacro("Missing Tcl name or source.");
    return;
    }

  // I hate having two collections: one for the vtk object, and one for its
  // Tcl name.  I should probably create an object to contain both.
  this->VTKSources->AddItem(source);
  this->VTKSourceTclNames->AddString(tclName);
    
  pvApp->Script("%s AddObserver ModifiedEvent {%s VTKSourceModifiedMethod}",
                tclName, this->GetTclName());
    
  pvApp->BroadcastScript(
    "%s AddObserver EndEvent {$Application LogStartEvent {Execute %s}}", 
    tclName, tclName);

  pvApp->BroadcastScript(
    "%s AddObserver EndEvent {$Application LogEndEvent {Execute %s}}", 
    tclName, tclName);

}

//----------------------------------------------------------------------------
void vtkPVSource::RemoveAllVTKSources()
{
  char *tclName;
  vtkPVApplication *pvApp = this->GetPVApplication();
  int num, idx;

  num = this->VTKSourceTclNames->GetNumberOfStrings();
  for (idx = 0; idx < num; ++ idx)
    {
    tclName = this->VTKSourceTclNames->GetString(idx);
    pvApp->BroadcastScript("%s Delete", tclName);
    }

  this->VTKSourceTclNames->RemoveAllItems();
  this->VTKSources->RemoveAllItems();
}

//----------------------------------------------------------------------------
int vtkPVSource::GetNumberOfVTKSources()
{
  return this->VTKSources->GetNumberOfItems();
}

//----------------------------------------------------------------------------
vtkSource* vtkPVSource::GetVTKSource(int idx)
{
  return static_cast<vtkSource*>(this->VTKSources->GetItemAsObject(idx));
}

//----------------------------------------------------------------------------
char* vtkPVSource::GetVTKSourceTclName(int idx)
{
  return this->VTKSourceTclNames->GetString(idx);
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
  // If the user has not set the parameters parent.
  if (this->ParametersParent == NULL)
    {
    vtkErrorMacro("ParametersParent has not been set.");
    }

  this->Notebook->SetParent(this->ParametersParent);
  this->Notebook->Create(this->Application,"");

  // Set up the pages of the notebook.
  if (!this->HideParametersPage)
    {
    this->Notebook->AddPage("Parameters");
    this->Parameters->SetParent(this->Notebook->GetFrame("Parameters"));
    }
  else
    {
    this->Parameters->SetParent(this->ParametersParent);
    }

  this->Parameters->Create(this->Application,"frame","");
  if (!this->HideParametersPage)
    {
    this->Script("pack %s -pady 2 -fill x -expand yes",
                 this->Parameters->GetWidgetName());
    }

  // For initializing the trace of the notebook.
  this->GetParametersParent()->SetTraceReferenceObject(this);
  this->GetParametersParent()->SetTraceReferenceCommand("GetParametersParent");

  // Set the description frame
  // Try to do something that looks like the parameters, i.e. fixed-width
  // labels and "expandable" values. This has to be fixed later when the
  // parameters will be properly aligned (i.e. gridded)

  this->DescriptionFrame ->SetParent(this->Parameters);
  this->DescriptionFrame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill both -expand t -side top -padx 2 -pady 2", 
               this->DescriptionFrame->GetWidgetName());

  const char *label1_opt = "-width 12 -anchor e";

  this->NameLabel->SetParent(this->DescriptionFrame);
  this->NameLabel->Create(this->Application);
  this->NameLabel->SetLabel("Name:");
  this->Script("%s configure -anchor w", 
               this->NameLabel->GetLabel2()->GetWidgetName());
  this->Script("%s config %s", 
               this->NameLabel->GetLabel()->GetWidgetName(), label1_opt);
  this->Script("pack %s -fill x -expand t", 
               this->NameLabel->GetLabel2()->GetWidgetName());
  vtkKWTkUtilities::ChangeFontWeightToBold(
    this->Application->GetMainInterp(),
    this->NameLabel->GetLabel2()->GetWidgetName());

  this->TypeLabel->SetParent(this->DescriptionFrame);
  this->TypeLabel->Create(this->Application);
  this->TypeLabel->GetLabel()->SetLabel("Class:");
  this->Script("%s configure -anchor w", 
               this->TypeLabel->GetLabel2()->GetWidgetName());
  this->Script("%s config %s", 
               this->TypeLabel->GetLabel()->GetWidgetName(), label1_opt);
  this->Script("pack %s -fill x -expand t", 
               this->TypeLabel->GetLabel2()->GetWidgetName());

  this->LabelEntry->SetParent(this->DescriptionFrame);
  this->LabelEntry->Create(this->Application);
  this->LabelEntry->GetLabel()->SetLabel("Label:");
  this->Script("%s config %s", 
               this->LabelEntry->GetLabel()->GetWidgetName(),label1_opt);
  this->Script("pack %s -fill x -expand t", 
               this->LabelEntry->GetEntry()->GetWidgetName());
  this->Script("bind %s <KeyPress-Return> {%s LabelEntryCallback}",
               this->LabelEntry->GetEntry()->GetWidgetName(), 
               this->GetTclName());

  this->LongHelpLabel->SetParent(this->DescriptionFrame);
  this->LongHelpLabel->Create(this->Application);
  this->LongHelpLabel->GetLabel()->SetLabel("Description:");
  this->LongHelpLabel->GetLabel2()->AdjustWrapLengthToWidthOn();
  this->Script("%s configure -anchor w", 
               this->LongHelpLabel->GetLabel2()->GetWidgetName());
  this->Script("%s config %s", 
               this->LongHelpLabel->GetLabel()->GetWidgetName(), label1_opt);
  this->Script("pack %s -fill x -expand t", 
               this->LongHelpLabel->GetLabel2()->GetWidgetName());

  this->Script("grid %s -sticky news", 
               this->NameLabel->GetWidgetName());
  this->Script("grid %s -sticky news", 
               this->TypeLabel->GetWidgetName());
  this->Script("grid %s -sticky news", 
               this->LabelEntry->GetWidgetName());
  this->Script("grid %s -sticky news", 
               this->LongHelpLabel->GetWidgetName());
  this->Script("grid columnconfigure %s 0 -weight 1", 
               this->LongHelpLabel->GetParent()->GetWidgetName());

  // The main parameter frame

  this->MainParameterFrame->SetParent(this->Parameters);
  this->MainParameterFrame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill both -expand t -side top", 
               this->MainParameterFrame->GetWidgetName());

  this->ButtonFrame->SetParent(this->MainParameterFrame);
  this->ButtonFrame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill both -expand t -side top", 
               this->ButtonFrame->GetWidgetName());

  this->ParameterFrame->SetParent(this->MainParameterFrame);

  this->ParameterFrame->ScrollableOn();
  this->ParameterFrame->Create(this->Application,0);
  this->Script("pack %s -fill both -expand t -side top", 
               this->ParameterFrame->GetWidgetName());

  vtkKWWidget *frame = vtkKWWidget::New();
  frame->SetParent(this->ButtonFrame);
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());  
  
  this->AcceptButton->SetParent(frame);
  this->AcceptButton->Create(this->Application, 
                             "-text Accept -background red1");
  this->AcceptButton->SetCommand(this, "PreAcceptCallback");
  this->AcceptButton->SetBalloonHelpString(
    "Cause the current values in the user interface to take effect");

  this->ResetButton->SetParent(frame);
  this->ResetButton->Create(this->Application, "-text Reset");
  this->ResetButton->SetCommand(this, "ResetCallback");
  this->ResetButton->SetBalloonHelpString(
    "Revert to the previous parameters of the module.");

  this->DeleteButton->SetParent(frame);
  this->DeleteButton->Create(this->Application, "-text Delete");
  this->DeleteButton->SetCommand(this, "DeleteCallback");
  this->DeleteButton->SetBalloonHelpString(
    "Remove the current module.  "
    "This can only be done if no other modules depends on the current one.");

  this->Script("pack %s %s %s -padx 2 -pady 2 -side left -fill x -expand t",
               this->AcceptButton->GetWidgetName(), 
               this->ResetButton->GetWidgetName(), 
               this->DeleteButton->GetWidgetName());

  frame->Delete();  
 
  this->UpdateProperties();

  vtkPVWidget *pvWidget;
  this->Widgets->InitTraversal();
  int i;
  for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
    {
    pvWidget = this->Widgets->GetNextPVWidget();
    pvWidget->SetParent(this->ParameterFrame->GetFrame());
    pvWidget->Create(this->Application);
    this->Script("pack %s -side top -fill x -expand t", 
                 pvWidget->GetWidgetName());
    }

}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateDescriptionFrame()
{
  if (!this->Application)
    {
    return;
    }

  if (this->NameLabel && this->NameLabel->IsCreated())
    {
    this->NameLabel->GetLabel2()->SetLabel(this->Name ? this->Name : "");
    }

  if (this->TypeLabel && this->TypeLabel->IsCreated())
    {
    if (this->GetVTKSource()) 
      {
      this->TypeLabel->GetLabel2()->SetLabel(
        this->GetVTKSource()->GetClassName());
      if (this->DescriptionFrame->IsPacked())
        {
        this->Script("grid %s", this->TypeLabel->GetWidgetName());
        }
      }
    else
      {
      this->TypeLabel->GetLabel2()->SetLabel("");
      if (this->DescriptionFrame->IsPacked())
        {
        this->Script("grid remove %s", this->TypeLabel->GetWidgetName());
        }
      }
    }

  if (this->LabelEntry && this->LabelEntry->IsCreated())
    {
    this->LabelEntry->GetEntry()->SetValue(this->Label);
    }

  if (this->LongHelpLabel && this->LongHelpLabel->IsCreated())
    {
    if (this->LongHelp && 
        !(this->GetPVWindow() && 
          !this->GetPVWindow()->GetShowSourcesLongHelp())) 
      {
      this->LongHelpLabel->GetLabel2()->SetLabel(this->LongHelp);
      if (this->DescriptionFrame->IsPacked())
        {
        this->Script("grid %s", this->LongHelpLabel->GetWidgetName());
        }
      }
    else
      {
      this->LongHelpLabel->GetLabel2()->SetLabel("");
      if (this->DescriptionFrame->IsPacked())
        {
        this->Script("grid remove %s", this->LongHelpLabel->GetWidgetName());
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::GrabFocus()
{
  this->SourceGrabbed = 1;

  this->GetPVWindow()->DisableToolbarButtons();
  this->GetPVWindow()->DisableMenus();
  this->GetPVRenderView()->UpdateNavigationWindow(this, 1);
}

//----------------------------------------------------------------------------
void vtkPVSource::UnGrabFocus()
{

  if ( this->SourceGrabbed )
    {
    this->GetPVWindow()->EnableToolbarButtons();
    this->GetPVWindow()->EnableMenus();
    this->GetPVRenderView()->UpdateNavigationWindow(this, 0);
    }
  this->SourceGrabbed = 0;
}

//----------------------------------------------------------------------------
void vtkPVSource::Pack()
{
  // The update is needed to work around a packing problem which
  // occur for large windows. Do not remove.
  this->Script("update");

  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->ParametersParent->GetWidgetName());
  this->Script("pack %s -side top -fill both -expand t",
               this->GetPVRenderView()->GetNavigationFrame()->GetWidgetName());
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVSource::Select()
{
  this->Pack();

  vtkPVData *data;
  
  this->UpdateProperties();
  // This may best be merged with the UpdateProperties call but ...
  // We make the call here to update the input menu, 
  // which is now just another pvWidget.
  this->UpdateParameterWidgets();
  
  data = this->GetPVOutput();
  if (data)
    {
    // This has a side effect of gathering and displaying information.
    data->GetPVSource()->GetDataInformation();
    }

  if (this->GetPVRenderView())
    {
    this->GetPVRenderView()->UpdateNavigationWindow(this, this->SourceGrabbed);
    }

  int i;
  vtkPVWidget *pvw = 0;
  this->Widgets->InitTraversal();
  for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
    {
    pvw = this->Widgets->GetNextPVWidget();
    pvw->Select();
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::Deselect(int doPackForget)
{
  if (doPackForget)
    {
    this->Script("pack forget %s", this->Notebook->GetWidgetName());
    }
  int i;
  vtkPVWidget *pvw = 0;
  this->Widgets->InitTraversal();
  for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
    {
    pvw = this->Widgets->GetNextPVWidget();
    pvw->Deselect();
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
  
  // Make sure the description frame is upto date.
  this->UpdateDescriptionFrame();

  // Update the nav window (that usually might display name + description)
  if (this->GetPVRenderView())
    {
    this->GetPVRenderView()->UpdateNavigationWindow(this, this->SourceGrabbed);
    }
} 

//----------------------------------------------------------------------------
char* vtkPVSource::GetLabel() 
{ 
  // Design choice: if the description is empty, initialize it with the
  // Tcl name, so that the user knows what to overrides in the nav window.

  if (this->Label == NULL)
    {
    this->SetLabelNoTrace(this->GetName());
    }
  return this->Label;
}

//----------------------------------------------------------------------------
void vtkPVSource::SetLabel(const char* arg) 
{ 
  this->SetLabelNoTrace(arg);

  // Update the nav window (that usually might display name + description)
  vtkPVSource* current = this->GetPVWindow()->GetCurrentPVSource();
  if (this->GetPVRenderView() && current)
    {
    this->GetPVRenderView()->UpdateNavigationWindow(
      current, current->SourceGrabbed);
    }
  // Trace here, not in SetLabel (design choice)
  this->GetPVApplication()->AddTraceEntry("$kw(%s) SetLabel {%s}",
                                          this->GetTclName(),
                                          this->Label);
  this->GetPVApplication()->AddTraceEntry("$kw(%s) LabelEntryCallback",
                                          this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVSource::SetLabelNoTrace(const char* arg) 
{ 
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " 
                << this->Label << " to " << arg ); 
  if ( this->Label && arg && (!strcmp(this->Label,arg))) 
    { 
    return;
    } 
  if (this->Label) 
    { 
    delete [] this->Label; 
    } 
  if (arg) 
    { 
    this->Label = new char[strlen(arg)+1]; 
    strcpy(this->Label,arg); 
    } 
  else 
    { 
    this->Label = NULL;
    }
  this->Modified();

  // Make sure the description frame is upto date.
  this->UpdateDescriptionFrame();

} 

//----------------------------------------------------------------------------
void vtkPVSource::LabelEntryCallback()
{
  this->SetLabel(this->LabelEntry->GetValue());
}

//----------------------------------------------------------------------------
void vtkPVSource::SetVisibility(int v)
{
  if (this->PVOutput)
    {
    this->PVOutput->SetVisibility(v);
    }
}

//----------------------------------------------------------------------------
int vtkPVSource::GetVisibility()
{
  if ( this->GetPVOutput() && this->GetPVOutput()->GetVisibility() )
    {
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVSource::AcceptCallback()
{
  // This method is purposely not virtual.  The AcceptCallbackFlag
  // must be 1 for the duration of the accept callback no matter what
  // subclasses might do.  All of the real AcceptCallback funcionality
  // should be implemented in AcceptCallbackInternal.
  this->AcceptCallbackFlag = 1;
  this->AcceptCallbackInternal();
  this->AcceptCallbackFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVSource::PreAcceptCallback()
{
  this->Script("%s configure -cursor watch",
               this->GetPVWindow()->GetWidgetName());
  this->Script("after idle {%s AcceptCallback}", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVSource::AcceptCallbackInternal()
{
  // I had trouble with this object destructing too early
  // in this method, because of an unregistered reference ...
  // It was an error condition (no output was created).
  this->Register(this);

  int dialogAlreadyUp=0;
  // This is here to prevent the user from killing the
  // application from inside the accept callback.
  if (!this->Application->GetDialogUp())
    {
    this->Application->SetDialogUp(1);
    }
  else
    {
    dialogAlreadyUp=1;
    }
  this->Accept(0);
  this->GetPVApplication()->AddTraceEntry("$kw(%s) AcceptCallback",
                                          this->GetTclName());
  if (!dialogAlreadyUp)
    {
    this->Application->SetDialogUp(0);
    }


  // I had trouble with this object destructing too early
  // in this method, because of an unregistered reference ...
  this->UnRegister(this);
}

//----------------------------------------------------------------------------
void vtkPVSource::Accept(int hideFlag, int hideSource)
{
  vtkPVWindow *window;
  int idx, num;
  vtkPVPart *part;
  vtkPVSource *input;

  this->Script("update");

  window = this->GetPVWindow();

  this->SetAcceptButtonColorToWhite();
  
  // We need to pass the parameters from the UI to the VTK objects before
  // we check whether to insert ExtractPieces.  Otherwise, we'll get errors
  // about unspecified file names, etc., when ExecuteInformation is called on
  // the VTK source.  (The vtkPLOT3DReader is a good example of this.)
  this->UpdateVTKSourceParameters();
  
  // Moved from creation of the source. (InitializeClone)
  // Initialize the output if necessary.
  // This has to be after the widgets are accepted (UpdateVTKSOurceParameters)
  // because they can change the number of parts.
  if ( ! this->Initialized)
    { // This is the first time, create the data.
    this->InitializeData();
    }
    
  // Initialize the output if necessary.
  if ( ! this->Initialized)
    { // This is the first time, initialize data.    
    vtkPVData *pvd;
    
    pvd = this->GetPVOutput();
    if (pvd == NULL)
      { // I suppose we should try and delete the source.
      vtkErrorMacro("Could not get output.");
      this->DeleteCallback();    
      return;
      }

    // I need to update the data before 
    // UpdateFilterMenu which is called by SetCurrentPVSource.
    // Although tis fixes my problem,  I should not have to actually update.
    // All I need is the data type from the information which should be
    // obtainable after initialize data ...
    // The bug I was fixing is the ExtractGrid button was active
    // for a polydata and imagedata collection.
    this->Update();
    
    window->GetMainView()->AddPVSource(this);
    if (!this->GetHideDisplayPage())
      {
      this->Notebook->AddPage("Display");
      }
    if (!this->GetHideInformationPage())
      {
      this->Notebook->AddPage("Information");
      }

    pvd->CreateProperties();

    // Make the last data invisible.
    input = this->GetPVInput(0);
    if (input)
      {
      if (this->ReplaceInput && input->GetPVOutput()->GetPropertiesCreated() && hideSource)
        {
        input->SetVisibilityInternal(0);
        }
      }


    this->UnGrabFocus();

    // Set the current data of the window.
    if ( ! hideFlag)
      {
      window->SetCurrentPVSource(this);
      }
    else
      {
      this->SetVisibilityInternal(0);
      }

    // We need to update so we will have correct information for initialization.
    if (this->GetPVOutput())
      {
      // Update the VTK data.
      this->Update();
      }

    // The best test I could come up with to only reset
    // the camera when the first source is created.
    if (window->GetSourceList("Sources")->GetNumberOfItems() == 1)
      {
      float bds[6];
      this->GetDataInformation()->GetBounds(bds);
      window->SetCenterOfRotation(0.5*(bds[0]+bds[1]), 
                                  0.5*(bds[2]+bds[3]),
                                  0.5*(bds[4]+bds[5]));
      window->ResetCenterCallback();
      window->GetMainView()->GetRenderer()->ResetCamera(bds);
      }

    pvd->Initialize();
    this->Initialized = 1;
    }

  window->GetMenuView()->CheckRadioButton(
                                  window->GetMenuView(), "Radio", 2);
  this->UpdateProperties();
  this->GetPVRenderView()->EventuallyRender();

  // Update the selection menu.
  window->UpdateSelectMenu();

  // Regenerate the data property page in case something has changed.
  vtkPVData *pvd = this->GetPVOutput();
  if (pvd)
    {
    num = this->GetNumberOfPVParts();
    for (idx = 0; idx < num; ++idx)
      {
      part = this->GetPVPart(idx);
      // This has a side effect of gathering and displaying information.
      // Should this be part->Update() !!!!!!!!!!!!!!??????
      this->GetPVPart()->Update();
      }
    pvd->UpdateProperties();
    }

  this->Script("update");  

#ifdef _WIN32
  this->Script("%s configure -cursor arrow", window->GetWidgetName());
#else
  this->Script("%s configure -cursor left_ptr", window->GetWidgetName());
#endif  

  // Causes the data information to be updated if the filter executed.
  // Note has to be done here because tcl update causes render which
  // causes the filter to execute.
  pvd->UpdateProperties();

  window->UpdateFilterMenu();
}

//----------------------------------------------------------------------------
void vtkPVSource::ResetCallback()
{
  this->UpdateParameterWidgets();
  if (this->Initialized)
    {
    this->GetPVRenderView()->EventuallyRender();
    this->Script("update");

    this->SetAcceptButtonColorToWhite();
    }
}

//---------------------------------------------------------------------------
void vtkPVSource::DeleteCallback()
{
  int i;
  int initialized = this->Initialized;
  vtkPVSource *prev = NULL;
  vtkPVSource *current = 0;
  vtkPVWindow *window = this->GetPVWindow();

  if (this->GetPVOutput())
    {  
    this->GetPVOutput()->DeleteCallback();
    }

  // Just in case cursor was left in a funny state.
#ifdef _WIN32
  this->Script("%s configure -cursor arrow", window->GetWidgetName());
#else
  this->Script("%s configure -cursor left_ptr", window->GetWidgetName());
#endif  

  if ( ! this->Initialized)
    {
    // Remove the local grab
    this->UnGrabFocus();

    this->Script("update");   
    this->Initialized = 1;
    }
  
  if (this->GetNumberOfPVConsumers() > 0 )
    {
    vtkErrorMacro("An output is used.  We cannot delete this source.");
    return;
    }
  
  // Save this action in the trace file.
  this->GetPVApplication()->AddTraceEntry("$kw(%s) DeleteCallback",
                                          this->GetTclName());

  // Get the input so we can make it visible and make it current.
  if (this->GetNumberOfPVInputs() > 0)
    {
    prev = this->PVInputs[0];
    // Just a sanity check
    if (prev == NULL)
      {
      vtkErrorMacro("Expecting an input but none found.");
      }
    else
      {
      prev->SetVisibilityInternal(1);
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
  // Just remember it. We set the current pv source later.
  //this->GetPVWindow()->SetCurrentPVSourceCallback(prev);
  if ( prev == NULL && window->GetSourceList("Sources")->GetNumberOfItems() > 0 )
    {
    vtkCollectionIterator *it = window->GetSourceList("Sources")->NewIterator();
    it->InitTraversal();
    while ( !it->IsDoneWithTraversal() )
      {
      prev = static_cast<vtkPVSource*>( it->GetObject() );
      if ( prev != this )
        {
        break;
        }
      else
        {
        prev = 0;
        }
      it->GoToNextItem();
      }
    it->Delete();
    }

  current = window->GetCurrentPVSource();
  if ( this == current )
    {
    current = prev;
  
    if (prev == NULL)
      {
      // Unpack the properties.  This is required if prev is NULL.
      this->Script("catch {eval pack forget [pack slaves %s]}",
                   this->ParametersParent->GetWidgetName());
      
      // Show the 3D View settings
      vtkPVApplication *pvApp = 
        vtkPVApplication::SafeDownCast(this->Application);
      vtkPVWindow *window = pvApp->GetMainWindow();
      this->Script("%s invoke \"%s\"", 
                   window->GetMenuView()->GetWidgetName(),
                   VTK_PV_VIEW_MENU_LABEL);
      }
    }
        
  // Remove all of the actors mappers. from the renderer.
  if (this->PVOutput)
    {
    this->GetPVRenderView()->RemovePVSource(this);
    }    

  this->SetPVOutput(NULL);
  
  if ( initialized )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
  
  // This should delete this source.
  // "this" will no longer be valid after the call.
  window->RemovePVSource("Sources", this);
  window->SetCurrentPVSourceCallback(current);
}

//----------------------------------------------------------------------------
void vtkPVSource::VTKSourceModifiedMethod()
{
  if(!this->AcceptCallbackFlag)
    {
    this->UpdateParameterWidgets();
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateParameterWidgets()
{
  vtkPVWidget *pvw;
  vtkCollectionIterator *it = this->Widgets->NewIterator();
  it->InitTraversal();
  while( !it->IsDoneWithTraversal() )
    {
    pvw = static_cast<vtkPVWidget*>(it->GetObject());
    // Do not try to reset the widget if it is not initialized
    if (pvw->GetApplication())
      {
      pvw->Reset();
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkPVSource::RaiseSourcePage()
{
  this->Notebook->Raise("Source");
}

//----------------------------------------------------------------------------
// This should be apart of AcceptCallbackInternal.
void vtkPVSource::UpdateVTKSourceParameters()
{
  vtkPVWidget *pvw;
  vtkCollectionIterator *it;

  it = this->Widgets->NewIterator();
  it->InitTraversal();
  while( !it->IsDoneWithTraversal() )
    {
    pvw = static_cast<vtkPVWidget*>(it->GetObject());
    if (pvw->GetModifiedFlag())
      {
      pvw->Accept();
      }
    it->GoToNextItem();
    }
  it->Delete();
}



//----------------------------------------------------------------------------
void vtkPVSource::UpdateProperties()
{
  // --------------------------------------
  // Change the state of the delete button based on if there are any users.
  // Only filters at the end of a pipeline can be deleted.
  if ( this->IsDeletable() )
      {
      this->Script("%s configure -state normal",
                   this->DeleteButton->GetWidgetName());
      }
    else
      {
      this->Script("%s configure -state disabled",
                   this->DeleteButton->GetWidgetName());
      }
  
  this->UpdateDescriptionFrame();
}

//----------------------------------------------------------------------------
int vtkPVSource::IsDeletable()
{
  if (this->IsPermanent || this->GetNumberOfPVConsumers() > 0 )
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVSource::SetParametersParent(vtkKWWidget *parent)
{
  if (this->ParametersParent == parent)
    {
    return;
    }
  if (this->ParametersParent)
    {
    vtkErrorMacro("Cannot reparent properties.");
    return;
    }
  this->ParametersParent = parent;
  parent->Register(this);
}


  

//---------------------------------------------------------------------------
void vtkPVSource::SetNumberOfPVInputs(int num)
{
  int idx;
  vtkPVSource** inputs;

  // in case nothing has changed.
  if (num == this->NumberOfPVInputs)
    {
    return;
    }
  
  // Allocate new arrays.
  inputs = new vtkPVSource* [num];

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
void vtkPVSource::SetNthPVInput(int idx, vtkPVSource *pvs)
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
  if (pvs == this->PVInputs[idx])
    {
    return;
    }
  
  if (this->PVInputs[idx])
    {
    this->PVInputs[idx]->RemovePVConsumer(this);
    this->PVInputs[idx]->UnRegister(this);
    this->PVInputs[idx] = NULL;
    }
  
  if (pvs)
    {
    pvs->Register(this);
    pvs->AddPVConsumer(this);
    this->PVInputs[idx] = pvs;
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
      this->SetNthPVInput(idx, NULL);
      }

    delete [] this->PVInputs;
    this->PVInputs = NULL;
    this->NumberOfPVInputs = 0;
    this->Modified();
    }
}

//---------------------------------------------------------------------------
vtkPVSource *vtkPVSource::GetNthPVInput(int idx)
{
  if (idx >= this->NumberOfPVInputs)
    {
    return NULL;
    }
  
  return (vtkPVSource *)(this->PVInputs[idx]);
}


//---------------------------------------------------------------------------
void vtkPVSource::SetPVOutput(vtkPVData *pvd)
{  
  // Does this change anything?  Yes, it keeps the object from being modified.
  if (pvd == this->PVOutput)
    {
    return;
    }
  
  if (this->PVOutput)
    {
    // Manage backward pointer.
    this->PVOutput->SetPVSource(this);
    this->PVOutput->UnRegister(this);
    this->PVOutput = NULL;
    }
  
  if (pvd)
    {
    pvd->Register(this);
    this->PVOutput = pvd;
    // Manage backward pointer.
    pvd->SetPVSource(this);
    }

  this->Modified();
}


//----------------------------------------------------------------------------
void vtkPVSource::SaveInBatchScript(ofstream *file)
{
  int i, numWidgets;
  vtkPVWidget *widget;

  // Detect special sources we do not handle yet.
  if (this->GetVTKSource() == NULL)
    {
    return;
    }

  // This should not be needed, but We can check anyway.
  if (this->VisitedFlag)
    {
    return;
    }

  // This is the recursive part.
  this->VisitedFlag = 1;
  // Loop through all of the inputs
  for (i = 0; i < this->NumberOfPVInputs; ++i)
    {
    if (this->PVInputs[i] && this->PVInputs[i]->GetVisitedFlag() != 2)
      {
      this->PVInputs[i]->SaveInBatchScript(file);
      }
    }
  
  // Save the object in the script.
  *file << "\n"; 
  int numSources, sourceIdx;
  numSources = this->GetNumberOfVTKSources();
  for (sourceIdx = 0; sourceIdx < numSources; ++sourceIdx)
    {
    *file << this->GetVTKSource(sourceIdx)->GetClassName()
          << " " << this->GetVTKSourceTclName(sourceIdx) << "\n";
    }

  // Handle this here.
  this->SetInputsInBatchScript(file);

  // Let the PVWidgets set up the object.
  numWidgets = this->Widgets->GetNumberOfItems();
  for (i = 0; i < numWidgets; i++)
    {
    widget = vtkPVWidget::SafeDownCast(this->Widgets->GetItemAsObject(i));
    if (widget)
      {
      widget->SaveInBatchScript(file);
      }
    }

  // Add the mapper, actor, scalar bar actor ...
  this->GetPVOutput()->SaveInBatchScript(file);
}

//----------------------------------------------------------------------------
void vtkPVSource::SaveState(ofstream *file)
{
  int i, numWidgets;
  vtkPVWidget *widget;

  // Detect if this source is in Glyph sourcesm and already exists.
  if (this->GetTraceReferenceCommand())
    {
    *file << "set kw(" << this->GetTclName() << ") [$kw(" 
          << this->GetTraceReferenceObject()->GetTclName() << ") " 
          << this->GetTraceReferenceCommand() << "]\n";
    return;
    }

  // Detect special sources we do not handle yet.
  if (this->GetVTKSource() == NULL)
    {
    return;
    }

  // This should not be needed, but We can check anyway.
  if (this->VisitedFlag)
    {
    return;
    }

  // This is the recursive part.
  this->VisitedFlag = 1;
  // Loop through all of the inputs
  for (i = 0; i < this->NumberOfPVInputs; ++i)
    {
    if (this->PVInputs[i] && this->PVInputs[i]->GetVisitedFlag() != 2)
      {
      this->PVInputs[i]->SaveState(file);
      }
    }
  
  // We have to set the first input as the current source,
  // because CreatePVSource uses it as default input.
  // We may not have a input menu to set it for us.
  if (this->GetPVInput(0))
    {
    *file << "$kw(" << this->GetPVWindow()->GetTclName() << ") "
          << "SetCurrentPVSourceCallback $kw("
          << this->GetPVInput(0)->GetTclName() << ")\n";
    }

  // Save the object in the script.
  *file << "set kw(" << this->GetTclName() << ") "
        << "[$kw(" << this->GetPVWindow()->GetTclName() << ") "
        << "CreatePVSource " << this->GetModuleName() << "]" << endl;

  // Let the PVWidgets set up the object.
  numWidgets = this->Widgets->GetNumberOfItems();
  for (i = 0; i < numWidgets; i++)
    {
    widget = vtkPVWidget::SafeDownCast(this->Widgets->GetItemAsObject(i));
    if (widget)
      {
      widget->SaveState(file);
      }
    }

  // Call accept.
  *file << "$kw(" << this->GetTclName() << ") AcceptCallback" << endl;

  // What about visibility?  
  // A second pass to set visibility?
  // Can we disable rendering and changing input visibility?

  // Let the output set its state.
  this->GetPVOutput()->SaveState(file);
}



//----------------------------------------------------------------------------
// Duplicates functionality in Input menu.
// This method sets inputs for standard input.
// Others (source) are set by InputMenuWidget.
void vtkPVSource::SetInputsInBatchScript(ofstream *file)
{
  int num, idx;
  int numSources, numOutputs;
  int sourceCount, outputCount;
  vtkPVSource *pvs;

  if (this->GetNumberOfPVInputs() == 0)
    {
    return;
    }

  num = this->GetNumberOfVTKSources();

  // Special case for filters that take multiple inputs like append.
  if (this->GetVTKMultipleInputsFlag())
    {
    if (num != 1)
      {
      vtkErrorMacro("Expecting only a single source.");
      return;
      }
    // Loop through all of the PVSources.
    for (idx = 0; idx < this->NumberOfPVInputs; ++idx)
      {
      pvs = this->GetNthPVInput(idx);
      if (pvs == NULL)
        {
        vtkErrorMacro("Empty Input.");
        return;
        }
      // Loop through all of the vtk sources (group).
      numSources = pvs->GetNumberOfVTKSources();
      for (sourceCount = 0; sourceCount < numSources; ++sourceCount)
        {
        // Loop through all of the outputs of this vtk source (multiple outputs).
        numOutputs = pvs->GetVTKSource(sourceCount)->GetNumberOfOutputs();
        for (outputCount = 0; outputCount < numOutputs; ++outputCount)
          {
          *file << "\t";
          // This is a bit of a hack to get the input name.
          *file << this->GetVTKSourceTclName(0) << " AddInput [" 
                << pvs->GetVTKSourceTclName(sourceCount) 
                << " GetOutput " << outputCount << "]\n";
          }
        }
      }
    return;
    }


  // Just PVInput 0 for now.
  pvs = this->GetNthPVInput(0);
  // Maybe this output traversal should be a part of PVSource.
  numSources = pvs->GetNumberOfVTKSources();
  sourceCount = -1;
  numOutputs = 0;
  outputCount = 0;
  if (pvs == NULL)
    {
    return;
    }
  // Loop through our sources.
  for (idx = 0; idx < num; ++idx)
    {
    // Pick off the input outputs one by one.
    while (outputCount >= numOutputs)
      {
      ++sourceCount;
      if (sourceCount >= numSources)
        { // sanity check.
        vtkErrorMacro("Ran out of sources.");
        return;
        }
      outputCount = 0;
      numOutputs = pvs->GetVTKSource(sourceCount)->GetNumberOfOutputs();
      }

    *file << "\t";
    *file << this->GetVTKSourceTclName(idx) << " Set"
          << this->GetInputProperty(idx)->GetName() << " [" 
          << pvs->GetVTKSourceTclName(sourceCount) 
          << " GetOutput " << outputCount << "]\n";
    
    ++outputCount;
    }
}


//----------------------------------------------------------------------------
void vtkPVSource::AddPVWidget(vtkPVWidget *pvw)
{
  char str[512];
  this->Widgets->AddItem(pvw);  

  if (pvw->GetTraceName() == NULL)
    {
    vtkWarningMacro("TraceName not set. Widget class: " 
                    << pvw->GetClassName());
    return;
    }

  pvw->SetTraceReferenceObject(this);
  sprintf(str, "GetPVWidget {%s}", pvw->GetTraceName());
  pvw->SetTraceReferenceCommand(str);
  pvw->Select();
}


//----------------------------------------------------------------------------
vtkIdType vtkPVSource::GetNumberOfInputProperties()
{
  return this->InputProperties->GetNumberOfItems();
}

//----------------------------------------------------------------------------
vtkPVInputProperty* vtkPVSource::GetInputProperty(int idx)
{
  return static_cast<vtkPVInputProperty*>(this->InputProperties->GetItemAsObject(idx));
}


//----------------------------------------------------------------------------
vtkPVInputProperty* vtkPVSource::GetInputProperty(const char* name)
{
  int idx, num;
  vtkPVInputProperty *inProp;
  
  num = this->GetNumberOfInputProperties();
  for (idx = 0; idx < num; ++idx)
    {
    inProp = static_cast<vtkPVInputProperty*>(this->GetInputProperty(idx));
    if (strcmp(name, inProp->GetName()) == 0)
      {
      return inProp;
      }
    }

  // Propery has not been created yet.  Create and  save one.
  inProp = vtkPVInputProperty::New();
  inProp->SetName(name);
  this->InputProperties->AddItem(inProp);
  inProp->Delete();

  return inProp;
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

//----------------------------------------------------------------------------
vtkPVInputMenu *vtkPVSource::AddInputMenu(char *label, char *inputName, 
                                          char *help, 
                                          vtkPVSourceCollection *sources)
{
  vtkPVInputMenu *inputMenu;

  inputMenu = vtkPVInputMenu::New();
  inputMenu->SetPVSource(this);
  inputMenu->SetSources(sources);
  inputMenu->SetParent(this->ParameterFrame->GetFrame());
  inputMenu->SetLabel(label);
  inputMenu->SetModifiedCommand(this->GetTclName(), 
                                "SetAcceptButtonColorToRed");
  inputMenu->Create(this->Application);
  inputMenu->SetInputName(inputName);
  inputMenu->SetBalloonHelpString(help);

  this->AddPVWidget(inputMenu);
  inputMenu->Delete();

  return inputMenu;
}

//----------------------------------------------------------------------------
int vtkPVSource::CloneAndInitialize(int makeCurrent, vtkPVSource*& clone )
{

  int retVal = this->ClonePrototypeInternal(clone);
  if (retVal != VTK_OK)
    {
    return retVal;
    }

  vtkPVSource *current = this->GetPVWindow()->GetCurrentPVSource();
  retVal = clone->InitializeClone(current, makeCurrent);

  if (retVal != VTK_OK)
    {
    clone->Delete();
    clone = 0;
    return retVal;
    }

  // Accept button is always red when a source is first created.
  clone->SetAcceptButtonColorToRed();

  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkPVSource::ClonePrototype(vtkPVSource*& clone)
{
  return this->ClonePrototypeInternal(clone);
}

//----------------------------------------------------------------------------
int vtkPVSource::ClonePrototypeInternal(vtkPVSource*& clone)
{
  int idx;

  clone = 0;

  // Create instance
  vtkPVSource* pvs = this->NewInstance();
  // Copy properties
  pvs->SetApplication(this->Application);
  pvs->SetReplaceInput(this->ReplaceInput);
  pvs->SetParametersParent(this->ParametersParent);

  pvs->SetShortHelp(this->GetShortHelp());
  pvs->SetLongHelp(this->GetLongHelp());
  pvs->SetVTKMultipleInputsFlag(this->GetVTKMultipleInputsFlag());

  pvs->SetSourceClassName(this->SourceClassName);
  // Copy the VTK input stuff.
  vtkIdType numItems = this->GetNumberOfInputProperties();
  vtkIdType id;
  vtkPVInputProperty *inProp;
  for(id=0; id<numItems; id++)
    {
    inProp = this->GetInputProperty(id);
    pvs->GetInputProperty(inProp->GetName())->Copy(inProp);
    }

  pvs->SetModuleName(this->ModuleName);

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(pvs->Application);
  if (!pvApp)
    {
    vtkErrorMacro("vtkPVApplication is not set properly. Aborting clone.");
    pvs->Delete();
    return VTK_ERROR;
    }

  // Create a (unique) name for the PVSource.
  // Beware: If two prototypes have the same name, the name 
  // will not be unique.
  // Does this name really have to be unique?
  char tclName[1024];
  if (this->Name && this->Name[0] != '\0')
    { 
    sprintf(tclName, "%s%d", this->Name, this->PrototypeInstanceCount);
    }
  else
    {
    vtkErrorMacro("The prototype must have a name. Cloning aborted.");
    pvs->Delete();
    return VTK_ERROR;
    }
  pvs->SetName(tclName);


  // We need oue source for each part.
  int numSources = 1;
  // Set the input if necessary.
  if (this->GetNumberOfInputProperties() > 0)
    {
    vtkPVSource *input = this->GetPVWindow()->GetCurrentPVSource();
    numSources = input->GetNumberOfPVParts();
    }
  // If the VTK filter takes multiple inputs (vtkAppendPolyData)
  // then we create only one filter with each part as an input.
  if (this->GetVTKMultipleInputsFlag())
    {
    numSources = 1;
    }

  for (idx = 0; idx < numSources; ++idx)
    {
    if (numSources > 1)
      {
      // Create a (unique) name for the source.
      // Beware: If two prototypes have the same name, the name 
      // will not be unique.
      sprintf(tclName, "%s_%d", pvs->GetName(), idx);
      }

    // Create a vtkSource
    vtkSource* vtksource = 
      static_cast<vtkSource *>(pvApp->MakeTclObject(this->SourceClassName, 
                                                    tclName));
    if (vtksource == NULL)
      {
      vtkErrorMacro("Could not get pointer from object.");
      pvs->Delete();
      return VTK_ERROR;
      }

    pvs->AddVTKSource(vtksource, tclName);
    }

  pvs->SetView(this->GetPVWindow()->GetMainView());

  pvs->PrototypeInstanceCount = this->PrototypeInstanceCount;
  this->PrototypeInstanceCount++;

  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* widgetMap =
    vtkArrayMap<vtkPVWidget*, vtkPVWidget*>::New();


  // Copy all widgets
  vtkPVWidget *pvWidget, *clonedWidget;
  this->Widgets->InitTraversal();
  int i;
  for (i = 0; i < this->Widgets->GetNumberOfItems(); i++)
    {
    pvWidget = this->Widgets->GetNextPVWidget();
    clonedWidget = pvWidget->ClonePrototype(pvs, widgetMap);
    pvs->AddPVWidget(clonedWidget);
    clonedWidget->Delete();
    }
  widgetMap->Delete();

  clone = pvs;
  return VTK_OK;
}


//----------------------------------------------------------------------------
int vtkPVSource::InitializeClone(vtkPVSource* input,
                                 int makeCurrent)

{
  // Set the input if necessary.
  if (this->GetNumberOfInputProperties() > 0)
    {
    this->SetPVInput(0, input);
    }

  // Create the properties frame etc.
  this->CreateProperties();

  if (makeCurrent)
    {
    this->GetPVWindow()->SetCurrentPVSourceCallback(this);
    } 

  return VTK_OK;
}



//----------------------------------------------------------------------------
// This stuff used to be in Initialize clone.
// I want to initialize the output after accept is called.
int vtkPVSource::InitializeData()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  int numSources, sourceIdx;
  vtkSource* source;
  char* sourceTclName;
  int numOutputs, idx;
  char dataName[1024];
  int outputCount = 0;
  vtkPVPart* part;
  vtkPVSource* input = this->GetPVInput(0);
  vtkPVData* pvd;

  // Create the output.
  pvd = vtkPVData::New();
  pvd->SetPVApplication(pvApp);

  numSources = this->GetNumberOfVTKSources();
  for (sourceIdx = 0; sourceIdx < numSources; ++sourceIdx)
    {
    source = this->GetVTKSource(sourceIdx);
    sourceTclName = this->GetVTKSourceTclName(sourceIdx);
    numOutputs = source->GetNumberOfOutputs();

    // When ever the source executes, data information needs to be recomputed.
    this->Script("%s AddObserver EndEvent {%s InvalidateDataInformation}", 
                 sourceTclName, this->GetTclName());
    for (idx = 0; idx < numOutputs; ++idx)
      {
      ++outputCount;
      sprintf(dataName, "%sOutput%d", this->GetName(), outputCount);
      pvApp->BroadcastScript("set %s [%s GetOutput %d]", dataName, 
                             sourceTclName, idx);
      part = vtkPVPart::New();
      part->SetPVApplication(pvApp);
      this->Script("%s SetVTKData ${%s} {${%s}}", part->GetTclName(),
                   dataName, dataName);
      pvApp->GetProcessModule()->InitializePVPartPartition(part);
      this->AddPVPart(part);
      // This initialization should be done by a render module.
      part->SetCollectionDecision(this->GetPVWindow()->MakeCollectionDecision());
      part->SetLODCollectionDecision(this->GetPVWindow()->MakeLODCollectionDecision());

      // Create the extent translator (sources with no inputs only).
      // Needs to be before "ExtractPieces" because translator propagates.
      char translatorTclName[1024];
      translatorTclName[0] = '\0';
      if ( ! input)
        {
        sprintf(translatorTclName, "%sTranslator%d", this->GetName(), idx);
        pvApp->BroadcastScript("vtkPVExtentTranslator %s", translatorTclName);
        pvApp->BroadcastScript("$%s SetExtentTranslator %s",
                               dataName, translatorTclName);
        }

      part->InsertExtractPiecesIfNecessary();
      
      if (translatorTclName[0])
        {
        // Translator has to be set on source because it is propagated.
        // Original source is set after transmit so reference
        // loop can be broken when pvPart is deleted.
        pvApp->BroadcastScript("%s SetOriginalSource $%s",
                               translatorTclName, dataName);
        pvApp->BroadcastScript("%s Delete", translatorTclName);
        }

      part->Delete();  
      }
    }

  this->SetPVOutput(pvd);
  pvd->Delete();

  return VTK_OK;
}


//----------------------------------------------------------------------------
void vtkPVSource::SetLODResolution(int dim)
{
  vtkPVPart *part;
  int idx, num;
  vtkPVApplication *pvApp = this->GetPVApplication();
    
  if (this->LODResolution == dim)
    {
    return;
    }
  this->LODResolution = dim;

  num = this->GetNumberOfPVParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPVPart(idx);
    pvApp->BroadcastScript("%s SetNumberOfDivisions %d %d %d", 
                           part->GetLODDeciTclName(), this->LODResolution,
                           this->LODResolution, this->LODResolution); 
    }
}



//----------------------------------------------------------------------------
void vtkPVSource::SerializeSelf(ostream& os, vtkIndent indent)
{
  int cc;
  this->Superclass::SerializeSelf(os, indent);
  os << indent << "ModuleName " << this->ModuleName << endl;
  os << indent << "HideDisplayPage " << this->HideDisplayPage << endl;
  os << indent << "HideParametersPage " << this->HideParametersPage << endl;
  os << indent << "HideInformationPage " << this->HideInformationPage << endl;
  os << indent << "NumberOfPVInputs " << this->GetNumberOfPVInputs() << endl;
  for ( cc = 0; cc < this->GetNumberOfPVInputs(); cc ++ )
    {
    if ( this->GetNthPVInput(cc) )
      {
      os << indent << "Input " << cc << " " 
         << this->GetNthPVInput(cc)->GetName() 
         << endl;
      }
    }
  os << indent << "Output ";
  this->GetPVOutput()->Serialize(os, indent);
  if ( this->Widgets )
    {
    os << indent << "Widgets " << endl;
    vtkCollectionIterator* it = this->Widgets->NewIterator();
    it->InitTraversal();
    vtkIndent indentp = indent.GetNextIndent();
    os << indentp << "{" << endl;
    int cc =0;
    while( !it->IsDoneWithTraversal() )
      {
      vtkPVWidget* widget = static_cast<vtkPVWidget*>( it->GetObject() );
      os << indentp << "Widget \"" << widget->GetTraceName() << "\" ";
      widget->Serialize(os, indentp);

      it->GoToNextItem();
      cc ++;
      }
    os << indentp << "}" << endl;
    it->Delete();
    }
}

//------------------------------------------------------------------------------
void vtkPVSource::SerializeToken(istream& is, const char token[1024])
{
  if ( vtkString::Equals(token, "HideDisplayPage") )
    {
    int cor = 0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->HideDisplayPage = cor;
    }
  else if ( vtkString::Equals(token, "HideParametersPage") )
    {
    int cor;
    cor = 0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->HideParametersPage = cor;
    }
  else if ( vtkString::Equals(token, "HideInformationPage") )
    {
    int cor;
    cor = 0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->HideInformationPage = cor;
    }
  else if ( vtkString::Equals(token, "ModuleName") )
    {
    char name[1024];
    name[0] = 0;
    if ( ! (is >> name ) )
      {
      vtkErrorMacro("Problem Parsing session file");
      return;
      }
    this->SetModuleName(name);
    }
  else if ( vtkString::Equals(token, "Output") )
    {
    this->Accept(0, 0);
    vtkPVData* output = this->GetPVOutput();
    if (!output )
      {
      output = vtkPVData::New();
      output->SetPVApplication(
        vtkPVApplication::SafeDownCast(this->Application));
      this->SetPVOutput(output);
      output->Delete();
      }
    //this->AcceptCallback();
    output->Serialize(is);
    }
  else if ( vtkString::Equals(token, "NumberOfPVInputs") )
    {
    int cor = 0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    this->SetNumberOfPVInputs(cor);
    }
  else if ( vtkString::Equals(token, "Input--does-not-work") )
    {
    int cor = 0;
    if (! (is >> cor) )
      {
      vtkErrorMacro("Problem Parsing session file");
      }
    //cout << "Reading input: " << cor << endl;
    char name[1024];
    name[0] = 0;
    if ( ! (is >> name ) )
      {
      vtkErrorMacro("Problem Parsing session file");
      return;
      }
    //cout << "Input " << cor << " is: " << name << endl;
    vtkPVApplication *application 
      = vtkPVApplication::SafeDownCast(this->Application);
    vtkPVSource* isource 
      = application->GetMainWindow()->GetSourceFromName(name);
    if ( !isource )
      {
      vtkErrorMacro("Cannot find input to this source");
      return;
      }
    this->SetNthPVInput(cor, isource);
    this->UpdateParameterWidgets();
    this->Accept(0,0);
    }
  else if ( vtkString::Equals(token, "Widgets") )
    {
    char ntoken[1024];
    int done = 0;;
    while(!done)
      {
      ntoken[0] = 0;
      if ( !( is >> ntoken ) )
        {
        vtkErrorMacro("Error when parsing widgets");
        return;
        }
      if ( vtkString::Equals(ntoken, "}") )
        {
        done = 1;
        }
      else if ( vtkString::Equals(ntoken, "Widget") )
        {
        char tracename[1024];
        vtkKWSerializer::GetNextToken(&is,tracename);
        vtkPVWidget* widget = this->GetPVWidget(tracename);
        if ( !widget )
          {
          vtkErrorMacro("Widget " << tracename << " does not exists in "
                        << this->GetModuleName());
          return;
          }
        //cout << "Widget: " << tracename << " (" << widget << ") " 
        //     << widget->GetClassName() << endl;
        widget->Serialize(is);
        }
      else 
        {
        //cout << "Unknown token: " << ntoken << endl;
        }
      }
    }
  else
    {
    //cout << "Unknown Token for " << this->GetClassName() << ": " 
    //     << token << endl;
    this->Superclass::SerializeToken(is,token);  
    }
}

//------------------------------------------------------------------------------
void vtkPVSource::SerializeRevision(ostream& os, vtkIndent indent)
{
  this->Superclass::SerializeRevision(os,indent);
  os << indent << "vtkPVSource ";
  this->ExtractRevision(os,"$Revision: 1.280 $");
}

//----------------------------------------------------------------------------
void vtkPVSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Initialized: " << (this->Initialized?"yes":"no") << endl;
  os << indent << "Name: " << (this->Name ? this->Name : "none") << endl;
  os << indent << "LongHelp: " << (this->LongHelp ? this->LongHelp : "none") 
     << endl;
  os << indent << "ShortHelp: " << (this->ShortHelp ? this->ShortHelp : "none") 
     << endl;
  os << indent << "Description: " 
     << (this->Label ? this->Label : "none") << endl;
  os << indent << "ModuleName: " << (this->ModuleName?this->ModuleName:"none")
     << endl;
  os << indent << "MenuName: " << (this->MenuName?this->MenuName:"none")
     << endl;
  os << indent << "AcceptButton: " << this->GetAcceptButton() << endl;
  os << indent << "DeleteButton: " << this->GetDeleteButton() << endl;
  os << indent << "MainParameterFrame: " << this->GetMainParameterFrame() 
     << endl;
  os << indent << "DescriptionFrame: " << this->DescriptionFrame 
     << endl;
  os << indent << "Notebook: " << this->GetNotebook() << endl;
  os << indent << "NumberOfPVInputs: " << this->GetNumberOfPVInputs() << endl;
  os << indent << "ParameterFrame: " << this->GetParameterFrame() << endl;
  os << indent << "ParametersParent: " << this->GetParametersParent() << endl;
  os << indent << "ReplaceInput: " << this->GetReplaceInput() << endl;
  os << indent << "View: " << this->GetView() << endl;
  os << indent << "VisitedFlag: " << this->GetVisitedFlag() << endl;
  os << indent << "Widgets: " << this->GetWidgets() << endl;
  os << indent << "IsPermanent: " << this->IsPermanent << endl;
  os << indent << "SourceClassName: " 
     << (this->SourceClassName?this->SourceClassName:"null") << endl;
  os << indent << "HideParametersPage: " << this->HideParametersPage << endl;
  os << indent << "HideDisplayPage: " << this->HideDisplayPage << endl;
  os << indent << "HideInformationPage: " << this->HideInformationPage << endl;
  os << indent << "ToolbarModule: " << this->ToolbarModule << endl;

  os << indent << "PVOutput: " << this->PVOutput << endl;
  os << indent << "NumberOfPVConsumers: " << this->GetNumberOfPVConsumers() << endl;
  os << indent << "NumberOfPVParts: " << this->GetNumberOfPVParts() << endl;

  os << indent << "VTKMultipleInputsFlag: " << this->VTKMultipleInputsFlag << endl;
  os << indent << "InputProperties: \n";
  vtkIndent i2 = indent.GetNextIndent();
  int num, idx;
  num = this->GetNumberOfInputProperties();
  for (idx = 0; idx < num; ++idx)
    {
    this->GetInputProperty(idx)->PrintSelf(os, i2);
    }
  os << indent << "LODResolution: " << this->LODResolution << endl;
}
