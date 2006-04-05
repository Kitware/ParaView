/*=========================================================================

  Module:    vtkKWSpinButtons.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWSpinButtons - A set of spin-buttons.
// .SECTION Description
// This widget implements a small set of two buttons that can be used
// to switch to the next or previous value of an external variable through
// callbacks.
// The buttons can be set to display up/down or left/right arrows, and laid
// out vertically or horizontally.
// The 'previous' button is mapped to the up/left arrow, the 'next' button
// is mapped to the 'down/right' arrow.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWSpinButtons_h
#define __vtkKWSpinButtons_h

#include "vtkKWCompositeWidget.h"

class vtkKWPushButton;

class KWWidgets_EXPORT vtkKWSpinButtons : public vtkKWCompositeWidget
{
public:
  static vtkKWSpinButtons* New();
  vtkTypeRevisionMacro(vtkKWSpinButtons,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();
  
  // Description:
  // Get the buttons
  vtkGetObjectMacro(PreviousButton, vtkKWPushButton);
  vtkGetObjectMacro(NextButton, vtkKWPushButton);

  // Description:
  // Specifies the commands to associate to the next and previous 
  // buttons.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetPreviousCommand(vtkObject *object, const char *method);
  virtual void SetNextCommand(vtkObject *object, const char *method);

  // Description:
  // Set/Get the arrow orientation of the spin buttons.
  // If set to horizontal, left/right arrows will be used. If set to
  // vertical, up/down arrows will be used.
  //BTX
  enum 
  {
    ArrowOrientationHorizontal = 0,
    ArrowOrientationVertical
  };
  //ETX
  virtual void SetArrowOrientation(int);
  vtkGetMacro(ArrowOrientation, int);
  virtual void SetArrowOrientationToHorizontal()
    { this->SetArrowOrientation(
      vtkKWSpinButtons::ArrowOrientationHorizontal); };
  virtual void SetArrowOrientationToVertical()
    { this->SetArrowOrientation(
      vtkKWSpinButtons::ArrowOrientationVertical); };

  // Description:
  // Set/Get the layout of the spin buttons.
  // If set to horizontal, the 'previous' button is packed to the 
  // left of the 'next' button. If set to vertical, the 'previous' button
  // is packed on top of the 'next' button.
  //BTX
  enum 
  {
    LayoutOrientationHorizontal = 0,
    LayoutOrientationVertical
  };
  //ETX
  virtual void SetLayoutOrientation(int);
  vtkGetMacro(LayoutOrientation, int);
  virtual void SetLayoutOrientationToHorizontal()
    { this->SetLayoutOrientation(
      vtkKWSpinButtons::LayoutOrientationHorizontal); };
  virtual void SetLayoutOrientationToVertical()
    { this->SetLayoutOrientation(
      vtkKWSpinButtons::LayoutOrientationVertical); };

  // Description:
  // Set/Get the padding that will be applied around each buttons.
  // (default to 0).
  virtual void SetButtonsPadX(int);
  vtkGetMacro(ButtonsPadX, int);
  virtual void SetButtonsPadY(int);
  vtkGetMacro(ButtonsPadY, int);

  // Description:
  // Set the buttons width/height.
  // No effects if called before Create()
  virtual void SetButtonsWidth(int w);
  virtual int GetButtonsWidth();
  virtual void SetButtonsHeight(int h);
  virtual int GetButtonsHeight();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWSpinButtons();
  ~vtkKWSpinButtons();

  vtkKWPushButton *PreviousButton;
  vtkKWPushButton *NextButton;

  int ArrowOrientation;
  int LayoutOrientation;

  int ButtonsPadX;
  int ButtonsPadY;

  virtual void Pack();
  virtual void UpdateArrowOrientation();

private:
  vtkKWSpinButtons(const vtkKWSpinButtons&); // Not implemented
  void operator=(const vtkKWSpinButtons&); // Not implemented
};


#endif



