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
// .NAME vtkKWCheckButtonWithChangeColor - a check button and color change button
// .SECTION Description
// This packs a checkbutton and a color change button inside a frame

#ifndef __vtkKWCheckButtonWithChangeColor_h
#define __vtkKWCheckButtonWithChangeColor_h

#include "vtkKWWidget.h"

class vtkKWChangeColorButton;
class vtkKWCheckButton;

class VTK_EXPORT vtkKWCheckButtonWithChangeColor : public vtkKWWidget
{
public:
  static vtkKWCheckButtonWithChangeColor* New();
  vtkTypeRevisionMacro(vtkKWCheckButtonWithChangeColor, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Get the internal objects
  vtkGetObjectMacro(CheckButton, vtkKWCheckButton);
  vtkGetObjectMacro(ChangeColorButton, vtkKWChangeColorButton);
  
  // Description:
  // Refresh the interface given the current value of the widgets and Ivars
  virtual void Update();

  // Description:
  // Disable the color button when the checkbutton is not checked.
  // You will have to call the Update() method manually though, to reflect
  // that state.
  virtual void SetDisableChangeColorButtonWhenNotChecked(int);
  vtkBooleanMacro(DisableChangeColorButtonWhenNotChecked, int);
  vtkGetMacro(DisableChangeColorButtonWhenNotChecked, int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWCheckButtonWithChangeColor();
  ~vtkKWCheckButtonWithChangeColor();

  vtkKWCheckButton       *CheckButton;
  vtkKWChangeColorButton *ChangeColorButton;

  int DisableChangeColorButtonWhenNotChecked;

  // Pack or repack the widget

  virtual void Pack();

private:
  vtkKWCheckButtonWithChangeColor(const vtkKWCheckButtonWithChangeColor&); // Not implemented
  void operator=(const vtkKWCheckButtonWithChangeColor&); // Not implemented
};

#endif

