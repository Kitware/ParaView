/*=========================================================================

  Module:    vtkKWStateMachineTransition.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWStateMachineTransition - a state machine transition.
// .SECTION Description
// This class is the basis for a state machine transition. 
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

#ifndef __vtkKWStateMachineTransition_h
#define __vtkKWStateMachineTransition_h

#include "vtkKWObject.h"

class vtkKWStateMachineState;
class vtkKWStateMachineInput;

class KWWidgets_EXPORT vtkKWStateMachineTransition : public vtkKWObject
{
public:
  static vtkKWStateMachineTransition* New();
  vtkTypeRevisionMacro(vtkKWStateMachineTransition, vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get id.
  vtkGetMacro(Id, vtkIdType);

  // Description:
  // Set/Get the state this transition is originating from.
  vtkGetObjectMacro(OriginState, vtkKWStateMachineState);
  virtual void SetOriginState(vtkKWStateMachineState*);

  // Description:
  // Set/Get the input this transition is triggered by.
  vtkGetObjectMacro(Input, vtkKWStateMachineInput);
  virtual void SetInput(vtkKWStateMachineInput*);

  // Description:
  // Set/Get the state this transition is leading to.
  vtkGetObjectMacro(DestinationState, vtkKWStateMachineState);
  virtual void SetDestinationState(vtkKWStateMachineState*);

  // Description:
  // Get if the transition is complete, i.e. it has an originating 
  // state (OriginState), a destination state (DestinationState), and 
  // an input (Input).
  virtual int IsComplete();

  // Description:
  // Trigger the action associated to this transition. In the current
  // implementation, this will call InvokeCommand().
  virtual void TriggerAction();

  // Description:
  // Specifies a command to associate with this transition. This command is 
  // typically invoked when the transition is fired by the state machine. 
  // Note that the state machine will not call InvokeCommand directly, 
  // but will call TriggerAction() instead, which in turn will call
  // InvokeCommand() and other form of events/actions/callbacks associated
  // to this transition.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetCommand(vtkObject *object, const char *method);
  virtual void InvokeCommand();

  // Description:
  // Specifies an event to associate with this transition. This event is 
  // typically invoked when the transition is fired by the state machine. 
  // Note that the state machine will not call InvokeEvent() directly, 
  // but will call TriggerAction() instead, which in turn will call
  // InvokeEvent() and other form of events/actions/callbacks associated
  // to this transition. Defaults to vtkCommand::NoEvent.
  vtkSetMacro(Event, unsigned long);
  vtkGetMacro(Event, unsigned long);

protected:
  vtkKWStateMachineTransition();
  ~vtkKWStateMachineTransition();

  vtkIdType Id;
  vtkKWStateMachineState *OriginState;
  vtkKWStateMachineInput *Input;
  vtkKWStateMachineState *DestinationState;
  unsigned long Event;
  char *Command;

private:

  static vtkIdType IdCounter;

  vtkKWStateMachineTransition(const vtkKWStateMachineTransition&); // Not implemented
  void operator=(const vtkKWStateMachineTransition&); // Not implemented
};

#endif
