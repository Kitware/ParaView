/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtractDataSetsWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVExtractDataSetsWidget.h"

#include "vtkKWLabel.h"
#include "vtkKWListBox.h"
#include "vtkKWPushButton.h"
#include "vtkKWWidget.h"
#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVSource.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMPart.h"

#include <vtkstd/vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVExtractDataSetsWidget);
vtkCxxRevisionMacro(vtkPVExtractDataSetsWidget, "1.11");

struct vtkPVExtractDataSetsWidgetInternals
{
  vtkstd::vector<int> GroupIndices;
  vtkstd::vector<int> GroupSelected;
};

//----------------------------------------------------------------------------
vtkPVExtractDataSetsWidget::vtkPVExtractDataSetsWidget()
{
  this->ButtonFrame = vtkKWFrame::New();
  this->AllOnButton = vtkKWPushButton::New();
  this->AllOffButton = vtkKWPushButton::New();

  this->PartSelectionList = vtkKWListBox::New();

  this->Internal = new vtkPVExtractDataSetsWidgetInternals;
}

//----------------------------------------------------------------------------
vtkPVExtractDataSetsWidget::~vtkPVExtractDataSetsWidget()
{
  this->ButtonFrame->Delete();
  this->ButtonFrame = NULL;
  this->AllOnButton->Delete();
  this->AllOnButton = NULL;
  this->AllOffButton->Delete();
  this->AllOffButton = NULL;

  this->PartSelectionList->Delete();
  this->PartSelectionList = NULL;

  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVExtractDataSetsWidget::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create();
  this->Script("pack %s -side top -fill x",
               this->ButtonFrame->GetWidgetName());

  this->AllOnButton->SetParent(this->ButtonFrame);
  this->AllOnButton->Create();
  this->AllOnButton->SetText("All On");
  this->AllOnButton->SetCommand(this, "AllOnCallback");

  this->AllOffButton->SetParent(this->ButtonFrame);
  this->AllOffButton->Create();
  this->AllOffButton->SetText("All Off");
  this->AllOffButton->SetCommand(this, "AllOffCallback");

  this->Script("pack %s %s -side left -fill x -expand t",
               this->AllOnButton->GetWidgetName(),
               this->AllOffButton->GetWidgetName());

  this->PartSelectionList->SetParent(this);
  this->PartSelectionList->Create();
  this->PartSelectionList->SetSingleClickCommand(this, "PartSelectionCallback");
  this->PartSelectionList->SetSelectionModeToExtended();
  this->PartSelectionList->ExportSelectionOff();
  this->PartSelectionList->SetHeight(0);

  this->Script("pack %s -side top -fill both -expand t",
               this->PartSelectionList->GetWidgetName());
}

