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

class KWWIDGETS_EXPORT vtkKWChangeColorButton : public vtkKWWidgetWithLabel
{
public:
  static vtkKWChangeColorButton* New();
  vtkTypeRevisionMacro(vtkKWChangeColorButton,vtkKWWidgetWithLabel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

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
  // Note that the current color is passed too, as 3 RGB (double) parameters.
  virtual void SetCommand(vtkObject *obj, const char *method);

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
  // Callbacks (handle button press and release events, etc.)
  virtual void ButtonPressCallback();
  virtual void ButtonReleaseCallback();
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWChangeColorButton();
  ~vtkKWChangeColorButton();

  vtkKWLabel  *ColorButton;
  vtkKWFrame  *ButtonFrame;

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
  virtual void CreateLabel(vtkKWApplication *app);

  // Description:
  // Create the button frame
  virtual void CreateButtonFrame(vtkKWApplication *app);

private:

  int ButtonDown;
  
  vtkKWChangeColorButton(const vtkKWChangeColorButton&); // Not implemented
  void operator=(const vtkKWChangeColorButton&); // Not implemented
};

#endif

