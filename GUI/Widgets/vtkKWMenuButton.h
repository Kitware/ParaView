/*=========================================================================

  Module:    vtkKWMenuButton.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWMenuButton - an option menu widget
// .SECTION Description
// A widget that looks like a button but when pressed provides a list
// of options that the user can select.

#ifndef __vtkKWMenuButton_h
#define __vtkKWMenuButton_h

#include "vtkKWCoreWidget.h"

class vtkKWMenu;
class vtkKWIcon;

class KWWidgets_EXPORT vtkKWMenuButton : public vtkKWCoreWidget
{
public:
  static vtkKWMenuButton* New();
  vtkTypeRevisionMacro(vtkKWMenuButton,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Set/Get the current entry of this option menu.
  // This can be an image name if any entry in the menu uses an image
  // instead of a label.
  virtual const char *GetValue();
  virtual void SetValue(const char *name);

  // Description:
  // Set/Get the current entry to the previous or next entry.
  // and call the corresponding callback if any.
  virtual void NextValue();
  virtual void PreviousValue();

  // Description:
  // Get the menu object
  vtkGetObjectMacro(Menu, vtkKWMenu);

  // Description
  // Set the indicator On/Off. To be called after creation.
  virtual void SetIndicatorVisibility(int ind);
  virtual int GetIndicatorVisibility();
  vtkBooleanMacro(IndicatorVisibility, int);

  // Description:
  // Set the button width (in chars if text, in pixels if image).
  virtual void SetWidth(int width);
  virtual int GetWidth();
  
  // Description:
  // Set/Get the maximum width of the option menu label.
  // This does not modify the internal value, this is just for display
  // purposes: the option menu button can therefore be automatically
  // shrinked, while the menu associated to it will display all entries
  // correctly.
  // Set width to 0 (default) to prevent auto-cropping.
  virtual void SetMaximumLabelWidth(int);
  vtkGetMacro(MaximumLabelWidth, int);

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
  virtual void TracedVariableChangedCallback(
    const char *, const char *, const char *);

  // Description:
  // Add all the default observers needed by that object, or remove
  // all the observers that were added through AddCallbackCommandObserver.
  // Subclasses can override these methods to add/remove their own default
  // observers, but should call the superclass too.
  virtual void AddCallbackCommandObservers();
  virtual void RemoveCallbackCommandObservers();

protected:
  vtkKWMenuButton();
  ~vtkKWMenuButton();

  vtkGetStringMacro(CurrentValue);
  vtkSetStringMacro(CurrentValue);

  char      *CurrentValue;  
  vtkKWMenu *Menu;
  int       MaximumLabelWidth;

  virtual void UpdateMenuButtonLabel();

  // Description:
  // Processes the events that are passed through CallbackCommand (or others).
  // Subclasses can oberride this method to process their own events, but
  // should call the superclass too.
  virtual void ProcessCallbackCommandEvents(
    vtkObject *caller, unsigned long event, void *calldata);
  
private:
  vtkKWMenuButton(const vtkKWMenuButton&); // Not implemented
  void operator=(const vtkKWMenuButton&); // Not implemented
};


#endif



