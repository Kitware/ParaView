/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVWebInteractionEvent
// .SECTION Description
//

#ifndef __vtkPVWebInteractionEvent_h
#define __vtkPVWebInteractionEvent_h

#include "vtkObject.h"
#include "vtkParaViewWebModule.h" // needed for exports

class VTK_EXPORT vtkPVWebInteractionEvent : public vtkObject
{
public:
  static vtkPVWebInteractionEvent* New();
  vtkTypeMacro(vtkPVWebInteractionEvent, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  enum MouseButton
    {
    LEFT_BUTTON = 0x01,
    MIDDLE_BUTTON = 0x02,
    RIGHT_BUTTON = 0x04,
    };

  enum ModifierKeys
    {
    SHIFT_KEY = 0x01,
    CTRL_KEY = 0x02,
    ALT_KEY = 0x04,
    META_KEY = 0x08
    };

  // Description:
  // Set/Get the mouse buttons state.
  vtkSetMacro(Buttons, unsigned int);
  vtkGetMacro(Buttons, unsigned int);

  // Description:
  // Set/Get modifier state.
  vtkSetMacro(Modifiers, unsigned int);
  vtkGetMacro(Modifiers, unsigned int);

  // Description:
  // Set/Get the chart code.
  vtkSetMacro(KeyCode, char);
  vtkGetMacro(KeyCode, char);

  // Description:
  // Set/Get event position.
  vtkSetMacro(X, double);
  vtkGetMacro(X, double);
  vtkSetMacro(Y, double);
  vtkGetMacro(Y, double);

//BTX
protected:
  vtkPVWebInteractionEvent();
  ~vtkPVWebInteractionEvent();

  unsigned int Buttons;
  unsigned int Modifiers;
  char KeyCode;
  double X;
  double Y;

private:
  vtkPVWebInteractionEvent(const vtkPVWebInteractionEvent&); // Not implemented
  void operator=(const vtkPVWebInteractionEvent&); // Not implemented
//ETX
};

#endif
