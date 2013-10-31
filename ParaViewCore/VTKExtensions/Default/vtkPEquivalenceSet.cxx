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
#include "vtkObjectFactory.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"

vtkStandardNewMacro (vtkPEquivalenceSet);

vtkPEquivalenceSet::vtkPEquivalenceSet ()
{
}

vtkPEquivalenceSet::~vtkPEquivalenceSet ()
{
}

void vtkPEquivalenceSet::PrintSelf (ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf (os, indent);
}

int vtkPEquivalenceSet::ResolveEquivalences ()
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController ();
  int myProc = controller->GetLocalProcessId ();
  int numProcs = controller->GetNumberOfProcesses ();

  vtkIntArray* workingSet = vtkIntArray::New ();

  int tag = 475893745;
  int pivot = (numProcs + 1) / 2;
  while (pivot > 1)
    {
    if (myProc >= pivot)
      {
      controller->Send (this->EquivalenceArray, myProc - pivot, tag + pivot);
      }
    else if ((myProc + pivot) < numProcs)
      {
      controller->Receive (workingSet, myProc + pivot, tag + pivot);
      while (workingSet->GetNumberOfTuples () > this->EquivalenceArray->GetNumberOfTuples ())
        {
        this->EquivalenceArray->InsertNextTuple1 (-1);
        }
      for (int i = 0; i < workingSet->GetNumberOfTuples (); i ++)
        {
        int existingVal = this->EquivalenceArray->GetValue (i);
        int workingVal = workingSet->GetValue (i);
        if (existingVal < 0 || workingVal < existingVal)
          {
          this->EquivalenceArray->SetValue (i, workingVal);
          }
        }
      }
    pivot /= 2;
    }
  controller->Broadcast (this->EquivalenceArray, 0);

  this->Superclass::ResolveEquivalences ();
}
