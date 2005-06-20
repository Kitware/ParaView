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
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPart.h"

#include <vtkstd/vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVExtractDataSetsWidget);
vtkCxxRevisionMacro(vtkPVExtractDataSetsWidget, "1.5");

int vtkPVExtractDataSetsWidgetCommand(ClientData cd, Tcl_Interp *interp,
                                int argc, char *argv[]);

struct vtkPVExtractDataSetsWidgetInternals
{
  vtkstd::vector<int> LevelIndices;
  vtkstd::vector<int> LevelSelected;
};

//----------------------------------------------------------------------------
vtkPVExtractDataSetsWidget::vtkPVExtractDataSetsWidget()
{
  this->CommandFunction = vtkPVExtractDataSetsWidgetCommand;
  
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
void vtkPVExtractDataSetsWidget::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);

  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create(pvApp);
  this->Script("pack %s -side top -fill x",
               this->ButtonFrame->GetWidgetName());

  this->AllOnButton->SetParent(this->ButtonFrame);
  this->AllOnButton->Create(pvApp);
  this->AllOnButton->SetText("All On");
  this->AllOnButton->SetCommand(this, "AllOnCallback");

  this->AllOffButton->SetParent(this->ButtonFrame);
  this->AllOffButton->Create(pvApp);
  this->AllOffButton->SetText("All Off");
  this->AllOffButton->SetCommand(this, "AllOffCallback");

  this->Script("pack %s %s -side left -fill x -expand t",
               this->AllOnButton->GetWidgetName(),
               this->AllOffButton->GetWidgetName());

  this->PartSelectionList->SetParent(this);
  this->PartSelectionList->Create(app);
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
  unsigned int numLevels = this->Internal->LevelIndices.size();
  for (unsigned int i=0; i<numLevels; i++)
    {
    if (selIdx == static_cast<int>(this->Internal->LevelIndices[i]))
      {
      // Level entries should never be selected
      this->PartSelectionList->SetSelectState(selIdx, 0);

      // Select or unselect all entries belonging to this level
      unsigned int begin = this->Internal->LevelIndices[i] + 1;
      unsigned int end = this->PartSelectionList->GetNumberOfItems();
      if (i < this->Internal->LevelIndices.size() - 1)
        {
        end = this->Internal->LevelIndices[i+1];
        }

      if (this->Internal->LevelSelected[i])
        {
        this->Internal->LevelSelected[i] = 0;
        for (unsigned int j=begin; j<end; j++)
          {
          this->PartSelectionList->SetSelectState(j, 0);
          }
        }
      else
        {
        this->Internal->LevelSelected[i] = 1;
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
  unsigned int numLevels = this->Internal->LevelIndices.size();
  // For each level, add the selected datasets
  for (unsigned int i=0; i<numLevels; i++)
    {
    // All items between this level and the next
    unsigned int begin = this->Internal->LevelIndices[i]+1;
    unsigned int end = this->PartSelectionList->GetNumberOfItems();
    if (i < numLevels - 1)
      {
      end = this->Internal->LevelIndices[i+1];
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

  this->Internal->LevelIndices.clear();
  this->Internal->LevelSelected.clear();
  this->PartSelectionList->DeleteAll();

  // Loop through all of the datasets of the input adding to the list.
  vtkPVSource* input = this->PVSource->GetPVInput(0);
  vtkPVCompositeDataInformation* cdi =
    input->GetDataInformation()->GetCompositeDataInformation();

  unsigned int numLevels = cdi->GetNumberOfLevels();

  unsigned int i;
  int firstTime = 1;
  for (i=0; i<numLevels; i++)
    {
    // If there are more than one level, add a label showing
    // the level number before listing the blocks for that
    // level. Store the index of this item to be used later.
    if (numLevels > 1)
      {
      this->Internal->LevelIndices.push_back(idx);
      ostrstream levelStr;
      levelStr << "Level " << i << ":" << ends;
      this->PartSelectionList->InsertEntry(idx++, levelStr.str());
      delete[] levelStr.str();
      }
    else
      {
      this->Internal->LevelIndices.push_back(-1);
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

  // Initially, no levels are selected
  numLevels = this->Internal->LevelIndices.size();
  this->Internal->LevelSelected.resize(numLevels);
  for (i=0; i<numLevels; i++)
    {
    this->Internal->LevelSelected[i] = 0;
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
    int level = ivp->GetElement(2*i  );
    int didx  = ivp->GetElement(2*i+1);
    int entryIdx = this->Internal->LevelIndices[level] + didx + 1;
    this->PartSelectionList->SetSelectState(entryIdx, 1);
    }

  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVExtractDataSetsWidget::AllOnCallback()
{
  int num, idx;

  unsigned int numLevels = this->Internal->LevelIndices.size();

  // Select all but level labels
  num = this->PartSelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    int isLevelEntry = 0;
    for (unsigned int i=0; i<numLevels; i++)
      {
      if (idx == this->Internal->LevelIndices[i])
        {
        isLevelEntry = 1;
        break;
        }
      }
    if (!isLevelEntry)
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
    int level = ivp->GetElement(2*i  );
    int didx  = ivp->GetElement(2*i+1);
    int entryIdx = this->Internal->LevelIndices[level] + didx + 1;
    *file << "$kw(" << this->GetTclName() << ") SetSelectState "
          << entryIdx << " 1" << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractDataSetsWidget::SaveInBatchScript(ofstream *file)
{
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);

  if (sourceID.ID == 0 || !this->SMPropertyName)
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
