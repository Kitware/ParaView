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
#include "vtkPVDisplayGUI.h"
//#include "vtkSMPointLabelDisplay.h"
#include "vtkPVApplication.h"
#include "vtkSMPart.h"
#include "vtkSMSourceProxy.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSourceNotebook.h"
#include "vtkPVWindow.h"
#include "vtkPVFileEntry.h"
#include "vtkPVSelectWidget.h"
#include "vtkCallbackCommand.h"
#include "vtkPVSourceNotebook.h"


#include "vtkArrayMap.txx"

#include "vtkKWPushButton.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWScale.h"
#include "vtkKWPopupButton.h"
#include "vtkKWEvent.h"

#include "vtkObjectFactory.h"
//#include "vtkPVAnimationInterfaceEntry.h"
//#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVReaderModule.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkStringList.h"
#include "vtkCommand.h"

#include "vtkSMStringListDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkPVScale.h"
//#include "vtkSMPartDisplay.h"
#include "vtkKWMessageDialog.h"
#include "vtkPVLabeledToggle.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVFieldMenu.h"
#include "vtkPVInputMenu.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVColorMap.h"
#include "vtkClientServerID.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVPointWidget.h"
#include "vtkPVBoxWidget.h"
#include "vtkPVWidgetCollection.h"

#include "vtkSMPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVAdvancedReaderModule.h"
//#include "vtkPVPointAttributeEditor.h"
#include "vtkPVPick.h"
//#include <kwsys/SystemTools.hxx>

#include "vtkPVRenderView.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include <vtkstd/string>

#include "vtkPVFileEntry.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerID.h"
#include "vtkPVLabeledToggle.h"
//#include "vtkPVArraySelection.h"
//#include "vtkPVScale.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
//#include "vtkKWEvent.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVPickSphereWidget.h"
#include "vtkPVPickBoxWidget.h"
#include "vtkKWFrameWithScrollbar.h"
#include "vtkSMDisplayProxy.h"
#include "vtkKWCheckButton.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVAnimationScene.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVAttributeEditor);
vtkCxxRevisionMacro(vtkPVAttributeEditor, "1.1");


//----------------------------------------------------------------------------
vtkPVAttributeEditor::vtkPVAttributeEditor()
{
  this->Frame = vtkKWFrame::New();
  this->DataFrame = vtkKWFrame::New();
  this->LabelWidget = vtkKWLabel::New();
  this->Entry = vtkKWEntry::New();
  this->BrowseButton = vtkKWPushButton::New();
  this->SaveButton = vtkKWPushButton::New();

  this->WriterID.ID = 0;

  this->ForceEdit = 0;
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
  this->LabelWidget->Delete();
  this->LabelWidget = NULL;
  this->Frame->Delete();
  this->Frame = NULL;

  this->EventCallbackCommand->SetClientData(0);
  this->EventCallbackCommand->SetCallback(0);
  this->EventCallbackCommand->Delete();
  this->EventCallbackCommand = 0;
}

//----------------------------------------------------------------------------
void vtkPVAttributeEditor::SetVisibilityNoTrace(int val)
{
  this->Superclass::SetVisibilityNoTrace(val);
}


//----------------------------------------------------------------------------
void vtkPVAttributeEditor::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  // Call the superclass to create the widget and set the appropriate flags
  this->Superclass::CreateProperties();

  vtkPVReaderModule *mod = this->GetPVWindow()->GetCurrentPVReaderModule();
  if(strcmp(mod->GetModuleName(),"ExodusReader")!=0)
    {
    return;
    }

  this->Frame->SetParent(this->ParameterFrame->GetFrame());
  this->Frame->Create(pvApp);

  this->LabelWidget->SetParent(this->Frame);
  this->Entry->SetParent(this->Frame);
  this->BrowseButton->SetParent(this->Frame);
  this->SaveButton->SetParent(this->Frame);
  
  // Now a label
  this->LabelWidget->Create(pvApp);
  this->LabelWidget->SetText("Filename");
  this->LabelWidget->SetJustificationToRight();
  this->LabelWidget->SetWidth(18);
  this->Script("pack %s -side left", this->LabelWidget->GetWidgetName());
  
  // Now the entry
  this->Entry->Create(pvApp);
  this->Entry->SetValue(this->GetPVWindow()->GetCurrentPVReaderModule()->GetFileEntry()->GetValue());
