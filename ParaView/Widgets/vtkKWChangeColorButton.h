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

#ifndef __vtkKWChangeColorButton_h
#define __vtkKWChangeColorButton_h

#include "vtkKWLabeledWidget.h"

class VTK_EXPORT vtkKWChangeColorButton : public vtkKWLabeledWidget
{
public:
  static vtkKWChangeColorButton* New();
  vtkTypeRevisionMacro(vtkKWChangeColorButton,vtkKWLabeledWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Set/Get the current color
  void SetColor(float c[3]) {this->SetColor(c[0], c[1], c[2]);};
  void SetColor(float r, float g, float b);
  virtual float *GetColor() {return this->Color;};

  // Description:
  // Set the label to be used on the button.
  vtkSetStringMacro(Text);
  vtkGetStringMacro(Text);
 
  // Description:
  // Set the text that will be used on the title of the color selection dialog.
  vtkSetStringMacro(DialogText);
  vtkGetStringMacro(DialogText);

  // Description:
  // Set the command that is called when the color is changed - the object is
  // the KWObject that will have the method called on it.  The second argument
  // is the name of the method to be called and any arguments in string form.
  // The calling is done via TCL wrappers for the KWObject.
  virtual void SetCommand(vtkKWObject* Object, const char *MethodAndArgString);

  // Description:
  // Chaining method to serialize an object and its superclasses.
  virtual void SerializeSelf(ostream& os, vtkIndent indent);
  virtual void SerializeToken(istream& is, const char *token);
  virtual void SerializeRevision(ostream& os, vtkIndent indent);

  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification(int j);

  // Description:
  // Set the label to be placed after the color button. Default is before.
  virtual void SetLabelAfterColor(int);
  vtkGetMacro(LabelAfterColor, int);
  vtkBooleanMacro(LabelAfterColor, int);

  // Description:
  // Set the label to be outside the color button. Default is inside. This option
  // has to be set before Create() is called.
  vtkSetMacro(LabelOutsideButton, int);
  vtkGetMacro(LabelOutsideButton, int);
  vtkBooleanMacro(LabelOutsideButton, int);

  // Description:
  // Query user for color
  void QueryUserForColor();

  // Description:
  // Callbacks (handle button press and release events, etc.)
  void ButtonPressCallback(int x, int y);
  void ButtonReleaseCallback(int x, int y);
  
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

  vtkKWWidget *ColorButton;
  vtkKWWidget *MainFrame;

  char        *Command;
  char        *Text;
  char        *DialogText;
  float       Color[3];
  int         LabelAfterColor;
  int         LabelOutsideButton;

  void Bind();
  void UnBind();
  void UpdateColorButton();

  // Pack or repack the widget

  virtual void Pack();

  int ButtonDown;
  
private:
  vtkKWChangeColorButton(const vtkKWChangeColorButton&); // Not implemented
  void operator=(const vtkKWChangeColorButton&); // Not implemented
};

#endif

