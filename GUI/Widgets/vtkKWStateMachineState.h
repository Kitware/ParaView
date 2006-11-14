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

  // Description:
  // Enter the state. This method should be invoked by the state machine when
  // it enters this state. It will take care of calling the corresponding 
  // callbacks (EnterCommand) and events (EnterEvent). Subclasses that
  // override this method should make sure they call their superclass's 
  // Enter() method.
  virtual void Enter();

  // Description:
  // Leave the state. This method should be invoked by the state machine when
  // it leaves this state. It will take care of calling the corresponding 
  // callbacks (LeaveCommand) and events (LeaveEvent). Subclasses that
  // override this method should make sure they call their superclass's 
  // Leave() method.
  virtual void Leave();

  // Description:
  // Specifies a command to associate with this state. This command
  // should be invoked by the state machine when it enters this state.
  // State machine (sub)classes should call the Enter() method most of the
  // time, which will take care of triggering this callback and firing
  // the EnterEvent as well.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetEnterCommand(vtkObject *object, const char *method);
  virtual void InvokeEnterCommand();
  virtual int HasEnterCommand();

  // Description:
  // Specifies a command to associate with this state. This command
  // should be invoked by the state machine when it leaves this state.
  // State machine (sub)classes should call the Leave() method most of the
  // time, which will take care of triggering this callback and firing
  // the EnterEvent as well.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetLeaveCommand(vtkObject *object, const char *method);
  virtual void InvokeLeaveCommand();
  virtual int HasLeaveCommand();

  // Description:
  // Events. The EnterEvent should be fired when the state machine enters 
  // this state. The LeaveEvent should be fired when the state machine
  // leaves this state. In both case, state machine (sub)classes should call
  // the corresponding Enter() end Leave() methods most of the time, which will
  // take care of triggering both the callbacks and firing the events.
  //BTX
  enum
  {
    EnterEvent = 10000,
    LeaveEvent
  };
  //ETX

  // Description:
  // Set/Get if the set is an accepting state. This is mainly used for
  // display or IO purposes (see vtkKWStateMachineDOTWriter).
  vtkBooleanMacro(Accepting, int);
  vtkGetMacro(Accepting, int);
  vtkSetMacro(Accepting, int);

protected:
  vtkKWStateMachineState();
  ~vtkKWStateMachineState();

  vtkIdType Id;
  char *Name;
  char *Description;
  int Accepting;

  char *EnterCommand;
  char *LeaveCommand;

private:

  static vtkIdType IdCounter;

  vtkKWStateMachineState(const vtkKWStateMachineState&); // Not implemented
  void operator=(const vtkKWStateMachineState&); // Not implemented
};

#endif
