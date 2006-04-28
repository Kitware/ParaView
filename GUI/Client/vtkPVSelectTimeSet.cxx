/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectTimeSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSelectTimeSet.h"

#include "vtkDataArrayCollection.h"
#include "vtkFloatArray.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkPVTraceHelper.h"
#include "vtkKWTree.h"
#include "vtkKWTreeWithScrollbars.h"
#include "vtkSMSourceProxy.h"

#include <vtksys/stl/string>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSelectTimeSet);
vtkCxxRevisionMacro(vtkPVSelectTimeSet, "1.66");

//-----------------------------------------------------------------------------
vtkPVSelectTimeSet::vtkPVSelectTimeSet()
{
  this->LabeledFrame = vtkKWFrameWithLabel::New();
  this->LabeledFrame->SetParent(this);
  
  this->TimeLabel = vtkKWLabel::New();
  this->TimeLabel->SetParent(this->LabeledFrame->GetFrame());

  this->Tree = vtkKWTreeWithScrollbars::New();
  this->Tree->SetParent(this->LabeledFrame->GetFrame());

  this->TimeValue = 0.0;

  this->FrameLabel = 0;
  
  this->TimeSets = vtkDataArrayCollection::New();
  
  this->ServerSideID.ID = 0;
}

//-----------------------------------------------------------------------------
vtkPVSelectTimeSet::~vtkPVSelectTimeSet()
{
  this->LabeledFrame->Delete();
  this->Tree->Delete();
  this->TimeLabel->Delete();
  this->SetFrameLabel(0);
  this->TimeSets->Delete();
  if(this->ServerSideID.ID)
    {
    vtkProcessModule* pm = this->GetPVApplication()->GetProcessModule();
    vtkClientServerStream stream;
    pm->DeleteStreamObject(this->ServerSideID, stream);
    pm->SendStream(
     vtkProcessModuleConnectionManager::GetRootServerConnectionID(),  
      vtkProcessModule::DATA_SERVER, stream);
    }
}

//-----------------------------------------------------------------------------
void vtkPVSelectTimeSet::SetLabel(const char* label)
{
  this->SetFrameLabel(label);
  if (this->GetApplication())
    {
    this->LabeledFrame->SetLabelText(label);
    }
}

//-----------------------------------------------------------------------------
const char* vtkPVSelectTimeSet::GetLabel()
{
  return this->GetFrameLabel();
}

//-----------------------------------------------------------------------------
void vtkPVSelectTimeSet::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->SetBorderWidth(2);

  // For getting the widget in a script.
  if ((this->GetTraceHelper()->GetObjectNameState() == 
       vtkPVTraceHelper::ObjectNameStateUninitialized ||
       this->GetTraceHelper()->GetObjectNameState() == 
       vtkPVTraceHelper::ObjectNameStateDefault) )
    {
    this->GetTraceHelper()->SetObjectName("SelectTimeSet");
    this->GetTraceHelper()->SetObjectNameState(
      vtkPVTraceHelper::ObjectNameStateSelfInitialized);
    }
  
  this->LabeledFrame->Create();
  if (this->FrameLabel)
    {
    this->LabeledFrame->SetLabelText(this->FrameLabel);
    }
  this->TimeLabel->Create();

  char label[32];
  sprintf(label, "Time value: %12.5e", 0.0);
  this->TimeLabel->SetText(label);
  this->Script("pack %s", this->TimeLabel->GetWidgetName());
  
  this->Tree->Create();
  this->Tree->SetReliefToSunken();
  this->Tree->SetBorderWidth(2);

  vtkKWTree *tree = this->Tree->GetWidget();
  tree->SetBackgroundColor(1.0, 1.0, 1.0);
  tree->SetWidth(15);
  tree->SetRedrawOnIdle(1);
  tree->SetSelectionBackgroundColor(1.0, 0.0, 0.0);
  tree->SetSingleClickOnNodeCommand(this, "SetTimeValueCallback");
  
  this->Script("pack %s -expand t -fill x", this->Tree->GetWidgetName());

  this->Script("pack %s -side top -expand t -fill x", 
               this->LabeledFrame->GetWidgetName());

}

//-----------------------------------------------------------------------------
void vtkPVSelectTimeSet::SetTimeValue(float time)
{
  if (this->TimeValue != time ||
      !this->TimeLabel->GetText() ||
      !strcmp(this->TimeLabel->GetText(), "No timesets available."))
    { 
    this->TimeValue = time; 
    
    char label[32];
    sprintf(label, "Time value: %12.5e", time);
    this->TimeLabel->SetText(label);
    this->Modified(); 
    } 
}

