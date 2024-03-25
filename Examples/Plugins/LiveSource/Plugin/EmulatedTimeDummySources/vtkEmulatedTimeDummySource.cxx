// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEmulatedTimeDummySource.h"

#include "vtkConeSource.h"
#include "vtkCubeSource.h"
#include "vtkDataObject.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"
#include "vtkStreamingDemandDrivenPipeline.h"

namespace
{
constexpr unsigned short TIMESTEPS_SIZE = 4;
constexpr double SPHERE_TIMESTEPS[::TIMESTEPS_SIZE] = { 15.50, 16.20, 16.95, 18.10 };
constexpr double CONE_TIMESTEPS[::TIMESTEPS_SIZE] = { 16.70, 17.10, 17.80, 18.40 };
constexpr double CUBE_TIMESTEPS[::TIMESTEPS_SIZE] = { 30.30, 30.80, 31.70, 34.60 };

const double* getCorrespondingTimesteps(int preset)
{
  switch (preset)
  {
    case vtkEmulatedTimeDummySource::SourcePreset::CONE:
      return ::CONE_TIMESTEPS;

    case vtkEmulatedTimeDummySource::SourcePreset::CUBE:
      return ::CUBE_TIMESTEPS;

    case vtkEmulatedTimeDummySource::SourcePreset::SPHERE:
    default:
      return ::SPHERE_TIMESTEPS;
  }
}
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkEmulatedTimeDummySource);

//------------------------------------------------------------------------------
vtkEmulatedTimeDummySource::vtkEmulatedTimeDummySource()
{
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkEmulatedTimeDummySource::~vtkEmulatedTimeDummySource() = default;

//------------------------------------------------------------------------------
int vtkEmulatedTimeDummySource::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//------------------------------------------------------------------------------
int vtkEmulatedTimeDummySource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  double requiredTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  const double* timesteps = ::getCorrespondingTimesteps(this->SourcePresets);
  requiredTime = std::max(std::min(requiredTime, timesteps[::TIMESTEPS_SIZE - 1]), timesteps[0]);
  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);

  double completionRatio =
    (requiredTime - timesteps[0]) / (timesteps[::TIMESTEPS_SIZE - 1] - timesteps[0]);
  switch (this->SourcePresets)
  {
    case SourcePreset::CONE:
    {
      vtkNew<vtkConeSource> cone;
      double height = 1 + 2.5 * completionRatio;
      cone->SetHeight(height);
      cone->Update();
      output->ShallowCopy(cone->GetOutput());
      break;
    }

    case SourcePreset::CUBE:
    {
      vtkNew<vtkCubeSource> cube;
      double size = 1 + 2 * completionRatio;
      cube->SetCenter(0, size, 0);
      cube->Update();
      output->ShallowCopy(cube->GetOutput());
      break;
    }

    case SourcePreset::SPHERE:
    default:
    {
      vtkNew<vtkSphereSource> sphere;
      double theta = 90 + 180 * completionRatio;
      sphere->SetStartTheta(theta);
      sphere->Update();
      output->ShallowCopy(sphere->GetOutput());
      break;
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkEmulatedTimeDummySource::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  const double* timesteps = ::getCorrespondingTimesteps(this->SourcePresets);

  double timeRange[2];
  timeRange[0] = timesteps[0];
  timeRange[1] = timesteps[::TIMESTEPS_SIZE - 1];
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timesteps, ::TIMESTEPS_SIZE);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  return 1;
}

//------------------------------------------------------------------------------
void vtkEmulatedTimeDummySource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SourcePresets: ";
  switch (this->SourcePresets)
  {
    case vtkEmulatedTimeDummySource::SourcePreset::CONE:
      os << "Cone";

    case vtkEmulatedTimeDummySource::SourcePreset::CUBE:
      os << "Cube";

    case vtkEmulatedTimeDummySource::SourcePreset::SPHERE:
    default:
      os << "Sphere";
  }
  os << "\n";
}
