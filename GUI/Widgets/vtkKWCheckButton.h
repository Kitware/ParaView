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
// and queried using the GetState and SetState methods.

#ifndef __vtkKWCheckButton_h
#define __vtkKWCheckButton_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class KWWIDGETS_EXPORT vtkKWCheckButton : public vtkKWWidget
{
public:
  static vtkKWCheckButton* New();
  vtkTypeRevisionMacro(vtkKWCheckButton,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set/Get the state of the check button 0 = off 1 = on
  void SetState(int );
  virtual int GetState();

  // Description:
  // Tell the widget whether it should use an indicator (check box)
  void SetIndicator(int ind);

  // Description:
  // Set the text.
  void SetText(const char* txt);
  const char* GetText();

  // Description:
  // Set the variable name.
  vtkGetStringMacro(VariableName);
  virtual void SetVariableName(const char *);

  // Description:
  // Overriden from vtkKWWidget. If blend_color_option = 0 and 
  // image_option = 0, this function is automatically called with the same
  // parameters but blend_color_option = -selectcolor, 
  // image_option = -selectimage, so that the aspect is correct when the
  // button is checked.
  virtual void SetImageOption(int icon_index,
                              const char *blend_color_option = 0,
                              const char *image_option = 0);
  virtual void SetImageOption(vtkKWIcon *icon,
                              const char *blend_color_option = 0,
                              const char *image_option = 0);
  virtual void SetImageOption(const unsigned char *data, 
                              int width, int height, int pixel_size = 4,
                              unsigned long buffer_length = 0,
                              const char *blend_color_option = 0,
                              const char *image_option = 0);
  virtual void SetImageOption(const char *image_name,
                              const char *image_option = 0);

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

  int IndicatorOn;
  char *MyText;
  char *VariableName;

  void Configure();

private:
  vtkKWCheckButton(const vtkKWCheckButton&); // Not implemented
  void operator=(const vtkKWCheckButton&); // Not Implemented
};


#endif