//----------------------------------------------------------------------------
// This is a bit kludgey. Needs more work.
void vtkPVExtractDataSetsWidget::PartSelectionCallback()
{
  int selIdx = this->PartSelectionList->GetSelectionIndex();
  unsigned int numGroups = this->Internal->GroupIndices.size();
  for (unsigned int i=0; i<numGroups; i++)
    {
    if (selIdx == static_cast<int>(this->Internal->GroupIndices[i]))
      {
      // Group entries should never be selected
      this->PartSelectionList->SetSelectState(selIdx, 0);

      // Select or unselect all entries belonging to this group
      unsigned int begin = this->Internal->GroupIndices[i] + 1;
      unsigned int end = this->PartSelectionList->GetNumberOfItems();
      if (i < this->Internal->GroupIndices.size() - 1)
        {
        end = this->Internal->GroupIndices[i+1];
        }

      if (this->Internal->GroupSelected[i])
        {
        this->Internal->GroupSelected[i] = 0;
        for (unsigned int j=begin; j<end; j++)
          {
          this->PartSelectionList->SetSelectState(j, 0);
          }
        }
      else
        {
        this->Internal->GroupSelected[i] = 1;
        for (unsigned int j=begin; j<end; j++)
          {
          this->PartSelectionList->SetSelectState(j, 1);
          }
        }
      break;
      }
    }
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVExtractDataSetsWidget::Accept()
{
  // Now loop through the input mask setting the selection states.
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (!ivp)
    {
    vtkErrorMacro(
      "Could not find property of name: "
      << (this->GetSMPropertyName()?this->GetSMPropertyName():"(null)")
      << " for widget: " << this->GetTraceHelper()->GetObjectName());
    return;
    }

  ivp->SetNumberOfElements(0);

  unsigned int idx=0;
  unsigned int numGroups = this->Internal->GroupIndices.size();
  // For each group, add the selected datasets
  for (unsigned int i=0; i<numGroups; i++)
    {
    // All items between this group and the next
    unsigned int begin = this->Internal->GroupIndices[i]+1;
    unsigned int end = this->PartSelectionList->GetNumberOfItems();
    if (i < numGroups - 1)
      {
      end = this->Internal->GroupIndices[i+1];
      }
    for (unsigned int j=begin; j<end; j++)
      {
      if (this->PartSelectionList->GetSelectState(j))
        {
        ivp->SetElement(idx++, i);
        ivp->SetElement(idx++, j-begin);
        }
      }
    }

  this->Superclass::Accept();
}


//---------------------------------------------------------------------------
void vtkPVExtractDataSetsWidget::SetSelectState(int idx, int val)
{
  this->PartSelectionList->SetSelectState(idx, val);
}


//----------------------------------------------------------------------------
void vtkPVExtractDataSetsWidget::CommonInit()
{
  int idx=0;

  this->Internal->GroupIndices.clear();
  this->Internal->GroupSelected.clear();
  this->PartSelectionList->DeleteAll();

  // Loop through all of the datasets of the input adding to the list.
  vtkPVSource* input = this->PVSource->GetPVInput(0);
  vtkPVCompositeDataInformation* cdi =
    input->GetDataInformation()->GetCompositeDataInformation();

  unsigned int numGroups = cdi->GetNumberOfGroups();

  unsigned int i;
  int firstTime = 1;
  for (i=0; i<numGroups; i++)
    {
    // If there are more than one group, add a label showing
    // the group number before listing the blocks for that
    // group. Store the index of this item to be used later.
    if (numGroups > 1)
      {
      this->Internal->GroupIndices.push_back(idx);
      ostrstream groupStr;
      groupStr << "Group " << i << ":" << ends;
      this->PartSelectionList->InsertEntry(idx++, groupStr.str());
      delete[] groupStr.str();
      }
    else
      {
      this->Internal->GroupIndices.push_back(-1);
      }
    unsigned int numDataSets = cdi->GetNumberOfDataSets(i);
    for (unsigned int j=0; j<numDataSets; j++)
      {
      vtkPVDataInformation* dataInfo = cdi->GetDataInformation(i, j);
      if (dataInfo)
        {
        ostrstream dataStr;
        dataStr << "  " << dataInfo->GetName() << ends;
        this->PartSelectionList->InsertEntry(idx++, dataStr.str());
        delete[] dataStr.str();
        if (firstTime)
          {
          //By default select first one
          this->PartSelectionList->SetSelectionIndex(idx-1);
          this->PartSelectionCallback();
          firstTime = 0;
          }
        }
      }
    }

  // Initially, no groups are selected
  numGroups = this->Internal->GroupIndices.size();
  this->Internal->GroupSelected.resize(numGroups);
  for (i=0; i<numGroups; i++)
    {
    this->Internal->GroupSelected[i] = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractDataSetsWidget::Initialize()
{
  this->CommonInit();
}

//----------------------------------------------------------------------------
void vtkPVExtractDataSetsWidget::ResetInternal()
{
  this->CommonInit();

  // Now loop through the input mask setting the selection states.
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (!ivp)
    {
    vtkErrorMacro(
      "Could not find property of name: "
      << (this->GetSMPropertyName()?this->GetSMPropertyName():"(null)")
      << " for widget: " << this->GetTraceHelper()->GetObjectName());
    return;
    }

  unsigned int numElems = ivp->GetNumberOfElements();
  unsigned int numDataSets = numElems / 2;
  for (unsigned int i=0; i<numDataSets; i++)
    {
    int group = ivp->GetElement(2*i  );
    int didx  = ivp->GetElement(2*i+1);
    int entryIdx = this->Internal->GroupIndices[group] + didx + 1;
    this->PartSelectionList->SetSelectState(entryIdx, 1);
    }

  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVExtractDataSetsWidget::AllOnCallback()
{
  int num, idx;

  unsigned int numGroups = this->Internal->GroupIndices.size();

  // Select all but group labels
  num = this->PartSelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    int isGroupEntry = 0;
    for (unsigned int i=0; i<numGroups; i++)
      {
      if (idx == this->Internal->GroupIndices[i])
        {
        isGroupEntry = 1;
        break;
        }
      }
    if (!isGroupEntry)
      {
      this->PartSelectionList->SetSelectState(idx, 1);
      }
    }

  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVExtractDataSetsWidget::AllOffCallback()
{
  int num, idx;

  num = this->PartSelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    this->PartSelectionList->SetSelectState(idx, 0);
    }

  this->ModifiedCallback();
}

//---------------------------------------------------------------------------
void vtkPVExtractDataSetsWidget::Trace(ofstream *file)
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());

  if ( ! this->GetTraceHelper()->Initialize(file) || !ivp)
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ")" << " AllOffCallback" << endl;

  unsigned int numElems = ivp->GetNumberOfElements();
  unsigned int numDataSets = numElems / 2;
  for (unsigned int i=0; i<numDataSets; i++)
    {
    int group = ivp->GetElement(2*i  );
    int didx  = ivp->GetElement(2*i+1);
    int entryIdx = this->Internal->GroupIndices[group] + didx + 1;
    *file << "$kw(" << this->GetTclName() << ") SetSelectState "
          << entryIdx << " 1" << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractDataSetsWidget::SaveInBatchScript(ofstream *file)
{
  const char* sourceID = this->PVSource->GetProxy()->GetSelfIDAsString();

  if (!sourceID || !this->SMPropertyName)
    {
    vtkErrorMacro("Sanity check failed. " << this->GetClassName());
    return;
    }

  // Now loop through the input mask setting the selection states.
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (!ivp)
    {
    vtkErrorMacro(
      "Could not find property of name: "
      << (this->GetSMPropertyName()?this->GetSMPropertyName():"(null)")
      << " for widget: " << this->GetTraceHelper()->GetObjectName());
    return;
    }

  unsigned int numElems = ivp->GetNumberOfElements();

  *file << "  [$pvTemp" << sourceID << " GetProperty "
        << this->SMPropertyName << "] SetNumberOfElements "
        << numElems << endl;

  for (unsigned int i=0; i<numElems; i++)
    {
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << this->SMPropertyName << "] SetElement "
          << i << " " << ivp->GetElement(i)
          << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractDataSetsWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->ButtonFrame);
  this->PropagateEnableState(this->AllOnButton);
  this->PropagateEnableState(this->AllOffButton);

  this->PropagateEnableState(this->PartSelectionList);
}

//----------------------------------------------------------------------------
void vtkPVExtractDataSetsWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
