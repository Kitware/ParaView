/*=========================================================================

  Module:    vtkKWStateMachineInput.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWStateMachineInput - a state machine input.
// .SECTION Description
// This class is the basis for a state machine input. 
// A state machine is defined by a set of states, a set of inputs and a
// transition matrix that defines for each pair of (state,input) what is
// the next state to assume.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.
// .SECTION See Also
// vtkKWStateMachine vtkKWStateMachineState vtkKWStateMachineTransition

#ifndef __vtkKWStateMachineInput_h
#define __vtkKWStateMachineInput_h

#include "vtkKWObject.h"

class KWWidgets_EXPORT vtkKWStateMachineInput : public vtkKWObject
{
public:
  static vtkKWStateMachineInput* New();
  vtkTypeRevisionMacro(vtkKWStateMachineInput, vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get id.
  vtkGetMacro(Id, vtkIdType);

  // Description:
  // Set/Get simple name.
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

protected:
  vtkKWStateMachineInput();
  ~vtkKWStateMachineInput();

  vtkIdType Id;
  char *Name;

private:

  static vtkIdType IdCounter;

  vtkKWStateMachineInput(const vtkKWStateMachineInput&); // Not implemented
  void operator=(const vtkKWStateMachineInput&); // Not implemented
};

#endif
