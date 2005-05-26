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

#include "vtkKWPopupFrameCheckButton.h"

class vtkKWFrame;
class vtkKWEntryLabeled;
class vtkKWPopupButtonLabeled;
class vtkKWScalarComponentSelectionWidget;
class vtkKWScale;
class vtkKWTextPropertyEditor;
class vtkKWThumbWheel;
class vtkScalarBarWidget;
class vtkVolumeProperty;

class KWWIDGETS_EXPORT vtkKWScalarBarAnnotation : public vtkKWPopupFrameCheckButton
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

  //BTX
  // Description:
  // Set/Get the vtkScalarBarWidget that owns the scalar bar actor.
  virtual void SetScalarBarWidget(vtkScalarBarWidget*);
  vtkGetObjectMacro(ScalarBarWidget, vtkScalarBarWidget);
  //ETX

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

  //BTX
  // Description:
  // Set/get the volume property that can be used to set the LUT of the
  // scalar bar actor (i.e. allow the user to choose which component to
  // visualize)
  virtual void SetVolumeProperty(vtkVolumeProperty *prop);
  vtkGetObjectMacro(VolumeProperty, vtkVolumeProperty);
  //ETX

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
  // Show/Hide the LabelFormat UI, which might be a bit confusion
  virtual void SetShowLabelFormat(int i);
  vtkGetMacro(ShowLabelFormat, int);
  vtkBooleanMacro(ShowLabelFormat, int);

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
  int ShowLabelFormat;

  vtkScalarBarWidget      *ScalarBarWidget;
  vtkVolumeProperty       *VolumeProperty;

  // GUI

  vtkKWScalarComponentSelectionWidget *ComponentSelectionWidget;
  vtkKWFrame                          *TitleFrame;
  vtkKWEntryLabeled                   *TitleEntry;
  vtkKWTextPropertyEditor             *TitleTextPropertyWidget;
  vtkKWPopupButtonLabeled             *TitleTextPropertyPopupButton;
  vtkKWFrame                          *LabelFrame;
  vtkKWEntryLabeled                   *LabelFormatEntry;
  vtkKWTextPropertyEditor             *LabelTextPropertyWidget;
  vtkKWPopupButtonLabeled             *LabelTextPropertyPopupButton;
  vtkKWThumbWheel                     *MaximumNumberOfColorsThumbWheel;
  vtkKWScale                          *NumberOfLabelsScale;

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

