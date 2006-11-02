/*=========================================================================

  Module:    vtkKWStateMachineCluster.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWStateMachineCluster - a state machine cluster.
// .SECTION Description
// This class is the basis for a state machine cluster, i.e. a means
// to logicall group states together. 
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
// vtkKWStateMachine vtkKWStateMachineState

#ifndef __vtkKWStateMachineCluster_h
#define __vtkKWStateMachineCluster_h

#include "vtkKWObject.h"

class vtkKWStateMachineState;
class vtkKWStateMachineClusterInternals;

class KWWidgets_EXPORT vtkKWStateMachineCluster : public vtkKWObject
{
public:
  static vtkKWStateMachineCluster* New();
  vtkTypeRevisionMacro(vtkKWStateMachineCluster, vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get id.
  vtkGetMacro(Id, vtkIdType);

  // Description:
  // Set/Get simple name.
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

  // Description:
  // Add a state.
  // Return 1 on success, 0 otherwise.
  virtual int AddState(vtkKWStateMachineState *state);
  virtual int HasState(vtkKWStateMachineState *state);
  virtual int GetNumberOfStates();
  virtual vtkKWStateMachineState* GetNthState(int rank);

protected:
  vtkKWStateMachineCluster();
  ~vtkKWStateMachineCluster();

  vtkIdType Id;
  char *Name;

  // Description:
  // Remove state(s).
  virtual void RemoveState(vtkKWStateMachineState *state);
  virtual void RemoveAllStates();

  // PIMPL Encapsulation for STL containers
  //BTX
  vtkKWStateMachineClusterInternals *Internals;
  //ETX

private:

  static vtkIdType IdCounter;

  vtkKWStateMachineCluster(const vtkKWStateMachineCluster&); // Not implemented
  void operator=(const vtkKWStateMachineCluster&); // Not implemented
};

#endif
