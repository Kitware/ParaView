/*=========================================================================

  Program:   ParaView
  Module:    vtkPEquivalenceSet.cxx

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  Copyright 2013 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.

=========================================================================*/
#include "vtkPEquivalenceSet.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPEquivalenceSet);

vtkPEquivalenceSet::vtkPEquivalenceSet() = default;

vtkPEquivalenceSet::~vtkPEquivalenceSet() = default;

void vtkPEquivalenceSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkPEquivalenceSet::ResolveEquivalences()
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  int myProc = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();

  vtkIntArray* workingSet = vtkIntArray::New();
  workingSet->SetNumberOfComponents(1);

  int tag = 475893745;
  int pivot = (numProcs + 1) / 2;
  while (pivot > 0 && myProc < (pivot * 2))
  {
    int tuples;
    if (myProc >= pivot)
    {
      tuples = this->EquivalenceArray->GetNumberOfTuples();
      controller->Send(&tuples, 1, myProc - pivot, tag + pivot + 0);
      controller->Send(this->EquivalenceArray, myProc - pivot, tag + pivot + 1);
    }
    else if ((myProc + pivot) < numProcs)
    {
      controller->Receive(&tuples, 1, myProc + pivot, tag + pivot + 0);
      workingSet->SetNumberOfTuples(tuples);

      controller->Receive(workingSet, myProc + pivot, tag + pivot + 1);
      while (workingSet->GetNumberOfTuples() > this->EquivalenceArray->GetNumberOfTuples())
      {
        this->EquivalenceArray->InsertNextTuple1(0);
      }
      for (int i = 0; i < workingSet->GetNumberOfTuples(); i++)
      {
        int workingVal = workingSet->GetValue(i);
        if (workingVal == 0)
        {
          continue;
        }
        int existingVal = this->EquivalenceArray->GetValue(i);
        this->EquivalenceArray->SetValue(i, workingVal);
        if (existingVal != 0 && existingVal < workingVal)
        {
          this->EquateInternal(existingVal, workingVal);
        }
      }
    }
    pivot /= 2;
  }
  controller->Broadcast(this->EquivalenceArray, 0);

  this->Superclass::ResolveEquivalences();
  return 1;
}
