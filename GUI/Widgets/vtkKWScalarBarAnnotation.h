/*=========================================================================

  Module:    vtkKWScalarBarAnnotation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWScalarBarAnnotation - a scalar bar annotation widget
// .SECTION Description
// A class that provides a UI for a scalar bar widget (vtkScalarBarWidget).

#ifndef __vtkKWScalarBarAnnotation_h
#define __vtkKWScalarBarAnnotation_h

#include "vtkKWCheckButtonWithPopupFrame.h"

class vtkKWFrame;
class vtkKWEntryWithLabel;
class vtkKWPopupButtonWithLabel;
class vtkKWScalarComponentSelectionWidget;
class vtkKWScaleWithEntry;
class vtkKWTextPropertyEditor;
class vtkKWThumbWheel;
class vtkScalarBarWidget;
class vtkVolumeProperty;

class KWWidgets_EXPORT vtkKWScalarBarAnnotation : public vtkKWCheckButtonWithPopupFrame
{
public:
  static vtkKWScalarBarAnnotation* New();
  vtkTypeRevisionMacro(vtkKWScalarBarAnnotation,vtkKWCheckButtonWithPopupFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Makes the text property sub-widgets popup (instead of displaying the
  // whole text property UI, which can be long).
  // This has to be called before Create(). Ignored if PopupMode is true.
  vtkSetMacro(PopupTextProperty, int);
  vtkGetMacro(PopupTextProperty, int);
  vtkBooleanMacro(PopupTextProperty, int);

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
  // Set/Get the volume property that can be used to set the LUT of the
  // scalar bar actor (i.e. allow the user to choose which component to
  // visualize)
  virtual void SetVolumeProperty(vtkVolumeProperty *prop);
  vtkGetObjectMacro(VolumeProperty, vtkVolumeProperty);

  // Description:
  // Set/Get the number of components corresponding to the data represented
  // by the volume property
  virtual void SetNumberOfComponents(int);
  vtkGetMacro(NumberOfComponents, int);

  // Description:
  // Set/Get the event invoked when the component is changed.
  // Defaults to vtkKWEvent::ScalarComponentChangedEvent
  vtkSetMacro(ScalarComponentChangedEvent, int);
  vtkGetMacro(ScalarComponentChangedEvent, int);

  // Description:
  // Set/Get the LabelFormat UI visibility, which might be a bit confusing
  virtual void SetLabelFormatVisibility(int i);
  vtkGetMacro(LabelFormatVisibility, int);
  vtkBooleanMacro(LabelFormatVisibility, int);

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

  // Description:
  // Callbacks. Internal, do not use.
  virtual void CheckButtonCallback(int state);
  virtual void SelectedComponentCallback(int);
  virtual void ScalarBarTitleCallback(const char *value);
  virtual void ScalarBarLabelFormatCallback(const char *value);
  virtual void TitleTextPropertyCallback();
  virtual void LabelTextPropertyCallback();
  virtual void MaximumNumberOfColorsEndCallback(double value);
  virtual void NumberOfLabelsEndCallback(double value);

protected:
  vtkKWScalarBarAnnotation();
  ~vtkKWScalarBarAnnotation();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  int PopupTextProperty;
  int AnnotationChangedEvent;
  int ScalarComponentChangedEvent;
  int NumberOfComponents;
  int LabelFormatVisibility;

  vtkScalarBarWidget      *ScalarBarWidget;
  vtkVolumeProperty       *VolumeProperty;

  // GUI

  vtkKWScalarComponentSelectionWidget *ComponentSelectionWidget;
  vtkKWFrame                          *TitleFrame;
  vtkKWEntryWithLabel                   *TitleEntry;
  vtkKWTextPropertyEditor             *TitleTextPropertyWidget;
  vtkKWPopupButtonWithLabel             *TitleTextPropertyPopupButton;
  vtkKWFrame                          *LabelFrame;
  vtkKWEntryWithLabel                   *LabelFormatEntry;
  vtkKWTextPropertyEditor             *LabelTextPropertyWidget;
  vtkKWPopupButtonWithLabel             *LabelTextPropertyPopupButton;
  vtkKWThumbWheel                     *MaximumNumberOfColorsThumbWheel;
  vtkKWScaleWithEntry                  *NumberOfLabelsScale;

  virtual void PackLabelFrameChildren();
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

