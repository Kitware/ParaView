/*=========================================================================

  Program:   ParaView
  Module:    vtkPVArraySelection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVArraySelection.h"

#include "vtkArrayMap.txx"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkDataArraySelection.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringListRangeDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkTclUtil.h"

#include <vtkstd/string>
#include <vtkstd/set>
#include <vtkstd/vector>

typedef vtkstd::set<vtkstd::string> vtkPVArraySelectionArraySetBase;
class vtkPVArraySelectionArraySet: public vtkPVArraySelectionArraySetBase {};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVArraySelection);
vtkCxxRevisionMacro(vtkPVArraySelection, "1.59");

//----------------------------------------------------------------------------
int vtkDataArraySelectionCommand(ClientData cd, Tcl_Interp *interp,
                                 int argc, char *argv[]);
int vtkPVArraySelectionCommand(ClientData cd, Tcl_Interp *interp,
                               int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVArraySelection::vtkPVArraySelection()
{
  this->CommandFunction = vtkPVArraySelectionCommand;
  
  this->LabelText = 0;
  
  this->LabeledFrame = vtkKWLabeledFrame::New();
  this->ButtonFrame = vtkKWWidget::New();
  this->AllOnButton = vtkKWPushButton::New();
  this->AllOffButton = vtkKWPushButton::New();

  this->CheckFrame = vtkKWWidget::New();
  this->ArrayCheckButtons = vtkCollection::New();

  this->ArraySet = new vtkPVArraySelectionArraySet;

  this->NoArraysLabel = vtkKWLabel::New();
  this->Selection = vtkDataArraySelection::New();

}

//----------------------------------------------------------------------------
vtkPVArraySelection::~vtkPVArraySelection()
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
void vtkPVArraySelection::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
  
  this->LabeledFrame->SetParent(this);
  this->LabeledFrame->ShowHideFrameOn();
  this->LabeledFrame->Create(app, 0);
  if (this->LabelText)
    {
    this->LabeledFrame->SetLabel(this->LabelText);
    }
  else
    {
    this->LabeledFrame->SetLabel(this->TraceName);
    }
  app->Script("pack %s -fill x -expand t -side top",
              this->LabeledFrame->GetWidgetName());

  this->ButtonFrame->SetParent(this->LabeledFrame->GetFrame());
  this->ButtonFrame->Create(app, "frame", "");
  app->Script("pack %s -fill x -side top -expand t",
              this->ButtonFrame->GetWidgetName());

  this->AllOnButton->SetParent(this->ButtonFrame);
  this->AllOnButton->Create(app, "");
  this->AllOnButton->SetLabel("All On");
  this->AllOnButton->SetCommand(this, "AllOnCallback");

  this->AllOffButton->SetParent(this->ButtonFrame);
  this->AllOffButton->Create(app, "");
  this->AllOffButton->SetLabel("All Off");
  this->AllOffButton->SetCommand(this, "AllOffCallback");

  app->Script("pack %s %s -fill x -side left -expand t -padx 2 -pady 2",
              this->AllOnButton->GetWidgetName(),
              this->AllOffButton->GetWidgetName());

  this->CheckFrame->SetParent(this->LabeledFrame->GetFrame());
  this->CheckFrame->Create(app, "frame", "");

  app->Script("pack %s -side top -expand f -anchor w",
              this->CheckFrame->GetWidgetName());

  this->NoArraysLabel->SetParent(this->CheckFrame);
  this->NoArraysLabel->Create(app, 0);
  this->NoArraysLabel->SetLabel("No arrays");

  // This creates the check buttons and packs the button frame.
  this->Reset();
}

//----------------------------------------------------------------------------
void vtkPVArraySelection::UpdateSelections(int fromReader)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());

  vtkSMStringVectorProperty* isvp = 0;

  if (svp)
    {
    isvp = 
      vtkSMStringVectorProperty::SafeDownCast(svp->GetInformationProperty());
    }
  
  vtkSMStringVectorProperty* prop;
  if (fromReader)
    {
    prop = isvp;
    this->Selection->RemoveAllArrays();
    }
  else
    {
    prop = svp;
    }
  if (svp && prop)
    {
    vtkSMStringListRangeDomain* dom = vtkSMStringListRangeDomain::SafeDownCast(
      svp->GetDomain("array_list"));
    if (dom)
      {
      unsigned int numStrings = dom->GetNumberOfStrings();
      
      // Obtain parameters from the domain (that obtained them
      // from the information property that obtained them from the server)
      for(unsigned int i=0; i < numStrings; ++i)
        {
        const char* name = dom->GetString(i);
        int found=0;
        unsigned int idx = prop->GetElementIndex(name, found);
        if (!found)
          {
          continue;
          }
        int onoff = atoi(prop->GetElement(idx+1));
        if (onoff)
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
void vtkPVArraySelection::UpdateGUI()
{
  vtkKWCheckButton* checkButton;

  // See if we need to create new check buttons.
  vtkPVArraySelectionArraySet newSet;
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
        checkButton->Create(this->GetApplication(), "");
        this->Script("%s SetText {%s}", checkButton->GetTclName(), 
                     this->Selection->GetArrayName(idx));
        this->Script("grid %s -row %d -sticky w", checkButton->GetWidgetName(), row);
        ++row;
        checkButton->SetCommand(this, "ModifiedCallback");
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
    vtkKWCheckButton* check = static_cast<vtkKWCheckButton*>(it->GetObject());
    check->SetState(this->Selection->ArrayIsEnabled(check->GetText()));
    }
  it->Delete();

  
}

//----------------------------------------------------------------------------
void vtkPVArraySelection::ResetInternal()
{
  if (!this->AcceptCalled)
    {
    // Update our local vtkDataArraySelection instance with the reader's
    // settings.
    this->UpdateSelections(1);
    }
  else
    {
    // Or update from the property
    this->UpdateSelections(0);
    }

  this->UpdateGUI();

  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
}

//---------------------------------------------------------------------------
void vtkPVArraySelection::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  vtkCollectionIterator* it = this->ArrayCheckButtons->NewIterator();
  for(it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkKWCheckButton* check = static_cast<vtkKWCheckButton*>(it->GetObject());
    *file << "$kw(" << this->GetTclName() << ") SetArrayStatus {"
          << check->GetText() << "} " << check->GetState() << endl;
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkPVArraySelection::Accept()
{
  int modFlag = this->GetModifiedFlag();

  // Create new check buttons.
  if (!this->PVSource->GetVTKSourceID(0).ID)
    {
    vtkErrorMacro("VTKReader has not been set.");
    }

  this->AcceptCalled = 1;

  this->SetPropertyFromGUI();

  this->ModifiedFlag = 0;
  
  // I put this after the accept internal, because
  // vtkPVGroupWidget inactivates and builds an input list ...
  // Putting this here simplifies subclasses AcceptInternal methods.
  if (modFlag)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    ofstream* file = pvApp->GetTraceFile();
    if (file)
      {
      this->Trace(file);
      }
    }

}

//----------------------------------------------------------------------------
void vtkPVArraySelection::PostAccept()
{
  // In case changing the selection caused changes in other 
  // selections, we update information and GUI from reader
  this->GetPVSource()->GetProxy()->UpdateInformation();
  this->UpdateSelections(1);
  this->UpdateGUI();
}

//----------------------------------------------------------------------------
void vtkPVArraySelection::SetPropertyFromGUI()
{
  vtkSMStringVectorProperty *svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());

  if (svp)
    {
    vtkCollectionIterator* it = this->ArrayCheckButtons->NewIterator();
    int elemCount = 0;
    for(it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
      {
      vtkKWCheckButton* check = static_cast<vtkKWCheckButton*>(it->GetObject());
      const char* aname = check->GetText();
      int state = check->GetState();
      // Only send changes to the server. If the state of the
      // widget is the same as the state of the selection, we
      // know that on the server side, the value does not have
      // to change.
      if ( (state && !this->Selection->ArrayIsEnabled(aname)) ||
           (!state && this->Selection->ArrayIsEnabled(aname)) )
        {
        ostrstream str;
        str << state << ends;
        svp->SetElement(elemCount, aname);
        svp->SetElement(elemCount+1, str.str());
        elemCount += 2;
        delete[] str.str();
        }
      }
    svp->SetNumberOfElements(elemCount);
    it->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVArraySelection::AllOnCallback()
{
  vtkKWCheckButton *check;
  int modified = 0;;

  this->ArrayCheckButtons->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    if (check->GetState() == 0)
      {
      modified = 1;
      check->SetState(1);
      }
    }
   
  if (modified)
    {
    this->ModifiedCallback();
    }   
}


//----------------------------------------------------------------------------
void vtkPVArraySelection::AllOffCallback()
{
  vtkKWCheckButton *check;
  int modified = 0;;

  this->ArrayCheckButtons->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    if (check->GetState() != 0)
      {
      modified = 1;
      check->SetState(0);
      }
    }
   
  if (modified)
    {
    this->ModifiedCallback();
    }   
}

//----------------------------------------------------------------------------
void vtkPVArraySelection::SetArrayStatus(const char *name, int status)
{
  vtkKWCheckButton *check;

  this->ArrayCheckButtons->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->ArrayCheckButtons->GetNextItemAsObject())) )
    {
    if ( strcmp(check->GetText(), name) == 0)
      {
      check->SetState(status);
      return;
      }
    }  
  vtkErrorMacro("Could not find array: " << name);
}


//----------------------------------------------------------------------------
void vtkPVArraySelection::SaveInBatchScript(ofstream *file)
{
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);

  if (!sourceID.ID || !this->SMPropertyName)
    {
    vtkErrorMacro("Sanity check failed. " << this->GetClassName());
    return;
    }

  this->UpdateSelections(1);
  vtkCollectionIterator* it = this->ArrayCheckButtons->NewIterator();

  int numElems=0;
  for(it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    numElems++;
    }

  if (numElems > 0)
    {
    // Need to update information before setting array selections.
    *file << "  " 
          << "$pvTemp" << sourceID << " UpdateVTKObjects\n";
    *file << "  " 
          << "$pvTemp" << sourceID << " UpdateInformation\n";
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << this->SMPropertyName << "] SetNumberOfElements " 
          << 2*numElems << endl;
    }
  numElems=0;
  for(it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkKWCheckButton* check = static_cast<vtkKWCheckButton*>(it->GetObject());
    // Since they default to on.
    if(this->Selection->ArrayIsEnabled(check->GetText()))
      {
      *file << "  [$pvTemp" << sourceID << " GetProperty "
            << this->SMPropertyName << "] SetElement " 
            << 2*numElems << " {" << check->GetText() << "}" << endl;
      *file << "  [$pvTemp" << sourceID << " GetProperty "
            << this->SMPropertyName << "] SetElement " 
            << 2*numElems+1 << " " << 1 << endl;
      }
    else
      {
      *file << "  [$pvTemp" << sourceID << " GetProperty "
            << this->SMPropertyName << "] SetElement " 
            << 2*numElems << " {" << check->GetText() << "}" << endl;
      *file << "  [$pvTemp" << sourceID << " GetProperty "
            << this->SMPropertyName << "] SetElement " 
            << 2*numElems+1 << " " << 0 << endl;
      }
    numElems++;
    }
   it->Delete();
}

//----------------------------------------------------------------------------
vtkPVArraySelection* vtkPVArraySelection::ClonePrototype(vtkPVSource* pvSource,
                                  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVArraySelection::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVArraySelection::CopyProperties(vtkPVWidget* clone,
                                         vtkPVSource* pvSource,
                                         vtkArrayMap<vtkPVWidget*,
                                         vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVArraySelection* pvas = vtkPVArraySelection::SafeDownCast(clone);
  if (pvas)
    {
    pvas->SetLabelText(this->LabelText);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVArraySelection.");
    }
}

//----------------------------------------------------------------------------
int vtkPVArraySelection::ReadXMLAttributes(vtkPVXMLElement* element,
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
int vtkPVArraySelection::GetNumberOfArrays()
{
  return this->ArrayCheckButtons->GetNumberOfItems();
}

//----------------------------------------------------------------------------
void vtkPVArraySelection::UpdateEnableState()
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
    this->PropagateEnableState(vtkKWWidget::SafeDownCast(it->GetObject()));
    }
  it->Delete();
  this->PropagateEnableState(this->NoArraysLabel);

}

//----------------------------------------------------------------------------
void vtkPVArraySelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "LabelText: " << (this->LabelText?this->LabelText:"none") << endl;
}
