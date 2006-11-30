/*=========================================================================

  Module:    vtkKWWizardWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWWizardWidget.h"

#include "vtkObjectFactory.h"

#include "vtkKWApplication.h"
#include "vtkKWPushButton.h"
#include "vtkKWSeparator.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabelWithLabel.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWInternationalization.h"
#include "vtkKWWizardWorkflow.h"
#include "vtkKWWizardStep.h"
#include "vtkKWIcon.h"

#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWizardWidget);
vtkCxxRevisionMacro(vtkKWWizardWidget, "1.6");

//----------------------------------------------------------------------------
vtkKWWizardWidget::vtkKWWizardWidget()
{
  this->TitleFrame              = NULL;
  this->TitleLabel              = NULL;
  this->SubTitleLabel           = NULL;
  this->TitleIconLabel          = NULL;

  this->SeparatorAfterTitleArea = NULL;

  this->LayoutFrame             = NULL;
  this->PreTextLabel            = NULL;
  this->ClientArea              = NULL;
  this->PostTextLabel           = NULL;
  this->ErrorTextLabel          = NULL;

  this->SeparatorBeforeButtons  = NULL;

  this->ButtonFrame             = NULL;
  this->BackButton              = NULL;
  this->NextButton              = NULL;
  this->FinishButton            = NULL;
  this->CancelButton            = NULL;
  this->OKButton                = NULL;

  this->WizardWorkflow          = vtkKWWizardWorkflow::New();

  this->BackButtonVisibility    = 1;
  this->NextButtonVisibility    = 1;
  this->FinishButtonVisibility  = 1;
  this->CancelButtonVisibility  = 1;
  this->OKButtonVisibility      = 1;
}

//----------------------------------------------------------------------------
vtkKWWizardWidget::~vtkKWWizardWidget()
{
  if (this->WizardWorkflow)
    {
    this->WizardWorkflow->Delete();
    this->WizardWorkflow = NULL;
    }

  if (this->TitleFrame)
    {
    this->TitleFrame->Delete();
    this->TitleFrame = NULL;
    }
  
  if (this->TitleLabel)
    {
    this->TitleLabel->Delete();
    this->TitleLabel = NULL;
    }
  
  if (this->SubTitleLabel)
    {
    this->SubTitleLabel->Delete();
    this->SubTitleLabel = NULL;
    }
  
  if (this->TitleIconLabel)
    {
    this->TitleIconLabel->Delete();
    this->TitleIconLabel = NULL;
    }

  if (this->SeparatorAfterTitleArea)
    {
    this->SeparatorAfterTitleArea->Delete();
    this->SeparatorAfterTitleArea = NULL;
    }
 
  if (this->LayoutFrame)
    {
    this->LayoutFrame->Delete();
    this->LayoutFrame = NULL;
    }
  
  if (this->PreTextLabel)
    {
    this->PreTextLabel->Delete();
    this->PreTextLabel = NULL;
    }
  
  if (this->ClientArea)
    {
    this->ClientArea->Delete();
    this->ClientArea = NULL;
    }
  
  if (this->PostTextLabel)
    {
    this->PostTextLabel->Delete();
    this->PostTextLabel = NULL;
    }

  if (this->ErrorTextLabel)
    {
    this->ErrorTextLabel->Delete();
    this->ErrorTextLabel = NULL;
    }

  if (this->SeparatorBeforeButtons)
    {
    this->SeparatorBeforeButtons->Delete();
    this->SeparatorBeforeButtons = NULL;
    }
  
  if (this->ButtonFrame)
    {
    this->ButtonFrame->Delete();
    this->ButtonFrame = NULL;
    }
  
  if (this->BackButton)
    {
    this->BackButton->Delete();
    this->BackButton = NULL;
    }
  
  if (this->NextButton)
    {
    this->NextButton->Delete();
    this->NextButton = NULL;
    }
  
  if (this->FinishButton)
    {
    this->FinishButton->Delete();
    this->FinishButton = NULL;
    }

  if (this->CancelButton)
    {
    this->CancelButton->Delete();
    this->CancelButton = NULL;
    }

  if (this->OKButton)
    {
    this->OKButton->Delete();
    this->OKButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  // -------------------------------------------------------------------
  // Workflow

  if (!this->WizardWorkflow)
    {
    this->WizardWorkflow = vtkKWWizardWorkflow::New();
    }
  if (!this->WizardWorkflow->GetApplication())
    {
    this->WizardWorkflow->SetApplication(this->GetApplication());
    }

  // -------------------------------------------------------------------
  // Title frame

  if (!this->TitleFrame)
    {
    this->TitleFrame = vtkKWFrame::New();
    }
  this->TitleFrame->SetParent(this);
  this->TitleFrame->Create();
  
  this->Script("pack %s -side top -fill x", 
               this->TitleFrame->GetWidgetName());

  // -------------------------------------------------------------------
  // Title frame: Title label

  if (!this->TitleLabel)
    {
    this->TitleLabel = vtkKWLabel::New();
    }
  this->TitleLabel->SetParent(this->TitleFrame);
  this->TitleLabel->Create();
  this->TitleLabel->SetAnchorToNorthWest();

  vtkKWTkUtilities::ChangeFontWeightToBold(this->TitleLabel);

  this->Script("grid %s -row 0 -column 0 -sticky news -padx 4 -pady 1",
               this->TitleLabel->GetWidgetName());

  this->Script("grid rowconfigure %s 0 -weight 0",
               this->TitleFrame->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 1",
               this->TitleFrame->GetWidgetName());

  // -------------------------------------------------------------------
  // Title frame: SubTitle label

  if (!this->SubTitleLabel)
    {
    this->SubTitleLabel = vtkKWLabel::New();
    }
  this->SubTitleLabel->SetParent(this->TitleFrame);
  this->SubTitleLabel->Create();
  this->SubTitleLabel->SetPadX(15);
  this->SubTitleLabel->AdjustWrapLengthToWidthOn();
  this->SubTitleLabel->SetAnchorToNorthWest();

  this->Script("grid %s -row 1 -column 0 -sticky news -padx 4 -pady 2",
               this->SubTitleLabel->GetWidgetName());

  this->Script("grid rowconfigure %s 1 -weight 1",
               this->TitleFrame->GetWidgetName());

  // -------------------------------------------------------------------
  // Title frame: Icon

  if (!this->TitleIconLabel)
    {
    this->TitleIconLabel = vtkKWLabel::New();
    }
  this->TitleIconLabel->SetParent(this->TitleFrame);
  this->TitleIconLabel->Create();
  this->TitleIconLabel->SetReliefToFlat();
  this->TitleIconLabel->SetHighlightThickness(0);
  this->TitleIconLabel->SetPadX(0);
  this->TitleIconLabel->SetPadY(0);

  this->Script("grid %s -row 0 -column 1 -sticky nsew -rowspan 2 -padx 8",
               this->TitleIconLabel->GetWidgetName());

  this->Script("grid columnconfigure %s 1 -weight 0",
               this->TitleFrame->GetWidgetName());

  // -------------------------------------------------------------------
  // Separator (between title frame and layout frame

  if (!this->SeparatorAfterTitleArea)
    {
    this->SeparatorAfterTitleArea = vtkKWSeparator::New();
    }
  this->SeparatorAfterTitleArea->SetParent(this);
  this->SeparatorAfterTitleArea->Create();

  this->Script("pack %s -side top -fill x", this->SeparatorAfterTitleArea->GetWidgetName());

  // -------------------------------------------------------------------
  //  Layout frame

  if (!this->LayoutFrame)
    {
    this->LayoutFrame = vtkKWFrame::New();
    }
  this->LayoutFrame->SetParent(this);
  this->LayoutFrame->Create();
  this->LayoutFrame->SetBorderWidth(0);

  this->Script("pack %s -side top -fill both -expand y -padx 0 -pady 0", 
               this->LayoutFrame->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 1", 
               this->LayoutFrame->GetWidgetName());

  // -------------------------------------------------------------------
  // Layout frame: Pre-text

  if (!this->PreTextLabel)
    {
    this->PreTextLabel = vtkKWLabel::New();
    }
  this->PreTextLabel->SetParent(this->LayoutFrame);
  this->PreTextLabel->Create();
  this->PreTextLabel->AdjustWrapLengthToWidthOn();
  this->PreTextLabel->SetJustificationToLeft();
  this->PreTextLabel->SetAnchorToNorthWest();

  this->Script("grid %s -row 0 -sticky nsew -padx 2 -pady 4",
               this->PreTextLabel->GetWidgetName());

  this->Script("grid rowconfigure %s 0 -weight 0",
               this->LayoutFrame->GetWidgetName());

  // -------------------------------------------------------------------
  // Layout frame: Client area

  if (!this->ClientArea)
    {
    this->ClientArea = vtkKWFrame::New();
    }
  this->ClientArea->SetParent(this->LayoutFrame);
  this->ClientArea->Create();

  this->Script("grid %s -row 1 -sticky nsew -padx 0 -pady 0",
               this->ClientArea->GetWidgetName());

  this->Script("grid rowconfigure %s 1 -weight 1",
               this->LayoutFrame->GetWidgetName());

  // -------------------------------------------------------------------
  // Layout frame: Post-text

  if (!this->PostTextLabel)
    {
    this->PostTextLabel = vtkKWLabel::New();
    }
  this->PostTextLabel->SetParent(this->LayoutFrame);
  this->PostTextLabel->Create();
  this->PostTextLabel->AdjustWrapLengthToWidthOn();
  this->PostTextLabel->SetJustificationToLeft();
  this->PostTextLabel->SetAnchorToNorthWest();
  
  this->Script("grid %s -row 2 -sticky ew -padx 2 -pady 4",
               this->PostTextLabel->GetWidgetName());

  this->Script("grid rowconfigure %s 2 -weight 0",
               this->LayoutFrame->GetWidgetName());

  // -------------------------------------------------------------------
  // Layout frame: Error-text

  if (!this->ErrorTextLabel)
    {
    this->ErrorTextLabel = vtkKWLabelWithLabel::New();
    }
  this->ErrorTextLabel->SetParent(this->LayoutFrame);
  this->ErrorTextLabel->Create();
  this->ErrorTextLabel->ExpandWidgetOn();

  this->ErrorTextLabel->GetWidget()->SetJustificationToLeft();
  this->ErrorTextLabel->GetWidget()->SetAnchorToNorthWest();
  this->ErrorTextLabel->GetWidget()->AdjustWrapLengthToWidthOn();
  
  this->Script("grid %s -row 3 -sticky ew -padx 2 -pady 2",
               this->ErrorTextLabel->GetWidgetName());

  this->Script("grid rowconfigure %s 3 -weight 0",
               this->LayoutFrame->GetWidgetName());

  // -------------------------------------------------------------------
  // Separator (between layout frame and buttons frame)

  if (!this->SeparatorBeforeButtons)
    {
    this->SeparatorBeforeButtons = vtkKWSeparator::New();
    }
  this->SeparatorBeforeButtons->SetParent(this);
  this->SeparatorBeforeButtons->Create();

  this->Script("pack %s -side top -fill x -pady 2", 
               this->SeparatorBeforeButtons->GetWidgetName());

  // -------------------------------------------------------------------
  // Button frame

  if (!this->ButtonFrame)
    {
    this->ButtonFrame = vtkKWFrame::New();
    }
  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create();
  this->ButtonFrame->SetBorderWidth(0);

  this->Script("pack %s -side top -fill x -padx 0 -pady 0", 
               this->ButtonFrame->GetWidgetName());

  // -------------------------------------------------------------------
  // Button frame: Back

  if (!this->BackButton)
    {
    this->BackButton = vtkKWPushButton::New();
    }
  this->BackButton->SetParent(this->ButtonFrame);
  vtksys_stl::string back("< ");
  back += ks_("Wizard|Button|Back");
  this->BackButton->SetText(back.c_str());
  this->BackButton->Create();
  this->BackButton->SetWidth(8);
  this->BackButton->SetCommand(
    this->WizardWorkflow, "AttemptToGoToPreviousStep");

  // -------------------------------------------------------------------
  // Button frame: Next

  if (!this->NextButton)
    {
    this->NextButton = vtkKWPushButton::New();
    }
  this->NextButton->SetParent(this->ButtonFrame);
  vtksys_stl::string next(ks_("Wizard|Button|Next"));
  next += " >";
  this->NextButton->SetText(next.c_str());
  this->NextButton->Create();
  this->NextButton->SetWidth(8);
  this->NextButton->SetCommand(
    this->WizardWorkflow, "AttemptToGoToNextStep");

  // -------------------------------------------------------------------
  // Button frame: Finish

  if (!this->FinishButton)
    {
    this->FinishButton = vtkKWPushButton::New();
    }
  this->FinishButton->SetParent(this->ButtonFrame);
  this->FinishButton->Create();
  this->FinishButton->SetWidth(8);
  this->FinishButton->SetText(ks_("Wizard|Button|Finish"));
  this->FinishButton->SetCommand(
    this->WizardWorkflow, "AttemptToGoToFinishStep");

  // -------------------------------------------------------------------
  // Button frame: Cancel

  if (!this->CancelButton)
    {
    this->CancelButton = vtkKWPushButton::New();
    }
  this->CancelButton->SetParent(this->ButtonFrame);
  this->CancelButton->SetText(ks_("Wizard|Button|Cancel"));
  this->CancelButton->Create();
  this->CancelButton->SetWidth(8);

  // -------------------------------------------------------------------
  // Button frame: OK

  if (!this->OKButton)
    {
    this->OKButton = vtkKWPushButton::New();
    }
  this->OKButton->SetParent(this->ButtonFrame);
  this->OKButton->SetText(ks_("Wizard|Button|OK"));
  this->OKButton->Create();
  this->OKButton->SetWidth(8);

  this->PackButtons();

  // The pre and post text will initially not be visible. They will pop into
  // existence if they are configured to have a value

  this->Script("grid remove %s %s",
               this->PreTextLabel->GetWidgetName(),
               this->PostTextLabel->GetWidgetName()
               );

  this->SetTitleAreaBackgroundColor(1.0, 1.0, 1.0);

  this->AddCallbackCommandObservers();
  this->Update();
}

//---------------------------------------------------------------------------
void vtkKWWizardWidget::Update()
{
  this->UpdateEnableState();

  this->PackButtons();

  vtkKWWizardStep *current_step = 
    this->WizardWorkflow->GetCurrentStep();
  vtkKWWizardStep *finish_step = 
    this->WizardWorkflow->GetFinishStep();

  vtkKWWizardStep *previous_step = NULL;
  int nb_steps_in_stack = 
    this->WizardWorkflow->GetNumberOfStepsInNavigationStack();
  if (nb_steps_in_stack >= 2) // both current step and previous step on stack
    {
    previous_step = 
      this->WizardWorkflow->GetNthStepInNavigationStack(nb_steps_in_stack - 2);
    }

  // Update title

  this->SetTitle(current_step ? current_step->GetName() : NULL);
  this->SetSubTitle(current_step ? current_step->GetDescription() : NULL);

  // Update buttons

  if (this->BackButton)
    {
    int can_go = (previous_step && 
                  previous_step != current_step);
    this->BackButton->SetEnabled(can_go ? this->GetEnabled() : 0);
    }

  if (this->NextButton)
    {
    int can_go = (current_step != finish_step);
    this->NextButton->SetEnabled(can_go ? this->GetEnabled() : 0);
    }

  if (this->FinishButton)
    {
    int can_go = (finish_step && 
                  finish_step != current_step && 
                  finish_step->InvokeCanGoToSelfCommand());
    this->FinishButton->SetEnabled(can_go ? this->GetEnabled() : 0);
    }

  if (this->OKButton)
    {
    int can_go = (finish_step && 
                  finish_step == current_step);
    this->OKButton->SetEnabled(can_go ? this->GetEnabled() : 0);
    }
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::PackButtons()
{
  this->ButtonFrame->UnpackChildren();

  vtkKWWizardStep *current_step = 
    this->WizardWorkflow->GetCurrentStep();
  vtkKWWizardStep *finish_step = 
    this->WizardWorkflow->GetFinishStep();

  if (this->CancelButtonVisibility && 
      this->CancelButton && this->CancelButton->IsCreated())
    {
    this->Script("pack %s -side right", 
                 this->CancelButton->GetWidgetName());
    }

  if (this->OKButtonVisibility && 
      current_step && current_step == finish_step &&
      this->OKButton && this->OKButton->IsCreated())
    {
    this->Script("pack %s -side right -padx 4", 
                 this->OKButton->GetWidgetName());
    }

  if (this->FinishButtonVisibility && 
      current_step && current_step != finish_step &&
      this->FinishButton && this->FinishButton->IsCreated())
    {
    this->Script("pack %s -side right -padx 4", 
                 this->FinishButton->GetWidgetName());
    }

  if (this->NextButtonVisibility && 
      this->NextButton && this->NextButton->IsCreated())
    {
    this->Script("pack %s -side right", 
                 this->NextButton->GetWidgetName());
    }
  
  if (this->BackButtonVisibility && 
      this->BackButton && this->BackButton->IsCreated())
    {
    this->Script("pack %s -side right", 
                 this->BackButton->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::SetClientAreaMinimumHeight(int arg)
{
  if (this->LayoutFrame && this->LayoutFrame->IsCreated())
    {
    this->Script("grid rowconfigure %s 1 -minsize %d",
                 this->LayoutFrame->GetWidgetName(), arg);
    }
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::AddCallbackCommandObservers()
{
  this->Superclass::AddCallbackCommandObservers();

  this->AddCallbackCommandObserver(
    this->WizardWorkflow, vtkKWWizardWorkflow::NavigationStackedChangedEvent);
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::RemoveCallbackCommandObservers()
{
  this->Superclass::RemoveCallbackCommandObservers();

  this->RemoveCallbackCommandObserver(
    this->WizardWorkflow, vtkKWWizardWorkflow::NavigationStackedChangedEvent);
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::ProcessCallbackCommandEvents(vtkObject *caller,
                                                     unsigned long event,
                                                     void *calldata)
{
  if (caller == this->WizardWorkflow)
    {
    switch (event)
      {
      case vtkKWWizardWorkflow::NavigationStackedChangedEvent:
        this->Update();
        break;
      }
    }

  this->Superclass::ProcessCallbackCommandEvents(caller, event, calldata);
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::GetTitleAreaBackgroundColor(double *r, double *g, double *b)
{
  if (this->TitleFrame)
    {
    this->TitleFrame->GetBackgroundColor(r, g, b);
    }
}

//----------------------------------------------------------------------------
double* vtkKWWizardWidget::GetTitleAreaBackgroundColor()
{
  if (this->TitleFrame)
    {
    return this->TitleFrame->GetBackgroundColor();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::SetTitleAreaBackgroundColor(double r, double g, double b)
{
  if (this->TitleFrame)
    {
    this->TitleFrame->SetBackgroundColor(r, g, b);
    }

  if (this->TitleLabel)
    {
    this->TitleLabel->SetBackgroundColor(r, g, b);
    }

  if (this->SubTitleLabel)
    {
    this->SubTitleLabel->SetBackgroundColor(r, g, b);
    }

  if (this->TitleIconLabel)
    {
    this->TitleIconLabel->SetForegroundColor(r, g, b);
    this->TitleIconLabel->SetBackgroundColor(r, g, b);
    }
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::SetBackButtonVisibility(int arg)
{
  if (this->BackButtonVisibility == arg)
    {
    return;
    }

  this->BackButtonVisibility = arg;
  this->Modified();

  this->PackButtons();
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::SetNextButtonVisibility(int arg)
{
  if (this->NextButtonVisibility == arg)
    {
    return;
    }

  this->NextButtonVisibility = arg;
  this->Modified();

  this->PackButtons();
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::SetFinishButtonVisibility(int arg)
{
  if (this->FinishButtonVisibility == arg)
    {
    return;
    }

  this->FinishButtonVisibility = arg;
  this->Modified();

  this->PackButtons();
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::SetCancelButtonVisibility(int arg)
{
  if (this->CancelButtonVisibility == arg)
    {
    return;
    }

  this->CancelButtonVisibility = arg;
  this->Modified();

  this->PackButtons();
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::SetOKButtonVisibility(int arg)
{
  if (this->OKButtonVisibility == arg)
    {
    return;
    }

  this->OKButtonVisibility = arg;
  this->Modified();

  this->PackButtons();
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::ClearPage()
{
  this->ClientArea->UnpackChildren();
  this->SetPreText(NULL);
  this->SetPostText(NULL);
  this->SetErrorText(NULL);
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::SetPreText(const char* str)
{
  if (this->PreTextLabel)
    {
    this->PreTextLabel->SetText(str);
    }

  if (this->IsCreated())
    {
    this->Script("grid %s %s",
                 ((str && *str) ? "" : "remove"),
                 this->PreTextLabel->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
char* vtkKWWizardWidget::GetPreText()
{
  return this->PreTextLabel->GetText();
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::SetPostText(const char* str)
{
  this->PostTextLabel->SetText(str);

  if (this->IsCreated())
    {
    this->Script("grid %s %s",
                 ((str && *str) ? "" : "remove"),
                 this->PostTextLabel->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
char* vtkKWWizardWidget::GetPostText()
{
  return this->PostTextLabel->GetText();
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::SetErrorText(const char* str)
{
  this->ErrorTextLabel->GetWidget()->SetText(str);
  if (str && *str)
    {
    this->ErrorTextLabel->GetLabel()->SetImageToPredefinedIcon(
      vtkKWIcon::IconWarningMini);
    }
  else
    {
    this->ErrorTextLabel->GetLabel()->SetImageToIcon(NULL);
    }
}

//----------------------------------------------------------------------------
char* vtkKWWizardWidget::GetErrorText()
{
  return this->ErrorTextLabel->GetWidget()->GetText();
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::SetTitle(const char* str)
{
  this->TitleLabel->SetText(str);
}

//----------------------------------------------------------------------------
char* vtkKWWizardWidget::GetTitle()
{
  return this->TitleLabel->GetText();
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::SetSubTitle(const char* str)
{
  this->SubTitleLabel->SetText(str);
}

//----------------------------------------------------------------------------
char* vtkKWWizardWidget::GetSubTitle()
{
  return this->SubTitleLabel->GetText();
}

//---------------------------------------------------------------------------
void vtkKWWizardWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->TitleFrame);
  this->PropagateEnableState(this->TitleLabel);
  this->PropagateEnableState(this->SubTitleLabel);

  this->PropagateEnableState(this->SeparatorAfterTitleArea);

  this->PropagateEnableState(this->LayoutFrame);
  this->PropagateEnableState(this->PreTextLabel);
  this->PropagateEnableState(this->ClientArea);
  this->PropagateEnableState(this->PostTextLabel);
  this->PropagateEnableState(this->ErrorTextLabel);

  this->PropagateEnableState(this->SeparatorBeforeButtons);

  this->PropagateEnableState(this->ButtonFrame);
  this->PropagateEnableState(this->BackButton);
  this->PropagateEnableState(this->NextButton);
  this->PropagateEnableState(this->CancelButton);
  this->PropagateEnableState(this->OKButton);
  this->PropagateEnableState(this->FinishButton);
}

//----------------------------------------------------------------------------
void vtkKWWizardWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "BackButtonVisibility: " 
     << (this->BackButtonVisibility ? "On" : "Off") << endl;

  os << indent << "NextButtonVisibility: " 
     << (this->NextButtonVisibility ? "On" : "Off") << endl;

  os << indent << "FinishButtonVisibility: " 
     << (this->FinishButtonVisibility ? "On" : "Off") << endl;

  os << indent << "CancelButtonVisibility: " 
     << (this->CancelButtonVisibility ? "On" : "Off") << endl;

  os << indent << "OKButtonVisibility: " 
     << (this->OKButtonVisibility ? "On" : "Off") << endl;
}
