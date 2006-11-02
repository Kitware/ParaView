/*=========================================================================

  Module:    vtkKWStateMachineState.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWStateMachineState - a state machine state.
// .SECTION Description
// This class is the basis for a state machine state. 
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
// vtkKWStateMachine vtkKWStateMachineInput vtkKWStateMachineTransition

#ifndef __vtkKWStateMachineState_h
#define __vtkKWStateMachineState_h

#include "vtkKWObject.h"

class KWWidgets_EXPORT vtkKWStateMachineState : public vtkKWObject
{
public:
  static vtkKWStateMachineState* New();
  vtkTypeRevisionMacro(vtkKWStateMachineState, vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get id.
  vtkGetMacro(Id, vtkIdType);

  // Description:
  // Set/Get simple name.
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

  // Description:
  // Set/Get longer description.
  vtkGetStringMacro(Description);
  vtkSetStringMacro(Description);

protected:
  vtkKWStateMachineState();
  ~vtkKWStateMachineState();

  vtkIdType Id;
  char *Name;
  char *Description;

private:

  static vtkIdType IdCounter;

  vtkKWStateMachineState(const vtkKWStateMachineState&); // Not implemented
  void operator=(const vtkKWStateMachineState&); // Not implemented
};

#endif
