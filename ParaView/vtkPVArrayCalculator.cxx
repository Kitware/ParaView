/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVArrayCalculator.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkPVArrayCalculator.h"
#include "vtkPVApplication.h"
#include "vtkStringList.h"

int vtkPVArrayCalculatorCommand(ClientData cd, Tcl_Interp *interp,
                                int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVArrayCalculator::vtkPVArrayCalculator()
{
  this->CommandFunction = vtkPVArrayCalculatorCommand;
  this->Calculator = NULL;
  
  this->AttributeModeFrame = vtkKWWidget::New();
  this->AttributeModeLabel = vtkKWLabel::New();
  this->AttributeModeMenu = vtkKWOptionMenu::New();
  this->ArrayNameFrame = vtkKWWidget::New();
  this->ArrayNameEntry = vtkKWLabeledEntry::New();
  
  this->CalculatorFrame = vtkKWLabeledFrame::New();
  this->FunctionEntry = vtkKWEntry::New();
  this->ButtonZero = vtkKWPushButton::New();
  this->ButtonOne = vtkKWPushButton::New();
  this->ButtonTwo = vtkKWPushButton::New();
  this->ButtonThree = vtkKWPushButton::New();
  this->ButtonFour = vtkKWPushButton::New();
  this->ButtonFive = vtkKWPushButton::New();
  this->ButtonSix = vtkKWPushButton::New();
  this->ButtonSeven = vtkKWPushButton::New();
  this->ButtonEight = vtkKWPushButton::New();
  this->ButtonNine = vtkKWPushButton::New();
  this->ButtonDivide = vtkKWPushButton::New();
  this->ButtonMultiply = vtkKWPushButton::New();
  this->ButtonSubtract = vtkKWPushButton::New();
  this->ButtonAdd = vtkKWPushButton::New();
  this->ButtonDecimal = vtkKWPushButton::New();
  this->ButtonDot = vtkKWPushButton::New();
  this->ButtonSin = vtkKWPushButton::New();
  this->ButtonCos = vtkKWPushButton::New();
  this->ButtonTan = vtkKWPushButton::New();
  this->ButtonASin = vtkKWPushButton::New();
  this->ButtonACos = vtkKWPushButton::New();
  this->ButtonATan = vtkKWPushButton::New();
  this->ButtonSinh = vtkKWPushButton::New();
  this->ButtonCosh = vtkKWPushButton::New();
  this->ButtonTanh = vtkKWPushButton::New();
  this->ButtonPow = vtkKWPushButton::New();
  this->ButtonSqrt = vtkKWPushButton::New();
  this->ButtonExp = vtkKWPushButton::New();
  this->ButtonCeiling = vtkKWPushButton::New();
  this->ButtonFloor = vtkKWPushButton::New();
  this->ButtonLog = vtkKWPushButton::New();
  this->ButtonAbs = vtkKWPushButton::New();
  this->ButtonMag = vtkKWPushButton::New();
  this->ButtonNorm = vtkKWPushButton::New();
  this->ButtonLeftParenthesis = vtkKWPushButton::New();
  this->ButtonRightParenthesis = vtkKWPushButton::New();
  this->ScalarsMenu = vtkKWMenuButton::New();
  this->VectorsMenu = vtkKWMenuButton::New();
}

//----------------------------------------------------------------------------
vtkPVArrayCalculator::~vtkPVArrayCalculator()
{
  this->AttributeModeLabel->Delete();
  this->AttributeModeLabel = NULL;
  this->AttributeModeMenu->Delete();
  this->AttributeModeMenu = NULL;
  this->AttributeModeFrame->Delete();
  this->AttributeModeFrame = NULL;
  this->ArrayNameFrame->Delete();
  this->ArrayNameFrame = NULL;
  this->ArrayNameEntry->Delete();
  this->ArrayNameEntry = NULL;
  
  this->FunctionEntry->Delete();
  this->FunctionEntry = NULL;
  this->ButtonZero->Delete();
  this->ButtonZero = NULL;
  this->ButtonOne->Delete();
  this->ButtonOne = NULL;
  this->ButtonTwo->Delete();
  this->ButtonTwo = NULL;
  this->ButtonThree->Delete();
  this->ButtonThree = NULL;
  this->ButtonFour->Delete();
  this->ButtonFour = NULL;
  this->ButtonFive->Delete();
  this->ButtonFive = NULL;
  this->ButtonSix->Delete();
  this->ButtonSix = NULL;
  this->ButtonSeven->Delete();
  this->ButtonSeven = NULL;
  this->ButtonEight->Delete();
  this->ButtonEight = NULL;
  this->ButtonNine->Delete();
  this->ButtonNine = NULL;
  this->ButtonDivide->Delete();
  this->ButtonDivide = NULL;
  this->ButtonMultiply->Delete();
  this->ButtonMultiply = NULL;
  this->ButtonSubtract->Delete();
  this->ButtonSubtract = NULL;
  this->ButtonAdd->Delete();
  this->ButtonAdd = NULL;
  this->ButtonDecimal->Delete();
  this->ButtonDecimal = NULL;
  this->ButtonDot->Delete();
  this->ButtonDot = NULL;
  this->ButtonSin->Delete();
  this->ButtonSin = NULL;
  this->ButtonCos->Delete();
  this->ButtonCos = NULL;
  this->ButtonTan->Delete();
  this->ButtonTan = NULL;
  this->ButtonASin->Delete();
  this->ButtonASin = NULL;
  this->ButtonACos->Delete();
  this->ButtonACos = NULL;
  this->ButtonATan->Delete();
  this->ButtonATan = NULL;
  this->ButtonSinh->Delete();
  this->ButtonSinh = NULL;
  this->ButtonCosh->Delete();
  this->ButtonCosh = NULL;
  this->ButtonTanh->Delete();
  this->ButtonTanh = NULL;
  this->ButtonPow->Delete();
  this->ButtonPow = NULL;
  this->ButtonSqrt->Delete();
  this->ButtonSqrt = NULL;
  this->ButtonExp->Delete();
  this->ButtonExp = NULL;
  this->ButtonCeiling->Delete();
  this->ButtonCeiling = NULL;
  this->ButtonFloor->Delete();
  this->ButtonFloor = NULL;
  this->ButtonLog->Delete();
  this->ButtonLog = NULL;
  this->ButtonAbs->Delete();
  this->ButtonAbs = NULL;
  this->ButtonMag->Delete();
  this->ButtonMag = NULL;
  this->ButtonNorm->Delete();
  this->ButtonNorm = NULL;
  this->ButtonLeftParenthesis->Delete();
  this->ButtonLeftParenthesis = NULL;
  this->ButtonRightParenthesis->Delete();
  this->ButtonRightParenthesis = NULL;
  this->ScalarsMenu->Delete();
  this->ScalarsMenu = NULL;
  this->VectorsMenu->Delete();
  this->VectorsMenu = NULL;
  this->CalculatorFrame->Delete();
  this->CalculatorFrame = NULL;
}

//----------------------------------------------------------------------------
vtkPVArrayCalculator* vtkPVArrayCalculator::New()
{
  return new vtkPVArrayCalculator();
}

//----------------------------------------------------------------------------
void vtkPVArrayCalculator::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  
  this->vtkPVSource::CreateProperties();
  
  this->Calculator = (vtkArrayCalculator*)this->GetVTKSource();
  if (!this->Calculator)
    {
    return;
    }
  
  this->AttributeModeFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->AttributeModeFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->AttributeModeFrame->GetWidgetName());
  
  this->AttributeModeLabel->SetParent(this->AttributeModeFrame);
  this->AttributeModeLabel->Create(pvApp, "");
  this->AttributeModeLabel->SetLabel("Attribute Mode:");
  this->AttributeModeMenu->SetParent(this->AttributeModeFrame);
  this->AttributeModeMenu->Create(pvApp, "");
  this->AttributeModeMenu->AddEntryWithCommand("Point Data", this,
                                               "ChangeAttributeMode point");
  this->AttributeModeMenu->AddEntryWithCommand("Cell Data", this,
                                               "ChangeAttributeMode cell");
  this->AttributeModeMenu->SetCurrentEntry("Point Data");
  this->Script("pack %s %s -side left",
               this->AttributeModeLabel->GetWidgetName(),
               this->AttributeModeMenu->GetWidgetName());
  
  this->ArrayNameFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->ArrayNameFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->ArrayNameFrame->GetWidgetName());
  
  this->ArrayNameEntry->SetParent(this->ArrayNameFrame);
  this->ArrayNameEntry->Create(pvApp);
  this->ArrayNameEntry->SetValue("resultArray");
  this->ArrayNameEntry->SetLabel("Result Array Name:");
  this->Script("pack %s -side left",
               this->ArrayNameEntry->GetWidgetName());
  
  // Command to update the UI.
  this->CancelCommands->AddString("%s SetValue [%s %s]",
                                  this->ArrayNameEntry->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  "GetResultArrayName");
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s [%s GetValue]",
                                  this->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  "SetResultArrayName",
                                  this->ArrayNameEntry->GetTclName());

  this->CalculatorFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->CalculatorFrame->Create(pvApp);
  this->CalculatorFrame->SetLabel("Calculator");
  this->Script("pack %s -fill x -expand t -side top",
               this->CalculatorFrame->GetWidgetName());

  this->FunctionEntry->SetParent(this->CalculatorFrame->GetFrame());
  this->FunctionEntry->Create(pvApp, "");
  this->FunctionEntry->SetValue("");
  this->Script("grid %s -columnspan 8 -sticky ew", this->FunctionEntry->GetWidgetName());

  
  // Command to update the UI.
  this->CancelCommands->AddString("%s SetValue [%s %s]",
                                  this->FunctionEntry->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  "GetFunction");
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s [%s GetValue]",
                                  this->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  "SetFunction",
                                  this->FunctionEntry->GetTclName());

  this->ButtonSin->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonSin->Create(pvApp, "");
  this->ButtonSin->SetLabel("sin");
  this->ButtonSin->SetCommand(this, "UpdateFunction sin");
  this->ButtonCos->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonCos->Create(pvApp, "");
  this->ButtonCos->SetLabel("cos");
  this->ButtonCos->SetCommand(this, "UpdateFunction cos");
  this->ButtonTan->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonTan->Create(pvApp, "");
  this->ButtonTan->SetLabel("tan");
  this->ButtonTan->SetCommand(this, "UpdateFunction tan");
  this->ButtonSeven->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonSeven->Create(pvApp, "");
  this->ButtonSeven->SetLabel("7");
  this->ButtonSeven->SetCommand(this, "UpdateFunction 7");
  this->ButtonEight->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonEight->Create(pvApp, "");
  this->ButtonEight->SetLabel("8");
  this->ButtonEight->SetCommand(this, "UpdateFunction 8");
  this->ButtonNine->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonNine->Create(pvApp, "");
  this->ButtonNine->SetLabel("9");
  this->ButtonNine->SetCommand(this, "UpdateFunction 9");
  this->ButtonDivide->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonDivide->Create(pvApp, "");
  this->ButtonDivide->SetLabel("/");
  this->ButtonDivide->SetCommand(this, "UpdateFunction /");
  this->ButtonLeftParenthesis->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonLeftParenthesis->Create(pvApp, "");
  this->ButtonLeftParenthesis->SetLabel("(");
  this->ButtonLeftParenthesis->SetCommand(this, "UpdateFunction (");
  this->Script("grid %s %s %s %s %s %s %s %s -sticky ew", this->ButtonSin->GetWidgetName(),
               this->ButtonCos->GetWidgetName(), this->ButtonTan->GetWidgetName(),
               this->ButtonSeven->GetWidgetName(), this->ButtonEight->GetWidgetName(),
               this->ButtonNine->GetWidgetName(), this->ButtonDivide->GetWidgetName(),
               this->ButtonLeftParenthesis->GetWidgetName());
  
  this->ButtonASin->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonASin->Create(pvApp, "");
  this->ButtonASin->SetLabel("asin");
  this->ButtonASin->SetCommand(this, "UpdateFunction asin");
  this->ButtonACos->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonACos->Create(pvApp, "");
  this->ButtonACos->SetLabel("acos");
  this->ButtonACos->SetCommand(this, "UpdateFunction acos");
  this->ButtonATan->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonATan->Create(pvApp, "");
  this->ButtonATan->SetLabel("atan");
  this->ButtonATan->SetCommand(this, "UpdateFunction atan");
  this->ButtonFour->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonFour->Create(pvApp, "");
  this->ButtonFour->SetLabel("4");
  this->ButtonFour->SetCommand(this, "UpdateFunction 4");
  this->ButtonFive->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonFive->Create(pvApp, "");
  this->ButtonFive->SetLabel("5");
  this->ButtonFive->SetCommand(this, "UpdateFunction 5");
  this->ButtonSix->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonSix->Create(pvApp, "");
  this->ButtonSix->SetLabel("6");
  this->ButtonSix->SetCommand(this, "UpdateFunction 6");
  this->ButtonMultiply->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonMultiply->Create(pvApp, "");
  this->ButtonMultiply->SetLabel("*");
  this->ButtonMultiply->SetCommand(this, "UpdateFunction *");
  this->ButtonRightParenthesis->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonRightParenthesis->Create(pvApp, "");
  this->ButtonRightParenthesis->SetLabel(")");
  this->ButtonRightParenthesis->SetCommand(this, "UpdateFunction )");
  this->Script("grid %s %s %s %s %s %s %s %s -sticky ew", this->ButtonASin->GetWidgetName(),
               this->ButtonACos->GetWidgetName(), this->ButtonATan->GetWidgetName(),
               this->ButtonFour->GetWidgetName(), this->ButtonFive->GetWidgetName(),
               this->ButtonSix->GetWidgetName(), this->ButtonMultiply->GetWidgetName(),
               this->ButtonRightParenthesis->GetWidgetName());
  
  this->ButtonSinh->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonSinh->Create(pvApp, "");
  this->ButtonSinh->SetLabel("sinh");
  this->ButtonSinh->SetCommand(this, "UpdateFunction sinh");
  this->ButtonCosh->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonCosh->Create(pvApp, "");
  this->ButtonCosh->SetLabel("cosh");
  this->ButtonCosh->SetCommand(this, "UpdateFunction cosh");
  this->ButtonTanh->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonTanh->Create(pvApp, "");
  this->ButtonTanh->SetLabel("tanh");
  this->ButtonTanh->SetCommand(this, "UpdateFunction tanh");
  this->ButtonOne->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonOne->Create(pvApp, "");
  this->ButtonOne->SetLabel("1");
  this->ButtonOne->SetCommand(this, "UpdateFunction 1");
  this->ButtonTwo->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonTwo->Create(pvApp, "");
  this->ButtonTwo->SetLabel("2");
  this->ButtonTwo->SetCommand(this, "UpdateFunction 2");
  this->ButtonThree->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonThree->Create(pvApp, "");
  this->ButtonThree->SetLabel("3");
  this->ButtonThree->SetCommand(this, "UpdateFunction 3");
  this->ButtonSubtract->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonSubtract->Create(pvApp, "");
  this->ButtonSubtract->SetLabel("-");
  this->ButtonSubtract->SetCommand(this, "UpdateFunction -");
  this->Script("grid %s %s %s %s %s %s %s -sticky ew", this->ButtonSinh->GetWidgetName(),
               this->ButtonCosh->GetWidgetName(), this->ButtonTanh->GetWidgetName(),
               this->ButtonOne->GetWidgetName(), this->ButtonTwo->GetWidgetName(),
               this->ButtonThree->GetWidgetName(), this->ButtonSubtract->GetWidgetName());

  this->ButtonPow->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonPow->Create(pvApp, "");
  this->ButtonPow->SetLabel("x^y");
  this->ButtonPow->SetCommand(this, "UpdateFunction ^");
  this->ButtonSqrt->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonSqrt->Create(pvApp, "");
  this->ButtonSqrt->SetLabel("sqrt");
  this->ButtonSqrt->SetCommand(this, "UpdateFunction sqrt");
  this->ButtonExp->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonExp->Create(pvApp, "");
  this->ButtonExp->SetLabel("e^x");
  this->ButtonExp->SetCommand(this, "UpdateFunction exp");
  this->ButtonLog->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonLog->Create(pvApp, "");
  this->ButtonLog->SetLabel("log");
  this->ButtonLog->SetCommand(this, "UpdateFunction log");
  this->ButtonZero->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonZero->Create(pvApp, "");
  this->ButtonZero->SetLabel("0");
  this->ButtonZero->SetCommand(this, "UpdateFunction 0");
  this->ButtonDecimal->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonDecimal->Create(pvApp, "");
  this->ButtonDecimal->SetLabel(".");
  this->ButtonDecimal->SetCommand(this, "UpdateFunction .");
  this->ButtonAdd->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonAdd->Create(pvApp, "");
  this->ButtonAdd->SetLabel("+");
  this->ButtonAdd->SetCommand(this, "UpdateFunction +");
  this->Script("grid %s %s %s %s %s %s %s -sticky ew", this->ButtonPow->GetWidgetName(),
               this->ButtonSqrt->GetWidgetName(), this->ButtonExp->GetWidgetName(),
               this->ButtonLog->GetWidgetName(), this->ButtonZero->GetWidgetName(),
               this->ButtonDecimal->GetWidgetName(), this->ButtonAdd->GetWidgetName()); 
  
  this->ButtonCeiling->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonCeiling->Create(pvApp, "");
  this->ButtonCeiling->SetLabel("ceil");
  this->ButtonCeiling->SetCommand(this, "UpdateFunction ceil");
  this->ButtonFloor->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonFloor->Create(pvApp, "");
  this->ButtonFloor->SetLabel("floor");
  this->ButtonFloor->SetCommand(this, "UpdateFunction floor");
  this->ButtonAbs->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonAbs->Create(pvApp, "");
  this->ButtonAbs->SetLabel("abs");
  this->ButtonAbs->SetCommand(this, "UpdateFunction abs");
  this->Script("grid %s %s %s -sticky ew", this->ButtonCeiling->GetWidgetName(),
               this->ButtonFloor->GetWidgetName(), this->ButtonAbs->GetWidgetName());
  
  this->ButtonDot->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonDot->Create(pvApp, "");
  this->ButtonDot->SetLabel("v1.v2");
  this->ButtonDot->SetCommand(this, "UpdateFunction .");
  this->ButtonMag->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonMag->Create(pvApp, "");
  this->ButtonMag->SetLabel("mag");
  this->ButtonMag->SetCommand(this, "UpdateFunction mag");
  this->ButtonNorm->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonNorm->Create(pvApp, "");
  this->ButtonNorm->SetLabel("norm");
  this->ButtonNorm->SetCommand(this, "UpdateFunction norm");
  this->Script("grid %s %s %s -sticky ew", this->ButtonDot->GetWidgetName(),
               this->ButtonMag->GetWidgetName(), this->ButtonNorm->GetWidgetName());
  
  this->ScalarsMenu->SetParent(this->CalculatorFrame->GetFrame());
  this->ScalarsMenu->Create(pvApp, "");
  this->ScalarsMenu->SetButtonText("scalars");
  this->VectorsMenu->SetParent(this->CalculatorFrame->GetFrame());
  this->VectorsMenu->Create(pvApp, "");
  this->VectorsMenu->SetButtonText("vectors");
  this->ChangeAttributeMode("point");
  this->Script("grid %s -row 5 -column 3 -columnspan 4 -sticky news",
               this->ScalarsMenu->GetWidgetName());
  this->Script("grid %s -row 6 -column 3 -columnspan 4 -sticky news",
               this->VectorsMenu->GetWidgetName());
  
  this->Script("grid columnconfigure %s 3 -minsize 40",
               this->CalculatorFrame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 4 -minsize 40",
               this->CalculatorFrame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 5 -minsize 40",
               this->CalculatorFrame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 6 -minsize 40",
               this->CalculatorFrame->GetFrame()->GetWidgetName());
}

