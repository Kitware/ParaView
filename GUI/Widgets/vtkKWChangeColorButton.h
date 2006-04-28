/*=========================================================================

  Module:    vtkKWChangeColorButton.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWChangeColorButton - a button for selecting colors
// .SECTION Description
// A button that can be pressed to select a color.
// Note: As a subclass of vtkKWWidgetWithLabel, it inherits a label and methods
// to set its position and visibility. Note that the default label position 
// implemented in this class is on the left of the color label. Only a subset
// of the specific positions listed in vtkKWWidgetWithLabel is supported: on 
// Left, and on Right of the color label. 
// .SECTION See Also
// vtkKWWidgetWithLabel

#ifndef __vtkKWChangeColorButton_h
#define __vtkKWChangeColorButton_h

#include "vtkKWWidgetWithLabel.h"

class vtkKWFrame;

class KWWidgets_EXPORT vtkKWChangeColorButton : public vtkKWWidgetWithLabel
{
public:
  static vtkKWChangeColorButton* New();
  vtkTypeRevisionMacro(vtkKWChangeColorButton,vtkKWWidgetWithLabel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the current color (RGB space)
  virtual void SetColor(double c[3]) {this->SetColor(c[0], c[1], c[2]);};
  virtual void SetColor(double r, double g, double b);
  virtual double *GetColor() {return this->Color;};

  // Description:
  // Set the text that will be used on the title of the color selection dialog.
  vtkSetStringMacro(DialogTitle);
  vtkGetStringMacro(DialogTitle);

  // Description:
  // Set the command that is called when the color is changed.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - selected RGB color: double, double, double
  virtual void SetCommand(vtkObject *object, const char *method);

  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // Set the label to be outside the color button. Default is inside.
  virtual void SetLabelOutsideButton(int);
  vtkGetMacro(LabelOutsideButton, int);
  vtkBooleanMacro(LabelOutsideButton, int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Callbacks. Internal, do not use.
  virtual void ButtonPressCallback();
  virtual void ButtonReleaseCallback();
  
protected:
  vtkKWChangeColorButton();
  ~vtkKWChangeColorButton();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  vtkKWLabel  *ColorButton;
  vtkKWFrame  *ButtonFrame;

  virtual void InvokeCommand(double r, double g, double b);
  char        *Command;

  char        *DialogTitle;
  double      Color[3];
  int         LabelOutsideButton;

  // Description:
  // Add/Remove interaction bindings
  virtual void Bind();
  virtual void UnBind();

  // Description:
  // Update the color of the button given the current color, or use
  // a 'disabled' color if the object is disabled.
  virtual void UpdateColorButton();

  // Description:
  // Query user for color
  virtual void QueryUserForColor();

  // Description:
  // Pack or repack the widget
  virtual void Pack();

  // Description:
  // Create the label (override the superclass)
  virtual void CreateLabel();

  // Description:
  // Create the button frame
  virtual void CreateButtonFrame();

private:

  int ButtonDown;
  
  vtkKWChangeColorButton(const vtkKWChangeColorButton&); // Not implemented
  void operator=(const vtkKWChangeColorButton&); // Not implemented
};

#endif

