// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkInitializationHelper.h"
#include "vtkNew.h"
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTransferFunctionManager.h"

#include <cassert>
#include <sstream>

int TestTransferFunctionManager(int argc, char* argv[])
{
  (void)argc;
  vtkInitializationHelper::Initialize(argv[0], vtkProcessModule::PROCESS_CLIENT);

  // Create a new session.
  vtkSMSession* session = vtkSMSession::New();
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMProxy* colorTF = mgr->GetColorTransferFunction("arrayOne", pxm);
  if (colorTF == nullptr)
  {
    cerr << "ERROR: Failed at line " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  // colorTF must match on multiple calls.
  if (colorTF != mgr->GetColorTransferFunction("arrayOne", pxm))
  {
    cerr << "ERROR: Failed at line " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  // colorTF must be different for different arrays.
  if (colorTF == mgr->GetColorTransferFunction("arrayTwo", pxm))
  {
    cerr << "ERROR: Failed at line " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  vtkSMProxy* opacityTF = mgr->GetOpacityTransferFunction("arrayOne", pxm);
  if (!opacityTF)
  {
    cerr << "ERROR: Failed at line " << __LINE__ << endl;
    return EXIT_FAILURE;
  }
  if (opacityTF != mgr->GetOpacityTransferFunction("arrayOne", pxm))
  {
    cerr << "ERROR: Failed at line " << __LINE__ << endl;
    return EXIT_FAILURE;
  }
  if (opacityTF == mgr->GetOpacityTransferFunction("arrayTwo", pxm))
  {
    cerr << "ERROR: Failed at line " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  if (vtkSMPropertyHelper(colorTF, "ScalarOpacityFunction").GetAsProxy() != opacityTF)
  {
    cerr << "ERROR: Failed at line " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  // **** Test scalar bar API ****

  // Create a view.
  vtkSMProxy* view = pxm->NewProxy("views", "RenderView");
  view->UpdateVTKObjects();

  vtkSMProxy* sbProxy = mgr->GetScalarBarRepresentation(nullptr, nullptr);
  assert(sbProxy == nullptr);

  sbProxy = mgr->GetScalarBarRepresentation(colorTF, view);
  if (sbProxy == nullptr)
  {
    cerr << "ERROR: Failed at line " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  if (sbProxy != mgr->GetScalarBarRepresentation(colorTF, view))
  {
    cerr << "ERROR: Failed at line " << __LINE__ << endl;
    return EXIT_FAILURE;
  }
  view->Delete();

  session->Delete();
  vtkInitializationHelper::Finalize();
  return EXIT_SUCCESS;
}
