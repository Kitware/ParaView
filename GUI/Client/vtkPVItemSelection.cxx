/*=========================================================================

  Program:   ParaView
  Module:    vtkPVItemSelection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVItemSelection.h"

#include "vtkObjectFactory.h"
#include "vtkArrayMap.txx"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkDataArraySelection.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWPushButton.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkPVTraceHelper.h"

#include <vtkstd/string>
#include <vtkstd/set>
#include <vtkstd/vector>
#include <vtksys/ios/sstream>

typedef vtkstd::set<vtkstd::string> vtkPVItemSelectionArraySetBase;
class vtkPVItemSelectionArraySet: public vtkPVItemSelectionArraySetBase {};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVItemSelection);
vtkCxxRevisionMacro(vtkPVItemSelection, "1.11");

//----------------------------------------------------------------------------
vtkPVItemSelection::vtkPVItemSelection()
{
  this->LabelText = 0;
  
  this->LabeledFrame = vtkKWFrameWithLabel::New();
  this->ButtonFrame = vtkKWFrame::New();
  this->AllOnButton = vtkKWPushButton::New();
  this->AllOffButton = vtkKWPushButton::New();

  this->CheckFrame = vtkKWFrame::New();
  this->ArrayCheckButtons = vtkCollection::New();

  this->ArraySet = new vtkPVItemSelectionArraySet;

  this->NoArraysLabel = vtkKWLabel::New();
  this->Selection = vtkDataArraySelection::New();

}

//----------------------------------------------------------------------------
vtkPVItemSelection::~vtkPVItemSelection()
{
  this->SetLabelText(0);

  this->LabeledFrame->Delete();
  this->LabeledFrame = NULL;

  this->ButtonFrame->Delete();
  this->ButtonFrame = NULL;

  this->AllOnButton->Delete();
  this->AllOnButton = NULL;

  this->AllOffButton->Delete();
  this->AllOffButton = NULL;

  this->CheckFrame->Delete();
  this->CheckFrame = NULL;

  this->ArrayCheckButtons->Delete();
  this->ArrayCheckButtons = NULL;

  this->NoArraysLabel->Delete();
  this->NoArraysLabel = 0;

  this->Selection->Delete();

  delete this->ArraySet;

}

//----------------------------------------------------------------------------
void vtkPVItemSelection::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();
  
  this->LabeledFrame->SetParent(this);
  this->LabeledFrame->Create();
  if (this->LabelText)
    {
    this->LabeledFrame->SetLabelText(this->LabelText);
    }
  else
    {
    this->LabeledFrame->SetLabelText(this->GetTraceHelper()->GetObjectName());
    }
  this->Script("pack %s -fill x -expand t -side top",
              this->LabeledFrame->GetWidgetName());

  this->ButtonFrame->SetParent(this->LabeledFrame->GetFrame());
  this->ButtonFrame->Create();
  this->Script("pack %s -fill x -side top -expand t",
              this->ButtonFrame->GetWidgetName());

  this->AllOnButton->SetParent(this->ButtonFrame);
  this->AllOnButton->Create();
  this->AllOnButton->SetText("All On");
  this->AllOnButton->SetCommand(this, "AllOnCallback");

  this->AllOffButton->SetParent(this->ButtonFrame);
  this->AllOffButton->Create();
  this->AllOffButton->SetText("All Off");
  this->AllOffButton->SetCommand(this, "AllOffCallback");

  this->Script("pack %s %s -fill x -side left -expand t -padx 2 -pady 2",
              this->AllOnButton->GetWidgetName(),
              this->AllOffButton->GetWidgetName());

  this->CheckFrame->SetParent(this->LabeledFrame->GetFrame());
  this->CheckFrame->Create();

  this->Script("pack %s -side top -expand f -anchor w",
              this->CheckFrame->GetWidgetName());

  this->NoArraysLabel->SetParent(this->CheckFrame);
  this->NoArraysLabel->Create();
  this->NoArraysLabel->SetText("No arrays");

  // This creates the check buttons and packs the button frame.
  this->Reset();
}

//----------------------------------------------------------------------------
void vtkPVItemSelection::UpdateSelections(int)
{
  vtkSMIntVectorProperty* svp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (svp)
    {
    vtkSMEnumerationDomain* dom3 = 0;
    if ( (dom3 = vtkSMEnumerationDomain::SafeDownCast(
        svp->GetDomain("array_list"))) )
      {
      unsigned int numStrings = dom3->GetNumberOfEntries();

      // Obtain parameters from the domain (that obtained them
      // from the information property that obtained them from the server)
      for(unsigned int i=0; i < numStrings; ++i)
        {
        const char* name = dom3->GetEntryText(i);
        if (!name)
          {
          continue;
          }
        int value = dom3->GetEntryValue(i);
        unsigned int cc;
        int found = 0;
        for ( cc = 0; cc < svp->GetNumberOfElements(); ++ cc )
          {
          if ( value == svp->GetElement(cc) )
            {
            found = 1;
            break;
            }
          }
        if ( found )
          {
          this->Selection->EnableArray(name);
          }
        else
          {
          this->Selection->DisableArray(name);
          }
        }
      }
    else
      {
      vtkErrorMacro("An appropriate domain (name: array_list) is not specified. "
                    "Can not update");
      }
    }
  else
    {
    vtkErrorMacro("An appropriate property not specified. "
                  "Can not update");
    }

}

//----------------------------------------------------------------------------
void vtkPVItemSelection::UpdateGUI()
{
  vtkKWCheckButton* checkButton;

  // See if we need to create new check buttons.
  vtkPVItemSelectionArraySet newSet;
  int i;
  for(i=0; i < this->Selection->GetNumberOfArrays(); ++i)
    {
    newSet.insert(this->Selection->GetArrayName(i));
    }
  
  if(newSet != *(this->ArraySet))
    {
    *(this->ArraySet) = newSet;

    // Clear out any old check buttons.
    this->Script("catch {eval pack forget [pack slaves %s]}",
                 this->CheckFrame->GetWidgetName());
    this->ArrayCheckButtons->RemoveAllItems();
    
    vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);

    // Create new check buttons.
    if (sourceID.ID)
      {
      int numArrays, idx;
      int row = 0;
      numArrays = this->Selection->GetNumberOfArrays();
      for (idx = 0; idx < numArrays; ++idx)
        {
        checkButton = vtkKWCheckButton::New();
        checkButton->SetParent(this->CheckFrame);
        checkButton->Create();
        this->Script("%s SetText {%s}", checkButton->GetTclName(), 
                     this->Selection->GetArrayName(idx));
        this->Script("grid %s -row %d -sticky w", checkButton->GetWidgetName(), row);
        ++row;
        checkButton->SetCommand(this, "CheckButtonCallback");
        this->ArrayCheckButtons->AddItem(checkButton);
        checkButton->Delete();
        }
      if ( numArrays == 0 )
        {
        this->Script("grid %s -row 0 -sticky w", this->NoArraysLabel->GetWidgetName());
        }
      }
    }
  
  vtkCollectionIterator* it = this->ArrayCheckButtons->NewIterator();
  for(it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkKWCheckButton* check = static_cast<vtkKWCheckButton*>(it->GetCurrentObject());
    check->SetSelectedState(this->Selection->ArrayIsEnabled(check->GetText()));
    }
  it->Delete();
 
}

//----------------------------------------------------------------------------
void vtkPVItemSelection::CheckButtonCallback(int)
{
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVItemSelection::Initialize()
{
  // Update our local vtkDataItemSelection instance with the reader's
  // settings.
  this->UpdateSelections(1);

  this->UpdateGUI();
}

//----------------------------------------------------------------------------
void vtkPVItemSelection::ResetInternal()
{
  // Or update from the property
  this->UpdateSelections(0);

  this->UpdateGUI();

  this->ModifiedFlag = 0;
}

//---------------------------------------------------------------------------
void vtkPVItemSelection::Trace(ofstream *file)
{
  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }

  vtkCollectionIterator* it = this->ArrayCheckButtons->NewIterator();
  for(it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkKWCheckButton* check = static_cast<vtkKWCheckButton*>(it->GetCurrentObject());
    *file << "$kw(" << this->GetTclName() << ") SetArrayStatus {"
          << check->GetText() << "} " << check->GetSelectedState() << endl;
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkPVItemSelection::Accept()
{
  // Create new check buttons.
  if (!this->PVSource->GetVTKSourceID(0).ID)
    {
    vtkErrorMacro("VTKReader has not been set.");
    }

  this->SetPropertyFromGUI();
  this->Superclass::Accept();
}

//----------------------------------------------------------------------------
void vtkPVItemSelection::PostAccept()
{
  // In case changing the selection caused changes in other 
  // selections, we update information and GUI from reader
  this->GetPVSource()->GetProxy()->UpdatePipelineInformation();
  this->UpdateSelections(1);
  this->UpdateGUI();
}

//----------------------------------------------------------------------------
void vtkPVItemSelection::SetPropertyFromGUI()
{
  vtkSMIntVectorProperty *svp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());

  if (svp)
    {
    svp->SetNumberOfElements(0);
    vtkSMEnumerationDomain* dom3 = 0;
    if ( (dom3 = vtkSMEnumerationDomain::SafeDownCast(
          svp->GetDomain("array_list"))) )
      {
      vtkCollectionIterator* it = this->ArrayCheckButtons->NewIterator();
      int elemCount = 0;
      for(it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
        {
        vtkKWCheckButton* check = static_cast<vtkKWCheckButton*>(it->GetCurrentObject());
        int state = check->GetSelectedState();
        if ( state )
          {
          int value;
          if ( this->GetNumberFromName(check->GetText(), &value) )
            {
            svp->SetElement(elemCount, value);
            elemCount ++;
            }
          else
            {
            abort();
            }
          }
        }
      svp->SetNumberOfElements(elemCount);
      it->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVItemSelection::AllOnCallback()
{
  vtkKWCheckButton *check;
  int modified = 0;;

  this->ArrayCheckButtons->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    if (check->GetSelectedState() == 0)
      {
      modified = 1;
      check->SetSelectedState(1);
      }
    }

  if (modified)
    {
    this->ModifiedCallback();
    }   
}


//----------------------------------------------------------------------------
void vtkPVItemSelection::AllOffCallback()
{
  vtkKWCheckButton *check;
  int modified = 0;;

  this->ArrayCheckButtons->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    if (check->GetSelectedState() != 0)
      {
      modified = 1;
      check->SetSelectedState(0);
      }
    }

  if (modified)
    {
    this->ModifiedCallback();
    }   
}

//----------------------------------------------------------------------------
void vtkPVItemSelection::SetArrayStatus(const char *name, int status)
{
  vtkKWCheckButton *check;

  this->ArrayCheckButtons->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    if ( strcmp(check->GetText(), name) == 0)
      {
      check->SetSelectedState(status);
      return;
      }
    }  
  vtkErrorMacro("Could not find array: " << name);
}


//----------------------------------------------------------------------------
void vtkPVItemSelection::SaveInBatchScript(ofstream *file)
{
  const char* sourceID = this->PVSource->GetProxy()->GetSelfIDAsString();

  if (!sourceID || !this->SMPropertyName)
    {
    vtkErrorMacro("Sanity check failed. " << this->GetClassName());
    return;
    }

  this->UpdateSelections(1);
  vtkCollectionIterator* it = this->ArrayCheckButtons->NewIterator();

  int numElems=0;
  for(it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkKWCheckButton* check = static_cast<vtkKWCheckButton*>(it->GetCurrentObject());
    if ( this->Selection->ArrayIsEnabled(check->GetText()) )
      {
      numElems++;
      }
    }

  if (numElems > 0)
    {
    // Need to update information before setting array selections.
    *file << "  " 
          << "$pvTemp" << sourceID << " UpdateVTKObjects\n";
    *file << "  " 
          << "$pvTemp" << sourceID << " UpdatePipelineInformation\n";
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << this->SMPropertyName << "] SetNumberOfElements " 
          << numElems << endl;
    }
  numElems=0;
  for(it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkKWCheckButton* check = 
      static_cast<vtkKWCheckButton*>(it->GetCurrentObject());
    // Since they default to on.
    if(this->Selection->ArrayIsEnabled(check->GetText()))
      {
      int value;
      this->GetNumberFromName(check->GetText(), &value);
      *file << "  [$pvTemp" << sourceID << " GetProperty "
        << this->SMPropertyName << "] SetElement " 
        << numElems << " " << value << " #--- " << check->GetText() << endl;
      numElems++;
      }
    }
  it->Delete();
}

//----------------------------------------------------------------------------
vtkPVItemSelection* vtkPVItemSelection::ClonePrototype(vtkPVSource* pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVItemSelection::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVItemSelection::CopyProperties(vtkPVWidget* clone,
  vtkPVSource* pvSource,
  vtkArrayMap<vtkPVWidget*,
  vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVItemSelection* pvas = vtkPVItemSelection::SafeDownCast(clone);
  if (pvas)
    {
    pvas->SetLabelText(this->LabelText);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVItemSelection.");
    }
}

//----------------------------------------------------------------------------
int vtkPVItemSelection::ReadXMLAttributes(vtkPVXMLElement* element,
  vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }

  const char* label_text = element->GetAttribute("label_text");
  if(label_text)
    {
    this->SetLabelText(label_text);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPVItemSelection::GetNumberOfArrays()
{
  return this->ArrayCheckButtons->GetNumberOfItems();
}

//----------------------------------------------------------------------------
void vtkPVItemSelection::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->LabeledFrame);

  this->PropagateEnableState(this->ButtonFrame);
  this->PropagateEnableState(this->AllOnButton);
  this->PropagateEnableState(this->AllOffButton);

  this->PropagateEnableState(this->CheckFrame);
  vtkCollectionIterator* it = this->ArrayCheckButtons->NewIterator();
  for ( it->InitTraversal();
    !it->IsDoneWithTraversal();
    it->GoToNextItem() )
    {
    this->PropagateEnableState(vtkKWWidget::SafeDownCast(it->GetCurrentObject()));
    }
  it->Delete();
  this->PropagateEnableState(this->NoArraysLabel);

}

//----------------------------------------------------------------------------
const char* vtkPVItemSelection::GetNameFromNumber(int num)
{
  vtkSMIntVectorProperty* svp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (!svp)
    {
    return 0;
    }

  vtkSMEnumerationDomain* dom3 = vtkSMEnumerationDomain::SafeDownCast(
    svp->GetDomain("array_list"));
  if (!dom3)
    {
    return 0;
    }
  unsigned int cc;
  for ( cc = 0; cc < dom3->GetNumberOfEntries(); ++ cc )
    {
    if ( num == dom3->GetEntryValue(cc) )
      {
      return dom3->GetEntryText(cc);
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVItemSelection::GetNumberFromName(const char* name, int* val)
{
  vtkSMIntVectorProperty* svp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (!svp)
    {
    return 0;
    }

  vtkSMEnumerationDomain* dom3 = vtkSMEnumerationDomain::SafeDownCast(
    svp->GetDomain("array_list"));
  if (!dom3)
    {
    return 0;
    }
  unsigned int cc;
  for ( cc = 0; cc < dom3->GetNumberOfEntries(); ++ cc )
    {
    if ( strcmp(name, dom3->GetEntryText(cc)) == 0 )
      {
      *val = dom3->GetEntryValue(cc);
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVItemSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "LabelText: " << (this->LabelText?this->LabelText:"none") << endl;
}
