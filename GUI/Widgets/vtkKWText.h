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

class vtkKWTextInternals;

class KWWidgets_EXPORT vtkKWText : public vtkKWCoreWidget
{
public:
  static vtkKWText* New();
  vtkTypeRevisionMacro(vtkKWText,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Set/Get the value of the text. AppendText() can also be used
  // to add text at the end. If a tag is provided, it will be used to 
  // tag the corresponding text.
  virtual char *GetText();
  virtual void SetText(const char *);
  virtual void SetText(const char *, const char *tag);
  virtual void AppendText(const char *);
  virtual void AppendText(const char *, const char *tag);

  // Description:
  // Set/Get if this text is read-only. Default is Off.
  virtual void SetReadOnly(int val);
  vtkGetMacro(ReadOnly, int);
  vtkBooleanMacro(ReadOnly, int);

  // Description:
  // Set/Get if quick formatting is enabled.
  // In this mode, strings can be tagged using markers:
  // ** : bold      (ex: this is **bold**)
  // ~~ : italic    (ex: this is ~~italic~~)
  // __ : underline (ex: this is __underline__)
  virtual void SetQuickFormatting(int);
  vtkGetMacro(QuickFormatting, int);
  vtkBooleanMacro(QuickFormatting, int);

  // Description:
  // Set/Get the width/height, in characters.
  virtual void SetWidth(int);
  virtual int GetWidth();
  virtual void SetHeight(int);
  virtual int GetHeight();

  // Description:
  // Set/Get the background color of the widget.
  virtual void GetBackgroundColor(double *r, double *g, double *b);
  virtual double* GetBackgroundColor();
  virtual void SetBackgroundColor(double r, double g, double b);
  virtual void SetBackgroundColor(double rgb[3])
    { this->SetBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the foreground color of the widget.
  virtual void GetForegroundColor(double *r, double *g, double *b);
  virtual double* GetForegroundColor();
  virtual void SetForegroundColor(double r, double g, double b);
  virtual void SetForegroundColor(double rgb[3])
    { this->SetForegroundColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the highlight thickness, a non-negative value indicating the
  // width of the highlight rectangle to draw around the outside of the
  // widget when it has the input focus.
  virtual void SetHighlightThickness(int);
  virtual int GetHighlightThickness();
  
  // Description:
  // Set/Get the border width, a non-negative value indicating the width of
  // the 3-D border to draw around the outside of the widget (if such a border
  // is being drawn; the Relief option typically determines this).
  virtual void SetBorderWidth(int);
  virtual int GetBorderWidth();
  
  // Description:
  // Set/Get the 3-D effect desired for the widget. 
  // The value indicates how the interior of the widget should appear
  // relative to its exterior. 
  // Valid constants can be found in vtkKWOptions::ReliefType.
  virtual void SetRelief(int);
  virtual int GetRelief();
  virtual void SetReliefToRaised();
  virtual void SetReliefToSunken();
  virtual void SetReliefToFlat();
  virtual void SetReliefToRidge();
  virtual void SetReliefToSolid();
  virtual void SetReliefToGroove();

  // Description:
  // Specifies the font to use when drawing text inside the widget. 
  // You can use predefined font names (e.g. 'system'), or you can specify
  // a set of font attributes with a platform-independent name, for example,
  // 'times 12 bold'. In this example, the font is specified with a three
  // element list: the first element is the font family, the second is the
  // size, the third is a list of style parameters (normal, bold, roman, 
  // italic, underline, overstrike). Example: 'times 12 {bold italic}'.
  // The Times, Courier and Helvetica font families are guaranteed to exist
  // and will be matched to the corresponding (closest) font on your system.
  // If you are familiar with the X font names specification, you can also
  // describe the font that way (say, '*times-medium-r-*-*-12*').
  virtual void SetFont(const char *font);
  virtual const char* GetFont();

  // Description:
  // Set/Get the padding that will be applied around each widget (in pixels).
  // Specifies a non-negative value indicating how much extra space to request
  // for the widget in the X and Y-direction. When computing how large a
  // window it needs, the widget will add this amount to the width it would
  // normally need (as determined by the width of the things displayed
  // in the widget); if the geometry manager can satisfy this request, the 
  // widget will end up with extra internal space around what it displays 
  // inside. 
  virtual void SetPadX(int);
  virtual int GetPadX();
  virtual void SetPadY(int);
  virtual int GetPadY();

  // Description:
  // Set the wrap mode.
  virtual void SetWrapToNone();
  virtual void SetWrapToWord();
  virtual void SetWrapToChar();

  // Description:
  // Add a tag matcher. Whenever a regular expression 'regexp' is matched
  // it will be tagged with 'tag'.
  virtual void AddTagMatcher(const char *regexp, const char *tag);

  // Description:
  // Set the resize-to-grid flag.
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

  int ReadOnly;
  int QuickFormatting;

  //BTX
  // PIMPL Encapsulation for STL containers
  vtkKWTextInternals *Internals;
  //ETX

  virtual void AppendTextInternalTagging(const char *, const char *tag);
  virtual void AppendTextInternal(const char *, const char *tag);

private:

  char *InternalTextString;
  vtkGetStringMacro(InternalTextString);
  vtkSetStringMacro(InternalTextString);

  vtkKWText(const vtkKWText&); // Not implemented
  void operator=(const vtkKWText&); // Not implemented
};

#endif
