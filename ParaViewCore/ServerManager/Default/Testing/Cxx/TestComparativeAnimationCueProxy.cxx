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
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include "vtkPVComparativeAnimationCue.h"
#include "vtkSMComparativeAnimationCueProxy.h"
#include "vtkSmartPointer.h"
#include <assert.h>

#define ERROR(msg)                                                                                 \
  cerr << "ERROR: " msg << endl;                                                                   \
  return 1;

int TestComparativeAnimationCueProxy(int argc, char* argv[])
{
  // Initialization
  vtkPVOptions* options = vtkPVOptions::New();
  vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_CLIENT, options);
  vtkSMSession* session = vtkSMSession::New();
  vtkProcessModule::GetProcessModule()->RegisterSession(session);
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(session);
  //---------------------------------------------------------------------------

  vtkSmartPointer<vtkSMComparativeAnimationCueProxy> cueProxy;
  cueProxy.TakeReference(vtkSMComparativeAnimationCueProxy::SafeDownCast(
    pxm->NewProxy("animation", "ComparativeAnimationCue")));
  cueProxy->UpdateVTKObjects();

  // When no values are added to the cueProxy, we still expect it to work.
  assert(cueProxy->GetValue(0, 0, 10, 10) == -1.0);

  cueProxy->UpdateWholeRange(1, 9);
  assert(cueProxy->GetValue(0, 0, 3, 3) == 1.0);
  assert(cueProxy->GetValue(1, 1, 3, 3) == 5.0);
  assert(cueProxy->GetValue(0, 2, 3, 3) == 7.0);

  cueProxy->UpdateXRange(1, 14, 14);
  assert(cueProxy->GetValue(0, 0, 3, 3) == 1.0);
  assert(cueProxy->GetValue(1, 1, 4, 4) == 14.0);
  assert(cueProxy->GetValue(5, 5, 6, 6) == 9.0);

  cueProxy->UpdateYRange(2, 13, 13);
  assert(cueProxy->GetValue(0, 0, 3, 3) == 1.0);
  assert(cueProxy->GetValue(2, 0, 4, 4) == 13.0);
  assert(cueProxy->GetValue(1, 1, 4, 4) == 14.0);
  assert(cueProxy->GetValue(5, 5, 6, 6) == 9.0);

  cueProxy->UpdateValue(0, 0, 60.0);
  assert(cueProxy->GetValue(0, 0, 3, 3) == 60.0);
  assert(cueProxy->GetValue(2, 0, 4, 4) == 13.0);
  assert(cueProxy->GetValue(1, 1, 4, 4) == 14.0);
  assert(cueProxy->GetValue(5, 5, 6, 6) == 9.0);

  cueProxy->UpdateWholeRange(1, 9);
  assert(cueProxy->GetValue(0, 0, 3, 3) == 1.0);
  assert(cueProxy->GetValue(1, 1, 3, 3) == 5.0);
  assert(cueProxy->GetValue(0, 2, 3, 3) == 7.0);
  session->Delete();

  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  options->Delete();
  return 0;
}
