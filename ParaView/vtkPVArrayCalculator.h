/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVArrayCalculator.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
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
// .NAME vtkPVArrayCalculator - A class to handle the UI for vtkArrayCalculatorInterface
// .SECTION Description


#ifndef __vtkPVArrayCalculator_h
#define __vtkPVArrayCalculator_h

#include "vtkPVSource.h"
#include "vtkKWPushButton.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenuButton.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWLabeledEntry.h"
#include "vtkArrayCalculator.h"

class VTK_EXPORT vtkPVArrayCalculator : public vtkPVSource
{
public:
  static vtkPVArrayCalculator* New();
  vtkTypeMacro(vtkPVArrayCalculator, vtkPVSource);
    
  // Description:
  // Set up the UI for this source
  void CreateProperties();

  // Description:
  // Tcl callback for the buttons in the calculator
  void UpdateFunction(const char* newSymbol);

  // Description:
  // Tcl callback for the attribute mode option menu
  void ChangeAttributeMode(const char* newMode);

  // Description:
  // Tcl callback for the entries in the scalars menu.
  void AddScalarVariable(const char* variableName, const char* arrayName,
                         int component);
  
  // Description:
  // Tcl callback for the entries in the vectors menu.
  void AddVectorVariable(const char* variableName, const char* arrayName);

  // Description:
  // Clear the function.
  void ClearFunction();
  
  // Description:
  // Save this source to a file.
  void SaveInTclScript(ofstream *file);
  
protected:
  vtkPVArrayCalculator();
  ~vtkPVArrayCalculator();
  vtkPVArrayCalculator(const vtkPVArrayCalculator&) {};
  void operator=(const vtkPVArrayCalculator&) {};

  vtkKWWidget* AttributeModeFrame;
  vtkKWLabel* AttributeModeLabel;
  vtkKWOptionMenu* AttributeModeMenu;
  
  vtkKWWidget* ArrayNameFrame;
  vtkKWLabeledEntry* ArrayNameEntry;
  
  vtkKWLabeledFrame* CalculatorFrame;
  vtkKWLabel* FunctionLabel;

  vtkKWPushButton* ButtonClear;
  vtkKWPushButton* ButtonZero;
  vtkKWPushButton* ButtonOne;
  vtkKWPushButton* ButtonTwo;
  vtkKWPushButton* ButtonThree;
  vtkKWPushButton* ButtonFour;
  vtkKWPushButton* ButtonFive;
  vtkKWPushButton* ButtonSix;
  vtkKWPushButton* ButtonSeven;
  vtkKWPushButton* ButtonEight;
  vtkKWPushButton* ButtonNine;
  vtkKWPushButton* ButtonDivide;
  vtkKWPushButton* ButtonMultiply;
  vtkKWPushButton* ButtonSubtract;
  vtkKWPushButton* ButtonAdd;
  vtkKWPushButton* ButtonDecimal;
  vtkKWPushButton* ButtonDot;
  vtkKWPushButton* ButtonSin;
  vtkKWPushButton* ButtonCos;
  vtkKWPushButton* ButtonTan;
  vtkKWPushButton* ButtonASin;
  vtkKWPushButton* ButtonACos;
  vtkKWPushButton* ButtonATan;
  vtkKWPushButton* ButtonSinh;
  vtkKWPushButton* ButtonCosh;
  vtkKWPushButton* ButtonTanh;
  vtkKWPushButton* ButtonPow;
  vtkKWPushButton* ButtonSqrt;
  vtkKWPushButton* ButtonExp;
  vtkKWPushButton* ButtonCeiling;
  vtkKWPushButton* ButtonFloor;
  vtkKWPushButton* ButtonLog;
  vtkKWPushButton* ButtonAbs;
  vtkKWPushButton* ButtonMag;
  vtkKWPushButton* ButtonNorm;
  vtkKWPushButton* ButtonLeftParenthesis;
  vtkKWPushButton* ButtonRightParenthesis;
  vtkKWMenuButton* ScalarsMenu;
  vtkKWMenuButton* VectorsMenu;
};

#endif
