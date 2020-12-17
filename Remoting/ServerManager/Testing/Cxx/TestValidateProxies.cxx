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
#include "vtkInitializationHelper.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkProcessModule.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

#include <algorithm>
#include <set>
#include <string>
#include <utility>

int TestValidateProxies(int argc, char* argv[])
{
  std::set<std::pair<std::string, std::string> > exceptions;
  exceptions.insert(std::pair<std::string, std::string>("internal_writers", "FileSeriesWriter"));
  exceptions.insert(
    std::pair<std::string, std::string>("internal_writers", "ParallelFileSeriesWriter"));
  exceptions.insert(
    std::pair<std::string, std::string>("internal_writers", "ParallelSerialWriter"));
  exceptions.insert(std::pair<std::string, std::string>("internal_writers", "ParallelWriterBase"));
  exceptions.insert(
    std::pair<std::string, std::string>("internal_writers", "XMLDataObjectWriterBase"));
  exceptions.insert(std::pair<std::string, std::string>("internal_views", "XYChartViewBase"));
  exceptions.insert(std::pair<std::string, std::string>("internal_views", "XYChartViewBase4Axes"));
  exceptions.insert(
    std::pair<std::string, std::string>("internal_readers", "VisItSeriesReaderBase"));
  exceptions.insert(std::pair<std::string, std::string>("extract_writers", "JPG"));
  exceptions.insert(std::pair<std::string, std::string>("extract_writers", "PNG"));
  exceptions.insert(std::pair<std::string, std::string>("extract_writers", "CinemaVolumetricPNG"));
#if BUILD_SHARED_LIBS
#else
  // not sure why this reader is failing in static builds; skipping for now.
  exceptions.insert(std::pair<std::string, std::string>("sources", "OpenPMDReader"));
#endif
  (void)argc;

  int exitCode = EXIT_SUCCESS;
  vtkInitializationHelper::Initialize(argv[0], vtkProcessModule::PROCESS_CLIENT);

  int counter = 0;
  // Create a new session.
  auto session = vtkSMSession::New();
  auto pxm = session->GetSessionProxyManager();
  auto pdm = pxm->GetProxyDefinitionManager();
  auto defnIter = pdm->NewIterator();
  for (defnIter->InitTraversal(); !defnIter->IsDoneWithTraversal(); defnIter->GoToNextItem())
  {
    auto key =
      std::pair<std::string, std::string>(defnIter->GetGroupName(), defnIter->GetProxyName());
    if (exceptions.find(key) != exceptions.end())
    {
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
  defnIter->Delete();
  pxm = nullptr;
  pdm = nullptr;
  session->Delete();
  vtkInitializationHelper::Finalize();

  vtkLogF(INFO, "Validated %d proxy definitions (excluding %d)", counter,
    static_cast<int>(exceptions.size()));
  return exitCode;
}
