/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWChangeColorButton.h
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
// .NAME vtkKWChangeColorButton - a button for selecting colors
// .SECTION Description
// A button that can be pressed to select a color.


#ifndef __vtkKWChangeColorButton_h
#define __vtkKWChangeColorButton_h

#include "vtkKWEntry.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWChangeColorButton : public vtkKWWidget
{
public:
  static vtkKWChangeColorButton* New();
  vtkTypeMacro(vtkKWChangeColorButton,vtkKWWidget);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, char *args);

  // Description:
  // Set/Get the current color
  void SetColor(float c[3]);
  virtual float *GetColor() {return this->Color;};

  // Description:
  // Set the label to be used on the button
  vtkSetStringMacro(Text);
  vtkGetStringMacro(Text);

  
  // Description:
  // Handle button press and release events
  void AButtonPress(int x, int y);
  void AButtonRelease(int x, int y);
  
  // Description:
  // Set the command that is called when the color is changed - the object is
  // the KWObject that will have the method called on it.  The second argument
  // is the name of the method to be called and any arguments in string form.
  // The calling is done via TCL wrappers for the KWObject.
  virtual void SetCommand(vtkKWObject* Object, const char *MethodAndArgString);

  // Description:
  // Chaining method to serialize an object and its superclasses.
  virtual void SerializeSelf(ostream& os, vtkIndent indent);
  virtual void SerializeToken(istream& is,const char token[1024]);
  virtual void SerializeRevision(ostream& os, vtkIndent indent);

  void        ChangeColor();

protected:
  vtkKWChangeColorButton();
  ~vtkKWChangeColorButton();
  vtkKWChangeColorButton(const vtkKWChangeColorButton&) {};
  void operator=(const vtkKWChangeColorButton&) {};

  vtkKWWidget *Label1;
  vtkKWWidget *Label2;
  char        *Command;
  char        *Text;
  float       Color[3];
};


#endif


