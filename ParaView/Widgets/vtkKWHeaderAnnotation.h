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
// .NAME vtkKWHeaderAnnotation - a header annotation widget
// .SECTION Description
// A class that provides a UI for header annotation (vtkTextActor).

#ifndef __vtkKWHeaderAnnotation_h
#define __vtkKWHeaderAnnotation_h

#include "vtkKWPopupFrameCheckButton.h"

class vtkKWLabeledEntry;
class vtkKWLabeledPopupButton;
class vtkKWRenderWidget;
class vtkKWTextProperty;
class vtkTextActor;

class VTK_EXPORT vtkKWHeaderAnnotation : public vtkKWPopupFrameCheckButton
{
public:
  static vtkKWHeaderAnnotation* New();
  vtkTypeRevisionMacro(vtkKWHeaderAnnotation,vtkKWPopupFrameCheckButton);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Makes the text property sub-widget popup (instead of displaying the
  // whole text property UI, which can be long).
  // This has to be called before Create(). Ignored if PopupMode is true.
  vtkSetMacro(PopupTextProperty, int);
  vtkGetMacro(PopupTextProperty, int);
  vtkBooleanMacro(PopupTextProperty, int);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char* args);

  // Description:
  // Set/Get the vtkKWRenderWidget that owns the header annotation.
  virtual void SetRenderWidget(vtkKWRenderWidget*);
  vtkGetObjectMacro(RenderWidget, vtkKWRenderWidget);

  // Description:
  // Set/Get the annotation visibility
  virtual void SetVisibility(int i);
  virtual int GetVisibility();
  vtkBooleanMacro(Visibility, int);

  // Description:
  // Set/Get the event invoked when the anything in the annotation is changed.
  // Defaults to vtkKWEvent::ViewAnnotationChangedEvent
  vtkSetMacro(AnnotationChangedEvent, int);
  vtkGetMacro(AnnotationChangedEvent, int);

  // Description:
  // Callbacks
  virtual void CheckButtonCallback();
  virtual void HeaderTextCallback();
  virtual void TextPropertyCallback();

  // Description:
  // Access to sub-widgets
  virtual vtkKWCheckButton* GetHeaderVisibilityButton()
    { return this->GetCheckButton(); };

  // Description:
  // Update the GUI according to the value of the ivars
  void Update();

protected:
  vtkKWHeaderAnnotation();
  ~vtkKWHeaderAnnotation();

  int AnnotationChangedEvent;

  vtkKWRenderWidget       *RenderWidget;

  // GUI

  int                     PopupTextProperty;

  vtkKWFrame              *TextFrame;
  vtkKWLabeledEntry       *TextEntry;
  vtkKWTextProperty       *TextPropertyWidget;
  vtkKWLabeledPopupButton *TextPropertyPopupButton;

  virtual void Render();
  virtual void SetHeaderText(const char *txt);

  // Get the value that should be used to set the checkbutton state
  // (i.e. depending on the value this checkbutton is supposed to reflect,
  // for example, an annotation visibility).
  // This does *not* return the state of the widget.
  virtual int GetCheckButtonState() { return this->GetVisibility(); };

  // Update the enable state. This should propagate similar calls to the
  // internal widgets.
  virtual void UpdateEnableState();

  // Send an event representing the state of the widget
  virtual void SendChangedEvent();

private:
  vtkKWHeaderAnnotation(const vtkKWHeaderAnnotation&); // Not implemented
  void operator=(const vtkKWHeaderAnnotation&); // Not Implemented
};

#endif

