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
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWLabeledLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWNotebook.h"
#include "vtkKWPushButton.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVPart.h"
#include "vtkPVPartDisplay.h"
#include "vtkPVInputProperty.h"
#include "vtkPVNumberOfOutputsInformation.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderModule.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVWidgetProperty.h"
#include "vtkPVWindow.h"
#include "vtkSource.h"
#include "vtkString.h"
#include "vtkRenderer.h"
#include "vtkArrayMap.txx"
#include "vtkStringList.h"
#include "vtkCollection.h"
#include "vtkPVAnimationInterface.h"
#include <vtkstd/vector>

class vtkClientServerIDList : public vtkstd::vector<vtkClientServerID>
{
public:
};


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSource);
vtkCxxRevisionMacro(vtkPVSource, "1.313.2.8");

int vtkPVSourceCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

vtkCxxSetObjectMacro(vtkPVSource, View, vtkKWView);

//----------------------------------------------------------------------------
vtkPVSource::vtkPVSource()
{
  this->CommandFunction = vtkPVSourceCommand;

  this->Parts = vtkCollection::New();
  this->DataInformation = vtkPVDataInformation::New();
  this->DataInformationValid = 0;

  this->NumberOfOutputsInformation = vtkPVNumberOfOutputsInformation::New();
  
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
  this->AcceptButtonRed = 0;

  // The notebook which holds Parameters, Display and Information pages.
  this->Notebook = vtkKWNotebook::New();
  this->Notebook->AlwaysShowTabsOn();

  this->PVInputs = NULL;
  this->NumberOfPVInputs = 0;
  this->PVOutput = NULL;

  this->NumberOfPVConsumers = 0;
  this->PVConsumers = 0;


  // The underlying VTK objects. PVSource supports multiple VTK sources/filters.
  this->VTKSourceIDs = new vtkClientServerIDList;
  this->VTKSourceOuputIDs = new vtkClientServerIDList;
  
  
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
      
  this->WidgetProperties = vtkCollection::New();
    
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

  this->UpdateSourceInBatch = 0;

  this->LabelSetByUser = 0;
}