//  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
//               this->Entry->GetWidgetName(), this->GetTclName());
//  this->Entry->BindCommand(this, "EntryChangedCallback");
  // Change the order of the bindings so that the
  // modified command gets called after the entry changes.
//  this->Script("bindtags %s [concat Entry [lreplace [bindtags %s] 1 1]]", 
//               this->Entry->GetWidgetName(), this->Entry->GetWidgetName());
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
//  this->SaveButton->SetStateOption(0);

  if (this->BalloonHelpString)
    {
    this->SetBalloonHelpString(this->BalloonHelpString);
    }
  this->Script("pack %s -side left", this->BrowseButton->GetWidgetName());
  this->Script("pack %s -side left", this->SaveButton->GetWidgetName());
//  this->Script("pack %s -fill both -expand 1", frame->GetWidgetName());
  this->Script("pack %s -pady 10 -side top -fill x -expand t", 
                this->Frame->GetWidgetName());


  this->DataFrame->SetParent(this->ParameterFrame->GetFrame());
  this->DataFrame->Create(pvApp);
  this->Script("pack %s",
               this->DataFrame->GetWidgetName());

  vtkPVSelectWidget *select = vtkPVSelectWidget::SafeDownCast(this->GetPVWidget("PickFunction"));
  select->SetModifiedCommand(this->GetTclName(),"PickMethodObserver");

  this->GetNotebook()->SetAutoAccept(0);

  // listen for the following events

  this->GetPVWindow()->GetInteractor()->AddObserver(vtkCommand::CharEvent, this->EventCallbackCommand, 1);
  this->GetPVWindow()->GetInteractor()->AddObserver(vtkCommand::RightButtonPressEvent, this->EventCallbackCommand, 1);
  this->GetPVWindow()->GetInteractor()->AddObserver(vtkCommand::RightButtonReleaseEvent, this->EventCallbackCommand, 1);
  this->GetPVWindow()->GetInteractor()->AddObserver(vtkCommand::LeftButtonPressEvent, this->EventCallbackCommand, 1);
  this->GetPVWindow()->GetInteractor()->AddObserver(vtkCommand::LeftButtonReleaseEvent, this->EventCallbackCommand, 1);
  this->GetPVWindow()->GetAnimationManager()->GetAnimationScene()->AddObserver(vtkKWEvent::TimeChangedEvent,this->EventCallbackCommand, 1);
  this->GetPVWindow()->GetCurrentPVReaderModule()->GetTimeStepWidget()->AddObserver(vtkKWEvent::TimeChangedEvent,this->EventCallbackCommand, 1);

}

