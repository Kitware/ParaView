/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCalculatorWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCalculatorWidget - Widget for the PVArrayCalculator.
// .SECTION Description
// I am removing the special vtkPVSource vtkPVArrayCalculator and
// using this special vtkPVWidget instead.  Unfortunately it uses
// the ivar PVSource alot.  I would like to stop using this ivar.
// To do this the widget has to maintain the state of all scalar
// and vector variables. ...


#ifndef __vtkPVCalculatorWidget_h
#define __vtkPVCalculatorWidget_h

#include "vtkPVWidget.h"

class vtkKWLabel;
class vtkKWLabeledFrame;
class vtkKWMenuButton;
class vtkKWOptionMenu;
class vtkKWPushButton;
class vtkKWWidget;
class vtkPVStringAndScalarListWidgetProperty;

class VTK_EXPORT vtkPVCalculatorWidget : public vtkPVWidget
{
public:
  static vtkPVCalculatorWidget* New();
  vtkTypeRevisionMacro(vtkPVCalculatorWidget, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  void Create(vtkKWApplication *app);

  // Description:
  // Tcl callback for the buttons in the calculator
  void UpdateFunction(const char* newSymbol);

  // Description:
  // Set the function in the function label
  void SetFunctionLabel(char *function);
  
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

  //BTX
  // Description:
  // Called when the Accept button is pressed.  It moves the widget values to the 
  // VTK calculator filter.
  virtual void AcceptInternal(vtkClientServerID);
  //ETX

  // Description:
  // This method resets the widget values from the VTK filter.
  virtual void ResetInternal();
    
  // Description:
  // Save this source to a file.  We need more than just the source tcl name.
  virtual void SaveInBatchScript(ofstream *file);

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Set/get the property to use with this widget.
  virtual void SetProperty(vtkPVWidgetProperty *prop);
  virtual vtkPVWidgetProperty* GetProperty();
  
  // Description:
  // Create the right property for use with this widget.
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();
  
protected:
  vtkPVCalculatorWidget();
  ~vtkPVCalculatorWidget();

  vtkKWWidget* AttributeModeFrame;
  vtkKWLabel* AttributeModeLabel;
  vtkKWOptionMenu* AttributeModeMenu;
  
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
  vtkKWPushButton* ButtonIHAT;
  vtkKWPushButton* ButtonJHAT;
  vtkKWPushButton* ButtonKHAT;
  vtkKWPushButton* ButtonLeftParenthesis;
  vtkKWPushButton* ButtonRightParenthesis;
  vtkKWMenuButton* ScalarsMenu;
  vtkKWMenuButton* VectorsMenu;

  char *LastAcceptedFunction;
  vtkSetStringMacro(LastAcceptedFunction);

  char **ScalarArrayNames;
  char **ScalarVariableNames;
  int *ScalarComponents;
  int NumberOfScalarVariables;
  char **VectorArrayNames;
  char **VectorVariableNames;
  int NumberOfVectorVariables;
  void ClearAllVariables();
  
  vtkPVStringAndScalarListWidgetProperty *Property;
  
private:
  vtkPVCalculatorWidget(const vtkPVCalculatorWidget&); // Not implemented
  void operator=(const vtkPVCalculatorWidget&); // Not implemented
};

#endif
