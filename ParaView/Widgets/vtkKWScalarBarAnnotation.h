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
// .NAME vtkKWScalarBarAnnotation - a scalar bar annotation widget
// .SECTION Description
// A class that provides a UI for a scalar bar widget (vtkScalarBarWidget).

#ifndef __vtkKWScalarBarAnnotation_h
#define __vtkKWScalarBarAnnotation_h

#include "vtkKWPopupFrameCheckButton.h"

class vtkKWFrame;
class vtkKWLabeledEntry;
class vtkKWLabeledPopupButton;
class vtkKWScalarComponentSelectionWidget;
class vtkKWScale;
class vtkKWTextProperty;
class vtkKWThumbWheel;
class vtkScalarBarWidget;
class vtkVolumeProperty;

class VTK_EXPORT vtkKWScalarBarAnnotation : public vtkKWPopupFrameCheckButton
{
public:
  static vtkKWScalarBarAnnotation* New();
  vtkTypeRevisionMacro(vtkKWScalarBarAnnotation,vtkKWPopupFrameCheckButton);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Makes the text property sub-widgets popup (instead of displaying the
  // whole text property UI, which can be long).
  // This has to be called before Create(). Ignored if PopupMode is true.
  vtkSetMacro(PopupTextProperty, int);
  vtkGetMacro(PopupTextProperty, int);
  vtkBooleanMacro(PopupTextProperty, int);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char* args);

  // Description:
  // Set/Get the vtkScalarBarWidget that owns the scalar bar actor.
  virtual void SetScalarBarWidget(vtkScalarBarWidget*);
  vtkGetObjectMacro(ScalarBarWidget, vtkScalarBarWidget);

  // Description:
  // Set/Get the scalar bar visibility
  virtual void SetVisibility(int i);
  virtual int GetVisibility();
  vtkBooleanMacro(Visibility, int);

  // Description:
  // Set/Get the event invoked when the anything in the annotation is changed.
  // Defaults to vtkKWEvent::ViewAnnotationChangedEvent
  vtkSetMacro(AnnotationChangedEvent, int);
  vtkGetMacro(AnnotationChangedEvent, int);

  // Description:
  // Set/get the volume property that can be used to set the LUT of the
  // scalar bar actor (i.e. allow the user to choose which component to
  // visualize)
  virtual void SetVolumeProperty(vtkVolumeProperty *prop);
  vtkGetObjectMacro(VolumeProperty, vtkVolumeProperty);

  // Description:
  // Set/get the number of components corresponding to the data represented
  // by the volume property
  virtual void SetNumberOfComponents(int);
  vtkGetMacro(NumberOfComponents, int);

  // Description:
  // Set/Get the event invoked when the component is changed.
  // Defaults to vtkKWEvent::ScalarComponentChangedEvent
  vtkSetMacro(ScalarComponentChangedEvent, int);
  vtkGetMacro(ScalarComponentChangedEvent, int);

  // Description:
  // Callbacks
  virtual void CheckButtonCallback();
  virtual void SelectedComponentCallback(int);
  virtual void ScalarBarTitleCallback();
  virtual void ScalarBarLabelFormatCallback();
  virtual void TitleTextPropertyCallback();
  virtual void LabelTextPropertyCallback();
  virtual void MaximumNumberOfColorsEndCallback();
  virtual void NumberOfLabelsEndCallback();

  // Description:
  // Access to sub-widgets
  virtual vtkKWCheckButton* GetScalarBarVisibilityButton()
    { return this->GetCheckButton(); };

  // Description:
  // Update the GUI according to the value of the ivars
  void Update();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWScalarBarAnnotation();
  ~vtkKWScalarBarAnnotation();

  int PopupTextProperty;
  int AnnotationChangedEvent;
  int ScalarComponentChangedEvent;
  int NumberOfComponents;

  vtkScalarBarWidget      *ScalarBarWidget;
  vtkVolumeProperty       *VolumeProperty;

  // GUI

  vtkKWScalarComponentSelectionWidget *ComponentSelectionWidget;
  vtkKWFrame                          *TitleFrame;
  vtkKWLabeledEntry                   *TitleEntry;
  vtkKWTextProperty                   *TitleTextPropertyWidget;
  vtkKWLabeledPopupButton             *TitleTextPropertyPopupButton;
  vtkKWFrame                          *LabelFrame;
  vtkKWLabeledEntry                   *LabelFormatEntry;
  vtkKWTextProperty                   *LabelTextPropertyWidget;
  vtkKWLabeledPopupButton             *LabelTextPropertyPopupButton;
  vtkKWThumbWheel                     *MaximumNumberOfColorsThumbWheel;
  vtkKWScale                          *NumberOfLabelsScale;

  virtual void Render();
  virtual void SetScalarBarTitle(const char *txt);
  virtual void SetScalarBarLabelFormat(const char *txt);

  // Get the value that should be used to set the checkbutton state
  // (i.e. depending on the value this checkbutton is supposed to reflect,
  // for example, an annotation visibility).
  // This does *not* return the state of the widget.
  virtual int GetCheckButtonState() { return this->GetVisibility(); };

  // Send an event representing the state of the widget
  virtual void SendChangedEvent();

private:
  vtkKWScalarBarAnnotation(const vtkKWScalarBarAnnotation&); // Not implemented
  void operator=(const vtkKWScalarBarAnnotation&); // Not Implemented
};

#endif

