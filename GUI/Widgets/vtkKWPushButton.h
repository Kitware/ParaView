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
  // Create the widget.
  virtual void Create();

  // Description:
  // Convenience method to set the contents label.
  virtual void SetText(const char *label);
  virtual char *GetText();

  // Description:
  // Convenience method to set/get the text width (in chars if the button
  // has a text contents, or pixels if it has an image contents).
  virtual void SetWidth(int width);
  virtual int GetWidth();
  
  // Description:
  // Convenience method to set/get the text height (in chars if the button
  // has a text contents, or pixels if it has an image contents).
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
  // Set/Get the anchoring.
  // Specifies how the information in a widget (e.g. text or a bitmap) is to
  // be displayed in the widget.
  // Valid constants can be found in vtkKWTkOptions::AnchorType.
  virtual void SetAnchor(int);
  virtual int GetAnchor();
  virtual void SetAnchorToNorth() 
    { this->SetAnchor(vtkKWTkOptions::AnchorNorth); };
  virtual void SetAnchorToNorthEast() 
    { this->SetAnchor(vtkKWTkOptions::AnchorNorthEast); };
  virtual void SetAnchorToEast() 
    { this->SetAnchor(vtkKWTkOptions::AnchorEast); };
  virtual void SetAnchorToSouthEast() 
    { this->SetAnchor(vtkKWTkOptions::AnchorSouthEast); };
  virtual void SetAnchorToSouth() 
    { this->SetAnchor(vtkKWTkOptions::AnchorSouth); };
  virtual void SetAnchorToSouthWest() 
    { this->SetAnchor(vtkKWTkOptions::AnchorSouthWest); };
  virtual void SetAnchorToWest() 
    { this->SetAnchor(vtkKWTkOptions::AnchorWest); };
  virtual void SetAnchorToNorthWest() 
    { this->SetAnchor(vtkKWTkOptions::AnchorNorthWest); };
  virtual void SetAnchorToCenter() 
    { this->SetAnchor(vtkKWTkOptions::AnchorCenter); };

  // Description:
  // Set/Get the 3-D effect desired for the widget. 
  // Specifies an alternative relief for the button, to be used when the mouse
  // cursor is over the widget. This option can be used to make toolbar 
  // buttons, by configuring SetRelief to Flat and OverRelief to Raised.
  // Valid constants can be found in vtkKWTkOptions::ReliefType.
  // If the value of this option is None, then no alternative relief is used
  // when the mouse cursor is over the checkbutton. 
  virtual void SetOverRelief(int);
  virtual int GetOverRelief();
  virtual void SetOverReliefToRaised() 
    { this->SetOverRelief(vtkKWTkOptions::ReliefRaised); };
  virtual void SetOverReliefToSunken() 
    { this->SetOverRelief(vtkKWTkOptions::ReliefSunken); };
  virtual void SetOverReliefToFlat() 
    { this->SetOverRelief(vtkKWTkOptions::ReliefFlat); };
  virtual void SetOverReliefToRidge() 
    { this->SetOverRelief(vtkKWTkOptions::ReliefRidge); };
  virtual void SetOverReliefToSolid() 
    { this->SetOverRelief(vtkKWTkOptions::ReliefSolid); };
  virtual void SetOverReliefToGroove() 
    { this->SetOverRelief(vtkKWTkOptions::ReliefGroove); };
  virtual void SetOverReliefToNone()
    { this->SetOverRelief(vtkKWTkOptions::ReliefUnknown); };

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

  vtkSetStringMacro(ButtonText);
  char* ButtonText;

  char *Command;
  virtual void InvokeCommand();

private:
  vtkKWPushButton(const vtkKWPushButton&); // Not implemented
  void operator=(const vtkKWPushButton&); // Not implemented
};


#endif



