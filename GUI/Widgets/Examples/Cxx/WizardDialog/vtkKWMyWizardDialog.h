#ifndef __vtkKWMyWizardDialog_h
#define __vtkKWMyWizardDialog_h

#include "vtkKWWizardDialog.h"

class vtkKWWizardStep;
class vtkKWRadioButtonSet;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWSpinBox;
class vtkKWStateMachineInput;

class vtkKWMyWizardDialog : public vtkKWWizardDialog
{
public:
  static vtkKWMyWizardDialog* New();
  vtkTypeRevisionMacro(vtkKWMyWizardDialog,vtkKWWizardDialog);

  // Description:
  // Operator step callbacks
  // Show the UI, validate the UI.
  virtual void ShowOperatorUserInterfaceCallback();
  virtual void ValidateOperatorCallback();

  // Description:
  // Operand 1 step callbacks
  // Show the UI, validate the UI.
  virtual void ShowOperand1UserInterfaceCallback();
  virtual void ValidateOperand1Callback();

  // Description:
  // Operand 2 step callbacks
  // Show the UI, validate the UI.
  virtual void ShowOperand2UserInterfaceCallback();
  virtual void ValidateOperand2Callback();

  // Description:
  // Result step callbacks (aka Finish step)
  // Show the UI, check if we can go directly to that step.
  virtual void ShowResultUserInterfaceCallback();
  virtual int CanGoToResultCallback();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Callbacks
  virtual int EntryChangedCallback(const char *value);

protected:
  vtkKWMyWizardDialog();
  ~vtkKWMyWizardDialog();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  //BTX
  enum 
  {
    OperatorAddition = 0,
    OperatorDivision,
    OperatorSquareRoot,
    OperatorUnknown
  };
  //ETX

  //BTX
  enum 
  {
    OperandIsValid = 0,
    OperandIsEmpty,
    OperandIsZero,
    OperandIsNegative
  };
  //ETX

  // Description:
  // Operator step
  vtkKWWizardStep *OperatorStep;
  vtkKWStateMachineInput *OperatorValidationSucceededForOneOperandInput;
  vtkKWRadioButtonSet *OperatorRadioButtonSet;
  virtual int GetSelectedOperator();

  // Description:
  // Operand 1/1 and 1/2  steps
  vtkKWWizardStep *Operand1Of1Step;
  vtkKWWizardStep *Operand1Of2Step;
  vtkKWEntry *Operand1Entry;
  virtual int IsOperand1Valid();

  // Description:
  // Operand 2 step
  vtkKWWizardStep *Operand2Step;
  vtkKWSpinBox *Operand2SpinBox;
  virtual int IsOperand2Valid();

  // Description:
  // Result step (aka Finish step)
  vtkKWWizardStep *ResultStep;
  vtkKWLabel *ResultLabel;

private:
  vtkKWMyWizardDialog(const vtkKWMyWizardDialog&);   // Not implemented.
  void operator=(const vtkKWMyWizardDialog&);  // Not implemented.
};

#endif