//-----------------------------------------------------------------------------
void vtkPVSelectTimeSet::SetTimeValueCallback(const char* item)
{
  if (this->TimeSets->GetNumberOfItems() == 0)
    {
    return;
    }

  if ( strncmp(item, "timeset", strlen("timeset")) == 0 )
    {
    if (this->Tree->GetWidget()->IsNodeOpen(item))
      {
      this->Tree->GetWidget()->CloseTree(item);
      }
    else
      {
      this->Tree->GetWidget()->OpenTree(item);
      }
    return;
    }

  this->Tree->GetWidget()->SelectSingleNode(item);
  const char* result = this->Tree->GetWidget()->GetNodeUserData(item);
  if (result[0] == '\0')
    {
    return;
    }

  int index[2];
  sscanf(result, "%d %d", &(index[0]), &(index[1]));

  this->SetTimeSetsFromReader();
  this->SetTimeValue(this->TimeSets->GetItem(index[0])->GetTuple1(index[1]));
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVSelectTimeSet::AddRootNode(const char* name, const char* text)
{
  if (!this->GetApplication())
    {
    return;
    }
  this->Tree->GetWidget()->AddNode(NULL, name, text);
}

//-----------------------------------------------------------------------------
void vtkPVSelectTimeSet::AddChildNode(const char* parent, const char* name, 
                                      const char* text, const char* data)
{
  if (!this->GetApplication())
    {
    return;
    }
  this->Tree->GetWidget()->AddNode(parent, name, text);
  this->Tree->GetWidget()->SetNodeUserData(name, data);
}


//-----------------------------------------------------------------------------
void vtkPVSelectTimeSet::SaveInBatchScript(ofstream *file)
{
  const char* sourceID = this->PVSource->GetProxy()->GetSelfIDAsString();
  
  if (!sourceID || !this->SMPropertyName)
    {
    vtkErrorMacro("Sanity check failed. " << this->GetClassName());
    return;
    }
  
  *file << "  [$pvTemp" << sourceID <<  " GetProperty "
        << this->SMPropertyName << "] SetElements1 " << this->TimeValue << endl;
}

//-----------------------------------------------------------------------------
void vtkPVSelectTimeSet::Accept()
{
  int modFlag = this->GetModifiedFlag();
  
  if (modFlag)
    {
    vtksys_stl::string sel(this->Tree->GetWidget()->GetSelection());
    this->GetTraceHelper()->AddEntry(
      "$kw(%s) SetTimeValueCallback {%s}", this->GetTclName(), sel.c_str());
    }

  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (dvp)
    {
    dvp->SetElement(0, this->TimeValue);
    }
  else
    {
    vtkErrorMacro(
      "Could not find property of name: "
      << (this->GetSMPropertyName()?this->GetSMPropertyName():"(null)")
      << " for widget: " << this->GetTraceHelper()->GetObjectName());
    }

  this->Superclass::Accept();
}

//---------------------------------------------------------------------------
void vtkPVSelectTimeSet::Trace(ofstream *file)
{
  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }

  vtksys_stl::string sel(this->Tree->GetWidget()->GetSelection());

  *file << "$kw(" << this->GetTclName() << ") SetTimeValueCallback {"
        << sel.c_str() << "}" << endl;
}


//-----------------------------------------------------------------------------
int vtkPVSelectTimeSet::GetNumberOfTimeSteps()
{
  int num =0;
  for (int i=0; i < this->TimeSets->GetNumberOfItems(); i ++ )
    {
    num += this->TimeSets->GetItem(i)->GetNumberOfTuples();
    }
  return num;
}

//-----------------------------------------------------------------------------
void vtkPVSelectTimeSet::CommonReset()
{
  // Command to update the UI.
  if (!this->Tree)
    {
    return;
    }

  this->Tree->GetWidget()->DeleteAllNodes();
  
  this->SetTimeSetsFromReader();

  int timeSetId=0;
  char timeSetName[32];
  char timeSetText[32];

  char timeValueName[32];
  char timeValueText[32];
  char indices[32];

  float actualTimeValue = 0;
  
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (dvp)
    {
    actualTimeValue = dvp->GetElement(0);
    }
  
  int matchFound = 0;

  this->ModifiedFlag = 0;

  if (this->TimeSets->GetNumberOfItems() == 0)
    {
    this->Script("pack forget %s", this->Tree->GetWidgetName());
    this->TimeLabel->SetText("No timesets available.");
    return;
    }
  else
    {
    this->SetTimeValue(actualTimeValue);
    this->Script("pack %s -expand t -fill x", this->Tree->GetWidgetName());
    }

  this->TimeSets->InitTraversal();
  vtkDataArray* da;
  while( (da=this->TimeSets->GetNextItem()) )
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
      str << timeSetId-1 << " " << tuple << ends;
      sprintf(indices, "%s", str.str());
      str.rdbuf()->freeze(0);
      this->AddChildNode(timeSetName, timeValueName, timeValueText, indices);
      if (actualTimeValue == timeValue && !matchFound)
        {
        matchFound=1;
        this->Tree->GetWidget()->SelectSingleNode(timeValueName);
        }
      }
    if (timeSetId == 1)
      {
      this->Tree->GetWidget()->OpenTree(timeSetName);
      }
    }
  
  this->SetTimeValue(actualTimeValue);

}

