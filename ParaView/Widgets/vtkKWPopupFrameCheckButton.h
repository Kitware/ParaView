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
// .NAME vtkKWPopupFrameCheckButton - a popup frame + checkbutton
// .SECTION Description
// A class that provides a checkbutton and a (popup) frame. In popup mode
// the checkbutton is visible on the left of the popup button that will
// display the frame. In normal mode, the checkbutton is the first item
// packed in the frame.

#ifndef __vtkKWPopupFrameCheckButton_h
#define __vtkKWPopupFrameCheckButton_h

#include "vtkKWPopupFrame.h"

class vtkKWCheckButton;

class VTK_EXPORT vtkKWPopupFrameCheckButton : public vtkKWPopupFrame
{
public:
  static vtkKWPopupFrameCheckButton* New();
  vtkTypeRevisionMacro(vtkKWPopupFrameCheckButton,vtkKWPopupFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char* args);

  // Description:
  // Automatically disable the popup button when the checkbutton is not 
  // checked.
  virtual void SetLinkPopupButtonStateToCheckButton(int);
  vtkBooleanMacro(LinkPopupButtonStateToCheckButton, int);
  vtkGetMacro(LinkPopupButtonStateToCheckButton, int);

  // Description:
  // Callbacks
  virtual void CheckButtonCallback();

  // Description:
  // Access to sub-widgets
  vtkGetObjectMacro(CheckButton, vtkKWCheckButton);

  // Description:
  // Update the GUI according to the value of the ivars
  void Update();

protected:
  vtkKWPopupFrameCheckButton();
  ~vtkKWPopupFrameCheckButton();

  // GUI

  int                     LinkPopupButtonStateToCheckButton;

  vtkKWCheckButton        *CheckButton;

  // Get the value that should be used to set the checkbutton state
  // (i.e. depending on the value this checkbutton is supposed to reflect,
  // for example, an annotation visibility).
  // This does *not* return the state of the widget.
  virtual int GetCheckButtonState() { return 0; };

  // Update the enable state. This should propagate similar calls to the
  // internal widgets.
  virtual void UpdateEnableState();

private:
  vtkKWPopupFrameCheckButton(const vtkKWPopupFrameCheckButton&); // Not implemented
  void operator=(const vtkKWPopupFrameCheckButton&); // Not Implemented
};

#endif
