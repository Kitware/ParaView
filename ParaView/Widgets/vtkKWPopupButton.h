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
// .NAME vtkKWPopupButton - a button that triggers a popup
// .SECTION Description
// The vtkKWPopupButton class creates a push button that
// will popup a window. User widgets should be inserted inside
// the PopupFrame ivar.

#ifndef __vtkKWPopupButton_h
#define __vtkKWPopupButton_h

#include "vtkKWPushButton.h"

class vtkKWPushButton;

class VTK_EXPORT vtkKWPopupButton : public vtkKWPushButton
{
public:
  static vtkKWPopupButton* New();
  vtkTypeRevisionMacro(vtkKWPopupButton, vtkKWPushButton);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Access to sub-widgets.
  // The PopupFrame widget is the place to put your own sub-widgets.
  vtkGetObjectMacro(PopupTopLevel, vtkKWWidget);
  vtkGetObjectMacro(PopupFrame, vtkKWWidget);
  vtkGetObjectMacro(PopupCloseButton, vtkKWPushButton);

  // Description:
  // Popup callbacks;
  virtual void DisplayPopupCallback();
  virtual void WithdrawPopupCallback();

  void SetPopupTitle(const char* title);
  vtkGetStringMacro(PopupTitle);

protected:
  vtkKWPopupButton();
  ~vtkKWPopupButton();

  vtkKWWidget     *PopupTopLevel;
  vtkKWWidget     *PopupFrame;
  vtkKWPushButton *PopupCloseButton;

  char* PopupTitle;

  // Update the enable state. This should propagate similar calls to the
  // internal widgets.
  virtual void UpdateEnableState();

private:
  vtkKWPopupButton(const vtkKWPopupButton&); // Not implemented
  void operator=(const vtkKWPopupButton&); // Not implemented
};

#endif

