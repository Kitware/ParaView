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
// .NAME vtkKWLabeledFrame - a frame with a grooved border and a label
// .SECTION Description
// The LabeledFrame creates a frame with a grooved border, and a label
// embedded in the upper left corner of the grooved border.


#ifndef __vtkKWLabeledFrame_h
#define __vtkKWLabeledFrame_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWFrame;
class vtkKWIcon;
class vtkKWLabel;

#define VTK_KW_LABEL_CASE_USER_SPECIFIED 0
#define VTK_KW_LABEL_CASE_UPPERCASE_FIRST 1
#define VTK_KW_LABEL_CASE_LOWERCASE_FIRST 2

class VTK_EXPORT vtkKWLabeledFrame : public vtkKWWidget
{
public:
  static vtkKWLabeledFrame* New();
  vtkTypeRevisionMacro(vtkKWLabeledFrame,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char* args);

  // Description:
  // Set the label for the frame.
  void SetLabel(const char *);
  
  // Description:
  // Ask the frame to readjust its tops margin according to the content of
  // the LabelFrame. This method if bound to a <Configure> event, so
  // the widget should adjust itself automatically most of the time.
  void AdjustMargin();
 
  // Description:
  // Get the internal frame.
  vtkGetObjectMacro(Frame, vtkKWFrame);

  // Description:
  // Get the internal frame containing the label.
  vtkGetObjectMacro(LabelFrame, vtkKWFrame);

  // Description:
  // Get the label.
  vtkGetObjectMacro(Label, vtkKWLabel);

  // Description:
  // Show or hide the frame.
  void PerformShowHideFrame();

  // Description:
  // Globally enable or disable show/hide frame.
  // By default it is globally disabled.
  static void AllowShowHideOn();
  static void AllowShowHideOff();

  // Description:
  // Set/Get ShowHide for this object.
  vtkSetMacro(ShowHideFrame, int);
  vtkBooleanMacro(ShowHideFrame, int);
  vtkGetMacro(ShowHideFrame, int);

  // Description:
  // Globally override the case of the label to ensure GUI consistency.
  // This will change the label when SetLabel() is called.
  static void SetLabelCase(int v);
  static int GetLabelCase();
  static void SetLabelCaseToUserSpecified() 
    { vtkKWLabeledFrame::SetLabelCase(VTK_KW_LABEL_CASE_USER_SPECIFIED);};
  static void SetLabelCaseToUppercaseFirst() 
    {vtkKWLabeledFrame::SetLabelCase(VTK_KW_LABEL_CASE_UPPERCASE_FIRST);};
  static void SetLabelCaseToLowercaseFirst() 
    {vtkKWLabeledFrame::SetLabelCase(VTK_KW_LABEL_CASE_LOWERCASE_FIRST);};

  // Description:
  // Globally enable or disable bold label.
  // By default it is globally disabled.
  static void BoldLabelOn();
  static void BoldLabelOff();

protected:

  vtkKWLabeledFrame();
  ~vtkKWLabeledFrame();

  vtkKWFrame *Frame;
  vtkKWFrame *LabelFrame;
  vtkKWLabel *Label;

  vtkKWWidget *Border;
  vtkKWWidget *Border2;
  vtkKWWidget *Groove;
  vtkKWLabel  *Icon;
  vtkKWIcon   *IconData;
  int Displayed;
  static int AllowShowHide;
  static int BoldLabel;
  static int LabelCase;
  int ShowHideFrame;

  // Update the enable state. This should propagate similar calls to the
  // internal widgets.
  virtual void UpdateEnableState();

private:
  vtkKWLabeledFrame(const vtkKWLabeledFrame&); // Not implemented
  void operator=(const vtkKWLabeledFrame&); // Not implemented
};

#endif