//-----------------------------------------------------------------------------
void vtkPVSelectTimeSet::Initialize()
{
  this->SetTimeSetsFromReader();

  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (dvp && this->TimeSets->GetNumberOfItems() > 0)
    {
    dvp->SetElement(0, this->TimeSets->GetItem(0)->GetComponent(0, 0));
    }
  
  this->CommonReset();
}

//-----------------------------------------------------------------------------
void vtkPVSelectTimeSet::ResetInternal()
{
  this->CommonReset();
  this->ModifiedFlag = 0;
}

//-----------------------------------------------------------------------------
vtkPVSelectTimeSet* vtkPVSelectTimeSet::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVSelectTimeSet::SafeDownCast(clone);
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void vtkPVSelectTimeSet::SetTimeSetsFromReader()
{
  vtkProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  this->TimeSets->RemoveAllItems();

  vtkClientServerStream stream;

  // Create the server-side helper if necessary.
  if(!this->ServerSideID.ID)
    {
    this->ServerSideID = pm->NewStreamObject("vtkPVServerSelectTimeSet", stream);
    pm->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::DATA_SERVER, stream);
    }

  // Get the time sets from the reader on the server.
  // Reader -> VTKSourceID (0). We assume that there is 1 VTKSource.
  stream << vtkClientServerStream::Invoke
         << this->ServerSideID << "GetTimeSets" << this->PVSource->GetVTKSourceID(0)
         << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
    vtkProcessModule::DATA_SERVER_ROOT, stream);
  vtkClientServerStream timeSets;
  if(!pm->GetLastResult(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(),
      vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &timeSets))
    {
    vtkErrorMacro("Error getting time sets from server.");
    return;
    }

  double min = VTK_LARGE_FLOAT;
  double max = -VTK_LARGE_FLOAT;
  
  // There is one time set per message.
  for(int m=0; m < timeSets.GetNumberOfMessages(); ++m)
    {
    // Each argument in the message is a time set entry.
    vtkFloatArray* timeSet = vtkFloatArray::New();
    int n = timeSets.GetNumberOfArguments(m);
    timeSet->SetNumberOfTuples(n);
    for(int i=0; i < n; ++i)
      {
      float value;
      if(!timeSets.GetArgument(m, i, &value))
        {
        vtkErrorMacro("Error reading time set value.");
        timeSet->Delete();
        return;
        }
      timeSet->SetTuple1(i, value);
      min = (min > value)? value : min;
      max = (max < value)? value : max;
      }
    this->TimeSets->AddItem(timeSet);
    timeSet->Delete();
    }

  if (min != VTK_LARGE_FLOAT && max != -VTK_LARGE_FLOAT)
    {
    // It's the resposibility of the Widget to keep the domain in Sync.
    vtkSMDoubleRangeDomain* domain = vtkSMDoubleRangeDomain::SafeDownCast(
      this->GetSMProperty()->GetDomain("range"));
    if (domain)
      {
      domain->RemoveAllMinima();
      domain->RemoveAllMaxima();
      domain->AddMinimum(0,min);
      domain->AddMaximum(0,max);
      }
    }
  
}

//----------------------------------------------------------------------------
void vtkPVSelectTimeSet::SaveInBatchScriptForPart(ofstream *file,
                                                  vtkClientServerID sourceID)
{
  if (sourceID.ID == 0)
    {
    vtkErrorMacro(<< this->GetClassName()
                  << " must not have SaveInBatchScript method.");
    return;
    } 

  *file << "\t" << "pvTemp" << sourceID
        << " SetTimeValue " << this->GetTimeValue()
        << endl;;
}

//-----------------------------------------------------------------------------
void vtkPVSelectTimeSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TimeValue: " << this->TimeValue << endl;
  os << indent << "LabeledFrame: " << this->LabeledFrame << endl;
}
