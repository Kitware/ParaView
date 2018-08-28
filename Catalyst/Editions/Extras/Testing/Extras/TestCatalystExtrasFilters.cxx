/*=========================================================================

Program:   ParaView
Module:    TestCatalystExtrasFilters.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitializationHelper.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // Initialization
  vtkPVOptions* options = vtkPVOptions::New();
  vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_BATCH, options);
  vtkSMSession* session = vtkSMSession::New();
  vtkProcessModule::GetProcessModule()->RegisterSession(session);
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(session);
  //---------------------------------------------------------------------------

  if (!pxm)
  {
    cout << "Null proxy manager" << endl;
    session->Delete();
    return EXIT_FAILURE;
  }

  static const char* const filters[] = { "PVExtractSelection", "ExtractHistogram", "Glyph",
    "GlyphLegacy", "WarpScalar", "WarpVector", "IntegrateAttributes", "DataSetSurfaceFilter",
    NULL };

  const char* const* name = &filters[0];

  int result = EXIT_SUCCESS;

  while (*name)
  {
    // Create proxy and change main radius value
    vtkSmartPointer<vtkSMProxy> proxy;
    proxy.TakeReference(pxm->NewProxy("filters", *name));
    ++name;

    if (!proxy)
    {
      cout << "Null proxy for " << *name << endl;
      result = EXIT_FAILURE;
      continue;
    }
    proxy->UpdateVTKObjects();
  }

  // *******************************************************************

  session->Delete();
  cout << "Exiting..." << endl;

  vtkInitializationHelper::Finalize();
  options->Delete();
  return result;
}
