/*=========================================================================

  Module:    vtkKWTextProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWTextProperty - a GUI component that can be used to edit vtkTextProperty objects
// .SECTION Description
// The vtkKWTextProperty creates a set of GUI components that can be displayed
// and used selectively to edit all or part of a vtkTextProperty object.


#ifndef __vtkKWTextProperty_h
#define __vtkKWTextProperty_h

#include "vtkKWWidget.h"

class vtkActor2D;
class vtkKWApplication;
class vtkKWChangeColorButton;
class vtkKWLabel;
class vtkKWCheckButtonSetLabeled;
class vtkKWOptionMenuLabeled;
class vtkKWPushButtonSetLabeled;
class vtkKWPushButton;
class vtkKWScale;
class vtkTextProperty;

class VTK_EXPORT vtkKWTextProperty : public vtkKWWidget
{
public:
  static vtkKWTextProperty* New();
  vtkTypeRevisionMacro(vtkKWTextProperty,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app);

  // Description
  // Refresh/Update the interface according to the value of the text property
  // and actor2d
  virtual void Update();

  //BTX
  // Description:
  // Set/Get the text property to control.
  virtual void SetTextProperty(vtkTextProperty*);
  vtkGetObjectMacro(TextProperty, vtkTextProperty);

  // Description:
  // Set/Get the actor that uses TextProperty. This is optional, but might
  // help to solve some backward compatibility issues. For example, the
  // default vtkTextProperty color is -1, -1, -1 to specify to the mapper
  // that the vtkActor2D color has to be used instead.
  virtual void SetActor2D(vtkActor2D*);
  vtkGetObjectMacro(Actor2D, vtkActor2D);
  //ETX

  // Description:
  // Set the widget aspect to be long, i.e. the widgets will be packed on 
  // several rows, with description labels. The default is short (all widgets
  // on a row).
  virtual void SetLongFormat(int);
  vtkBooleanMacro(LongFormat, int);
  vtkGetMacro(LongFormat, int);

  // Description:
  // Show the label on top (default, otherwise on left. 
  // Valid if LongFormat is On.
  virtual void SetLabelOnTop(int);
  vtkBooleanMacro(LabelOnTop, int);
  vtkGetMacro(LabelOnTop, int);

  // Description:
  // Show label.
  virtual void SetShowLabel(int);
  vtkBooleanMacro(ShowLabel, int);
  vtkGetMacro(ShowLabel, int);
  vtkGetObjectMacro(Label, vtkKWLabel);

  // Description:
  // Show color.
  virtual void SetShowColor(int);
  vtkBooleanMacro(ShowColor, int);
  vtkGetMacro(ShowColor, int);

  // Description:
  // Show font family.
  virtual void SetShowFontFamily(int);
  vtkBooleanMacro(ShowFontFamily, int);
  vtkGetMacro(ShowFontFamily, int);

  // Description:
  // Show style.
  virtual void SetShowStyles(int);
  vtkBooleanMacro(ShowStyles, int);
  vtkGetMacro(ShowStyles, int);

  // Description:
  // Show opacity.
  virtual void SetShowOpacity(int);
  vtkBooleanMacro(ShowOpacity, int);
  vtkGetMacro(ShowOpacity, int);

  // Description:
  // Set/Get the command executed each time a change is made to the
  // text property.
  virtual void SetChangedCommand(
    vtkKWObject *object, const char *method);

  // Description:
  // Set/Get the command executed each time a change is made to the
  // color of the text property (the ChangedCommand is run too).
  virtual void SetColorChangedCommand(
    vtkKWObject *object, const char *method);

  // Description:
  // Show copy button. This button can be used to synchronize different
  // text property widgets.
  virtual void SetShowCopy(int);
  vtkBooleanMacro(ShowCopy, int);
  vtkGetMacro(ShowCopy, int);
  virtual vtkKWPushButton* GetCopyButton();

  // Description:
  // Copy the values from another text widget
  virtual void CopyValuesFrom(vtkKWTextProperty*);

  // GUI components callbacks
  void ChangeColorButtonCallback(double, double, double);
  void SetColor(double, double, double);
  void SetColor(double *v) { this->SetColor(v[0], v[1], v[2]); };
  double* GetColor();
  void FontFamilyCallback();
  void SetFontFamily(int);
  void BoldCallback();
  void SetBold(int);
  void ItalicCallback();
  void SetItalic(int);
  void ShadowCallback();
  void SetShadow(int);
  void OpacityCallback();
  void OpacityEndCallback();
  void SetOpacity(float);
  void SetOpacityNoTrace(float);
  float GetOpacity();

  // Description:
  // Save out the text properties to a file.
  void SaveInTclScript(ofstream *file, const char *tcl_name = 0,
                       int tabify = 1);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWTextProperty();
  ~vtkKWTextProperty();

  virtual void Pack();

  void UpdateInterface();
  void UpdateLabel();
  void UpdateColorButton();
  void UpdateFontFamilyOptionMenu();
  void UpdateStylesCheckButtonSet();
  void UpdateBoldCheckButton();
  void UpdateItalicCheckButton();
  void UpdateShadowCheckButton();
  void UpdateOpacityScale();
  void UpdatePushButtonSet();

  vtkTextProperty *TextProperty;
  vtkActor2D *Actor2D;

  int LongFormat;

  int ShowLabel;
  int LabelOnTop;
  vtkKWLabel *Label;

  int ShowColor;
  vtkKWChangeColorButton *ChangeColorButton;

  int ShowFontFamily;
  vtkKWOptionMenuLabeled *FontFamilyOptionMenu;

  int ShowStyles;
  vtkKWCheckButtonSetLabeled *StylesCheckButtonSet;
  
  int ShowOpacity;
  vtkKWScale *OpacityScale;

  char *ChangedCommand;
  char *ColorChangedCommand;

  int ShowCopy;
  vtkKWPushButtonSetLabeled *PushButtonSet;

private:
  vtkKWTextProperty(const vtkKWTextProperty&); // Not implemented
  void operator=(const vtkKWTextProperty&); // Not implemented
};

#endif

