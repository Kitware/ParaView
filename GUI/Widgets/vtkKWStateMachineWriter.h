/*=========================================================================

  Module:    vtkKWStateMachineWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWStateMachineWriter - a state machine writer base-class.
// .SECTION Description
// This class is the basis for a state machine writer. 
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
// vtkKWStateMachineDOTWriter vtkKWStateMachine

#ifndef __vtkKWStateMachineWriter_h
#define __vtkKWStateMachineWriter_h

#include "vtkObject.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives

class vtkKWStateMachine;

class KWWidgets_EXPORT vtkKWStateMachineWriter : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkKWStateMachineWriter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get state machine to write.
  vtkGetObjectMacro(Input, vtkKWStateMachine);
  virtual void SetInput(vtkKWStateMachine*);

  // Description:
  // Set/Get if the writer should output transitions originating and leading
  // to the same node (self loops).
  vtkSetMacro(WriteSelfLoop, int);
  vtkGetMacro(WriteSelfLoop, int);
  vtkBooleanMacro(WriteSelfLoop, int);

protected:
  vtkKWStateMachineWriter();
  ~vtkKWStateMachineWriter();

  vtkKWStateMachine *Input;
  int WriteSelfLoop;

private:

  vtkKWStateMachineWriter(const vtkKWStateMachineWriter&); // Not implemented
  void operator=(const vtkKWStateMachineWriter&); // Not implemented
};

#endif
