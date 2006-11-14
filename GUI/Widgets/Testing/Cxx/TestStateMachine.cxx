#include "vtkKWStateMachine.h"
#include "vtkKWStateMachineState.h"
#include "vtkKWStateMachineInput.h"
#include "vtkKWStateMachineTransition.h"
#include "vtkKWStateMachineDOTWriter.h"
#include "vtkKWStateMachineCluster.h"

int main()
{
  vtkKWStateMachine *state_machine = vtkKWStateMachine::New();

  // States

  vtkKWStateMachineState *state_1 = vtkKWStateMachineState::New();
  state_1->SetName("Start");
  state_machine->AddState(state_1);
  state_1->Delete();

  vtkKWStateMachineState *state_2 = vtkKWStateMachineState::New();
  state_machine->AddState(state_2);
  state_2->Delete();

  vtkKWStateMachineState *state_3 = vtkKWStateMachineState::New();
  state_machine->AddState(state_3);
  state_3->Delete();

  // Inputs

  vtkKWStateMachineInput *input_next = vtkKWStateMachineInput::New();
  input_next->SetName("next");
  state_machine->AddInput(input_next);
  input_next->Delete();

  vtkKWStateMachineInput *input_skip = vtkKWStateMachineInput::New();
  input_skip->SetName("skip");
  state_machine->AddInput(input_skip);
  input_skip->Delete();

  vtkKWStateMachineInput *input_invalid = vtkKWStateMachineInput::New();
  input_invalid->SetName("invalid");
  state_machine->AddInput(input_invalid);
  input_invalid->Delete();

  // Transition: state_1 / next => state_2

  vtkKWStateMachineTransition *trans_a = vtkKWStateMachineTransition::New();
  trans_a->SetOriginState(state_1);
  trans_a->SetInput(input_next);
  trans_a->SetDestinationState(state_2);
  state_machine->AddTransition(trans_a);
  trans_a->Delete();

  // Transition: state_1 / skip => state_3
  // Transition: state_2 / next => state_3
  // Transition: state_3 / next => state_1
  // Transition: state_2 / invalid => state_2

  state_machine->CreateTransition(state_1, input_skip, state_3);
  state_machine->CreateTransition(state_2, input_next, state_3);
  state_machine->CreateTransition(state_3, input_next, state_1);
  state_machine->CreateTransition(state_2, input_invalid, state_2);

  // For I/O purposes, let's group some states inside a cluster

  vtkKWStateMachineCluster *cluster = vtkKWStateMachineCluster::New();
  cluster->AddState(state_2);
  cluster->AddState(state_3);
  state_machine->AddCluster(cluster);
  cluster->Delete();

  // Run the state machine

  state_machine->SetInitialState(state_1);
  state_1->AcceptingOn();
  
  state_machine->PushInput(input_next);    // state_1 to state_2
  state_machine->PushInput(input_invalid); // state_2 to state_2
  state_machine->PushInput(input_next);    // state_2 to state_3

  state_machine->ProcessInputs();

  state_machine->PushInput(input_next);    // state_3 to state_1
  state_machine->PushInput(input_skip);    // state_1 to state_3
  
  state_machine->ProcessInputs();

#if 0
  vtkKWStateMachineDOTWriter *writer = vtkKWStateMachineDOTWriter::New();
  writer->SetInput(state_machine);
  writer->SetGraphLabel("State Machine Example");
  writer->WriteToStream(cout);
  writer->Delete();
#endif

  int res = 0;
  if (state_machine->GetCurrentState() != state_3)
    {
    cout << "Error! The state machine did not reach the expected state!" 
         << endl;
    res = 1;
    }

  state_machine->Delete();

  return res;
}
