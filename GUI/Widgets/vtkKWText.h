/*=========================================================================

  Module:    vtkKWText.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWText - a multi-line text entry widget
// .SECTION Description
// A simple widget used for collecting keyboard input from the user. This
// widget provides support for multi-line input.

#ifndef __vtkKWText_h
#define __vtkKWText_h

#include "vtkKWWidget.h"

class vtkKWApplication;

class KWWIDGETS_EXPORT vtkKWText : public vtkKWWidget
{
public:
  static vtkKWText* New();
  vtkTypeRevisionMacro(vtkKWText,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  // This will create a frame, inside which a 'text' Tk widget
  // will be packed. Use GetTextWidget() to access/configure it.
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Access the internal 'text' Tk widget and the scrollbar.
  vtkGetObjectMacro(TextWidget, vtkKWWidget);
  vtkGetObjectMacro(VerticalScrollBar, vtkKWWidget);

  // Description:
  // Set/Get the value of the text. AppendValue() is a convenience function
  // to add text at the end.
  virtual char *GetValue();
  virtual void SetValue(const char *, const char *tag = NULL);
  virtual void AppendValue(const char *, const char *tag = NULL);

  // Description:
  // Set/Get if this text is editable by the user. Default is On.
  virtual void SetEditableText(int val);
  vtkGetMacro(EditableText, int);
  vtkBooleanMacro(EditableText, int);

  // Description:
  // Set/Get if quick formatting is enabled.
  // In this mode, strings can be tagged using markers:
  // ** : bold      (ex: this is **bold**)
  // ~~ : italic    (ex: this is ~~italic~~)
  // __ : underline (ex: this is __underline__)
  vtkSetMacro(QuickFormatting, int);
  vtkGetMacro(QuickFormatting, int);
  vtkBooleanMacro(QuickFormatting, int);

  // Description:
  // Use vertical scrollbar.
  virtual void SetUseVerticalScrollbar(int val);
  vtkGetMacro(UseVerticalScrollbar, int);
  vtkBooleanMacro(UseVerticalScrollbar, int);

  // Description:
  // Convenience method to set the width/height.
  virtual void SetWidth(int);
  virtual int GetWidth();
  virtual void SetHeight(int);
  virtual int GetHeight();

  // Description:
  // Convenience method to set the wrap mode.
  virtual void SetWrapToNone();
  virtual void SetWrapToWord();
  virtual void SetWrapToChar();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWText();
  ~vtkKWText();

  char *ValueString;
  vtkGetStringMacro(ValueString);
  vtkSetStringMacro(ValueString);

  vtkKWWidget *TextWidget;
  vtkKWWidget *VerticalScrollBar;

  int EditableText;
  int QuickFormatting;
  int UseVerticalScrollbar;

  // Description:
  // Create horizontal scrollbar
  virtual void CreateVerticalScrollbar(vtkKWApplication *app);

  // Description:
  // Pack.
  virtual void Pack();

private:
  vtkKWText(const vtkKWText&); // Not implemented
  void operator=(const vtkKWText&); // Not implemented
};

#endif
