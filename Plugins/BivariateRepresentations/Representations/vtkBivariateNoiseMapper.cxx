// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBivariateNoiseMapper.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBivariateNoiseMapperDelegator.h"

#include <chrono>

//----------------------------------------------------------------------------
struct vtkBivariateNoiseMapper::vtkInternals
{
  double Frequency = 30.0;
  double Amplitude = 0.5;
  double Speed = 1.0;
  int NbOfOctaves = 3;
  long StartTime = 0;
  bool Initialized = false;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBivariateNoiseMapper);

//----------------------------------------------------------------------------
vtkBivariateNoiseMapper::vtkBivariateNoiseMapper()
  : Internals(new vtkBivariateNoiseMapper::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkBivariateNoiseMapper::~vtkBivariateNoiseMapper() = default;

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Frequency: " << this->Internals->Frequency << endl;
  os << indent << "Amplitude: " << this->Internals->Amplitude << endl;
  os << indent << "Speed: " << this->Internals->Speed << endl;
  os << indent << "Nb of octaves: " << this->Internals->NbOfOctaves << endl;
}

//----------------------------------------------------------------------------
vtkCompositePolyDataMapperDelegator* vtkBivariateNoiseMapper::CreateADelegator()
{
  return vtkOpenGLBivariateNoiseMapperDelegator::New();
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapper::SetFrequency(double frequency)
{
  this->Internals->Frequency = frequency;
  this->Modified();
}

//----------------------------------------------------------------------------
double vtkBivariateNoiseMapper::GetFrequency() const
{
  return this->Internals->Frequency;
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapper::SetAmplitude(double amplitude)
{
  this->Internals->Amplitude = amplitude;
  this->Modified();
}

//----------------------------------------------------------------------------
double vtkBivariateNoiseMapper::GetAmplitude() const
{
  return this->Internals->Amplitude;
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapper::SetSpeed(double speed)
{
  this->Internals->Speed = speed;
  this->Modified();
}

//----------------------------------------------------------------------------
double vtkBivariateNoiseMapper::GetSpeed() const
{
  return this->Internals->Speed;
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapper::SetNbOfOctaves(int nbOfOctaves)
{
  this->Internals->NbOfOctaves = nbOfOctaves;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkBivariateNoiseMapper::GetNbOfOctaves() const
{
  return this->Internals->NbOfOctaves;
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapper::Render(vtkRenderer* ren, vtkActor* act)
{
  if (!this->Internals->Initialized)
  {
    this->Initialize();
  }
  this->Superclass::Render(ren, act);
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseMapper::Initialize()
{
  if (!this->Internals->Initialized)
  {
    this->Internals->StartTime = std::chrono::steady_clock::now().time_since_epoch().count();
    this->Internals->Initialized = true;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
long vtkBivariateNoiseMapper::GetStartTime() const
{
  return this->Internals->StartTime;
}
