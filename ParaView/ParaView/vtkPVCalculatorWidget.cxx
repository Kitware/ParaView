/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCalculatorWidget.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVCalculatorWidget.h"

#include "vtkArrayCalculator.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVPart.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVWidgetCollection.h"
#include "vtkPVWindow.h"
#include "vtkSource.h"
#include "vtkStringList.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCalculatorWidget);
vtkCxxRevisionMacro(vtkPVCalculatorWidget, "1.7.4.1");

int vtkPVCalculatorWidgetCommand(ClientData cd, Tcl_Interp *interp,
                                int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVCalculatorWidget::vtkPVCalculatorWidget()
{
  this->CommandFunction = vtkPVCalculatorWidgetCommand;
  
  this->AttributeModeFrame = vtkKWWidget::New();
  this->AttributeModeLabel = vtkKWLabel::New();
  this->AttributeModeMenu = vtkKWOptionMenu::New();
  
  this->CalculatorFrame = vtkKWLabeledFrame::New();
  this->FunctionLabel = vtkKWLabel::New();

  this->ButtonClear = vtkKWPushButton::New();
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
  this->ButtonIHAT = vtkKWPushButton::New();
  this->ButtonJHAT = vtkKWPushButton::New();
  this->ButtonKHAT = vtkKWPushButton::New();
  this->ButtonLeftParenthesis = vtkKWPushButton::New();
  this->ButtonRightParenthesis = vtkKWPushButton::New();
  this->ScalarsMenu = vtkKWMenuButton::New();
  this->VectorsMenu = vtkKWMenuButton::New();
  
  this->LastAcceptedFunction = 0;
}

//----------------------------------------------------------------------------
vtkPVCalculatorWidget::~vtkPVCalculatorWidget()
{
  this->AttributeModeLabel->Delete();
  this->AttributeModeLabel = NULL;
  this->AttributeModeMenu->Delete();
  this->AttributeModeMenu = NULL;
  this->AttributeModeFrame->Delete();
  this->AttributeModeFrame = NULL;
  
  this->FunctionLabel->Delete();
  this->FunctionLabel = NULL;
  
  this->ButtonClear->Delete();
  this->ButtonClear = NULL;
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
  this->ButtonIHAT->Delete();
  this->ButtonIHAT = NULL;
  this->ButtonJHAT->Delete();
  this->ButtonJHAT = NULL;
  this->ButtonKHAT->Delete();
  this->ButtonKHAT = NULL;
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
  
  this->SetLastAcceptedFunction(NULL);
}

//----------------------------------------------------------------------------
void vtkPVCalculatorWidget::Create(vtkKWApplication *app)
{
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);

  if (this->Application)
    {
    vtkErrorMacro("PVWidget already created");
    return;
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s -borderwidth 0 -relief flat", this->GetWidgetName());

  
  this->AttributeModeFrame->SetParent(this);
  this->AttributeModeFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->AttributeModeFrame->GetWidgetName());

  this->AttributeModeLabel->SetParent(this->AttributeModeFrame);
  this->AttributeModeLabel->Create(pvApp, "");
  this->AttributeModeLabel->SetLabel("Attribute Mode:");
  this->AttributeModeLabel->SetBalloonHelpString(
    "Select whether to operate on point or cell data");
  this->AttributeModeMenu->SetParent(this->AttributeModeFrame);
  this->AttributeModeMenu->Create(pvApp, "");
  this->AttributeModeMenu->AddEntryWithCommand("Point Data", this,
                                               "ChangeAttributeMode point");
  this->AttributeModeMenu->AddEntryWithCommand("Cell Data", this,
                                               "ChangeAttributeMode cell");
  this->AttributeModeMenu->SetCurrentEntry("Point Data");
  this->AttributeModeMenu->SetBalloonHelpString(
    "Select whether to operate on point or cell data");
  this->Script("pack %s %s -side left",
               this->AttributeModeLabel->GetWidgetName(),
               this->AttributeModeMenu->GetWidgetName());
  
  this->CalculatorFrame->SetParent(this);
  this->CalculatorFrame->ShowHideFrameOn();
  this->CalculatorFrame->Create(pvApp, 0);
  this->CalculatorFrame->SetLabel("Calculator");
  this->Script("pack %s -fill x -expand t -side top",
               this->CalculatorFrame->GetWidgetName());

  this->FunctionLabel->SetParent(this->CalculatorFrame->GetFrame());
  this->FunctionLabel->Create(pvApp, "-background white");
  this->FunctionLabel->SetLabel("");
  this->Script("grid %s -columnspan 8 -sticky ew", 
               this->FunctionLabel->GetWidgetName());
  
  this->ButtonClear->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonClear->Create(pvApp, "");
  this->ButtonClear->SetLabel("Clear");
  this->ButtonClear->SetCommand(this, "ClearFunction");
  this->ButtonLeftParenthesis->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonLeftParenthesis->Create(pvApp, "");
  this->ButtonLeftParenthesis->SetLabel("(");
  this->ButtonLeftParenthesis->SetCommand(this, "UpdateFunction (");
  this->ButtonRightParenthesis->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonRightParenthesis->Create(pvApp, "");
  this->ButtonRightParenthesis->SetLabel(")");
  this->ButtonRightParenthesis->SetCommand(this, "UpdateFunction )");
  this->ButtonIHAT->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonIHAT->Create(pvApp, "");
  this->ButtonIHAT->SetLabel("iHat");
  this->ButtonIHAT->SetCommand(this, "UpdateFunction iHat");
  this->ButtonJHAT->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonJHAT->Create(pvApp, "");
  this->ButtonJHAT->SetLabel("jhat");
  this->ButtonJHAT->SetCommand(this, "UpdateFunction jHat");
  this->ButtonKHAT->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonKHAT->Create(pvApp, "");
  this->ButtonKHAT->SetLabel("khat");
  this->ButtonKHAT->SetCommand(this, "UpdateFunction kHat");
  this->Script("grid %s %s %s %s %s %s -sticky ew",
               this->ButtonClear->GetWidgetName(),
               this->ButtonLeftParenthesis->GetWidgetName(),
               this->ButtonRightParenthesis->GetWidgetName(),
               this->ButtonIHAT->GetWidgetName(),
               this->ButtonJHAT->GetWidgetName(),
               this->ButtonKHAT->GetWidgetName());
  
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
  this->Script("grid %s %s %s %s %s %s %s -sticky ew",
               this->ButtonSin->GetWidgetName(),
               this->ButtonCos->GetWidgetName(),
               this->ButtonTan->GetWidgetName(),
               this->ButtonSeven->GetWidgetName(),
               this->ButtonEight->GetWidgetName(),
               this->ButtonNine->GetWidgetName(),
               this->ButtonDivide->GetWidgetName());
  
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
  this->Script("grid %s %s %s %s %s %s %s -sticky ew",
               this->ButtonASin->GetWidgetName(),
               this->ButtonACos->GetWidgetName(),
               this->ButtonATan->GetWidgetName(),
               this->ButtonFour->GetWidgetName(),
               this->ButtonFive->GetWidgetName(),
               this->ButtonSix->GetWidgetName(),
               this->ButtonMultiply->GetWidgetName());
  
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
  this->Script("grid %s %s %s %s %s %s %s -sticky ew",
               this->ButtonSinh->GetWidgetName(),
               this->ButtonCosh->GetWidgetName(),
               this->ButtonTanh->GetWidgetName(),
               this->ButtonOne->GetWidgetName(),
               this->ButtonTwo->GetWidgetName(),
               this->ButtonThree->GetWidgetName(),
               this->ButtonSubtract->GetWidgetName());

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
  this->Script("grid %s %s %s %s %s %s %s -sticky ew",
               this->ButtonPow->GetWidgetName(),
               this->ButtonSqrt->GetWidgetName(),
               this->ButtonExp->GetWidgetName(),
               this->ButtonLog->GetWidgetName(),
               this->ButtonZero->GetWidgetName(),
               this->ButtonDecimal->GetWidgetName(),
               this->ButtonAdd->GetWidgetName()); 
  
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
  this->Script("grid %s %s %s -sticky ew",
               this->ButtonCeiling->GetWidgetName(),
               this->ButtonFloor->GetWidgetName(),
               this->ButtonAbs->GetWidgetName());
  
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
  this->Script("grid %s %s %s -sticky ew", 
               this->ButtonDot->GetWidgetName(),
               this->ButtonMag->GetWidgetName(), 
               this->ButtonNorm->GetWidgetName());
  
  this->ScalarsMenu->SetParent(this->CalculatorFrame->GetFrame());
  this->ScalarsMenu->Create(pvApp, "");
  this->ScalarsMenu->SetButtonText("scalars");
  this->ScalarsMenu->SetBalloonHelpString("Select a scalar array to operate on");
  this->VectorsMenu->SetParent(this->CalculatorFrame->GetFrame());
  this->VectorsMenu->Create(pvApp, "");
  this->VectorsMenu->SetButtonText("vectors");
  this->VectorsMenu->SetBalloonHelpString("Select a vector array to operate on");
  this->ChangeAttributeMode("point");
  this->Script("grid %s -row 6 -column 3 -columnspan 4 -sticky news",
               this->ScalarsMenu->GetWidgetName());
  this->Script("grid %s -row 7 -column 3 -columnspan 4 -sticky news",
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

void vtkPVCalculatorWidget::UpdateFunction(const char* newSymbol)
{
  char* newFunction;
  const char* currentFunction = this->FunctionLabel->GetLabel();
  newFunction = new char[strlen(currentFunction)+strlen(newSymbol)+1];
  sprintf(newFunction, "%s%s", currentFunction, newSymbol);
  this->FunctionLabel->SetLabel(newFunction);
  delete [] newFunction;
  this->ModifiedCallback();
}

void vtkPVCalculatorWidget::ClearFunction()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->FunctionLabel->SetLabel("");

  // This should be done in accept.
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("Missing PVSource.");
    return;
    }
  int num, idx;
  num = this->PVSource->GetNumberOfVTKSources();
  for (idx = 0; idx < num; ++idx)
    {
    pvApp->BroadcastScript("%s RemoveAllVariables",
                           this->PVSource->GetVTKSourceTclName(idx));
    }
  
  this->ModifiedCallback();
}

