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
// to decrement/increment (spin) an external value through callbacks.
// The buttons can be laid out vertically (up/down arrows) or horizontally
// (left/right arrows)

#ifndef __vtkKWSpinButtons_h
#define __vtkKWSpinButtons_h

#include "vtkKWCompositeWidget.h"

class vtkKWApplication;
class vtkKWPushButton;

class KWWIDGETS_EXPORT vtkKWSpinButtons : public vtkKWCompositeWidget
{
public:
  static vtkKWSpinButtons* New();
  vtkTypeRevisionMacro(vtkKWSpinButtons,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);
  
  // Description:
  // Get the buttons
  vtkGetObjectMacro(DecrementButton, vtkKWPushButton);
  vtkGetObjectMacro(IncrementButton, vtkKWPushButton);

  // Description:
  // Specifies the commands to associate to the increment and decrement 
  // buttons.
  virtual void SetDecrementCommand(vtkObject *object, const char *method);
  virtual void SetIncrementCommand(vtkObject *object, const char *method);

  // Description:
  // Set/Get the orientation of the spin buttons.
  //BTX
  enum 
  {
    OrientationHorizontal = 0,
    OrientationVertical
  };
  //ETX
  virtual void SetOrientation(int);
  vtkGetMacro(Orientation, int);
  virtual void SetOrientationToHorizontal()
    { this->SetOrientation(vtkKWSpinButtons::OrientationHorizontal); };
  virtual void SetOrientationToVertical()
    { this->SetOrientation(vtkKWSpinButtons::OrientationVertical); };

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

  vtkKWPushButton *DecrementButton;
  vtkKWPushButton *IncrementButton;

  int Orientation;

  virtual void Pack();

private:
  vtkKWSpinButtons(const vtkKWSpinButtons&); // Not implemented
  void operator=(const vtkKWSpinButtons&); // Not implemented
};


#endif



