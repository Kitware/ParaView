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
// Use vtkKWTextWithScrollbars if you need scrollbars.
// .SECTION See Also
// vtkKWTextWithScrollbars

#ifndef __vtkKWText_h
#define __vtkKWText_h

#include "vtkKWCoreWidget.h"

class vtkKWApplication;
class vtkKWTextInternals;

class KWWIDGETS_EXPORT vtkKWText : public vtkKWCoreWidget
{
public:
  static vtkKWText* New();
  vtkTypeRevisionMacro(vtkKWText,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set/Get the value of the text. AppendValue() is a convenience function
  // to add text at the end. If a tag is provided, it will be used to 
  // tag the corresponding text.
  virtual char *GetValue();
  virtual void SetValue(const char *);
  virtual void SetValue(const char *, const char *tag);
  virtual void AppendValue(const char *);
  virtual void AppendValue(const char *, const char *tag);

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
  // Convenience method to get/set the width/height.
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
  // Add a tag matcher. Whenever a regular expression 'regexp' is matched
  // it will be tagged with 'tag'.
  virtual void AddTagMatcher(const char *regexp, const char *tag);

  // Description:
  // Convenience method to set the resize-to-grid flag.
  // Specifies a boolean value that determines whether this widget controls
  // the resizing grid for its top-level window. This option is typically
  // used in text widgets, where the information in the widget has a natural
  // size (the size of a character) and it makes sense for the window's
  // dimensions to be integral numbers of these units. These natural window
  // sizes form a grid. If the setGrid option is set to true then the widget
  // will communicate with the window manager so that when the user 
  // interactively resizes the top-level window that contains the widget, 
  // the dimensions of the window will be displayed to the user in grid units
  // and the window size will be constrained to integral numbers of grid units.
  vtkBooleanMacro(ResizeToGrid, int);
  virtual void SetResizeToGrid(int);
  virtual int GetResizeToGrid();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Some constants
  //BTX
  static const char *MarkerBold;
  static const char *MarkerItalic;
  static const char *MarkerUnderline;
  static const char *TagBold;
  static const char *TagItalic;
  static const char *TagUnderline;
  static const char *TagFgNavy;
  static const char *TagFgRed;
  static const char *TagFgBlue;
  static const char *TagFgDarkGreen;
  //ETX

protected:
  vtkKWText();
  ~vtkKWText();

  char *ValueString;
  vtkGetStringMacro(ValueString);
  vtkSetStringMacro(ValueString);

  int EditableText;
  int QuickFormatting;

  //BTX
  // PIMPL Encapsulation for STL containers
  vtkKWTextInternals *Internals;
  //ETX

  virtual void AppendValueInternalTagging(const char *, const char *tag);
  virtual void AppendValueInternal(const char *, const char *tag);

private:
  vtkKWText(const vtkKWText&); // Not implemented
  void operator=(const vtkKWText&); // Not implemented
};

#endif