void vtkPVCalculatorWidget::ChangeAttributeMode(const char* newMode)
{
  vtkPVDataSetAttributesInformation* fdi = NULL;
  int i, j;
  int numComponents;
  char menuCommand[256];
  char menuEntry[256];
  char* name;
  vtkPVApplication *pvApp = this->GetPVApplication();
    
  this->ScalarsMenu->GetMenu()->DeleteAllMenuItems();
  this->VectorsMenu->GetMenu()->DeleteAllMenuItems();
  this->FunctionLabel->SetLabel("");

  int num, idx;
  num = this->PVSource->GetNumberOfVTKSources();
  for (idx = 0; idx < num; ++idx)
    {
    pvApp->BroadcastScript("%s RemoveAllVariables",
                           this->PVSource->GetVTKSourceTclName(idx));
    if (strcmp(newMode, "point") == 0)
      {
      pvApp->BroadcastScript("%s SetAttributeModeToUsePointData",
                             this->PVSource->GetVTKSourceTclName());
      }
    else if (strcmp(newMode, "cell") == 0)
      {
      pvApp->BroadcastScript("%s SetAttributeModeToUseCellData",
                             this->PVSource->GetVTKSourceTclName());
      }
    }

  // Populate the scalar and array menu using collected data information.
  if (strcmp(newMode, "point") == 0)
    {
    fdi = this->PVSource->GetPVInput(0)->GetDataInformation()->GetPointDataInformation();
    }
  else if (strcmp(newMode, "cell") == 0)
    {
    fdi = this->PVSource->GetPVInput(0)->GetDataInformation()->GetCellDataInformation();
    }
  
  if (fdi)
    {
    for (i = 0; i < fdi->GetNumberOfArrays(); i++)
      {
      numComponents = fdi->GetArrayInformation(i)->GetNumberOfComponents();
      name = fdi->GetArrayInformation(i)->GetName();
      for (j = 0; j < numComponents; j++)
        {
        if (numComponents == 1)
          {
          sprintf(menuCommand, "AddScalarVariable {%s} {%s} 0", name, name);
          this->ScalarsMenu->GetMenu()->AddCommand(name, this,
                                                   menuCommand);
          }
        else
          {
          sprintf(menuEntry, "%s_%d", name, j);
          sprintf(menuCommand, "AddScalarVariable {%s} {%s} {%d}", menuEntry,
                  name, j);
          this->ScalarsMenu->GetMenu()->AddCommand(menuEntry, this, menuCommand);
          }
        }
      if (numComponents == 3)
        {
        sprintf(menuCommand, "AddVectorVariable {%s} {%s}",
                name, name);
        this->VectorsMenu->GetMenu()->AddCommand(name, this,
                                                 menuCommand);
        }
      }
    }

  this->ModifiedCallback();
}

