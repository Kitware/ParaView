/*=========================================================================

  Module:    vtkKWHeaderAnnotationEditor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWHeaderAnnotationEditor - a header annotation widget
// .SECTION Description
// A class that provides a UI for header annotation (vtkTextActor).

#ifndef __vtkKWHeaderAnnotationEditor_h
#define __vtkKWHeaderAnnotationEditor_h

#include "vtkKWCheckButtonWithPopupFrame.h"

class vtkKWEntryWithLabel;
class vtkKWPopupButtonWithLabel;
class vtkKWRenderWidget;
class vtkKWTextPropertyEditor;
class vtkTextActor;
class vtkKWFrame;

class KWWidgets_EXPORT vtkKWHeaderAnnotationEditor : public vtkKWCheckButtonWithPopupFrame
{
public:
  static vtkKWHeaderAnnotationEditor* New();
  vtkTypeRevisionMacro(vtkKWHeaderAnnotationEditor,vtkKWCheckButtonWithPopupFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Makes the text property sub-widget popup (instead of displaying the
  // whole text property UI, which can be long).
  // This has to be called before Create(). Ignored if PopupMode is true.
  vtkSetMacro(PopupTextProperty, int);
  vtkGetMacro(PopupTextProperty, int);
  vtkBooleanMacro(PopupTextProperty, int);

  // Description:
  // Set/Get the vtkKWRenderWidget that owns the header annotation.
  // It is not ref-counted.
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
  // Access to sub-widgets
  virtual vtkKWCheckButton* GetHeaderVisibilityButton()
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

  // Description:
  // Callbacks. Internal, do not use.
  virtual void CheckButtonCallback(int state);
  virtual void HeaderTextCallback(const char *value);
  virtual void TextPropertyCallback();

protected:
  vtkKWHeaderAnnotationEditor();
  ~vtkKWHeaderAnnotationEditor();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  int AnnotationChangedEvent;

  vtkKWRenderWidget       *RenderWidget;

  // GUI

  int                     PopupTextProperty;

  vtkKWFrame                *TextFrame;
  vtkKWEntryWithLabel       *TextEntry;
  vtkKWTextPropertyEditor   *TextPropertyWidget;
  vtkKWPopupButtonWithLabel *TextPropertyPopupButton;

  virtual void Render();
  virtual void SetHeaderText(const char *txt);

  // Get the value that should be used to set the checkbutton state
  // (i.e. depending on the value this checkbutton is supposed to reflect,
  // for example, an annotation visibility).
  // This does *not* return the state of the widget.
  virtual int GetCheckButtonState() { return this->GetVisibility(); };

  // Send an event representing the state of the widget
  virtual void SendChangedEvent();

private:
  vtkKWHeaderAnnotationEditor(const vtkKWHeaderAnnotationEditor&); // Not implemented
  void operator=(const vtkKWHeaderAnnotationEditor&); // Not Implemented
};

#endif

