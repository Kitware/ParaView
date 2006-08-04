/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAttributeEditor.cxx
Wylie, Brian
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAttributeEditor.h"

#include "vtkObjectFactory.h"
#include "vtkCollectionIterator.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"

#include "vtkSMPart.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMInputProperty.h"

#include "vtkPVDisplayGUI.h"
#include "vtkPVApplication.h"
#include "vtkPVSourceNotebook.h"
#include "vtkPVWindow.h"
#include "vtkPVFileEntry.h"
#include "vtkPVSelectWidget.h"
#include "vtkPVReaderModule.h"
#include "vtkPVLabeledToggle.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVInputMenu.h"
#include "vtkPVColorMap.h"
#include "vtkPVWidgetCollection.h"
#include "vtkPVSphereWidget.h"
#include "vtkPVBoxWidget.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVAnimationScene.h"
#include "vtkPVPointWidget.h"
#include "vtkPVGenericRenderWindowInteractor.h"

#include "vtkKWPushButton.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMenu.h"
#include "vtkKWScale.h"
#include "vtkKWPopupButton.h"
#include "vtkKWEvent.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrameWithScrollbar.h"
#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVAttributeEditor);
vtkCxxRevisionMacro(vtkPVAttributeEditor, "1.16");


//----------------------------------------------------------------------------
vtkPVAttributeEditor::vtkPVAttributeEditor()
{
  this->WriterID.ID = 0;

  this->ForceEdit = 0;
  this->ForceNoEdit = 0;
  this->IsScalingFlag = 0;
  this->IsMovingFlag = 0;
  this->EditedFlag = 0;
  this->PassSourceInput = 0;
  this->SaveButton = vtkKWPushButton::New();

  this->EventCallbackCommand = vtkCallbackCommand::New();
  this->EventCallbackCommand->SetClientData(this); 
  this->EventCallbackCommand->SetCallback(vtkPVAttributeEditor::ProcessEvents);
}

vtkPVAttributeEditor::~vtkPVAttributeEditor()
{
  this->EventCallbackCommand->SetClientData(0);
  this->EventCallbackCommand->SetCallback(0);
  this->EventCallbackCommand->Delete();
  this->EventCallbackCommand = 0;
  this->SaveButton->Delete();
}

//----------------------------------------------------------------------------
void vtkPVAttributeEditor::CreateProperties()
{
  // Call the superclass to create the widget and set the appropriate flags
  this->Superclass::CreateProperties();

  // listen for the following events
  vtkPVGenericRenderWindowInteractor *interactor = this->GetPVWindow()->GetInteractor();
  if(interactor)
    {
    interactor->AddObserver(vtkCommand::CharEvent, this->EventCallbackCommand, 1);
    interactor->AddObserver(vtkCommand::RightButtonPressEvent, this->EventCallbackCommand, 1);
    interactor->AddObserver(vtkCommand::RightButtonReleaseEvent, this->EventCallbackCommand, 1);
    interactor->AddObserver(vtkCommand::LeftButtonPressEvent, this->EventCallbackCommand, 1);
    interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, this->EventCallbackCommand, 1);
    // Currently only a timestep change from the animation manager will prompt the user to save changes:
    this->GetPVWindow()->GetAnimationManager()->GetAnimationScene()->AddObserver(vtkKWEvent::TimeChangedEvent,this->EventCallbackCommand, 1);
    if (this->GetPVWindow()->GetCurrentPVReaderModule())
      {
      this->GetPVWindow()->GetCurrentPVReaderModule()->GetTimeStepWidget()->AddObserver(vtkKWEvent::TimeChangedEvent,this->EventCallbackCommand, 1);
      }
    }

  vtkPVSelectWidget *select = vtkPVSelectWidget::SafeDownCast(this->GetPVWidget("PickFunction"));
  select->SetModifiedCommand(this->GetTclName(),"PickMethodObserver");

  // If this is not exodus data, do not pack the saving widgets
  //vtkPVReaderModule *mod = this->GetPVWindow()->GetCurrentPVReaderModule();
  //if(mod ==NULL || strcmp(mod->GetModuleName(),"ExodusReader")!=0)
  //  {
  //  return;
  //  }

  this->SaveButton->SetParent(this->ParameterFrame->GetFrame());
  this->SaveButton->Create();
  this->SaveButton->SetText("Save");
  this->SaveButton->SetCommand(this->GetPVWindow(), "WriteData");
  this->Script("pack %s -padx 2 -pady 4 -expand t", this->SaveButton->GetWidgetName());

  this->GetNotebook()->SetAutoAccept(0);
}


