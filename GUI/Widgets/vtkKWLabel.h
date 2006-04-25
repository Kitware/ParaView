/*=========================================================================

  Module:    vtkKWLabel.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLabel - label widget
// .SECTION Description
// A simple widget that represents a label. A label is a widget that displays
// a textual string (or image). If text is displayed, it must all be in
// a single font, but it can occupy multiple lines on the screen (if it
// contains newlines or if wrapping occurs because of the WrapLength option).
// For longer text and more justification options, see vtkKWMessage.
// .SECTION See Also
// vtkKWMessage

#ifndef __vtkKWLabel_h
#define __vtkKWLabel_h

#include "vtkKWCoreWidget.h"

class vtkKWIcon;

class KWWidgets_EXPORT vtkKWLabel : public vtkKWCoreWidget
{
public:
  static vtkKWLabel* New();
  vtkTypeRevisionMacro(vtkKWLabel,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();
  
  // Description:
  // Set the text on the label.
  virtual void SetText(const char*);
  vtkGetStringMacro(Text);

  // Description:
  // Set/Get the width of the label.
  // If an image is being displayed in the label then the value is in screen
  // units; for text it is in characters.
  virtual void SetWidth(int);
  virtual int GetWidth();

  // Description:
  // Set/Get the height of the label.
  // If an image is being displayed in the label then the value is in screen
  // units; for text it is in lines of text.
  virtual void SetHeight(int);
  virtual int GetHeight();

  // Description:
  // Set/Get the justification mode.
  // When there are multiple lines of text displayed in a widget, this option
  // determines how the lines line up with each other.
  // Valid constants can be found in vtkKWOptions::JustificationType.
  virtual void SetJustification(int);
  virtual int GetJustification();
  virtual void SetJustificationToLeft();
  virtual void SetJustificationToCenter();
  virtual void SetJustificationToRight();

  // Description:
  // Set/Get the anchoring.
  // Specifies how the information in a widget (e.g. text or a bitmap) is to
  // be displayed in the widget.
  // Valid constants can be found in vtkKWOptions::AnchorType.
  virtual void SetAnchor(int);
  virtual int GetAnchor();
  virtual void SetAnchorToNorth();
  virtual void SetAnchorToNorthEast();
  virtual void SetAnchorToEast();
  virtual void SetAnchorToSouthEast();
  virtual void SetAnchorToSouth();
  virtual void SetAnchorToSouthWest();
  virtual void SetAnchorToWest();
  virtual void SetAnchorToNorthWest();
  virtual void SetAnchorToCenter();

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
  // Set/Get the active background color of the widget. An element
  // (a widget or portion of a widget) is active if the mouse cursor is
  // positioned over the element and pressing a mouse button will cause some
  // action to occur.
  virtual void GetActiveBackgroundColor(double *r, double *g, double *b);
  virtual double* GetActiveBackgroundColor();
  virtual void SetActiveBackgroundColor(double r, double g, double b);
  virtual void SetActiveBackgroundColor(double rgb[3])
    { this->SetActiveBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the active foreground color of the widget. An element
  // (a widget or portion of a widget) is active if the mouse cursor is
  // positioned over the element and pressing a mouse button will cause some
  // action to occur.
  virtual void GetActiveForegroundColor(double *r, double *g, double *b);
  virtual double* GetActiveForegroundColor();
  virtual void SetActiveForegroundColor(double r, double g, double b);
  virtual void SetActiveForegroundColor(double rgb[3])
    { this->SetActiveForegroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the foreground color of the widget when it is disabled.
  virtual void GetDisabledForegroundColor(double *r, double *g, double *b);
  virtual double* GetDisabledForegroundColor();
  virtual void SetDisabledForegroundColor(double r, double g, double b);
  virtual void SetDisabledForegroundColor(double rgb[3])
    { this->SetDisabledForegroundColor(rgb[0], rgb[1], rgb[2]); };

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
  // Set/Get the wrap length mode.
  // For widgets that can perform word-wrapping, this option specifies the
  // maximum line length. Lines that would exceed this length are wrapped onto
  // the next line, so that no line is longer than the specified length. 
  // The value may be specified in any of the standard forms for screen
  // distances (i.e, "2i" means 2 inches).
  // If this value is less than or equal to 0 then no wrapping is done: lines
  // will break only at newline characters in the text. 
  virtual void SetWrapLength(const char *length);
  virtual const char* GetWrapLength();
  
  // Description:
  // Adjust the -wraplength argument so that it matches the width of
  // the widget automatically (through the Configure event).
  virtual void SetAdjustWrapLengthToWidth(int);
  vtkGetMacro(AdjustWrapLengthToWidth, int);
  vtkBooleanMacro(AdjustWrapLengthToWidth, int);

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
  // Specifies an image to display in the widget. Typically, if the image
  // is specified then it overrides other options that specify a bitmap or
  // textual value to display in the widget. Invoke vtkKWWidget's 
  // SetConfigurationOption("-image", imagename) to use a specific 
  // pre-existing Tk image, or call one of the following functions.
  // The SetImageToPredefinedIcon method accepts an index to one of the
  // predefined icon listed in vtkKWIcon.
  // The SetImageToPixels method sets the image using pixel data. It expects
  // a pointer to the pixels and the structure of the image, i.e. its width, 
  // height and the pixel_size (how many bytes per pixel, say 3 for RGB, or
  // 1 for grayscale). If buffer_length = 0, it is computed automatically
  // from the previous parameters. If it is not, it will most likely indicate
  // that the buffer has been encoded using base64 and/or zlib.
  // If pixel_size > 3 (i.e. RGBA), the image is blend the with background
  // color of the widget.
  virtual void SetImageToIcon(vtkKWIcon *icon);
  virtual void SetImageToPredefinedIcon(int icon_index);
  virtual void SetImageToPixels(
    const unsigned char *pixels, int width, int height, int pixel_size,
    unsigned long buffer_length = 0);
  
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
  virtual void AdjustWrapLengthToWidthCallback();

protected:
  vtkKWLabel();
  ~vtkKWLabel();

  virtual void UpdateBindings();
  virtual void UpdateText();

private:
  char* Text;
  int AdjustWrapLengthToWidth;

  vtkKWLabel(const vtkKWLabel&); // Not implemented
  void operator=(const vtkKWLabel&); // Not implemented
};

#endif