//----------------------------------------------------------------------------
vtkPVSource::~vtkPVSource()
{
  delete this->VTKSourceOuputIDs;
  this->SetPVOutput(NULL);  
  this->RemoveAllPVInputs();

  this->Parts->Delete();
  this->Parts = NULL;
  this->DataInformation->Delete();
  this->DataInformation = NULL;

  this->NumberOfOutputsInformation->Delete();
  this->NumberOfOutputsInformation = NULL;
  
  if (this->PVConsumers)
    {
    delete [] this->PVConsumers;
    this->PVConsumers = NULL;
    this->NumberOfPVConsumers = 0;
    }

  // We need to delete the Tcl object too.  This call does it.
  this->RemoveAllVTKSources();

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

  this->WidgetProperties->Delete();
  this->WidgetProperties = NULL;

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
  vtkClientServerID sourceID;
  const char* inputName;

  if (pvApp == NULL)
    {
    vtkErrorMacro(
      "No Application. Create the source before setting the input.");
    return;
    }
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
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
  numParts = pvs->GetNumberOfParts();
  vtkClientServerStream& stream = pm->GetStream();
  for (partIdx = 0; partIdx < numParts; ++partIdx)
    {
    part = pvs->GetPart(partIdx);
    if (this->VTKMultipleInputsFlag)
      { // Only one source takes all parts as input.
      sourceID = this->GetVTKSourceID(0);
      
      if (part->GetVTKDataID().ID == 0 || sourceID.ID == 0)
        { // Sanity check.
        vtkErrorMacro("Missing id.");
        }
      else
        {
        stream << vtkClientServerStream::Invoke << sourceID << "Add" << part->GetVTKDataID() 
               << vtkClientServerStream::End;
        pm->SendStreamToClientAndServer();
        }      
      }
    else
      { // One source for each part.
      sourceID = this->GetVTKSourceID(partIdx);
      if (part->GetVTKDataID().ID == 0 || sourceID.ID == 0)
        {
        vtkErrorMacro("Source data mismatch.");
        }
      else
        {
        stream << vtkClientServerStream::Invoke << sourceID << "Set" << part->GetVTKDataID() 
               << vtkClientServerStream::End;
        pm->SendStreamToClientAndServer();
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
int vtkPVSource::GetNumberOfParts()
{
  return this->Parts->GetNumberOfItems();
}

//----------------------------------------------------------------------------
void vtkPVSource::SetPart(vtkPVPart* part)
{
  if ( !part )
    {
    return;
    }
  this->Parts->AddItem(part);
}

//----------------------------------------------------------------------------
void vtkPVSource::AddPart(vtkPVPart* part)
{
  if ( !part )
    {
    return;
    }
  this->Parts->AddItem(part);
}

//----------------------------------------------------------------------------
vtkPVPart* vtkPVSource::GetPart(int idx)
{
  vtkPVPart *part;

  part = static_cast<vtkPVPart*>(this->Parts->GetItemAsObject(idx));
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
  vtkPVPart* part;
  this->DataInformationValid = 0;

  // All parts get invalidated too.
  this->Parts->InitTraversal();
  while ( ( part = (vtkPVPart*)(this->Parts->GetNextItemAsObject())) )
    {
    part->InvalidateDataInformation();
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::GatherDataInformation()
{
  vtkPVPart *part;

  this->DataInformation->Initialize();
  this->Parts->InitTraversal();
  while ( ( part = (vtkPVPart*)(this->Parts->GetNextItemAsObject())) )
    {
    part->GatherDataInformation();
    this->DataInformation->AddInformation(part->GetDataInformation());
    }
  this->DataInformationValid = 1;

  if (this->GetPVOutput())
    {
    // This will cause a recursive call, but the Valid flag will terminate it.
    this->GetPVOutput()->UpdateProperties();
    }
}


//----------------------------------------------------------------------------
void vtkPVSource::ForceUpdate(vtkPVApplication* vtkNotUsed(pvApp))
{
  vtkPVPart *part;

  if (this->UpdateTime > this->PipelineModifiedTime)
    {
    return;
    }
  this->UpdateTime.Modified();

  this->Parts->InitTraversal();
  while ( (part = (vtkPVPart*)(this->Parts->GetNextItemAsObject())) )
    {
    part->GetPartDisplay()->Update();
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::Update()
{
  vtkPVPart *part;

  this->Parts->InitTraversal();
  while ( (part = (vtkPVPart*)(this->Parts->GetNextItemAsObject())) )
    {
    if (part->GetPartDisplay())
      {
      part->GetPartDisplay()->Update();
      }
    }
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

  num = this->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = this->GetPart(idx);
    part->GetPartDisplay()->SetVisibility(v);
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


void vtkPVSource::AddVTKSource(vtkClientServerID id)
{
  this->VTKSourceIDs->push_back(id);
}

//----------------------------------------------------------------------------
// Tcl does the reference counting, so we are not going to put an 
// additional reference of the data.
void vtkPVSource::AddVTKSource(const char *tclName)
{
  vtkErrorMacro("No longer supported.");
}

//----------------------------------------------------------------------------
void vtkPVSource::RemoveAllVTKSources()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int num, idx;

  num = this->GetNumberOfVTKSources();
  for (idx = 0; idx < num; ++ idx)
    {
    vtkClientServerID id = this->GetVTKSourceID(idx);
    vtkClientServerStream& stream = pvApp->GetProcessModule()->GetStream();
    stream << vtkClientServerStream::Delete << id << vtkClientServerStream::End;
    pvApp->GetProcessModule()->SendStreamToServer();
    }

  delete this->VTKSourceIDs;
}

//----------------------------------------------------------------------------
int vtkPVSource::GetNumberOfVTKSources()
{
  return this->VTKSourceIDs->size();
}

//----------------------------------------------------------------------------
vtkClientServerID vtkPVSource::GetVTKSourceID(int idx)
{
  return (*this->VTKSourceIDs)[idx];
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
                             "-text Accept");
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
  vtkPVWidgetProperty *pvwProp;
  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  
  int i;
  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    pvwProp = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    pvWidget = pvwProp->GetWidget();
    pvWidget->SetParent(this->ParameterFrame->GetFrame());
    pvWidget->Create(this->Application);
    this->Script("pack %s -side top -fill x -expand t", 
                 pvWidget->GetWidgetName());
    it->GoToNextItem();
    }
  it->Delete();
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
    if (this->GetSourceClassName()) 
      {
      this->TypeLabel->GetLabel2()->SetLabel(
        this->GetSourceClassName());
//        this->GetVTKSource()->GetClassName());
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
        !(this->GetPVApplication() && 
          !this->GetPVApplication()->GetShowSourcesLongHelp())) 
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
  this->GetPVRenderView()->UpdateTclButAvoidRendering();

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
  vtkPVWidgetProperty *pvwProp = 0;
  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    pvwProp = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    pvwProp->GetWidget()->Select();
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkPVSource::Deselect(int doPackForget)
{
  if (doPackForget)
    {
    this->Script("pack forget %s", this->Notebook->GetWidgetName());
    }
  int i;
  vtkPVWidgetProperty *pvwProp = 0;
  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  
  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    pvwProp = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    pvwProp->GetWidget()->Deselect();
    it->GoToNextItem();
    }
  it->Delete();
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
void vtkPVSource::SetLabelOnce(const char* arg) 
{
  if (!this->LabelSetByUser)
    {
    this->SetLabelNoTrace(arg);
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::SetLabel(const char* arg) 
{ 
  this->LabelSetByUser = 1;

  this->SetLabelNoTrace(arg);

  if ( !this->Application )
    {
    return;
    }
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
  if ( ! this->AcceptButtonRed)
    {
    return;
    }
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

  // vtkPVSource is taking over some of the update descisions because
  // client does not have a real pipeline.
  if ( ! this->AcceptButtonRed)
    {
    return;
    } 

  window = this->GetPVWindow();

  this->SetAcceptButtonColorToWhite();
  this->GetPVRenderView()->UpdateTclButAvoidRendering();
  
  // We need to pass the parameters from the UI to the VTK objects before
  // we check whether to insert ExtractPieces.  Otherwise, we'll get errors
  // about unspecified file names, etc., when ExecuteInformation is called on
  // the VTK source.  (The vtkPLOT3DReader is a good example of this.)
  this->UpdateVTKSourceParameters();

  this->MarkSourcesForUpdate(1);

  // Moved from creation of the source. (InitializeClone)
  // Initialize the output if necessary.
  // This has to be after the widgets are accepted (UpdateVTKSOurceParameters)
  // because they can change the number of parts.
  if (this->GetPVOutput() == NULL)
    { // This is the first time, create the data.
    this->InitializeData();
    }
    
  // Initialize the output if necessary.
  if ( ! this->Initialized)
    { // This is the first time, initialize data.    
    vtkPVData *pvd;
    
    this->Initialized = 1;
    pvd = this->GetPVOutput();
    if (pvd == NULL)
      { // I suppose we should try and delete the source.
      vtkErrorMacro("Could not get output.");
      this->DeleteCallback();    
      return;
      }

    if (!this->GetHideDisplayPage())
      {
      this->Notebook->AddPage("Display");
      }
    if (!this->GetHideInformationPage())
      {
      this->Notebook->AddPage("Information");
      }

    pvd->CreateProperties();
    this->GetPVApplication()->GetRenderModule()->AddPVSource(this);

    // I need to update the data before 
    // UpdateFilterMenu which is called by SetCurrentPVSource.
    // Although tis fixes my problem,  I should not have to actually update.
    // All I need is the data type from the information which should be
    // obtainable after initialize data ...
    // The bug I was fixing is the ExtractGrid button was active
    // for a polydata and imagedata collection.
    this->Update();
    

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
      if (bds[0] <= bds[1] && bds[2] <= bds[3] && bds[4] <= bds[5])
        {
        window->SetCenterOfRotation(0.5*(bds[0]+bds[1]), 
                                    0.5*(bds[2]+bds[3]),
                                    0.5*(bds[4]+bds[5]));
        window->ResetCenterCallback();
        window->GetMainView()->GetRenderer()->ResetCamera(bds);
        }
      }

    pvd->Initialize();
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
    num = this->GetNumberOfParts();
    for (idx = 0; idx < num; ++idx)
      {
      part = this->GetPart(idx);
      // This has a side effect of gathering and displaying information.
      // Should this be part->Update() !!!!!!!!!!!!!!??????
      this->GetPart()->GetPartDisplay()->Update();
      }
    pvd->UpdateProperties();
    }

  this->GetPVRenderView()->UpdateTclButAvoidRendering();

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
void vtkPVSource::MarkSourcesForUpdate(int flag)
{
  int idx;
  vtkPVSource* consumer;

  if (flag)
    {
    this->InvalidateDataInformation();
    this->PipelineModifiedTime.Modified();
    // Get rid of caches.
    int idx, numParts;
    vtkPVPart *part;
    numParts = this->GetNumberOfParts();
    for (idx = 0; idx < numParts; ++idx)
      {
      part = this->GetPart(idx);
      part->GetPartDisplay()->InvalidateGeometry();
      }
    }
  else
    {
    this->UpdateTime = this->PipelineModifiedTime;
    }

  for (idx = 0; idx < this->NumberOfPVConsumers; ++idx)
    {
    consumer = this->GetPVConsumer(idx);
    consumer->MarkSourcesForUpdate(flag);
    }  
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

  window->GetAnimationInterface()->DeleteSource(this);

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
  if ( this == current || window->GetSourceList("Sources")->GetNumberOfItems() == 1)
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
    this->GetPVApplication()->GetRenderModule()->RemovePVSource(this);
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
  vtkPVWidgetProperty *pvwProp;
  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  while( !it->IsDoneWithTraversal() )
    {
    pvwProp = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    pvw = pvwProp->GetWidget();
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
  vtkPVWidgetProperty *pvwProp;
  vtkCollectionIterator *it;

  it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  while( !it->IsDoneWithTraversal() )
    {
    pvwProp = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    pvwProp->Accept();
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

  // Detect special sources we do not handle yet.
  if (this->GetSourceClassName() == NULL)
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
//    *file << this->GetVTKSource(sourceIdx)->GetClassName()
//    *file << this->GetSourceClassName()
//          << " " << this->GetVTKSourceTclName(sourceIdx) << "\n";
    }

  // Handle this here.
  this->SetInputsInBatchScript(file);

  // Let the PVWidgets set up the object.
  numWidgets = this->WidgetProperties->GetNumberOfItems();
  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  vtkPVWidgetProperty *pvwProp;
  
  for (i = 0; i < numWidgets; i++)
    {
    pvwProp = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    pvwProp->GetWidget()->SaveInBatchScript(file);
    it->GoToNextItem();
    }
  it->Delete();

  // Sub-classes which need to update the source before
  // connecting it to other objects should set UpdateSourceInBatch
  // to 1 (for example, vtkPVEnSightReaderModule)
  if (this->UpdateSourceInBatch)
    {
    for (sourceIdx = 0; sourceIdx < numSources; ++sourceIdx)
      {
//      *file << this->GetVTKSourceTclName(sourceIdx) << " Update\n";
      }
    }

  // Add the mapper, actor, scalar bar actor ...
  this->GetPVOutput()->SaveInBatchScript(file);
}

//----------------------------------------------------------------------------
void vtkPVSource::SaveState(ofstream *file)
{
  int i, numWidgets;
  vtkPVWidgetProperty *pvwProp;

  // Detect if this source is in Glyph sourcesm and already exists.
  if (this->GetTraceReferenceCommand())
    {
    *file << "set kw(" << this->GetTclName() << ") [$kw(" 
          << this->GetTraceReferenceObject()->GetTclName() << ") " 
          << this->GetTraceReferenceCommand() << "]\n";
    return;
    }

  // Detect special sources we do not handle yet.
//  if (this->GetVTKSource() == NULL)
//    {
//    return;
//    }

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
  numWidgets = this->WidgetProperties->GetNumberOfItems();
  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();
  for (i = 0; i < numWidgets; i++)
    {
    pvwProp = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    pvwProp->GetWidget()->SaveState(file);
    it->GoToNextItem();
    }
  it->Delete();
  
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
#if 0
  int num, idx;
  int numSources, numOutputs;
  int sourceCount, outputCount;
  vtkPVSource *pvs;
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();

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
        vtkClientServerStream& stream = pm->GetStream();
        stream.Reset();
        stream << vtkClientServerStream::Invoke << pvs->GetVTKSourceID(sourceCount) <<
          "GetNumberOfOutputs" << vtkClientServerStream::End;
        pm->SendStreamToServer();
        vtkClientServerStream* amsg = pm->GetLastServerResult();
        if(!amsg->GetArgument(0, 0, &numOutputs))
          {
          vtkErrorMacro("wrong return type for GetNumberOfOutputs call");
          }
        // should we have a GatherInformation for client server??
//         pm->GatherInformation(this->NumberOfOutputsInformation,
//                               (char*)(pvs->GetVTKSourceTclName(sourceCount)));
//         numOutputs = this->NumberOfOutputsInformation->GetNumberOfOutputs();
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
      pm->GatherInformation(this->NumberOfOutputsInformation,
                            (char*)(pvs->GetVTKSourceTclName(sourceCount)));
      numOutputs = this->NumberOfOutputsInformation->GetNumberOfOutputs();
      }

    vtkPVInputProperty* ip = this->GetInputProperty(idx);
    const char* inputName;
    if (ip)
      {
      inputName = ip->GetName();
      }
    else
      {
      inputName = "Input";
      }

    *file << "\t";
    *file << this->GetVTKSourceTclName(idx) << " Set"
          << inputName << " [" 
          << pvs->GetVTKSourceTclName(sourceCount) 
          << " GetOutput " << outputCount << "]\n";
    
    ++outputCount;
    }
#endif
}


//----------------------------------------------------------------------------
void vtkPVSource::AddPVWidget(vtkPVWidget *pvw)
{
  char str[512];
  vtkPVWidgetProperty *prop = pvw->CreateAppropriateProperty();
  prop->SetWidget(pvw);
  this->WidgetProperties->AddItem(prop);
  prop->Delete();

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
  if (this->AcceptButtonRed)
    {
    return;
    }
  this->AcceptButtonRed = 1;
  this->Script("%s configure -background red1",
               this->AcceptButton->GetWidgetName());
  this->Script("%s configure -activebackground red1",
               this->AcceptButton->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVSource::SetAcceptButtonColorToWhite()
{
  if (!this->AcceptButtonRed)
    {
    return;
    }
  this->AcceptButtonRed = 0;

#ifdef _WIN32
  this->Script("%s configure -background [lindex [%s configure -background] 3]",
               this->AcceptButton->GetWidgetName(),
               this->AcceptButton->GetWidgetName());
  this->Script("%s configure -activebackground "
               "[lindex [%s configure -activebackground] 3]",
               this->AcceptButton->GetWidgetName(),
               this->AcceptButton->GetWidgetName());
#else
  this->Script("%s configure -background #ccc",
               this->AcceptButton->GetWidgetName());
  this->Script("%s configure -activebackground #eee",
               this->AcceptButton->GetWidgetName());
#endif
}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVSource::GetPVWidget(const char *name)
{
  vtkObject *o;
  vtkPVWidget *pvw;
  vtkPVWidgetProperty *pvwProp;
  this->WidgetProperties->InitTraversal();
  
  while ( (o = this->WidgetProperties->GetNextItemAsObject()) )
    {
    pvwProp = vtkPVWidgetProperty::SafeDownCast(o);
    if (pvwProp)
      {
      pvw = pvwProp->GetWidget();
      if (pvw && pvw->GetTraceName() && strcmp(pvw->GetTraceName(), name) == 0)
        {
        return pvw;
        }
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

  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  
  // We need oue source for each part.
  int numSources = 1;
  // Set the input if necessary.
  if (this->GetNumberOfInputProperties() > 0)
    {
    vtkPVSource *input = this->GetPVWindow()->GetCurrentPVSource();
    numSources = input->GetNumberOfParts();
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
    pvs->AddVTKSource(pm->NewStreamObject(this->SourceClassName));
    }
  pm->SendStreamToServer();
  pvs->SetView(this->GetPVWindow()->GetMainView());

  pvs->PrototypeInstanceCount = this->PrototypeInstanceCount;
  this->PrototypeInstanceCount++;

  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* widgetMap =
    vtkArrayMap<vtkPVWidget*, vtkPVWidget*>::New();


  // Copy all widgets
  vtkPVWidget *pvWidget, *clonedWidget;
  vtkPVWidgetProperty *pvwProp;
  vtkCollectionIterator *it = this->WidgetProperties->NewIterator();
  it->InitTraversal();

  int i;
  for (i = 0; i < this->WidgetProperties->GetNumberOfItems(); i++)
    {
    pvwProp = static_cast<vtkPVWidgetProperty*>(it->GetObject());
    pvWidget = pvwProp->GetWidget();
    clonedWidget = pvWidget->ClonePrototype(pvs, widgetMap);
    pvs->AddPVWidget(clonedWidget);
    clonedWidget->Delete();
    it->GoToNextItem();
    }
  widgetMap->Delete();
  it->Delete();

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
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  int numSources, sourceIdx;
  int numOutputs, idx;
  int outputCount = 0;
  vtkPVPart* part;
  vtkPVSource* input = this->GetPVInput(0);
  vtkPVData* pvd;

  // Create the output.
  pvd = vtkPVData::New();
  pvd->SetPVApplication(pvApp);

  numSources = this->GetNumberOfVTKSources();
  vtkClientServerStream& stream = pm->GetStream();
  for (sourceIdx = 0; sourceIdx < numSources; ++sourceIdx)
    {
    vtkClientServerID sourceID = this->GetVTKSourceID(sourceIdx);
    stream.Reset();
    stream << vtkClientServerStream::Invoke << sourceID <<
      "GetNumberOfOutputs" << vtkClientServerStream::End;
    pm->SendStreamToServer();
    const vtkClientServerStream* amsg = pm->GetLastServerResult();
    if(!amsg->GetArgument(0, 0, &numOutputs))
      {
      vtkErrorMacro("wrong return type for GetNumberOfOutputs call");
      }

    for (idx = 0; idx < numOutputs; ++idx)
      {
      ++outputCount;
      stream << vtkClientServerStream::Invoke << sourceID 
             << "GetOutput" << idx <<  vtkClientServerStream::End;
      vtkClientServerID dataID = pm->GetUniqueID();
      stream << vtkClientServerStream::Assign << dataID
             << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
      pm->SendStreamToServer();
      part = vtkPVPart::New();
      part->SetPVApplication(pvApp);
      part->SetVTKDataID(dataID);
      this->AddPart(part);

      // Create the extent translator (sources with no inputs only).
      // Needs to be before "ExtractPieces" because translator propagates.
      vtkClientServerID translatorID = {0};
      if ( ! input)
        {
        vtkClientServerID translatorID = pm->NewStreamObject("vtkPVExtentTranslator");
        stream << vtkClientServerStream::Invoke << dataID 
               << "SetExtentTranslator" << translatorID 
               << vtkClientServerStream::End;
        pm->SendStreamToServer();
        }
      part->InsertExtractPiecesIfNecessary();
      if (translatorID.ID != 0)
        {
        // Translator has to be set on source because it is propagated.
        // Original source is set after transmit so reference
        // loop can be broken when pvPart is deleted.
        stream << vtkClientServerStream::Invoke << translatorID 
               << "SetOriginalSource" << dataID 
               << vtkClientServerStream::End;
        pm->DeleteStreamObject(translatorID); 
        pm->SendStreamToServer();
        }
      part->Delete();  
      }
    }

  this->SetPVOutput(pvd);
  pvd->Delete();

  return VTK_OK;
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
  os << indent << "WidgetProperties: " << this->GetWidgetProperties() << endl;
  os << indent << "IsPermanent: " << this->IsPermanent << endl;
  os << indent << "SourceClassName: " 
     << (this->SourceClassName?this->SourceClassName:"null") << endl;
  os << indent << "HideParametersPage: " << this->HideParametersPage << endl;
  os << indent << "HideDisplayPage: " << this->HideDisplayPage << endl;
  os << indent << "HideInformationPage: " << this->HideInformationPage << endl;
  os << indent << "ToolbarModule: " << this->ToolbarModule << endl;

  os << indent << "PVOutput: " << this->PVOutput << endl;
  os << indent << "NumberOfPVConsumers: " << this->GetNumberOfPVConsumers() << endl;
  os << indent << "NumberOfParts: " << this->GetNumberOfParts() << endl;

  os << indent << "VTKMultipleInputsFlag: " << this->VTKMultipleInputsFlag << endl;
  os << indent << "InputProperties: \n";
  vtkIndent i2 = indent.GetNextIndent();
  int num, idx;
  num = this->GetNumberOfInputProperties();
  for (idx = 0; idx < num; ++idx)
    {
    this->GetInputProperty(idx)->PrintSelf(os, i2);
    }
  os << indent << "NumberOfOutputsInformation: "
     << this->NumberOfOutputsInformation << endl;
}

const char *vtkPVSource::GetVTKSourceTclName(int idx)
{
  return "";
}

const char *vtkPVSource::GetVTKSourceTclName()
{
  return "";
}

