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
#include "vtkSMStringVectorProperty.h"
#include "vtkTclUtil.h"

#include <vtkstd/string>
#include <vtkstd/set>

typedef vtkstd::set<vtkstd::string> vtkPVArraySelectionArraySetBase;
class vtkPVArraySelectionArraySet: public vtkPVArraySelectionArraySetBase {};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVArraySelection);
vtkCxxRevisionMacro(vtkPVArraySelection, "1.52");

//----------------------------------------------------------------------------
int vtkDataArraySelectionCommand(ClientData cd, Tcl_Interp *interp,
                                 int argc, char *argv[]);
int vtkPVArraySelectionCommand(ClientData cd, Tcl_Interp *interp,
                               int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVArraySelection::vtkPVArraySelection()
{
  this->CommandFunction = vtkPVArraySelectionCommand;
  
  this->AttributeName = 0;
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
  this->ServerSideID.ID = 0;
}

//----------------------------------------------------------------------------
vtkPVArraySelection::~vtkPVArraySelection()
{
  this->SetAttributeName(0);
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

  if(this->ServerSideID.ID)
    {
    vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
    pm->DeleteStreamObject(this->ServerSideID);
    pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT);
    }

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
    if (strcmp(this->AttributeName, "Point") == 0)
      {
      this->LabeledFrame->SetLabel("Point Arrays");
      }
    else if (strcmp(this->AttributeName, "Cell") == 0)
      {
      this->LabeledFrame->SetLabel("Cell Arrays");
      }
    else 
      {
      ostrstream str;
      if ( this->AttributeName && this->AttributeName[0] )
        {
        str << this->AttributeName;
        }
      else
        {
        str << "Unnamed";
        }
      str << " Arrays" << ends;
      this->LabeledFrame->SetLabel(str.str());
      str.rdbuf()->freeze(0);
      }
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
void vtkPVArraySelection::SetLocalSelectionsFromReader()
{
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  this->Selection->RemoveAllArrays();
  
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);

  vtkSMStringVectorProperty *svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if(sourceID.ID && (svp || this->AcceptCalled))
    {
    this->CreateServerSide();
    pm->GetStream() << vtkClientServerStream::Invoke
                    << this->ServerSideID << "GetArraySettings"
                    << sourceID << this->AttributeName
                    << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT);
    vtkClientServerStream arrays;
    if(pm->GetLastResult(vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &arrays))
      {
      int numArrays = arrays.GetNumberOfArguments(0)/2;
      for(int i=0; i < numArrays; ++i)
        {
        // Get the array name.
        const char* name;
        if(!arrays.GetArgument(0, i*2, &name))
          {
          vtkErrorMacro("Error getting array name from reader.");
          break;
          }

         // Get the array status.
        int status;
        if(!arrays.GetArgument(0, i*2 + 1, &status))
          {
          vtkErrorMacro("Error getting array status from reader.");
          break;
          }

        // Set the selection to match the reader.
        if(status)
          {
          this->Selection->EnableArray(name);
          if (!this->AcceptCalled)
            {
            svp->SetElement(2*i, name);
            svp->SetElement(2*i+1, "1");
            }
          }
        else
          {
          this->Selection->DisableArray(name);
          if (!this->AcceptCalled)
            {
            svp->SetElement(2*i, name);
            svp->SetElement(2*i+1, "0");
            }
          }
        }
      }
    else
      {
      vtkErrorMacro("Error getting set of arrays from reader.");
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVArraySelection::ResetInternal()
{
  vtkKWCheckButton* checkButton;
  
  // Update our local vtkDataArraySelection instance with the reader's
  // settings.
  this->SetLocalSelectionsFromReader();
  
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
  
  // Now set the state of the check buttons.
  this->SetWidgetSelectionsFromLocal();
  
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

  this->SetLocalSelectionsFromReader();
  this->SetReaderSelectionsFromWidgets();
  this->SetLocalSelectionsFromReader();
  this->SetWidgetSelectionsFromLocal();

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

  this->AcceptCalled = 1;
}

//---------------------------------------------------------------------------
void vtkPVArraySelection::SetWidgetSelectionsFromLocal()
{
  vtkSMStringVectorProperty *svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (svp)
    {
    vtkCollectionIterator* it = this->ArrayCheckButtons->NewIterator();
    int checkIdx;
    for(it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
      {
      vtkKWCheckButton* check =
        static_cast<vtkKWCheckButton*>(it->GetObject());
      checkIdx = svp->GetElementIndex(check->GetText());
      if (checkIdx > -1)
        {
        check->SetState(atoi(svp->GetElement(checkIdx+1)));
        }
      }
    it->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVArraySelection::SetReaderSelectionsFromWidgets()
{
  vtkCollectionIterator* it = this->ArrayCheckButtons->NewIterator();

  vtkSMStringVectorProperty *svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  int elemCount = 0;

  if (svp)
    {
    for(it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
      {
      vtkKWCheckButton* check = static_cast<vtkKWCheckButton*>(it->GetObject());
      svp->SetElement(elemCount++, check->GetText());
      ostrstream str;
      str << check->GetState() << ends;
      svp->SetElement(elemCount++, str.str());
      str.rdbuf()->freeze();
      }
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

  this->SetLocalSelectionsFromReader();
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

vtkPVArraySelection* vtkPVArraySelection::ClonePrototype(vtkPVSource* pvSource,
                                  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVArraySelection::SafeDownCast(clone);
}

void vtkPVArraySelection::CopyProperties(vtkPVWidget* clone,
                                         vtkPVSource* pvSource,
                                         vtkArrayMap<vtkPVWidget*,
                                         vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVArraySelection* pvas = vtkPVArraySelection::SafeDownCast(clone);
  if (pvas)
    {
    pvas->SetAttributeName(this->AttributeName);
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
  
  const char* attribute_name = element->GetAttribute("attribute_name");
  if(attribute_name)
    {
    this->SetAttributeName(attribute_name);
    }
  else
    {
    vtkErrorMacro("No attribute_name specified.");
    return 0;
    }

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
void vtkPVArraySelection::CreateServerSide()
{
  if(!this->ServerSideID.ID)
    {
    vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
    this->ServerSideID = pm->NewStreamObject("vtkPVServerArraySelection");
    pm->GetStream() << vtkClientServerStream::Invoke
                    << this->ServerSideID << "SetProcessModule"
                    << pm->GetProcessModuleID()
                    << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT);
    }
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
  os << indent << "AttributeName: " 
     << (this->AttributeName?this->AttributeName:"none") << endl;
  os << indent << "LabelText: " << (this->LabelText?this->LabelText:"none") << endl;
}
