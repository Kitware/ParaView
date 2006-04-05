/*=========================================================================

  Module:    vtkKWTextPropertyEditor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWTextPropertyEditor - a GUI component that can be used to edit vtkTextProperty objects
// .SECTION Description
// The vtkKWTextPropertyEditor creates a set of GUI components that can be displayed
// and used selectively to edit all or part of a vtkTextProperty object.


#ifndef __vtkKWTextPropertyEditor_h
#define __vtkKWTextPropertyEditor_h

#include "vtkKWCompositeWidget.h"

class vtkActor2D;
class vtkKWChangeColorButton;
class vtkKWLabel;
class vtkKWCheckButtonSetWithLabel;
class vtkKWMenuButtonWithLabel;
class vtkKWPushButtonSetWithLabel;
class vtkKWPushButton;
class vtkKWScaleWithEntry;
class vtkTextProperty;

class KWWidgets_EXPORT vtkKWTextPropertyEditor : public vtkKWCompositeWidget
{
public:
  static vtkKWTextPropertyEditor* New();
  vtkTypeRevisionMacro(vtkKWTextPropertyEditor,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description
  // Refresh/Update the interface according to the value of the text property
  // and actor2d
  virtual void Update();

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

  // Description:
  // Set the widget aspect to be long, i.e. the widgets will be packed on 
  // several rows, with description labels. The default is short (all widgets
  // on a row).
  virtual void SetLongFormat(int);
  vtkBooleanMacro(LongFormat, int);
  vtkGetMacro(LongFormat, int);

  // Description:
  // Display the label on top (default, otherwise on left. 
  // Valid if LongFormat is On.
  virtual void SetLabelOnTop(int);
  vtkBooleanMacro(LabelOnTop, int);
  vtkGetMacro(LabelOnTop, int);

  // Description:
  // Set/Get the label visibility.
  virtual void SetLabelVisibility(int);
  vtkBooleanMacro(LabelVisibility, int);
  vtkGetMacro(LabelVisibility, int);
  vtkGetObjectMacro(Label, vtkKWLabel);

  // Description:
  // Set/Get the color interface visibility.
  virtual void SetColorVisibility(int);
  vtkBooleanMacro(ColorVisibility, int);
  vtkGetMacro(ColorVisibility, int);

  // Description:
  // Set/Get the font family interface visibility.
  virtual void SetFontFamilyVisibility(int);
  vtkBooleanMacro(FontFamilyVisibility, int);
  vtkGetMacro(FontFamilyVisibility, int);

  // Description:
  // Set/Get the style interface visibility.
  virtual void SetStylesVisibility(int);
  vtkBooleanMacro(StylesVisibility, int);
  vtkGetMacro(StylesVisibility, int);

  // Description:
  // Set/Get the opacity interface visibility.
  virtual void SetOpacityVisibility(int);
  vtkBooleanMacro(OpacityVisibility, int);
  vtkGetMacro(OpacityVisibility, int);

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked each time a change is made to the text property.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetChangedCommand(vtkObject *object, const char *method);

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked each time a change is made to the color of
  // the text property (the ChangedCommand is triggered too).
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - the new RGB color: double, double, double
  virtual void SetColorChangedCommand(vtkObject *object, const char *method);

  // Description:
  // Set/Get the copy button visibility. This button can be used to
  // synchronize different text property widgets.
  virtual void SetCopyVisibility(int);
  vtkBooleanMacro(CopyVisibility, int);
  vtkGetMacro(CopyVisibility, int);
  virtual vtkKWPushButton* GetCopyButton();

  // Description:
  // Copy the values from another text widget
  virtual void CopyValuesFrom(vtkKWTextPropertyEditor*);

  // Description:
  // Save out the text properties to a file.
  virtual void SaveInTclScript(ofstream *file, const char *tcl_name = 0,
                               int tabify = 1);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Set the text properties
  virtual void SetColor(double, double, double);
  virtual void SetColor(double *v) { this->SetColor(v[0], v[1], v[2]); };
  virtual double* GetColor();
  virtual void SetFontFamily(int);
  virtual void SetBold(int);
  virtual void SetItalic(int);
  virtual void SetShadow(int);
  virtual void SetOpacity(float);
  virtual float GetOpacity();

  // Description:
  // Callbacks. Internal, do not use.
  virtual void ChangeColorButtonCallback(double, double, double);
  virtual void FontFamilyCallback();
  virtual void BoldCallback(int state);
  virtual void ItalicCallback(int state);
  virtual void ShadowCallback(int state);
  virtual void OpacityCallback(double value);
  virtual void OpacityEndCallback(double value);

protected:
  vtkKWTextPropertyEditor();
  ~vtkKWTextPropertyEditor();

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

  int LabelVisibility;
  int LabelOnTop;
  vtkKWLabel *Label;

  int ColorVisibility;
  vtkKWChangeColorButton *ChangeColorButton;

  int FontFamilyVisibility;
  vtkKWMenuButtonWithLabel *FontFamilyOptionMenu;

  int StylesVisibility;
  vtkKWCheckButtonSetWithLabel *StylesCheckButtonSet;
  
  int OpacityVisibility;
  vtkKWScaleWithEntry *OpacityScale;

  char *ChangedCommand;
  char *ColorChangedCommand;

  virtual void InvokeChangedCommand();
  virtual void InvokeColorChangedCommand(double r, double g, double b);

  int CopyVisibility;
  vtkKWPushButtonSetWithLabel *PushButtonSet;

private:
  vtkKWTextPropertyEditor(const vtkKWTextPropertyEditor&); // Not implemented
  void operator=(const vtkKWTextPropertyEditor&); // Not implemented
};

#endif

