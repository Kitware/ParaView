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
#include "vtkTclUtil.h"

#include <vtkstd/string>
#include <vtkstd/set>

typedef vtkstd::set<vtkstd::string> vtkPVArraySelectionArraySetBase;
class vtkPVArraySelectionArraySet: public vtkPVArraySelectionArraySetBase {};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVArraySelection);
vtkCxxRevisionMacro(vtkPVArraySelection, "1.49");

//----------------------------------------------------------------------------
int vtkDataArraySelectionCommand(ClientData cd, Tcl_Interp *interp,
                                 int argc, char *argv[]);
int vtkPVArraySelectionCommand(ClientData cd, Tcl_Interp *interp,
                               int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVArraySelection::vtkPVArraySelection()
{
  this->CommandFunction = vtkPVArraySelectionCommand;
  
  this->VTKReaderID.ID = 0;
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
  if(this->VTKReaderID.ID)
    {
    this->CreateServerSide();
    pm->GetStream() << vtkClientServerStream::Invoke
                    << this->ServerSideID << "GetArraySettings"
                    << this->VTKReaderID << this->AttributeName
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
          }
        else
          {
          this->Selection->DisableArray(name);
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
    
    // Create new check buttons.
    if (this->VTKReaderID.ID)
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
void vtkPVArraySelection::AcceptInternal(vtkClientServerID id)
{
  this->Superclass::AcceptInternal(id);

  // Create new check buttons.
  if (!this->VTKReaderID.ID)
    {
    vtkErrorMacro("VTKReader has not been set.");
    }

  this->SetLocalSelectionsFromReader();
  this->SetReaderSelectionsFromWidgets();
  this->SetLocalSelectionsFromReader();
  this->SetWidgetSelectionsFromLocal();
}

//---------------------------------------------------------------------------
void vtkPVArraySelection::SetWidgetSelectionsFromLocal()
{
  vtkCollectionIterator* it = this->ArrayCheckButtons->NewIterator();
  for(it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkKWCheckButton* check = static_cast<vtkKWCheckButton*>(it->GetObject());
    check->SetState(this->Selection->ArrayIsEnabled(check->GetText()));
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkPVArraySelection::SetReaderSelectionsFromWidgets()
{
  vtkPVApplication *pvApp = this->GetPVApplication();  
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkCollectionIterator* it = this->ArrayCheckButtons->NewIterator();
  vtkstd::string setArrayStatus = "Set";
  setArrayStatus += this->AttributeName;
  setArrayStatus += "ArrayStatus";
  for(it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkKWCheckButton* check = static_cast<vtkKWCheckButton*>(it->GetObject());
    // This test is only here to try to avoid extra lines in the trace
    // file.  We could make every check button a pv widget.
    if(this->Selection->ArrayIsEnabled(check->GetText()) != check->GetState())
      {
      pm->GetStream() << vtkClientServerStream::Invoke
                      << this->VTKReaderID
                      << setArrayStatus.c_str()
                      << check->GetText() << check->GetState()
                      << vtkClientServerStream::End;
      }
    }
  it->Delete();
  pm->SendStream(vtkProcessModule::DATA_SERVER);
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
  if (!this->VTKReaderID.ID)
    {
    vtkErrorMacro("VTKReader has not been set.");
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
          << "$pvTemp" << this->VTKReaderID << " UpdateVTKObjects\n";
    *file << "  " 
          << "$pvTemp" << this->VTKReaderID << " UpdateInformation\n";
    *file << "  [$pvTemp" << this->VTKReaderID << " GetProperty "
          << this->AttributeName << "ArrayStatus ] SetNumberOfElements " 
          << 2*numElems << endl;
    }
  numElems=0;
  for(it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkKWCheckButton* check = static_cast<vtkKWCheckButton*>(it->GetObject());
    // Since they default to on.
    if(this->Selection->ArrayIsEnabled(check->GetText()))
      {
      *file << "  [$pvTemp" << this->VTKReaderID << " GetProperty "
            << this->AttributeName << "ArrayStatus ] SetElement " 
            << 2*numElems << " {" << check->GetText() << "}" << endl;
      *file << "  [$pvTemp" << this->VTKReaderID << " GetProperty "
            << this->AttributeName << "ArrayStatus ] SetElement " 
            << 2*numElems+1 << " " << 1 << endl;
      }
    else
      {
      *file << "  [$pvTemp" << this->VTKReaderID << " GetProperty "
            << this->AttributeName << "ArrayStatus ] SetElement " 
            << 2*numElems << " {" << check->GetText() << "}" << endl;
      *file << "  [$pvTemp" << this->VTKReaderID << " GetProperty "
            << this->AttributeName << "ArrayStatus ] SetElement " 
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
    // It is assumed that there is only one VTK source id.
    // Since this is a source object (reader probably), this
    // is a reasonable assumption.
    pvas->VTKReaderID = pvSource->GetVTKSourceID(0);
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
  os << indent << "VTKReaderID: " << this->VTKReaderID.ID << endl;
  os << indent << "LabelText: " << (this->LabelText?this->LabelText:"none") << endl;
}
