/*=========================================================================

  Program:   ParaView
  Module:    vtkPVArrayCalculator.h
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
