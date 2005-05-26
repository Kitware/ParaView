/*=========================================================================

  Module:    vtkKWPopupFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWPopupFrame - a popup frame
// .SECTION Description
// A class that provides a frame that can be collapsed as a popup button.

#ifndef __vtkKWPopupFrame_h
#define __vtkKWPopupFrame_h

#include "vtkKWWidget.h"

class vtkKWFrameLabeled;
class vtkKWPopupButton;

class KWWIDGETS_EXPORT vtkKWPopupFrame : public vtkKWWidget
{
public:
  static vtkKWPopupFrame* New();
  vtkTypeRevisionMacro(vtkKWPopupFrame,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Display the frame as a popup. This has to be called before Create().
  vtkSetMacro(PopupMode, int);
  vtkGetMacro(PopupMode, int);
  vtkBooleanMacro(PopupMode, int);
  
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char* args);

  // Description:
  // Access to sub-widgets
  vtkGetObjectMacro(PopupButton, vtkKWPopupButton);
  vtkGetObjectMacro(Frame, vtkKWFrameLabeled);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWPopupFrame();
  ~vtkKWPopupFrame();

  // GUI

  int                     PopupMode;

  vtkKWPopupButton        *PopupButton;
  vtkKWFrameLabeled       *Frame;

private:
  vtkKWPopupFrame(const vtkKWPopupFrame&); // Not implemented
  void operator=(const vtkKWPopupFrame&); // Not Implemented
};

#endif
