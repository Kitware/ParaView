// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCaveSynchronizedRenderers.h"

#include "vtkCamera.h"
#include "vtkDisplayConfiguration.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPerspectiveTransform.h"
#include "vtkProcessModule.h"
#include "vtkRemotingCoreConfiguration.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkUnsignedCharArray.h"

#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cassert>
#include <cmath>

vtkStandardNewMacro(vtkCaveSynchronizedRenderers);
//----------------------------------------------------------------------------
vtkCaveSynchronizedRenderers::vtkCaveSynchronizedRenderers()
{
  // Allocate one display on creation
  this->SetNumberOfDisplays(1);

  this->SetParallelController(vtkMultiProcessController::GetGlobalController());

  // Initialize using pvx file specified on the command line options.
  auto config = vtkRemotingCoreConfiguration::GetInstance();
  auto displayConfig = vtkRemotingCoreConfiguration::GetInstance()->GetDisplayConfiguration();
  if (!displayConfig)
  {
    vtkErrorMacro("Are you sure vtkCaveSynchronizedRenderers is created on "
                  "an appropriate processes?");
  }
  else
  {
    this->SetNumberOfDisplays(displayConfig->GetNumberOfDisplays());
    const auto rank = this->ParallelController ? this->ParallelController->GetLocalProcessId() : -1;
    if (rank >= 0 && rank < this->NumberOfDisplays)
    {
      if (auto env = displayConfig->GetEnvironment(rank))
      {
        // PutEnv() avoids memory leak.
        vtksys::SystemTools::PutEnv(env);
      }
      if (displayConfig->GetHasCorners(rank))
      {
        this->DefineDisplay(rank, displayConfig->GetLowerLeft(rank).GetData(),
          displayConfig->GetLowerRight(rank).GetData(),
          displayConfig->GetUpperRight(rank).GetData());
      }
    }
    this->SetEyeSeparation(config->GetEyeSeparation());
    this->SetUseOffAxisProjection(config->GetUseOffAxisProjection());
  }
}

//----------------------------------------------------------------------------
void vtkCaveSynchronizedRenderers::HandleStartRender()
{
  this->ImageReductionFactor = 1;
  this->Superclass::HandleStartRender();
  this->InitializeCamera(this->GetRenderer()->GetActiveCamera());
  this->GetRenderer()->ResetCameraClippingRange();
}

//-----------------------------------------------------------------------------
void vtkCaveSynchronizedRenderers::SetNumberOfDisplays(int numberOfDisplays)
{
  if (numberOfDisplays != this->NumberOfDisplays)
  {
    this->Displays.resize(
      numberOfDisplays, { -0.5, -0.5, -0.5, 1.0, 0.5, -0.5, -0.5, 1.0, 0.5, 0.5, -0.5, 1.0 });
    this->NumberOfDisplays = numberOfDisplays;
    this->Modified();
  }
}

//-------------------------------------------------------------------------
bool vtkCaveSynchronizedRenderers::DefineDisplay(
  int idx, double origin[3], double x[3], double y[3])
{
  if (idx >= this->NumberOfDisplays)
  {
    vtkErrorMacro("idx is higher than the number of displays, aborting.");
    return false;
  }

  std::copy(origin, origin + 3, this->Displays[idx].data());
  std::copy(x, x + 3, this->Displays[idx].data() + 4);
  std::copy(y, y + 3, this->Displays[idx].data() + 8);
  if (idx == this->GetParallelController()->GetLocalProcessId())
  {
    std::copy(origin, origin + 3, this->DisplayOrigin.data());
    std::copy(x, x + 3, this->DisplayX.data());
    std::copy(y, y + 3, this->DisplayY.data());
  }
  this->Modified();
  return true;
}

//-------------------------------------------------------------------------
void vtkCaveSynchronizedRenderers::ComputeCamera(vtkCamera* cam)
{
  this->InitializeCamera(cam);
}

//-------------------------------------------------------------------------
void vtkCaveSynchronizedRenderers::InitializeCamera(vtkCamera* camera)
{
  if (!this->CameraInitialized)
  {
    double eyePosition[3] = { 0.0, 0.0, 0.5 };
    camera->SetScreenBottomLeft(this->DisplayOrigin.data());
    camera->SetScreenBottomRight(this->DisplayX.data());
    camera->SetScreenTopRight(this->DisplayY.data());
    camera->SetEyePosition(eyePosition);
    camera->SetEyeSeparation(this->EyeSeparation);
    camera->SetUseOffAxisProjection(this->UseOffAxisProjection);
    this->CameraInitialized = true;
  }
}

//----------------------------------------------------------------------------
void vtkCaveSynchronizedRenderers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfDisplays: " << this->NumberOfDisplays << endl;
  vtkIndent rankIndent = indent.GetNextIndent();
  for (int i = 0; i < this->NumberOfDisplays; ++i)
  {
    os << rankIndent;
    for (int j = 0; j < 12; ++j)
    {
      os << this->Displays[i][j] << " ";
    }
    os << endl;
  }
  os << indent << "Origin: " << this->DisplayOrigin[0] << " " << this->DisplayOrigin[1] << " "
     << this->DisplayOrigin[2] << endl;
  os << indent << "X: " << this->DisplayX[0] << " " << this->DisplayX[1] << " " << this->DisplayX[2]
     << endl;
  os << indent << "Y: " << this->DisplayY[0] << " " << this->DisplayY[1] << " " << this->DisplayY[2]
     << endl;
}

//----------------------------------------------------------------------------
void vtkCaveSynchronizedRenderers::SetRenderer(vtkRenderer* renderer)
{
  this->Superclass::SetRenderer(renderer);
  if (this->NumberOfDisplays != 1)
  {
    return;
  }

  auto displayConfig = vtkRemotingCoreConfiguration::GetInstance()->GetDisplayConfiguration();
  if (displayConfig->GetNumberOfDisplays() != 1)
  {
    return;
  }

  const auto geometry = displayConfig->GetGeometry(0);
  if (!displayConfig->GetHasCorners(0) && geometry[2] != 0)
  {
    vtkCamera* camera = this->GetRenderer() ? this->GetRenderer()->GetActiveCamera() : nullptr;
    double angle = camera ? camera->GetViewAngle() / 2. : 15.;
    double ratio = static_cast<double>(geometry[3]) / geometry[2];
    double z = ratio / tan(vtkMath::RadiansFromDegrees(angle));
    double lowerLeft[3] = { 1., ratio, z };
    double lowerRight[3] = { -1., ratio, z };
    double upperRight[3] = { -1., -ratio, z };
    this->DefineDisplay(0, lowerLeft, lowerRight, upperRight);
  }
}