void vtkPVCalculatorWidget::AddScalarVariable(const char* variableName,
                                              const char* arrayName,
                                              int component)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->UpdateFunction(variableName);

  // This should be in accept.
  int num, idx;
  num = this->PVSource->GetNumberOfVTKSources();
  for (idx = 0; idx < num; ++idx)
    {
    pvApp->BroadcastScript("%s AddScalarVariable {%s} {%s} {%d}",
                           this->PVSource->GetVTKSourceTclName(idx),
                           variableName, arrayName, component);
    }
  
  this->AddTraceEntry("$kw(%s) AddScalarVariable {%s} {%s} {%d}",
                       this->GetTclName(), variableName, arrayName, component);
}

void vtkPVCalculatorWidget::AddVectorVariable(const char* variableName,
                                             const char* arrayName)
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  this->UpdateFunction(variableName);


  // This should be in accept.
  int num, idx;
  num = this->PVSource->GetNumberOfVTKSources();
  for (idx = 0; idx < num; ++idx)
    {
    pvApp->BroadcastScript("%s AddVectorVariable {%s} {%s} 0 1 2",
                           this->PVSource->GetVTKSourceTclName(idx),
                           variableName, arrayName);
    }

  this->AddTraceEntry("$kw(%s) AddVectorVariable {%s} {%s}",
                       this->GetTclName(), variableName, arrayName);
}


