#include "vtkKWMyWizardDialog.h"

#include "vtkObjectFactory.h"

#include "vtkKWApplication.h"
#include "vtkKWWizardStep.h"
#include "vtkKWWizardWidget.h"
#include "vtkKWWizardWorkflow.h"
#include "vtkKWRadioButtonSet.h"
#include "vtkKWRadioButton.h"
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkKWIcon.h"
#include "vtkKWSpinBox.h"
#include "vtkKWStateMachineInput.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMyWizardDialog );
vtkCxxRevisionMacro(vtkKWMyWizardDialog, "1.2");

//----------------------------------------------------------------------------
vtkKWMyWizardDialog::vtkKWMyWizardDialog()
{
  this->OperatorStep = NULL;
  this->OperatorRadioButtonSet = vtkKWRadioButtonSet::New();

  this->OperatorValidationSucceededForOneOperandInput = NULL;

  this->Operand1Of2Step = NULL;
  this->Operand1Of1Step = NULL;
  this->Operand1Entry = vtkKWEntry::New();

  this->Operand2Step = NULL;
  this->Operand2SpinBox = vtkKWSpinBox::New();

  this->ResultStep = NULL;
  this->ResultLabel = vtkKWLabel::New();
}

//----------------------------------------------------------------------------
vtkKWMyWizardDialog::~vtkKWMyWizardDialog()
{
  if (this->OperatorStep)
    {
    this->OperatorStep->Delete();
    this->OperatorStep = NULL;
    }

  if (this->OperatorRadioButtonSet)
    {
    this->OperatorRadioButtonSet->Delete();
    this->OperatorRadioButtonSet = NULL;
    }

  if (this->OperatorValidationSucceededForOneOperandInput)
    {
    this->OperatorValidationSucceededForOneOperandInput->Delete();
    this->OperatorValidationSucceededForOneOperandInput = NULL;
    }

  if (this->Operand1Of2Step)
    {
    this->Operand1Of2Step->Delete();
    this->Operand1Of2Step = NULL;
    }

  if (this->Operand1Of1Step)
    {
    this->Operand1Of1Step->Delete();
    this->Operand1Of1Step = NULL;
    }

  if (this->Operand1Entry)
    {
    this->Operand1Entry->Delete();
    this->Operand1Entry = NULL;
    }

  if (this->Operand2Step)
    {
    this->Operand2Step->Delete();
    this->Operand2Step = NULL;
    }

  if (this->Operand2SpinBox)
    {
    this->Operand2SpinBox->Delete();
    this->Operand2SpinBox = NULL;
    }

  if (this->ResultStep)
    {
    this->ResultStep->Delete();
    this->ResultStep = NULL;
    }

  if (this->ResultLabel)
    {
    this->ResultLabel->Delete();
    this->ResultLabel = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWMyWizardDialog::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("class already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  vtkKWWizardWorkflow *wizard_workflow = this->GetWizardWorkflow();
  vtkKWWizardWidget *wizard_widget = this->GetWizardWidget();

  wizard_widget->GetTitleIconLabel()->SetImageToPredefinedIcon(
    vtkKWIcon::IconCalculator);

  // -----------------------------------------------------------------
  // Operator step

  if (!this->OperatorStep)
    {
    this->OperatorStep = vtkKWWizardStep::New();
    }
  this->OperatorStep->SetName("Operator");
  this->OperatorStep->SetDescription("Select a mathematical operator");
  this->OperatorStep->SetShowUserInterfaceCommand(
    this, "ShowOperatorUserInterfaceCallback");
  this->OperatorStep->SetHideUserInterfaceCommand(wizard_widget, "ClearPage");
  this->OperatorStep->SetValidationCommand(this, "ValidateOperatorCallback");

  wizard_workflow->AddStep(this->OperatorStep);

  // -----------------------------------------------------------------
  // Operand 1/2 step (addition, division)

  if (!this->Operand1Of2Step)
    {
    this->Operand1Of2Step = vtkKWWizardStep::New();
    }
  this->Operand1Of2Step->SetName("Operand 1/2");
  this->Operand1Of2Step->SetDescription("Enter an operand");
  this->Operand1Of2Step->SetShowUserInterfaceCommand(
    this, "ShowOperand1UserInterfaceCallback");
  this->Operand1Of2Step->SetHideUserInterfaceCommand(
    wizard_widget, "ClearPage");
  this->Operand1Of2Step->SetValidationCommand(
    this, "ValidateOperand1Callback");

  wizard_workflow->AddNextStep(this->Operand1Of2Step);

  // -----------------------------------------------------------------
  // Operand 2/2 step (addition, division)

  if (!this->Operand2Step)
    {
    this->Operand2Step = vtkKWWizardStep::New();
    }
  this->Operand2Step->SetName("Operand 2/2");
  this->Operand2Step->SetDescription("Enter a second operand");
  this->Operand2Step->SetShowUserInterfaceCommand(
    this, "ShowOperand2UserInterfaceCallback");
  this->Operand2Step->SetHideUserInterfaceCommand(wizard_widget, "ClearPage");
  this->Operand2Step->SetValidationCommand(this, "ValidateOperand2Callback");

  wizard_workflow->AddNextStep(this->Operand2Step);

  // -----------------------------------------------------------------
  // Result step (addition, division, square root) (aka Finish step)

  if (!this->ResultStep)
    {
    this->ResultStep = vtkKWWizardStep::New();
    }
  this->ResultStep->SetName("Result");
  this->ResultStep->SetDescription("Here is the result of the operation");
  this->ResultStep->SetShowUserInterfaceCommand(
    this, "ShowResultUserInterfaceCallback");
  this->ResultStep->SetHideUserInterfaceCommand(wizard_widget, "ClearPage");
  this->ResultStep->SetCanGoToSelfCommand(this, "CanGoToResultCallback");

  wizard_workflow->AddNextStep(this->ResultStep);

  // -----------------------------------------------------------------
  // Operand 1/1 step (square root)

  if (!this->Operand1Of1Step)
    {
    this->Operand1Of1Step = vtkKWWizardStep::New();
    }
  this->Operand1Of1Step->SetName("Operand 1/1");
  this->Operand1Of1Step->SetDescription("Enter an operand");
  this->Operand1Of1Step->SetShowUserInterfaceCommand(
    this, "ShowOperand1UserInterfaceCallback");
  this->Operand1Of1Step->SetHideUserInterfaceCommand(
    wizard_widget, "ClearPage");
  this->Operand1Of1Step->SetValidationCommand(
    this, "ValidateOperand1Callback");

  wizard_workflow->AddStep(this->Operand1Of1Step);

  // Manually connect the operator step to this step

  if (!this->OperatorValidationSucceededForOneOperandInput)
    {
    this->OperatorValidationSucceededForOneOperandInput =
      vtkKWStateMachineInput::New();
    }
  this->OperatorValidationSucceededForOneOperandInput->SetName("valid 1/1");

  wizard_workflow->AddInput(
    this->OperatorValidationSucceededForOneOperandInput);
  wizard_workflow->CreateNextTransition(
    this->OperatorStep,
    this->OperatorValidationSucceededForOneOperandInput,
    this->Operand1Of1Step);
  wizard_workflow->CreateBackTransition(
    this->OperatorStep, this->Operand1Of1Step);

  // Manually connect this step to the result step

  wizard_workflow->CreateNextTransition(
    this->Operand1Of1Step,
    vtkKWWizardStep::GetValidationSucceededInput(),
    this->ResultStep);
  wizard_workflow->CreateBackTransition(
    this->Operand1Of1Step, this->ResultStep);

  // -----------------------------------------------------------------
  // Initial and finish step

  wizard_workflow->SetFinishStep(this->ResultStep);
  wizard_workflow->SetInitialStep(this->OperatorStep);
  wizard_workflow->CreateGoToTransitionsToFinishStep();
}

//----------------------------------------------------------------------------
int vtkKWMyWizardDialog::EntryChangedCallback(const char *)
{
  this->GetWizardWidget()->Update();
  return 1;
}

//----------------------------------------------------------------------------
void vtkKWMyWizardDialog::ShowOperatorUserInterfaceCallback()
{
  if (!this->OperatorRadioButtonSet)
    {
    this->OperatorRadioButtonSet = vtkKWRadioButtonSet::New();
    }

  // Create radio buttons for each of the mathematical operator we support

  if (!this->OperatorRadioButtonSet->IsCreated())
    {
    this->OperatorRadioButtonSet->SetParent(
      this->GetWizardWidget()->GetClientArea());
    this->OperatorRadioButtonSet->Create();

    vtkKWRadioButton *radiob;

    radiob = this->OperatorRadioButtonSet->AddWidget(
      vtkKWMyWizardDialog::OperatorAddition);
    radiob->SetText("Addition");
    radiob->SetCommand(this->GetWizardWidget(), "Update");

    radiob = this->OperatorRadioButtonSet->AddWidget(
      vtkKWMyWizardDialog::OperatorDivision);
    radiob->SetText("Division");
    radiob->SetCommand(this->GetWizardWidget(), "Update");

    radiob = this->OperatorRadioButtonSet->AddWidget(
      vtkKWMyWizardDialog::OperatorSquareRoot);
    radiob->SetText("Square Root");
    radiob->SetCommand(this->GetWizardWidget(), "Update");

    this->OperatorRadioButtonSet->GetWidget(
      vtkKWMyWizardDialog::OperatorAddition)->Select();
    }
  
  this->Script("pack %s -side top -expand y -fill none -anchor center", 
               this->OperatorRadioButtonSet->GetWidgetName());
}

//----------------------------------------------------------------------------
int vtkKWMyWizardDialog::GetSelectedOperator()
{
  if (this->OperatorRadioButtonSet->GetWidget(
        vtkKWMyWizardDialog::OperatorAddition)->GetSelectedState())
    {
    return vtkKWMyWizardDialog::OperatorAddition;
    }
  if (this->OperatorRadioButtonSet->GetWidget(
        vtkKWMyWizardDialog::OperatorDivision)->GetSelectedState())
    {
    return vtkKWMyWizardDialog::OperatorDivision;
    }
  if (this->OperatorRadioButtonSet->GetWidget(
        vtkKWMyWizardDialog::OperatorSquareRoot)->GetSelectedState())
    {
    return vtkKWMyWizardDialog::OperatorSquareRoot;
    }
  return vtkKWMyWizardDialog::OperatorUnknown;
}

//----------------------------------------------------------------------------
void vtkKWMyWizardDialog::ValidateOperatorCallback()
{
  vtkKWWizardWorkflow *wizard_workflow = this->GetWizardWorkflow();

  // If the operator is square root, we need to move to a specific step
  // that requires only one operand. Otherwise just validate that step
  // which will move us to the path that requires two operands.

  if (this->GetSelectedOperator() == vtkKWMyWizardDialog::OperatorSquareRoot)
    {
    wizard_workflow->PushInput(
      this->OperatorValidationSucceededForOneOperandInput);
      }
  else
    {
    wizard_workflow->PushInput(
      vtkKWWizardStep::GetValidationSucceededInput());
    }

  wizard_workflow->ProcessInputs();
}

//----------------------------------------------------------------------------
void vtkKWMyWizardDialog::ShowOperand1UserInterfaceCallback()
{
  if (!this->Operand1Entry)
    {
    this->Operand1Entry = vtkKWEntry::New();
    }
  if (!this->Operand1Entry->IsCreated())
    {
    this->Operand1Entry->SetParent(
      this->GetWizardWidget()->GetClientArea());
    this->Operand1Entry->Create();
    this->Operand1Entry->SetRestrictValueToDouble();
    this->Operand1Entry->SetCommandTriggerToAnyChange();
    this->Operand1Entry->SetCommand(this, "EntryChangedCallback");
    }
  
  this->Script("pack %s -side top -expand y -fill none -anchor center", 
               this->Operand1Entry->GetWidgetName());
}

//----------------------------------------------------------------------------
int vtkKWMyWizardDialog::IsOperand1Valid()
{
  // If the operand is empty, we are invalid!

  if (!this->Operand1Entry || 
      !this->Operand1Entry->GetValue() || !*this->Operand1Entry->GetValue())
    {
    return vtkKWMyWizardDialog::OperandIsEmpty;
    }

  // If the operand is negative and we are using square root, we are invalid!

  if (this->Operand1Entry->GetValueAsDouble() < 0.0 &&
      this->GetSelectedOperator() == vtkKWMyWizardDialog::OperatorSquareRoot)
    {
    return vtkKWMyWizardDialog::OperandIsNegative;
    }

  return vtkKWMyWizardDialog::OperandIsValid;
}

//----------------------------------------------------------------------------
void vtkKWMyWizardDialog::ValidateOperand1Callback()
{
  vtkKWWizardWorkflow *wizard_workflow = this->GetWizardWorkflow();

  // If the operand is invalid, display an error and push the input
  // that will bring us back to the same state. Otherwise move on
  // to the next step!

  int valid = this->IsOperand1Valid();
  if (valid == vtkKWMyWizardDialog::OperandIsEmpty)
    {
    this->GetWizardWidget()->SetErrorText("Empty operand!");
    wizard_workflow->PushInput(vtkKWWizardStep::GetValidationFailedInput());
    }
  else if (valid == vtkKWMyWizardDialog::OperandIsNegative)
    {
    this->GetWizardWidget()->SetErrorText("Negative operand!");
    wizard_workflow->PushInput(vtkKWWizardStep::GetValidationFailedInput());
    }
  else
    {
    wizard_workflow->PushInput(
      vtkKWWizardStep::GetValidationSucceededInput());
    }

  wizard_workflow->ProcessInputs();
}

//----------------------------------------------------------------------------
void vtkKWMyWizardDialog::ShowOperand2UserInterfaceCallback()
{
  if (!this->Operand2SpinBox)
    {
    this->Operand2SpinBox = vtkKWSpinBox::New();
    }
  if (!this->Operand2SpinBox->IsCreated())
    {
    this->Operand2SpinBox->SetParent(
      this->GetWizardWidget()->GetClientArea());
    this->Operand2SpinBox->Create();
    this->Operand2SpinBox->SetRange(-10000, 10000);
    this->Operand2SpinBox->SetRestrictValueToDouble();
    this->Operand2SpinBox->SetCommandTriggerToAnyChange();
    this->Operand2SpinBox->SetCommand(this, "EntryChangedCallback");
    }
  
  this->Script("pack %s -side top -expand y -fill none -anchor center", 
               this->Operand2SpinBox->GetWidgetName());
}

//----------------------------------------------------------------------------
int vtkKWMyWizardDialog::IsOperand2Valid()
{
  // If the operand is empty, we are invalid!

  if (!this->Operand2SpinBox)
    {
    return vtkKWMyWizardDialog::OperandIsEmpty;
    }

  // If the operand is 0 and the operator is a division, we are 
  // invalid!

  if (this->Operand2SpinBox->GetValue() == 0.0 &&
      this->GetSelectedOperator() == vtkKWMyWizardDialog::OperatorDivision)
    {
    return vtkKWMyWizardDialog::OperandIsZero;
    }

  return vtkKWMyWizardDialog::OperandIsValid;
}

//----------------------------------------------------------------------------
void vtkKWMyWizardDialog::ValidateOperand2Callback()
{
  vtkKWWizardWorkflow *wizard_workflow = this->GetWizardWorkflow();

  // If the operand is invalid, display an error and push the input
  // that will bring us back to the same state. Otherwise move on
  // to the next step!

  int valid = this->IsOperand2Valid();
  if (valid == vtkKWMyWizardDialog::OperandIsEmpty)
    {
    this->GetWizardWidget()->SetErrorText("Empty operand!");
    wizard_workflow->PushInput(vtkKWWizardStep::GetValidationFailedInput());
    }
  else if (valid == vtkKWMyWizardDialog::OperandIsZero)
    {
    this->GetWizardWidget()->SetErrorText("Can not divide by zero!");
    wizard_workflow->PushInput(vtkKWWizardStep::GetValidationFailedInput());
    }
  else
    {
    wizard_workflow->PushInput(vtkKWWizardStep::GetValidationSucceededInput());
    }
 
  wizard_workflow->ProcessInputs();
}

//----------------------------------------------------------------------------
void vtkKWMyWizardDialog::ShowResultUserInterfaceCallback()
{
  if (!this->ResultLabel)
    {
    this->ResultLabel = vtkKWLabel::New();
    }
  if (!this->ResultLabel->IsCreated())
    {
    this->ResultLabel->SetParent(
      this->GetWizardWidget()->GetClientArea());
    this->ResultLabel->Create();
    }
  
  this->Script("pack %s -side top -expand y -fill none -anchor center", 
               this->ResultLabel->GetWidgetName());
 
  double operand1 = this->Operand1Entry->GetValueAsDouble();
  double operand2;
  double result = 0.0;

  ostrstream str;

  switch (this->GetSelectedOperator())
    {
    case vtkKWMyWizardDialog::OperatorAddition:
      operand2 = this->Operand2SpinBox->GetValue();
      str << operand1 << " + " << operand2;
      result = operand1 + operand2;
      break;

    case vtkKWMyWizardDialog::OperatorDivision:
      operand2 = this->Operand2SpinBox->GetValue();
      str << operand1 << " / " << operand2;
      result = operand1 / operand2;
      break;

    case vtkKWMyWizardDialog::OperatorSquareRoot:
      str << "sqrt(" << operand1 << ")";
      result = sqrt(operand1);
      break;
    }

  str << " = " << result << ends;

  this->ResultLabel->SetText(str.str());
}

//----------------------------------------------------------------------------
int vtkKWMyWizardDialog::CanGoToResultCallback()
{
  // We can only go directly to the finish/result step if all the operands
  // needed for the current operator are valid.

  return 
    this->IsOperand1Valid() == vtkKWMyWizardDialog::OperandIsValid &&
    (this->GetSelectedOperator() == vtkKWMyWizardDialog::OperatorSquareRoot ||
     this->IsOperand2Valid() == vtkKWMyWizardDialog::OperandIsValid);
}

//---------------------------------------------------------------------------
void vtkKWMyWizardDialog::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->OperatorRadioButtonSet);
  this->PropagateEnableState(this->Operand1Entry);
  this->PropagateEnableState(this->Operand2SpinBox);
  this->PropagateEnableState(this->ResultLabel);
}
