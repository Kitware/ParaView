/*=========================================================================

  Module:    vtkKWPushButton.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWPushButton - push button widget
// .SECTION Description
// A simple widget that represents a push button. 

#ifndef __vtkKWPushButton_h
#define __vtkKWPushButton_h

#include "vtkKWCoreWidget.h"

class vtkKWIcon;

class KWWidgets_EXPORT vtkKWPushButton : public vtkKWCoreWidget
{
public:
  static vtkKWPushButton* New();
  vtkTypeRevisionMacro(vtkKWPushButton,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the contents label.
  virtual void SetText(const char *label);
  virtual char *GetText();

  // Description:
  // Set/Get the text width (in chars if the button has a text contents, or
  // pixels if it has an image contents).
  virtual void SetWidth(int width);
  virtual int GetWidth();
  
  // Description:
  // Set/Get the text height (in chars if the button has a text contents, or
  // pixels if it has an image contents).
  virtual void SetHeight(int height);
  virtual int GetHeight();

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when button is pressed.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetCommand(vtkObject *object, const char *method);

  // Description:
  // Events. The InvokedEvent is triggered when the button is pressed.
  //BTX
  enum
  {
    InvokedEvent = 10000
  };
  //ETX

  // Description:
  // Set/Get the background color of the widget
  virtual void GetBackgroundColor(double *r, double *g, double *b);
  virtual double* GetBackgroundColor();
  virtual void SetBackgroundColor(double r, double g, double b);
  virtual void SetBackgroundColor(double rgb[3])
    { this->SetBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the foreground color of the widget
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
  // Set/Get the active background color of
  // the widget. An element (a widget or portion of a widget) is active
  // if the mouse cursor is positioned over the element and pressing a mouse
  // button will cause some action to occur.
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
  // Set/Get the 3-D effect desired for the widget. 
  // Specifies an alternative relief for the button, to be used when the mouse
  // cursor is over the widget. This option can be used to make toolbar 
  // buttons, by configuring SetRelief to Flat and OverRelief to Raised.
  // Valid constants can be found in vtkKWOptions::ReliefType.
  // If the value of this option is None, then no alternative relief is used
  // when the mouse cursor is over the checkbutton. 
  virtual void SetOverRelief(int);
  virtual int GetOverRelief();
  virtual void SetOverReliefToRaised();
  virtual void SetOverReliefToSunken();
  virtual void SetOverReliefToFlat();
  virtual void SetOverReliefToRidge();
  virtual void SetOverReliefToSolid();
  virtual void SetOverReliefToGroove();
  virtual void SetOverReliefToNone();

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
  virtual void CommandCallback();

protected:
  vtkKWPushButton();
  ~vtkKWPushButton();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  vtkSetStringMacro(ButtonText);
  char* ButtonText;

  char *Command;
  virtual void InvokeCommand();

private:
  vtkKWPushButton(const vtkKWPushButton&); // Not implemented
  void operator=(const vtkKWPushButton&); // Not implemented
};


#endif



