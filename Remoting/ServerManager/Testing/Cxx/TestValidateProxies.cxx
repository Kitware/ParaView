/*=========================================================================

Program:   ParaView
Module:    TestValidateProxies.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * This test iterates over defined proxies and verifies that they can be instantiated.
 */

#include "vtkInitializationHelper.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkProcessModule.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

#include "vtkDummyController.h"

#include <algorithm>
#include <set>
#include <string>
#include <utility>

int TestValidateProxies(int argc, char* argv[])
{
  vtkNew<vtkDummyController> contr;
  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  // Create a new session.
  vtkNew<vtkSMSession> session;
  auto pxm = session->GetSessionProxyManager();
  auto pdm = pxm->GetProxyDefinitionManager();

  // Some proxies should not be instantiated or depend on a proxy that might not be present
  // in the current edition.
  using PairType = std::pair<std::string, std::string>;
  std::set<PairType> exceptions;
  exceptions.insert(PairType("internal_writers", "FileSeriesWriter"));
  exceptions.insert(PairType("internal_writers", "ParallelFileSeriesWriter"));
  exceptions.insert(PairType("internal_writers", "ParallelSerialWriter"));
  exceptions.insert(PairType("internal_writers", "ParallelWriterBase"));
  exceptions.insert(PairType("writers", "PPLYWriter"));
  exceptions.insert(PairType("internal_writers", "XMLDataObjectWriterBase"));
  exceptions.insert(PairType("internal_views", "XYChartViewBase"));
  exceptions.insert(PairType("internal_views", "XYChartViewBase4Axes"));
  exceptions.insert(PairType("internal_readers", "VisItSeriesReaderBase"));
  exceptions.insert(PairType("extract_writers", "JPG"));
  exceptions.insert(PairType("extract_writers", "PNG"));
  if (!pdm->HasDefinition("writers", "PPLYWriter"))
  {
    exceptions.insert(PairType("extract_writers", "PLY"));
  }
  exceptions.insert(PairType("extract_writers", "CinemaVolumetricPNG"));
  if (!pdm->HasDefinition("misc", "SaveAnimationExtracts"))
  {
    exceptions.insert(PairType("pythontracing", "PythonStateOptions"));
  }

  // requires reader factory
  exceptions.insert(PairType("sources", "EnsembleDataReader"));

  // reports errors when required Python modules are missing.
  exceptions.insert(PairType("sources", "OpenPMDReader"));
  (void)argc;

  int exitCode = EXIT_SUCCESS;
  vtkInitializationHelper::Initialize(argv[0], vtkProcessModule::PROCESS_CLIENT);

  int counter = 0;
  vtkSmartPointer<vtkPVProxyDefinitionIterator> defnIter;
  defnIter.TakeReference(pdm->NewIterator());
  for (defnIter->InitTraversal(); !defnIter->IsDoneWithTraversal(); defnIter->GoToNextItem())
  {
    auto key = PairType(defnIter->GetGroupName(), defnIter->GetProxyName());
    if (exceptions.find(key) != exceptions.end())
    {
      vtkLogF(INFO, "Skipping: (`%s`, `%s`)", key.first.c_str(), key.second.c_str());
      continue;
    }

    ++counter;
    vtkLogF(INFO, "Creating: (`%s`, `%s`)", key.first.c_str(), key.second.c_str());
    auto proxy = pxm->NewProxy(key.first.c_str(), key.second.c_str());
    if (!proxy)
    {
      vtkLogF(ERROR, "ERROR: Failed to create (`%s`, `%s`)", key.first.c_str(), key.second.c_str());
      exitCode = EXIT_FAILURE;
    }
    else
    {
      proxy->UpdateVTKObjects();
      proxy->Delete();
    }
  }
  pxm = nullptr;
  pdm = nullptr;
  vtkInitializationHelper::Finalize();

  vtkLogF(INFO, "Validated %d proxy definitions (excluding %d)", counter,
    static_cast<int>(exceptions.size()));
  return exitCode;
}
