/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWChangeColorButton.h
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
  void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set/Get the current color
  void SetColor(float c[3]) {this->SetColor(c[0], c[1], c[2]);};
  void SetColor(float r, float g, float b);
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

  vtkKWWidget *Label1;
  vtkKWWidget *Label2;
  char        *Command;
  char        *Text;
  float       Color[3];
private:
  vtkKWChangeColorButton(const vtkKWChangeColorButton&); // Not implemented
  void operator=(const vtkKWChangeColorButton&); // Not implemented
};


#endif