void vtkPVArrayCalculator::UpdateFunction(const char* newSymbol)
{
  char* currentFunction = this->FunctionEntry->GetValue();
  strcat(currentFunction, newSymbol);
  this->FunctionEntry->SetValue(currentFunction);
  this->Script("%s xview %d", this->FunctionEntry->GetWidgetName(),
               strlen(this->FunctionEntry->GetValue()));
}

void vtkPVArrayCalculator::ChangeAttributeMode(const char* newMode)
{
  vtkFieldData* fd = NULL;
  int i, j;
  int numComponents;
  char menuCommand[256];
  char menuEntry[256];
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->ScalarsMenu->GetMenu()->DeleteAllMenuItems();
  this->VectorsMenu->GetMenu()->DeleteAllMenuItems();
  this->FunctionEntry->SetValue("");
  this->Calculator->RemoveAllVariables();

  pvApp->BroadcastScript("%s RemoveAllVariables",
                         this->GetVTKSourceTclName());
  
  if (strcmp(newMode, "point") == 0)
    {
    this->Calculator->SetAttributeModeToUsePointData();
    pvApp->BroadcastScript("%s SetAttributeModeToUsePointData",
                           this->GetVTKSourceTclName());
    fd = this->Calculator->GetInput()->GetPointData()->GetFieldData();
    }
  else if (strcmp(newMode, "cell") == 0)
    {
    this->Calculator->SetAttributeModeToUseCellData();
    pvApp->BroadcastScript("%s SetAttributeModeToUseCellData",
                           this->GetVTKSourceTclName());
    fd = this->Calculator->GetInput()->GetCellData()->GetFieldData();
    }
  
  if (fd)
    {
    for (i = 0; i < fd->GetNumberOfArrays(); i++)
      {
      numComponents = fd->GetArray(i)->GetNumberOfComponents();
      for (j = 0; j < numComponents; j++)
        {
        if (numComponents == 1)
          {
          sprintf(menuCommand, "AddScalarVariable %s %s 0", fd->GetArrayName(i),
                  fd->GetArrayName(i));
          this->ScalarsMenu->GetMenu()->AddCommand(fd->GetArrayName(i), this,
                                                   menuCommand);
          }
        else
          {
          sprintf(menuEntry, "%s_%d", fd->GetArrayName(i), j);
          sprintf(menuCommand, "AddScalarVariable %s %s %d", menuEntry,
                  fd->GetArrayName(i), j);
          this->ScalarsMenu->GetMenu()->AddCommand(menuEntry, this, menuCommand);
          }
        }
      if (numComponents == 3)
        {
        sprintf(menuCommand, "AddVectorVariable %s %s", fd->GetArrayName(i),
                fd->GetArrayName(i));
        this->VectorsMenu->GetMenu()->AddCommand(fd->GetArrayName(i), this,
                                                 menuCommand);
        }
      }
    }
}

void vtkPVArrayCalculator::AddScalarVariable(const char* variableName,
                                             const char* arrayName,
                                             int component)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->UpdateFunction(variableName);
  this->Calculator->AddScalarVariable(variableName, arrayName, component);
  pvApp->BroadcastScript("%s AddScalarVariable %s %s %d",
                         this->GetVTKSourceTclName(),
                         variableName, arrayName, component);
}

void vtkPVArrayCalculator::AddVectorVariable(const char* variableName,
                                             const char* arrayName)
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  this->UpdateFunction(variableName);
  this->Calculator->AddVectorVariable(variableName, arrayName);
  pvApp->BroadcastScript("%s AddVectorVariable %s %s 0 1 2",
                         this->GetVTKSourceTclName(),
                         variableName, arrayName);
}
