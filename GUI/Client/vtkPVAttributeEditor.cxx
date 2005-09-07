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
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMDataObjectDisplayProxy.h"

#include "vtkPVDisplayGUI.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
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
#include "vtkPVPickSphereWidget.h"
#include "vtkPVPickBoxWidget.h"
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

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVAttributeEditor);
vtkCxxRevisionMacro(vtkPVAttributeEditor, "1.5");


//----------------------------------------------------------------------------
vtkPVAttributeEditor::vtkPVAttributeEditor()
{
  this->Frame = vtkKWFrame::New();
  this->DataFrame = vtkKWFrame::New();
  this->Label = vtkKWLabel::New();
  this->Entry = vtkKWEntry::New();
  this->BrowseButton = vtkKWPushButton::New();
  this->SaveButton = vtkKWPushButton::New();

  this->WriterID.ID = 0;

  this->ForceEdit = 0;
  this->ForceNoEdit = 0;
  this->IsScalingFlag = 0;
  this->IsMovingFlag = 0;
  this->EditedFlag = 0;

  this->EventCallbackCommand = vtkCallbackCommand::New();
  this->EventCallbackCommand->SetClientData(this); 
  this->EventCallbackCommand->SetCallback(vtkPVAttributeEditor::ProcessEvents);
}

vtkPVAttributeEditor::~vtkPVAttributeEditor()
{  
  this->DataFrame->Delete();
  this->DataFrame = NULL;
  this->BrowseButton->Delete();
  this->BrowseButton = NULL;
  this->SaveButton->Delete();
  this->SaveButton = NULL;
  this->Entry->Delete();
  this->Entry = NULL;
  this->Label->Delete();
  this->Label = NULL;
  this->Frame->Delete();
  this->Frame = NULL;

  this->EventCallbackCommand->SetClientData(0);
  this->EventCallbackCommand->SetCallback(0);
  this->EventCallbackCommand->Delete();
  this->EventCallbackCommand = 0;
}


//----------------------------------------------------------------------------
void vtkPVAttributeEditor::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();

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
    this->GetPVWindow()->GetCurrentPVReaderModule()->GetTimeStepWidget()->AddObserver(vtkKWEvent::TimeChangedEvent,this->EventCallbackCommand, 1);
    }

  vtkPVSelectWidget *select = vtkPVSelectWidget::SafeDownCast(this->GetPVWidget("PickFunction"));
  select->SetModifiedCommand(this->GetTclName(),"PickMethodObserver");

  // If this is not exodus data, do not pack the saving widgets
  vtkPVReaderModule *mod = this->GetPVWindow()->GetCurrentPVReaderModule();
  if(mod ==NULL || strcmp(mod->GetModuleName(),"ExodusReader")!=0)
    {
    return;
    }

  this->Frame->SetParent(this->ParameterFrame->GetFrame());
  this->Frame->Create(pvApp);

  this->Label->SetParent(this->Frame);
  this->Entry->SetParent(this->Frame);
  this->BrowseButton->SetParent(this->Frame);
  this->SaveButton->SetParent(this->Frame);
  
  // Now a label
  this->Label->Create(pvApp);
  this->Label->SetText("Filename");
  this->Label->SetJustificationToRight();
  this->Label->SetWidth(18);
  this->Script("pack %s -side left", this->Label->GetWidgetName());
  
  // Now the entry
  this->Entry->Create(pvApp);
  this->Entry->SetValue(this->GetPVWindow()->GetCurrentPVReaderModule()->GetFileEntry()->GetValue());
  this->Script("pack %s -side left -fill x -expand t",
               this->Entry->GetWidgetName());
  
  // Now the push button
  this->BrowseButton->Create(pvApp);
  this->BrowseButton->SetText("Browse");
  this->BrowseButton->SetCommand(this, "BrowseCallback");

  // Now the push button
  this->SaveButton->Create(pvApp);
  this->SaveButton->SetText("Save");
  this->SaveButton->SetCommand(this, "SaveCallback");

  this->Script("pack %s -side left", this->BrowseButton->GetWidgetName());
  this->Script("pack %s -side left", this->SaveButton->GetWidgetName());
  this->Script("pack %s -pady 10 -side top -fill x -expand t", 
                this->Frame->GetWidgetName());

  this->DataFrame->SetParent(this->ParameterFrame->GetFrame());
  this->DataFrame->Create(pvApp);
  this->Script("pack %s",
               this->DataFrame->GetWidgetName());

  this->GetNotebook()->SetAutoAccept(0);
}

//----------------------------------------------------------------------------
void vtkPVAttributeEditor::BrowseCallback()
{
  ostrstream str;
  vtkKWLoadSaveDialog* saveDialog;
  const char* fname = this->Entry->GetValue();

  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVWindow* win = 0;
  if (!pvApp)
    {
    return;
    }

  saveDialog = pvApp->NewLoadSaveDialog();
  win = pvApp->GetMainWindow();

  saveDialog->SetLastPath(fname);
  saveDialog->Create(this->GetPVApplication());
  if (win) 
    { 
    saveDialog->SetParent(this); 
    }
  saveDialog->SaveDialogOn();
  saveDialog->SetTitle("Select File");
  char *ext = this->GetPVWindow()->GetCurrentPVReaderModule()->GetFileEntry()->GetExtension();
  if(ext)
    {
    saveDialog->SetDefaultExtension(ext);
    str << "{{} {." << ext << "}} ";
    }
  str << "{{All files} {*}}" << ends;  
  saveDialog->SetFileTypes(str.str());
  str.rdbuf()->freeze(0);  
  if(saveDialog->Invoke())
    {
    this->Script("%s SetValue {%s}", this->Entry->GetTclName(),
                 saveDialog->GetFileName());
    }

  saveDialog->Delete();
}


