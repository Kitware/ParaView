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
// .NAME vtkKWCornerAnnotation - a corner annotation widget
// .SECTION Description
// A class that provides a UI for vtkCornerAnnotation. User can set the
// text for each corner, set the color of the text, and turn the annotation
// on and off.

#ifndef __vtkKWCornerAnnotation_h
#define __vtkKWCornerAnnotation_h

#include "vtkKWPopupFrameCheckButton.h"

class vtkCornerAnnotation;
class vtkKWFrame;
class vtkKWGenericComposite;
class vtkKWLabel;
class vtkKWLabeledPopupButton;
class vtkKWLabeledText;
class vtkKWRenderWidget;
class vtkKWScale;
class vtkKWTextProperty;
class vtkKWView;

class VTK_EXPORT vtkKWCornerAnnotation : public vtkKWPopupFrameCheckButton
{
public:
  static vtkKWCornerAnnotation* New();
  vtkTypeRevisionMacro(vtkKWCornerAnnotation,vtkKWPopupFrameCheckButton);
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
  // Set/Get the vtkKWView or the vtkKWRenderWidget that owns this annotation.
  // vtkKWView and vtkKWRenderWidget are two different frameworks, choose one
  // or the other (ParaView uses vtkKWView, VolView uses vtkKWRenderWidget).
  // Note that in vtkKWView mode, each view has a vtkKWCornerAnnotation. 
  // In vtkKWRenderWidget, each widget has a vtkCornerAnnotation, which is 
  // controlled by a unique (decoupled) vtkKWCornerAnnotation in the GUI.
  virtual void SetView(vtkKWView*);
  vtkGetObjectMacro(View,vtkKWView);
  virtual void SetRenderWidget(vtkKWRenderWidget*);
  vtkGetObjectMacro(RenderWidget,vtkKWRenderWidget);

  // Description:
  // Get the underlying vtkCornerAnnotation. 
  // In vtkKWView mode, the CornerAnnotation is created automatically and 
  // handled by this class (i.e. each vtkKWCornerAnnotation has a 
  // vtkCornerAnnotation).
  // In vtkKWRenderWidget, the corner prop is part of vtkKWRenderWidget, and
  // this method is just a gateway to vtkKWRenderWidget::GetCornerAnnotation().
  vtkGetObjectMacro(CornerAnnotation, vtkCornerAnnotation);
  
  // Description:
  // Set/Get the annotation visibility
  virtual void SetVisibility(int i);
  virtual int GetVisibility();
  vtkBooleanMacro(Visibility, int);

  // Description:
  // Set/Get corner text
  virtual void SetCornerText(const char *txt, int corner);
  virtual char *GetCornerText(int i);

  // Description:
  // Change the color of the annotation
  virtual void SetTextColor(float r, float g, float b);
  virtual void SetTextColor(float *rgb)
               { this->SetTextColor(rgb[0], rgb[1], rgb[2]); }
  virtual float *GetTextColor();

  // Description:
  // Set/Get the maximum line height.
  virtual void SetMaximumLineHeight(float);
  virtual void SetMaximumLineHeightNoTrace(float);

  // Description:
  // Set the event invoked when the anything in the annotation is changed.
  // Defaults to vtkKWEvent::ViewAnnotationChangedEvent
  vtkSetMacro(AnnotationChangedEvent, int);
  vtkGetMacro(AnnotationChangedEvent, int);

  // Description:
  // Callbacks
  virtual void CheckButtonCallback();
  virtual void CornerTextCallback(int i);
  virtual void MaximumLineHeightCallback();
  virtual void MaximumLineHeightEndCallback();
  virtual void TextPropertyCallback();

  // Description:
  // Access to sub-widgets
  virtual vtkKWCheckButton* GetCornerVisibilityButton()
    { return this->GetCheckButton(); };

  // Description:
  // Update the GUI according to the value of the ivars
  void Update();

  // Description:
  // When used with a vtkKWView, close out and remove any composites/props 
  // prior to deletion. Has no impact when used with a vtkKWRenderWidget.
  virtual void Close();

  // Description:
  // Chaining method to serialize an object and its superclasses.
  virtual void SerializeSelf(ostream& os, vtkIndent indent);
  virtual void SerializeToken(istream& is, const char *token);
  virtual void SerializeRevision(ostream& os, vtkIndent indent);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Export the corner annotation to a file.
  void SaveState(ofstream *file);

  // Description:
  // Access to the TextPropertyWidget from a script.
  vtkGetObjectMacro(TextPropertyWidget, vtkKWTextProperty);
  
protected:
  vtkKWCornerAnnotation();
  ~vtkKWCornerAnnotation();

  int AnnotationChangedEvent;

  vtkCornerAnnotation     *CornerAnnotation;

  vtkKWRenderWidget       *RenderWidget;

  vtkKWView               *View;
  vtkKWGenericComposite   *InternalCornerComposite;
  vtkCornerAnnotation     *InternalCornerAnnotation;

  // GUI

  int                     PopupTextProperty;

  vtkKWFrame              *CornerFrame;
  vtkKWLabeledText        *CornerText[4];
  vtkKWFrame              *PropertiesFrame;
  vtkKWScale              *MaximumLineHeightScale;
  vtkKWTextProperty       *TextPropertyWidget;
  vtkKWLabeledPopupButton *TextPropertyPopupButton;

  void Render();

  // Get the value that should be used to set the checkbutton state
  // (i.e. depending on the value this checkbutton is supposed to reflect,
  // for example, an annotation visibility).
  // This does *not* return the state of the widget.
  virtual int GetCheckButtonState() { return this->GetVisibility(); };

  // Send an event representing the state of the widget
  virtual void SendChangedEvent();

private:
  vtkKWCornerAnnotation(const vtkKWCornerAnnotation&); // Not implemented
  void operator=(const vtkKWCornerAnnotation&); // Not Implemented
};

#endif

