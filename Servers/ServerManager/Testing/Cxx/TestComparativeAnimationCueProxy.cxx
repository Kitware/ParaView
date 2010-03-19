/*=========================================================================

  Program:   ParaView
  Module:    TestComparativeAnimationCueProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests vtkSMComparativeAnimationCueProxy to ensure that the parameter updating
// works are expected.

#include "vtkSMComparativeAnimationCueProxy.h"
#include "vtkSmartPointer.h"
#include <assert.h>

#define ERROR(msg)\
  cerr << "ERROR: " msg << endl;  \
  return 1;


int main(int, char**)
{
  vtkSmartPointer<vtkSMComparativeAnimationCueProxy> cue = 
    vtkSmartPointer<vtkSMComparativeAnimationCueProxy>::New();

  // When no values are added to the cue, we still expect it to work.
  assert(cue->GetValue(0, 0, 10, 10) == -1.0);

  cue->UpdateWholeRange(1, 9);
  assert(cue->GetValue(0, 0, 3, 3) == 1.0);
  assert(cue->GetValue(1, 1, 3, 3) == 5.0);
  assert(cue->GetValue(0, 2, 3, 3) == 7.0);

  cue->UpdateXRange(1, 14, 14);
  assert(cue->GetValue(0, 0, 3, 3) == 1.0);
  assert(cue->GetValue(1, 1, 4, 4) == 14.0);
  assert(cue->GetValue(5, 5, 6, 6) == 9.0);

  cue->UpdateYRange(2, 13, 13);
  assert(cue->GetValue(0, 0, 3, 3) == 1.0);
  assert(cue->GetValue(2, 0, 4, 4) == 13.0);
  assert(cue->GetValue(1, 1, 4, 4) == 14.0);
  assert(cue->GetValue(5, 5, 6, 6) == 9.0);

  cue->UpdateValue(0, 0, 60.0);
  assert(cue->GetValue(0, 0, 3, 3) == 60.0);
  assert(cue->GetValue(2, 0, 4, 4) == 13.0);
  assert(cue->GetValue(1, 1, 4, 4) == 14.0);
  assert(cue->GetValue(5, 5, 6, 6) == 9.0);

  cue->UpdateWholeRange(1, 9);
  assert(cue->GetValue(0, 0, 3, 3) == 1.0);
  assert(cue->GetValue(1, 1, 3, 3) == 5.0);
  assert(cue->GetValue(0, 2, 3, 3) == 7.0);

  return 0;
}
