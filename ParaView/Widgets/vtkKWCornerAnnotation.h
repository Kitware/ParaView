/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWCornerAnnotation.h
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
// .NAME vtkKWCornerAnnotation - a corner annotation widget
// .SECTION Description
// A class that provides a UI for vtkCornerAnnotation. User can set the
// text for each corner, set the color of the text, and turn the annotation
// on and off.

#ifndef __vtkKWCornerAnnotation_h
#define __vtkKWCornerAnnotation_h

#include "vtkKWLabeledFrame.h"

class vtkCornerAnnotation;
class vtkKWChangeColorButton;
class vtkKWCheckButton;
class vtkKWFrame;
class vtkKWGenericComposite;
class vtkKWLabel;
class vtkKWScale;
class vtkKWRenderWidget;
class vtkKWLabeledText;
class vtkKWTextProperty;
class vtkKWView;

class VTK_EXPORT vtkKWCornerAnnotation : public vtkKWLabeledFrame
{
public:
  static vtkKWCornerAnnotation* New();
  vtkTypeRevisionMacro(vtkKWCornerAnnotation,vtkKWLabeledFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char* args);

  // Description:
  // Set/Get the vtkKWView or the vtkKWRenderWidget that owns this annotation.
  // vtkKWView and vtkKWRenderWidget are two different frameworks, choose one or
  // the other (ParaView uses vtkKWView, VolView uses vtkKWRenderWidget).
  // Note that in vtkKWView mode, each view has a vtkKWCornerAnnotation. 
  // In vtkKWRenderWidget, each widget has a vtkCornerAnnotation, which is 
  // controlled by a unique (decoupled) vtkKWCornerAnnotation in the GUI.
  virtual void SetView(vtkKWView*);
  vtkGetObjectMacro(View,vtkKWView);
  virtual void SetRenderWidget(vtkKWRenderWidget*);
  vtkGetObjectMacro(RenderWidget,vtkKWRenderWidget);

  // Description:
  // Get the underlying vtkCornerAnnotation. 
  // In vtkKWView mode, the CornerProp is created automatically and handled
  // by this class (i.e. each vtkKWCornerAnnotation has a vtkCornerAnnotation).
  // In vtkKWRenderWidget, the corner prop is part of vtkKWRenderWidget, and
  // this method is just a gateway to vtkKWRenderWidget::GetCornerProp().
  vtkGetObjectMacro(CornerProp, vtkCornerAnnotation);
  
  // Description:
  // When used with a vtkKWView, close out and remove any composites/props 
  // prior to deletion. Has no impact when used with a vtkKWRenderWidget.
  virtual void Close();

  // Description:
  // In vtkKWView mode, displays and updates the property ui display
  virtual void ShowProperties();

  // Description:
  // Set/Get the annotation visibility
  virtual void SetVisibility(int i);
  virtual int  GetVisibility();
  vtkBooleanMacro(Visibility,int);
  virtual void DisplayCornerCallback();
  
  // Description:
  // Set/Get corner text
  virtual void SetCornerText(const char *txt, int corner);
  virtual char *GetCornerText(int i);
  virtual void CornerTextCallback(int i);

  // Description:
  // Change the color of the annotation
  virtual void SetTextColor(float r, float g, float b);
  virtual void SetTextColor(float *rgb)
               { this->SetTextColor(rgb[0], rgb[1], rgb[2]); }
  virtual float *GetTextColor();
  virtual void TextColorCallback();

  // Description:
  // Set/Get the maximum line height.
  virtual void SetMaximumLineHeight(float);
  virtual void SetMaximumLineHeightNoTrace(float);
  virtual void MaximumLineHeightCallback();
  virtual void MaximumLineHeightEndCallback();
  vtkGetObjectMacro(MaximumLineHeightScale, vtkKWScale);

  // Description:
  // GUI components access
  vtkGetObjectMacro(TextPropertyWidget, vtkKWTextProperty);
  void TextPropertyCallback();

  // Description:
  // Chaining method to serialize an object and its superclasses.
  virtual void SerializeSelf(ostream& os, vtkIndent indent);
  virtual void SerializeToken(istream& is,const char token[1024]);
  virtual void SerializeRevision(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the enabled state.
  // Override to pass down to children.
  virtual void SetEnabled(int);

  // Description:
  // Update the GUI according to the value of the ivars
  void Update();

  // Description:
  // Set the event invoked when the color of the annotation is changed.
  // The AnnotationChangedEvent will be invoked too.
  // Defaults to vtkKWEvent::AnnotationColorChangedEvent
  vtkSetMacro(AnnotationColorChangedEvent, int)
  vtkGetMacro(AnnotationColorChangedEvent, int)

  // Description:
  // Set the event invoked when the anything in the annotation is changed.
  // Defaults to vtkKWEvent::ViewAnnotationChangedEvent
  vtkSetMacro(AnnotationChangedEvent, int);
  vtkGetMacro(AnnotationChangedEvent, int);

protected:
  vtkKWCornerAnnotation();
  ~vtkKWCornerAnnotation();

  int AnnotationColorChangedEvent;
  int AnnotationChangedEvent;

  vtkCornerAnnotation    *CornerProp;

  vtkKWRenderWidget      *RenderWidget;

  vtkKWView              *View;
  vtkKWGenericComposite  *InternalCornerComposite;
  vtkCornerAnnotation    *InternalCornerProp;

  // GUI

  vtkKWCheckButton       *CornerVisibilityButton;
  vtkKWFrame             *CornerFrame;
  vtkKWLabeledText       *CornerText[4];
  vtkKWScale             *MaximumLineHeightScale;
  vtkKWTextProperty      *TextPropertyWidget;

  void Render();

private:
  vtkKWCornerAnnotation(const vtkKWCornerAnnotation&); // Not implemented
  void operator=(const vtkKWCornerAnnotation&); // Not Implemented
};


#endif