void vtkPVAttributeEditor::SaveCallback()
{
  vtkClientServerStream stream;
  int ghostLevel = 1;
  int editorFlag = 1;

  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  if(!pm)
    {
    return;
    }

  vtkPVArrayMenu *array = vtkPVArrayMenu::SafeDownCast(this->GetPVWidget("Scalars"));
  if(!array)
    {
    return;
    }

  // Send the source input to the output instead of the filter input so that the whole dataset gets written:
  vtkPVLabeledToggle *unfilteredFlag = vtkPVLabeledToggle::SafeDownCast(this->GetPVWidget("UnfilteredDataset"));
  unfilteredFlag->SetSelectedState(1);
  this->ForceNoEdit = 1;
  this->AcceptCallback();
  this->ForceNoEdit = 0;

  if(this->WriterID.ID==0)
    {
    this->WriterID = pm->NewStreamObject("vtkExodusIIWriter",stream);
    }

  vtkClientServerID dataID = this->GetPart()->GetID(0);

  stream << vtkClientServerStream::Invoke
                  << this->WriterID << "SetFileName" << this->Entry->GetValue()
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
                  << this->WriterID << "SetInput" << dataID
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
                  << this->WriterID << "SetGhostLevel" << ghostLevel
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
                  << this->WriterID << "SetEditorFlag" << editorFlag
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
                  << this->WriterID << "SetEditedVariableName" << array->GetValue()
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
                  << this->WriterID << "Write"
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
                  << this->WriterID << "GetErrorCode"
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);

  pm->DeleteStreamObject(this->WriterID, stream);
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
  this->WriterID.ID = 0;

  // turn the filter view back on
  unfilteredFlag->SetSelectedState(0);
  this->ForceNoEdit = 1;
  this->AcceptCallback();
  this->ForceNoEdit = 0;

  this->EditedFlag = 0;
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
      self->SetIsScalingFlag(0);
      break;
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
  if(this->GetEditedFlag())
    {
    if ( vtkKWMessageDialog::PopupYesNo(
          this->GetPVApplication(), this->GetPVWindow(), "UnsavedChanges",
          "Save Changes?", 
          "Would you like to save the changes you have made to the current time step in the Attribute Editor filter before continuing?", 
          vtkKWMessageDialog::QuestionIcon | vtkKWMessageDialog::RememberYes |
          vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault ))
      {
      this->SaveCallback();  
      }
    this->SetEditedFlag(0);
    }

  // This ensures the currently selected region won't be edited in the new timestep:
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProxy()->GetProperty("EditMode"));
  ivp->SetElements1(0);
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
    this->AcceptCallback();
    this->ForceEdit = 0;

    return;
    }
  else if (this->GetPVWindow()->GetInteractor()->GetKeyCode() == 't' ||
      this->GetPVWindow()->GetInteractor()->GetKeyCode() == 'T' )
    {
    vtkPVSelectWidget *select = vtkPVSelectWidget::SafeDownCast(this->GetPVWidget("PickFunction"));
    vtkPVPickBoxWidget *box = vtkPVPickBoxWidget::SafeDownCast(select->GetPVWidget("'e'dit within a box"));
    vtkPVPickSphereWidget *sphere = vtkPVPickSphereWidget::SafeDownCast(select->GetPVWidget("'e'dit within a draggable sphere"));
    if(strcmp(select->GetCurrentValue(),"'e'dit within a box") == 0)
      {
      box->GetMouseControlToggle()->ToggleSelectedState();
      box->SetMouseControlToggle();
      }
    else if(strcmp(select->GetCurrentValue(),"'e'dit within a draggable sphere") == 0)
      {
      sphere->GetMouseControlToggle()->ToggleSelectedState();
      sphere->SetMouseControlToggle();
      }

    return;
    }
}


//----------------------------------------------------------------------------
void vtkPVAttributeEditor::AcceptCallbackInternal()
{  
  int editFlag = 1;
  int inputModified = 0;

  // If this is the first time accept has been called on this source, make sure not to edit
  if(!this->GetInitialized())
    {
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
  
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProxy()->GetProperty("EditMode"));
  ivp->SetElements1(editFlag);
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
    vtkPVPickBoxWidget *box = vtkPVPickBoxWidget::SafeDownCast(select->GetPVWidget("'e'dit within a box"));
    if(box)
      {
      box->ActualPlaceWidget();
      }

    vtkPVReaderModule *mod = this->GetPVWindow()->GetCurrentPVReaderModule();
    if(mod)
      {
      this->Entry->SetValue(mod->GetFileEntry()->GetValue());
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVAttributeEditor::Select()
{
  vtkPVSource *input, *source;

  vtkPVInputMenu *filterInput = vtkPVInputMenu::SafeDownCast(this->GetPVWidget("Input"));
  input = filterInput->GetCurrentValue();
  vtkPVInputMenu *sourceInput = vtkPVInputMenu::SafeDownCast(this->GetPVWidget("Source"));
  source = sourceInput->GetCurrentValue();

  this->Superclass::Select();

  // Initialize the inputs when the source is selected in the selection window. 
  // This is kindof a hack because the state of the input menus was not being maintained
  // when returning to this source
  filterInput->SetCurrentValue(input);
  sourceInput->SetCurrentValue(source);
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
  
}