void vtkPVAttributeEditor::PickMethodObserver()
{
  vtkPVSelectWidget *select = vtkPVSelectWidget::SafeDownCast(this->GetPVWidget("PickFunction"));

  if(!strcmp(select->GetCurrentValue(),"'e'dit within a box") && this->GetInitialized())
    {
    this->GetNotebook()->SetAutoAccept(0);
    }
  else if(!strcmp(select->GetCurrentValue(),"'e'dit at a point") && this->GetInitialized())
    {
    this->GetNotebook()->SetAutoAccept(0);
    vtkPVPointWidget::SafeDownCast(select->GetPVWidget("'e'dit at a point"))->PositionResetCallback();
    }
  else if(!strcmp(select->GetCurrentValue(),"'e'dit within a draggable sphere") && this->GetInitialized())
    {
    this->GetNotebook()->SetAutoAccept(1);
    }
}

//----------------------------------------------------------------------------
void vtkPVAttributeEditor::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                       unsigned long event, 
                                       void* clientdata, 
                                       void* vtkNotUsed(calldata))
{
  vtkPVAttributeEditor* self = reinterpret_cast<vtkPVAttributeEditor *>( clientdata );
  int leftup = 0;
  int leftdown = 0;
  //look for char and delete events
  switch(event)
    {
    case vtkCommand::CharEvent:
      self->OnChar();
      break;
    case vtkCommand::RightButtonPressEvent:
      self->SetIsScalingFlag(1);
      break;
    case vtkCommand::RightButtonReleaseEvent:
      {
      self->SetForceNoEdit(1);
      self->SetIsScalingFlag(0);
      break;
      }
    case vtkCommand::LeftButtonPressEvent:
      leftdown = 1;
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      leftup = 1;
      break;
    case vtkKWEvent::TimeChangedEvent:
      self->OnTimestepChange();
      break;
    }

  // Toggle auto-accept if our widget is a sphere based on whether this is a mouse up or down event
  vtkPVSelectWidget *select = vtkPVSelectWidget::SafeDownCast(self->GetPVWidget("PickFunction"));
  if(!strcmp(select->GetCurrentValue(),"'e'dit within a draggable sphere") && self->GetInitialized())
    {
    if(leftdown)
      {
      self->GetNotebook()->SetAutoAccept(1);
      }
    else if(leftup)
      {
      self->GetNotebook()->SetAutoAccept(0);
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVAttributeEditor::OnTimestepChange()
{
  if(this->EditedFlag)
    {
    if ( vtkKWMessageDialog::PopupYesNo(
          this->GetPVApplication(), this->GetPVWindow(), "UnsavedChanges",
          "Save Changes?", 
          "Would you like to save the changes you have made to the current time step in the Attribute Editor filter before continuing?", 
          vtkKWMessageDialog::QuestionIcon | vtkKWMessageDialog::RememberYes |
          vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault ))
      {
      this->Select();
      this->GetPVWindow()->SetCurrentPVSource(this);
      this->GetPVWindow()->WriteData();
      }

    this->EditedFlag = 0;
    }
  vtkSMIntVectorProperty* vp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProxy()->GetProperty("ClearEdits"));
  vp->SetElements1(1);

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProxy()->GetProperty("EditMode"));
  ivp->SetElements1(0);

  this->GetProxy()->MarkAllPropertiesAsModified();
  this->GetProxy()->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVAttributeEditor::OnChar()
{
  if (this->GetPVWindow()->GetInteractor()->GetKeyCode() == 'e' ||
      this->GetPVWindow()->GetInteractor()->GetKeyCode() == 'E' )
    {
    // This is a hack to make accept think its been modified:
    this->Notebook->SetAcceptButtonColorToModified();
    // We want filter to edit no matter what (i.e. even if some pvwidget's state has changed)
    this->ForceEdit = 1;
    //this->Modified();
    this->AcceptCallback();
    this->ForceEdit = 0;

    return;
    }
/*
  else if (this->GetPVWindow()->GetInteractor()->GetKeyCode() == 't' ||
      this->GetPVWindow()->GetInteractor()->GetKeyCode() == 'T' )
    {
    vtkPVSelectWidget *select = vtkPVSelectWidget::SafeDownCast(this->GetPVWidget("PickFunction"));
    vtkPVPickSphereWidget *sphere = vtkPVPickSphereWidget::SafeDownCast(select->GetPVWidget("'e'dit within a draggable sphere"));

    if(strcmp(select->GetCurrentValue(),"'e'dit within a box") == 0)
      {
      box->GetMouseControlToggle()->ToggleSelectedState();
      box->SetMouseControlToggle(
        box->GetMouseControlToggle()->GetSelectedState());
      }
    vtkPVPickBoxWidget *box = vtkPVPickBoxWidget::SafeDownCast(select->GetPVWidget("'e'dit within a box"));

    else

    if(strcmp(select->GetCurrentValue(),"'e'dit within a draggable sphere") == 0)
      {
      sphere->GetMouseControlToggle()->ToggleSelectedState();
      sphere->SetMouseControlToggle(
        sphere->GetMouseControlToggle()->GetSelectedState());
      }

    return;
    }
*/
}


//----------------------------------------------------------------------------
void vtkPVAttributeEditor::AcceptCallbackInternal()
{  
  int editFlag = 1;
  int inputModified = 0;

  // If this is the first time accept has been called on this source, make sure not to edit
  if(!this->GetInitialized())
    {
/*
  vtkPVInputMenu *filterInput = vtkPVInputMenu::SafeDownCast(this->GetPVWidget("Input"));
  vtkPVInputMenu *sourceInput = vtkPVInputMenu::SafeDownCast(this->GetPVWidget("Source"));
  vtkPVSource *filterValue = filterInput->GetCurrentValue();
  vtkPVSource *sourceValue = sourceInput->GetCurrentValue();
  filterInput->SetCurrentValue(NULL);
  sourceInput->SetCurrentValue(NULL);
  filterInput->MenuEntryCallback(filterValue);
  sourceInput->MenuEntryCallback(sourceValue);
*/
/*
  filterInput->ModifiedCallback();
  sourceInput->ModifiedCallback();
  filterInput->Update();
  sourceInput->Update();
*/
    vtkPVReaderModule *reader = this->GetPVWindow()->GetCurrentPVReaderModule();
    if(reader)
      {
/*
      const char *fname = reader->GetFileEntry()->GetValue();
      const char *dname = vtksys::SystemTools::GetParentDirectory(fname).c_str();
      this->GetApplication()->SetRegistryValue(
        1, "RunTime", "SaveDataFile", dname);
*/
      this->GetProxy()->AddInput(reader->GetProxy(),"SetSource",0);
      }
    editFlag = 0;
    }
  else
    {
    // If any of the pvwidgets except the box/point/sphere have been modified, don't edit
    vtkPVWidgetCollection *col = this->GetWidgets();
    if(col)
      {
      vtkCollectionIterator *it = col->NewIterator();
      it->InitTraversal();
      while( !it->IsDoneWithTraversal() )
        {
        vtkPVWidget *widget = static_cast<vtkPVWidget*>(it->GetCurrentObject());
        if(widget->GetModifiedFlag())
          {
          if(widget->IsA("vtkPVInputMenu"))
            {
//            this->GetProxy()->AddInput(this->GetPVWindow()->GetCurrentPVReaderModule()->GetProxy(),"SetSource",0);
/*
            vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
              this->GetProxy()->GetProperty("Source"));
            ip->SetProxy(0,this->GetPVWindow()->GetCurrentPVReaderModule()->GetProxy());
*/
            vtkSMIntVectorProperty* vp = vtkSMIntVectorProperty::SafeDownCast(
              this->GetProxy()->GetProperty("ClearEdits"));
            vp->SetElements1(1);
            inputModified = 1;
            editFlag = 0;
            }
          else if(widget->IsA("vtkPVSelectWidget"))
            {
            // if the actual selection has been modified and not one of the point/box/sphere widgets, don't edit
            vtkPVSelectWidget *select = vtkPVSelectWidget::SafeDownCast(widget);
            if(select->GetPVWidget(select->GetCurrentValue())->GetModifiedFlag() == 0)
              {
              editFlag = 0;
              }
            }
          else if(widget->IsA("vtkPVArrayMenu"))
            {
            // if the attribute array has changed, tell filter to clear its stored arrays
            vtkSMIntVectorProperty* vp = vtkSMIntVectorProperty::SafeDownCast(
              this->GetProxy()->GetProperty("ClearEdits"));
            vp->SetElements1(1);
            //this->GetProxy()->UpdateVTKObjects();
            }
          else
            {
            editFlag = 0;
            }
          }
        it->GoToNextItem();
        }
      it->Delete();
      }
    }

  if(this->ForceEdit)
    {
    editFlag = 1;
    }
  else if(this->ForceNoEdit)
    {
    editFlag = 0;
    }

  if(this->PassSourceInput)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProxy()->GetProperty("UnfilteredDataset"));
    ivp->SetElements1(1);
    }
  
  if(editFlag)
    {
    vtkSMIntVectorProperty* vp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProxy()->GetProperty("ClearEdits"));
    vp->SetElements1(0);
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProxy()->GetProperty("EditMode"));
    ivp->SetElements1(editFlag);
    //this->GetProxy()->SetPropertyModifiedFlag("EditMode",1);
    //this->GetProxy()->UpdateProperty("EditMode",1);
    }

  this->GetProxy()->MarkAllPropertiesAsModified();
  this->GetProxy()->UpdateVTKObjects();

  // Has this source been edited yet?
  if(this->EditedFlag==0)
    {
    this->EditedFlag = editFlag;
    }

  this->Superclass::AcceptCallbackInternal();

  // update the widgets if new data (setting input)
  if(inputModified)
    {
    vtkPVSelectWidget *select = vtkPVSelectWidget::SafeDownCast(this->GetPVWidget("PickFunction"));
    vtkPVPointWidget *point = vtkPVPointWidget::SafeDownCast(select->GetPVWidget("'e'dit at a point"));
    if(point)
      {
      point->ActualPlaceWidget();
      }
    vtkPVBoxWidget *box = vtkPVBoxWidget::SafeDownCast(select->GetPVWidget("'e'dit within a box"));
    if(box)
      {
      box->ActualPlaceWidget();
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVAttributeEditor::Select()
{
  vtkPVSource *input;

  if(!this->GetInitialized())
    {
    this->Superclass::Select();
    return;
    }

  vtkPVInputMenu *filterInput = vtkPVInputMenu::SafeDownCast(this->GetPVWidget("Input"));
  input = filterInput->GetCurrentValue();
//  vtkPVInputMenu *sourceInput = vtkPVInputMenu::SafeDownCast(this->GetPVWidget("Source"));
//  source = sourceInput->GetCurrentValue();

  this->Superclass::Select();

  // Initialize the inputs when the source is selected in the selection window. 
  // This is kindof a hack because the state of the input menus was not being maintained
  // when returning to this source
  filterInput->SetCurrentValue(input);
//  sourceInput->SetCurrentValue(source);
  this->ForceNoEdit = 1;
  this->AcceptCallback();
  this->ForceNoEdit = 0;
}

//----------------------------------------------------------------------------
void vtkPVAttributeEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "SetEditedFlag" << this->GetEditedFlag() << endl;
  os << indent << "SetIsScalingFlag" << this->IsScalingFlag << endl;
  os << indent << "SetIsMovingFlag" << this->IsMovingFlag << endl;
  os << indent << "SetForceEdit" << this->ForceEdit << endl;
  os << indent << "SetForceNoEdit" << this->ForceNoEdit << endl;
  os << indent << "PassSourceInput" << this->PassSourceInput << endl;
  
}
