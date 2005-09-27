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

class KWWIDGETS_EXPORT vtkKWLabel : public vtkKWCoreWidget
{
public:
  static vtkKWLabel* New();
  vtkTypeRevisionMacro(vtkKWLabel,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);
  
  // Description:
  // Set the text on the label.
  virtual void SetText(const char*);
  vtkGetStringMacro(Text);

  // Description:
  // Set/Get width of the label.
  // If an image is being displayed in the label then the value is in screen
  // units; for text it is in characters.
  virtual void SetWidth(int);
  virtual int GetWidth();

  // Description:
  // Set/Get height of the label.
  // If an image is being displayed in the label then the value is in screen
  // units; for text it is in lines of text.
  virtual void SetHeight(int);
  virtual int GetHeight();

  // Description:
  // Set/Get the justification mode.
  // When there are multiple lines of text displayed in a widget, this option
  // determines how the lines line up with each other.
  // Valid constants can be found in vtkKWTkOptions::JustificationType.
  virtual void SetJustification(int);
  virtual int GetJustification();
  virtual void SetJustificationToLeft() 
    { this->SetJustification(vtkKWTkOptions::JustificationLeft); };
  virtual void SetJustificationToCenter() 
    { this->SetJustification(vtkKWTkOptions::JustificationCenter); };
  virtual void SetJustificationToRight() 
    { this->SetJustification(vtkKWTkOptions::JustificationRight); };

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
  // Callbacks. Do not use.
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
