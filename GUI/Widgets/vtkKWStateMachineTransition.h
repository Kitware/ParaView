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
  // Start the transition. This method should be invoked when the transition
  // is "started" by the state machine, as the state machine is about to 
  // leave the OriginState but has not yet entered the DestinationState.
  // This can prove useful to push bew inputs that rely on the fact that the
  // state machine is still at the OriginState.
  // It will take care of calling the corresponding callbacks (StartCommand)
  // and events (StartEvent). Subclasses that override this method should
  // make sure they call their superclass's Start() method.
  virtual void Start();

  // Description:
  // End the transition. This command should be invoked when the transition
  // is "ended" by the state machine, as the state machine has left the
  // OriginState and already entered the DestinationState. 
  // This can prove useful to push new inputs that rely on the fact that the
  // state machine is now at the DestinationState.
  // It will take care of calling the corresponding callbacks (EndCommand)
  // and events (EndEvent). Subclasses that override this method should
  // make sure they call their superclass's End() method.
  virtual void End();

  // Description:
  // Specifies a command to associate with this transition. This command
  // should be invoked when the transition is "started" by the state machine, 
  // as the state machine is about to leave the OriginState but has not yet
  // entered the DestinationState. State machine (sub)classes should call
  // the Start() method most of the time, which will take care of triggering
  // this callback and firing the StartEvent as well.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetStartCommand(vtkObject *object, const char *method);
  virtual void InvokeStartCommand();
  virtual int HasStartCommand();

  // Description:
  // Specifies a command to associate with this transition. This command
  // should be invoked when the transition is "ended" by the state machine, as
  // the state machine has left the OriginState and already entered
  // the DestinationState. State machine (sub)classes should call
  // the End() method most of the time, which will take care of triggering this
  // callback and firing the EndEvent as well.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetEndCommand(vtkObject *object, const char *method);
  virtual void InvokeEndCommand();
  virtual int HasEndCommand();

  // Description:
  // Events. The StartEvent should be fired when the transition is "started"
  // by the state machine, as the state machine is about to leave the 
  // OriginState but has not yet entered the DestinationState. The "EndEvent"
  // should be fired when the transition is "ended" by the state machine, as
  // the state machine has left the OriginState and already entered the 
  // DestinationState. In both case, State machine (sub)classes should call
  // the corresponding Start() end End() methods most of the time, which will
  // take care of triggering both the callbacks and firing the events.
  //BTX
  enum
  {
    StartEvent = 10000,
    EndEvent,
  };
  //ETX

protected:
  vtkKWStateMachineTransition();
  ~vtkKWStateMachineTransition();

  vtkIdType Id;
  vtkKWStateMachineState *OriginState;
  vtkKWStateMachineInput *Input;
  vtkKWStateMachineState *DestinationState;

  char *EndCommand;
  char *StartCommand;

private:

  static vtkIdType IdCounter;

  vtkKWStateMachineTransition(const vtkKWStateMachineTransition&); // Not implemented
  void operator=(const vtkKWStateMachineTransition&); // Not implemented
};

#endif
