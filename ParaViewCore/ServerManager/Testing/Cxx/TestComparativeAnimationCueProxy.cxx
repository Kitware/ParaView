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

#include "vtkInitializationHelper.h"
#include "vtkProcessModule.h"
#include "vtkPVServerOptions.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"

#include "vtkSMComparativeAnimationCueProxy.h"
#include "vtkPVComparativeAnimationCue.h"
#include "vtkSmartPointer.h"
#include <assert.h>

#define ERROR(msg)\
  cerr << "ERROR: " msg << endl;  \
  return 1;


int main(int argc, char* argv[])
{
  // Initialization
  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize(argc, argv,
                                      vtkProcessModule::PROCESS_CLIENT,
                                      options);
  vtkSMSession* session = vtkSMSession::New();
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkProcessModule::GetProcessModule()->RegisterSession(session);
  //---------------------------------------------------------------------------

  vtkSmartPointer<vtkSMComparativeAnimationCueProxy> cueProxy;
  cueProxy.TakeReference(vtkSMComparativeAnimationCueProxy::SafeDownCast(
      pxm->NewProxy("animation", "ComparativeAnimationCue")));
  vtkPVComparativeAnimationCue* cue = cueProxy->GetCue();

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
  session->Delete();

  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  options->Delete();
  return 0;
}