//----------------------------------------------------------------------------
void vtkPVAttributeEditor::BrowseCallback()
{

  ostrstream str;
  vtkKWLoadSaveDialog* saveDialog = this->GetPVApplication()->NewLoadSaveDialog();
  const char* fname = this->Entry->GetValue();

  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVWindow* win = 0;
  if (pvApp)
    {
    win = pvApp->GetMainWindow();
    }

    saveDialog->SetLastPath(fname);

  saveDialog->Create(this->GetPVApplication());
  if (win) 
    { 
    saveDialog->SetParent(this); 
    }
  saveDialog->SaveDialogOn();
  saveDialog->SetTitle("Select File");
  if(this->GetPVWindow()->GetCurrentPVReaderModule()->GetFileEntry()->GetExtension())
    {
    saveDialog->SetDefaultExtension(this->GetPVWindow()->GetCurrentPVReaderModule()->GetFileEntry()->GetExtension());
    str << "{{} {." << this->GetPVWindow()->GetCurrentPVReaderModule()->GetFileEntry()->GetExtension() << "}} ";
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
  char *srcName = new char[250];

  vtkPVLabeledToggle *unfilteredFlag = vtkPVLabeledToggle::SafeDownCast(this->GetPVWidget("UnfilteredDataset"));
  unfilteredFlag->SetSelectedState(1);
  this->AcceptCallback();
  
  sprintf(srcName,"%s",this->Entry->GetValue());

  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkClientServerStream stream;

  vtkPVArrayMenu *array = vtkPVArrayMenu::SafeDownCast(this->GetPVWidget("Scalars"));

  int ghostLevel = 1;

  if(this->WriterID.ID==0)
    {
    this->WriterID = pm->NewStreamObject("vtkExodusIIWriter",stream);
    }

//  vtkClientServerID dataID = this->GetPart(1)->GetID(0);
  vtkClientServerID dataID = this->GetPart()->GetID(0);
  stream << vtkClientServerStream::Invoke
                  << this->WriterID << "SetFileName" << srcName
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
                  << this->WriterID << "SetInput" << dataID
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
                  << this->WriterID << "SetGhostLevel" << ghostLevel
                  << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
                  << this->WriterID << "SetEditorFlag" << 1
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

  unfilteredFlag->SetSelectedState(0);
  this->AcceptCallback();

  this->EditedFlag = 0;

  delete [] srcName;

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
      if(self->GetEditedFlag())
        {
        if ( vtkKWMessageDialog::PopupYesNo(
              self->GetPVApplication(), self->GetPVWindow(), "UnsavedChanges",
              "Save Changes?", 
              "Would you like to save the changes you have made to the current time step in the Attribute Editor filter before continuing?", 
              vtkKWMessageDialog::QuestionIcon | vtkKWMessageDialog::RememberYes |
              vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault ))
          {
          self->SaveCallback();  
          }
        self->SetEditedFlag(0);
        }

      vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
        self->GetProxy()->GetProperty("EditMode"));
      ivp->SetElements1(0);
      self->GetProxy()->UpdateVTKObjects();

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
void vtkPVAttributeEditor::OnChar()
{
  if (this->GetPVWindow()->GetInteractor()->GetKeyCode() == 'e' ||
      this->GetPVWindow()->GetInteractor()->GetKeyCode() == 'E' )
    {
    this->Notebook->SetAcceptButtonColorToModified();
    this->ForceEdit = 1;
    this->AcceptCallback();
    this->ForceEdit = 0;

    return;
    }
  else if (this->GetPVWindow()->GetInteractor()->GetKeyCode() == 't' ||
      this->GetPVWindow()->GetInteractor()->GetKeyCode() == 'T' )
    {
    // should be a child of current reader module
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
void vtkPVAttributeEditor::SetPVInput(const char* vtkNotUsed(name), int vtkNotUsed(idx), vtkPVSource *pvs)
{
  this->AddPVInput(pvs);
}

//----------------------------------------------------------------------------
void vtkPVAttributeEditor::AcceptCallbackInternal()
{  
  int editFlag = 1;
  int inputModified = 0;

  if(!this->GetInitialized())
    {
    editFlag = 0;
    }
  else
    {
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

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProxy()->GetProperty("EditMode"));
  ivp->SetElements1(editFlag);
  this->GetProxy()->UpdateVTKObjects();

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

    this->Entry->SetValue(this->GetPVWindow()->GetCurrentPVReaderModule()->GetFileEntry()->GetValue());
    }
}


//----------------------------------------------------------------------------
void vtkPVAttributeEditor::Select()
{
  vtkPVSource *input, *source;

  vtkPVInputMenu *inputMenu1 = vtkPVInputMenu::SafeDownCast(this->GetPVWidget("Input"));
  input = inputMenu1->GetCurrentValue();
  vtkPVInputMenu *inputMenu2 = vtkPVInputMenu::SafeDownCast(this->GetPVWidget("Source"));
  source = inputMenu2->GetCurrentValue();

  this->Superclass::Select();

  inputMenu1->SetCurrentValue(input);
  inputMenu2->SetCurrentValue(source);
  this->AcceptCallback();
}

//----------------------------------------------------------------------------
void vtkPVAttributeEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "SetEditedFlag" << this->GetEditedFlag() << endl;
  os << indent << "SetIsScalingFlag" << this->IsScalingFlag << endl;
  os << indent << "SetIsMovingFlag" << this->IsMovingFlag << endl;
  
}
