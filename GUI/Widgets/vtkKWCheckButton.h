/*=========================================================================

  Module:    vtkKWCheckButton.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWCheckButton - check button widget
// .SECTION Description
// A simple widget that represents a check button. It can be modified 
// and queried using the GetSelectedState and SetSelectedState methods.

#ifndef __vtkKWCheckButton_h
#define __vtkKWCheckButton_h

#include "vtkKWCoreWidget.h"

class vtkKWIcon;

class KWWIDGETS_EXPORT vtkKWCheckButton : public vtkKWCoreWidget
{
public:
  static vtkKWCheckButton* New();
  vtkTypeRevisionMacro(vtkKWCheckButton,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set/Get/Toggle the state of the check button 0 = off 1 = on
  virtual void SetSelectedState(int );
  virtual int GetSelectedState();
  vtkBooleanMacro(SelectedState, int);
  virtual void ToggleSelectedState();
  virtual void Select() { this->SetSelectedState(1); };
  virtual void DeSelect() { this->SetSelectedState(0); };

  // Description:
  // Tell the widget whether it should use an indicator (check box)
  virtual void SetIndicator(int ind);
  vtkGetMacro(Indicator, int);
  vtkBooleanMacro(Indicator, int);

  // Description:
  // Set the text.
  virtual void SetText(const char* txt);
  virtual const char* GetText();

  // Description:
  // Set the variable name.
  vtkGetStringMacro(VariableName);
  virtual void SetVariableName(const char *);

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when mouse button 1 is released over the button.
  // The first argument is the object that will have the method called on it.
  // The second argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method
  // is evaluated as a simple command.
  virtual void SetCommand(vtkObject *object, const char *method);

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

protected:

  vtkSetStringMacro(MyText);

  vtkKWCheckButton();
  ~vtkKWCheckButton();

  int Indicator;
  char *MyText;
  char *VariableName;

  void Configure();

private:
  vtkKWCheckButton(const vtkKWCheckButton&); // Not implemented
  void operator=(const vtkKWCheckButton&); // Not Implemented
};


#endif



