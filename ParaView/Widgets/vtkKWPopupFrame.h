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
// .NAME vtkKWPopupFrame - a popup frame
// .SECTION Description
// A class that provides a frame that can be collapsed as a popup button.

#ifndef __vtkKWPopupFrame_h
#define __vtkKWPopupFrame_h

#include "vtkKWWidget.h"

class vtkKWLabeledFrame;
class vtkKWPopupButton;

class VTK_EXPORT vtkKWPopupFrame : public vtkKWWidget
{
public:
  static vtkKWPopupFrame* New();
  vtkTypeRevisionMacro(vtkKWPopupFrame,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Display the frame as a popup. This has to be called before Create().
  vtkSetMacro(PopupMode, int);
  vtkGetMacro(PopupMode, int);
  vtkBooleanMacro(PopupMode, int);
  
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char* args);

  // Description:
  // Access to sub-widgets
  vtkGetObjectMacro(PopupButton, vtkKWPopupButton);
  vtkGetObjectMacro(Frame, vtkKWLabeledFrame);

protected:
  vtkKWPopupFrame();
  ~vtkKWPopupFrame();

  // GUI

  int                     PopupMode;

  vtkKWPopupButton        *PopupButton;
  vtkKWLabeledFrame       *Frame;

  // Update the enable state. This should propagate similar calls to the
  // internal widgets.
  virtual void UpdateEnableState();

private:
  vtkKWPopupFrame(const vtkKWPopupFrame&); // Not implemented
  void operator=(const vtkKWPopupFrame&); // Not Implemented
};

#endif
