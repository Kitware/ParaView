// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNetworkImageSource.h"
#include "vtkNew.h"
#include "vtkPVSessionServer.h"
#include "vtkProcessModule.h"
#include "vtkTestUtilities.h"

extern int TestJpegNetworkImageSource(int argc, char* argv[])
{
  auto fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/clouds.jpeg");

  if (!vtkProcessModule::Initialize(vtkProcessModule::PROCESS_BATCH, argc, argv))
  {
    std::cerr << "Can not initialize vtkProcessModule" << std::endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkPVSessionServer> pvSession;

  vtkNew<vtkNetworkImageSource> networkImageSource;
  networkImageSource->SetFileName(fileName);
  networkImageSource->UpdateImage();

  if (!vtkProcessModule::Finalize())
  {
    std::cerr << "Can not finalize vtkProcessModule" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