//---------------------------------------------------------------------------
void vtkPVCalculatorWidget::Trace(ofstream *file)
{
  int num, idx;
  vtkArrayCalculator *calc = (vtkArrayCalculator*)(this->PVSource->GetVTKSource(0));
  char* variableName;
  char* arrayName;
  int   component;

  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  num = calc->GetNumberOfScalarArrays();
  for (idx = 0; idx < num; ++ idx)
    {
    variableName = calc->GetScalarVariableName(idx);
    arrayName = calc->GetScalarArrayName(idx);
    component = calc->GetSelectedScalarComponent(idx);
    *file << "$kw(" << this->GetTclName() << ") AddScalarVariable {"
          << variableName << "} {" << arrayName << "} " << component << endl;
    }

  num = calc->GetNumberOfVectorArrays();
  for (idx = 0; idx < num; ++ idx)
    {
    variableName = calc->GetVectorVariableName(idx);
    arrayName = calc->GetVectorArrayName(idx);
    *file << "$kw(" << this->GetTclName() << ") AddVectorVariable {"
          << variableName << "} {" << arrayName << "}" << endl;
    }

  *file << "$kw(" << this->GetTclName() << ") SetFunctionLabel {"
        << this->FunctionLabel->GetLabel() << "}" << endl;
}


//----------------------------------------------------------------------------
void vtkPVCalculatorWidget::AcceptInternal(const char* vtkSourceTclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  // Format a command to move value from widget to vtkObjects (on all
  // processes).  The VTK objects do not yet have to have the same Tcl
  // name!
  pvApp->BroadcastScript("%s SetFunction {%s}", vtkSourceTclName,
                         this->FunctionLabel->GetLabel());
  
  this->SetLastAcceptedFunction(this->FunctionLabel->GetLabel());
  
  this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVCalculatorWidget::ResetInternal()
{
  if ( this->FunctionLabel->IsCreated() )
    {
    this->FunctionLabel->SetLabel(this->LastAcceptedFunction);
    }
  
  this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVCalculatorWidget::SetFunctionLabel(char *function)
{
  this->ModifiedCallback();
  this->FunctionLabel->SetLabel(function);
}


//----------------------------------------------------------------------------
void vtkPVCalculatorWidget::SaveInBatchScript(ofstream *file)
{
  int i;
  int numSources, sourceIdx;

  numSources = this->PVSource->GetNumberOfVTKSources();
  for (sourceIdx = 0; sourceIdx < numSources; ++sourceIdx)
    {
    vtkArrayCalculator *calc = (vtkArrayCalculator*)(this->PVSource->GetVTKSource(sourceIdx));

    // Detect special sources we do not handle yet.
    if (calc == NULL)
      {
      return;
      }

    // This suff is what should be in PVWidgets.
    *file << "\t" << this->PVSource->GetVTKSourceTclName(sourceIdx) 
          << " SetAttributeModeToUse";
    if (strcmp(this->AttributeModeMenu->GetValue(), "Point Data") == 0)
      {
      *file << "PointData\n\t";
      }
    else
      {
      *file << "CellData\n\t";
      }
  
    for (i = 0; i < calc->GetNumberOfScalarArrays(); i++)
      {
      *file << this->PVSource->GetVTKSourceTclName(sourceIdx) 
            << " AddScalarVariable {"
            << calc->GetScalarVariableName(i) << "} {"
            << calc->GetScalarArrayName(i) 
            << "} " << calc->GetSelectedScalarComponent(i)
            << "\n\t";
      }
    for (i = 0; i < calc->GetNumberOfVectorArrays(); i++)
      {
      *file << this->PVSource->GetVTKSourceTclName(sourceIdx) 
            << " AddVectorVariable {"
            << calc->GetVectorVariableName(i) << "} {"
            << calc->GetVectorArrayName(i)
            << "} 0 1 2\n\t";
      }  

    if ( this->FunctionLabel->IsCreated() )
      {
      *file << "\t" << this->PVSource->GetVTKSourceTclName(sourceIdx) << " SetFunction {"  
            << this->FunctionLabel->GetLabel() << "}\n";
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVCalculatorWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
