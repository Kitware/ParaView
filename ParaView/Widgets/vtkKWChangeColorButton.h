/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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

  // Update the enable state. This should propagate similar calls to the
  // internal widgets.
  virtual void UpdateEnableState();

  int ButtonDown;
  
private:
  vtkKWChangeColorButton(const vtkKWChangeColorButton&); // Not implemented
  void operator=(const vtkKWChangeColorButton&); // Not implemented
};

#endif

