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

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWPushButton : public vtkKWWidget
{
public:
  static vtkKWPushButton* New();
  vtkTypeRevisionMacro(vtkKWPushButton,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Convenience method to set the contents label.
  virtual void SetText(const char *label);
  virtual char *GetText();

  // Description:
  // Convenience method to set/get the text width (in chars).
  virtual void SetWidth(int width);
  virtual int GetWidth();
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWPushButton();
  ~vtkKWPushButton();

  vtkSetStringMacro(ButtonText);
  char* ButtonText;

private:
  vtkKWPushButton(const vtkKWPushButton&); // Not implemented
  void operator=(const vtkKWPushButton&); // Not implemented
};


#endif



