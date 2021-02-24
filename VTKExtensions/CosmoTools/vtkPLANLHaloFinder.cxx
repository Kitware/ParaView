/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLANLHaloFinder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkPLANLHaloFinder.cxx

Copyright (c) 2007, 2009, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007, 2009. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef USE_VTK_COSMO
#define USE_VTK_COSMO
#endif

#include "vtkPLANLHaloFinder.h"

// CosmologyTools includes
#include "CosmoToolsMacros.h"

// VTK includes
#include "vtkCellType.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkDoubleArray.h"
#include "vtkDummyController.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

// halofinder includes
#include "ChainingMesh.h"
#include "CosmoHaloFinderP.h"
#include "FOFHaloProperties.h"
#include "HaloCenterFinder.h"
#include "Partition.h"
#include "SODHalo.h"

// C/C++ includes
#include <cassert>
#include <vector>

namespace HaloFinderInternals
{

class ParticleData
{
public:
  // Input particle data
  std::vector<POSVEL_T> xx, yy, zz, vx, vy, vz, mass, potential;
  std::vector<ID_T> tag;
  std::vector<STATUS_T> status;
  std::vector<MASK_T> mask;

  /**
   * @brief Resizes
   * @param numParticles
   */
  void Resize(const int numParticles)
  {
    this->xx.resize(numParticles);
    this->yy.resize(numParticles);
    this->zz.resize(numParticles);
    this->vx.resize(numParticles);
    this->vy.resize(numParticles);
    this->vz.resize(numParticles);
    this->mass.resize(numParticles);
    this->tag.resize(numParticles);
    this->status.resize(numParticles);
    this->mask.resize(numParticles);
    this->potential.resize(numParticles);
  }

  /**
   * @brief Clears all internal vectors.
   */
  void Clear()
  {
    this->xx.clear();
    this->yy.clear();
    this->zz.clear();
    this->vx.clear();
    this->vy.clear();
    this->vz.clear();
    this->mass.clear();
    this->tag.clear();
    this->status.clear();
    this->potential.clear();
    this->mask.clear();
  }
};

class HaloData
{
public:
  // Computed FOF properties
  std::vector<POSVEL_T> fofMass;     // mass of every halo
  std::vector<POSVEL_T> fofXPos;     // x-component of the FOF position
  std::vector<POSVEL_T> fofYPos;     // y-component of the FOF position
  std::vector<POSVEL_T> fofZPos;     // z-component of the FOF position
  std::vector<POSVEL_T> fofXVel;     // x-component of the FOF velocity
  std::vector<POSVEL_T> fofYVel;     // y-component of the FOF velocity
  std::vector<POSVEL_T> fofZVel;     // z-component of the FOF velocity
  std::vector<POSVEL_T> fofXCofMass; // x-component of the FOF center of mass
  std::vector<POSVEL_T> fofYCofMass; // y-component of the FOF center of mass
  std::vector<POSVEL_T> fofZCofMass; // z-component of the FOF center of mass
  std::vector<POSVEL_T> fofVelDisp;  // velocity dispersion of every halo
  std::vector<int> ExtractedHalos;   // list of halo IDs within the PMin thres.

  /**
   * @brief Clears all internal vectors.
   */
  void Clear()
  {
    this->fofMass.clear();
    this->fofXPos.clear();
    this->fofYPos.clear();
    this->fofZPos.clear();
    this->fofXVel.clear();
    this->fofYVel.clear();
    this->fofZVel.clear();
    this->fofXCofMass.clear();
    this->fofYCofMass.clear();
    this->fofZCofMass.clear();
    this->fofVelDisp.clear();
    this->ExtractedHalos.clear();
  }
};
}

vtkStandardNewMacro(vtkPLANLHaloFinder);

//------------------------------------------------------------------------------
vtkPLANLHaloFinder::vtkPLANLHaloFinder()
{
  this->SetNumberOfOutputPorts(2);

  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->NP = 256;
  this->RL = 100;
  this->Overlap = 5;
  this->BB = .2;
  this->PMin = 100;

  this->ComputeSOD = 0;
  this->CenterFindingMethod = AVERAGE;

  this->HaloFinder = nullptr;
  this->RhoC = cosmotk::RHO_C;
  this->SODMass = cosmotk::SOD_MASS;
  this->MinRadiusFactor = cosmotk::MIN_RADIUS_FACTOR;
  this->MaxRadiusFactor = cosmotk::MAX_RADIUS_FACTOR;
  this->SODBins = cosmotk::NUM_SOD_BINS;
  this->MinFOFSize = cosmotk::MIN_SOD_SIZE;
  this->MinFOFMass = cosmotk::MIN_SOD_MASS;

  this->Particles = new HaloFinderInternals::ParticleData();
  this->Halos = new HaloFinderInternals::HaloData();
}

//------------------------------------------------------------------------------
vtkPLANLHaloFinder::~vtkPLANLHaloFinder()
{
  this->Controller = nullptr;

  if (this->HaloFinder != nullptr)
  {
    delete this->HaloFinder;
  }

  if (this->Particles != nullptr)
  {
    this->Particles->Clear();
    delete this->Particles;
    this->Particles = nullptr;
  }

  if (this->Halos != nullptr)
  {
    this->Halos->Clear();
    delete this->Halos;
    this->Halos = nullptr;
  }

  ;
}

//------------------------------------------------------------------------------
void vtkPLANLHaloFinder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkPLANLHaloFinder::SetController(vtkMultiProcessController* c)
{
  assert("pre: cannot set a nullptr controller!" && (c != nullptr));
  this->Controller = c;
}

//------------------------------------------------------------------------------
vtkMultiProcessController* vtkPLANLHaloFinder::GetController()
{
  return (this->Controller);
}

//------------------------------------------------------------------------------
int vtkPLANLHaloFinder::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  assert("pre: controller should not be nullptr!" && (this->Controller != nullptr));

  // Reset previously calculated data
  this->ResetHaloFinderInternals();

  // STEP 0: Get input object
  vtkInformation* input = inputVector[0]->GetInformationObject(0);
  assert("pre: input information object is nullptr " && (input != nullptr));
  vtkUnstructuredGrid* inputParticles =
    vtkUnstructuredGrid::SafeDownCast(input->Get(vtkDataObject::DATA_OBJECT()));
  assert("pre: input particles is nullptr!" && (inputParticles != nullptr));

  // STEP 1: Get output objects. The output consists of two objects: (1) The
  // particles with halo information attached to it and (2) the halo centers
  // and generic FOF information.
  vtkInformation* output0 = outputVector->GetInformationObject(0);
  assert("pre: output information object is nullptr" && (output0 != nullptr));
  vtkUnstructuredGrid* outputParticles =
    vtkUnstructuredGrid::SafeDownCast(output0->Get(vtkDataObject::DATA_OBJECT()));
  assert("pre: output particles is nullptr" && (outputParticles != nullptr));

  vtkInformation* output1 = outputVector->GetInformationObject(1);
  assert("pre: output information object is nullptr" && (output1 != nullptr));
  vtkUnstructuredGrid* haloCenters =
    vtkUnstructuredGrid::SafeDownCast(output1->Get(vtkDataObject::DATA_OBJECT()));
  assert("pre: halocenters is nullptr" && (haloCenters != nullptr));

  if (inputParticles->GetNumberOfPoints() == 0)
  {
    // Empty input
    return 1;
  }

  // STEP 2: Get 1st output as a shallow-copy of the input & ensure integrity
  outputParticles->ShallowCopy(inputParticles);
  if (!this->CheckOutputIntegrity(outputParticles))
  {
    vtkErrorMacro("Missing arrays from output particles mesh!");
    return 0;
  }

  // STEP 3: Initialize the partitioner used by the halo-finder which uses
  // MPI cartesian topology. Currently, the LANL halofinder assumes
  // MPI_COMM_WORLD!!!!!!. This should be changed in the short future.
  cosmotk::Partition::initialize();

  // Delete previously computed results to re-compute with modified params
  // TODO: We need to get smarter about this in the future, e.g., determine
  // if we really have to run the halo-finder again or just filter differently
  // the results, just change the halo-centers etc.
  if (this->HaloFinder != nullptr)
  {
    delete this->HaloFinder;
  }
  this->HaloFinder = new cosmotk::CosmoHaloFinderP();

  // STEP 4: Compute the FOF halos
  this->ComputeFOFHalos(outputParticles, haloCenters);

  // STEP 5: Compute SOD halos
  if (this->ComputeSOD)
  {
    this->ComputeSODHalos(outputParticles, haloCenters);
  }

  // STEP 6: Synchronize processes
  this->Controller->Barrier();
  return 1;
}

//------------------------------------------------------------------------------
void vtkPLANLHaloFinder::ComputeSODHalos(
  vtkUnstructuredGrid* particles, vtkUnstructuredGrid* fofHaloCenters)
{
#ifdef NDEBUG
  (void)(particles);
#endif
  assert("pre: input particles should not be nullptr" && (particles != nullptr));
  assert("pre: FOF halo-centers should not be nullptr" && (fofHaloCenters != nullptr));

  // STEP 0: Initialize SOD arrays & acquire handles
  // "SODAveragePosition"
  // "SODCenterOfMass"
  // "SODMass"
  // "SODAverageVelocity"
  // "SODVelocityDispersion"
  // "SODRadius"
  this->InitializeSODHaloArrays(fofHaloCenters);
  vtkPointData* PD = fofHaloCenters->GetPointData();
  assert("pre: missing SODAveragePosition array" && PD->HasArray("SODAveragePosition"));
  assert("pre: missing SODCenterOfMass" && PD->HasArray("SODCenterOfMass"));
  assert("pre: missing SODMass" && PD->HasArray("SODMass"));
  assert("pre: missing SODAverageVelocity" && PD->HasArray("SODAverageVelocity"));
  assert("pre: missing SODVelocityDispersion" && PD->HasArray("SODVelocityDispersion"));
  assert("pre: missing SODRadius" && PD->HasArray("SODRadius"));
  vtkDoubleArray* sodPos = vtkDoubleArray::SafeDownCast(PD->GetArray("SODAveragePosition"));
  vtkDoubleArray* sodCofMass = vtkDoubleArray::SafeDownCast(PD->GetArray("SODCenterOfMass"));
  vtkDoubleArray* sodMass = vtkDoubleArray::SafeDownCast(PD->GetArray("SODMass"));
  vtkDoubleArray* sodVelocity = vtkDoubleArray::SafeDownCast(PD->GetArray("SODAverageVelocity"));
  vtkDoubleArray* sodDispersion =
    vtkDoubleArray::SafeDownCast(PD->GetArray("SODVelocityDispersion"));
  vtkDoubleArray* sodRadius = vtkDoubleArray::SafeDownCast(PD->GetArray("SODRadius"));

  // STEP 1: Construct the ChainingMesh
  cosmotk::ChainingMesh* chainMesh = new cosmotk::ChainingMesh(this->RL, this->Overlap,
    cosmotk::CHAIN_SIZE, this->Particles->xx.size(), &this->Particles->xx[0],
    &this->Particles->yy[0], &this->Particles->zz[0]);

  // STEP 2: Loop through all halos and compute SOD halos
  for (unsigned int i = 0; i < this->Halos->ExtractedHalos.size(); ++i)
  {
    int internalHaloIdx = this->Halos->ExtractedHalos[i];
    int haloSize = this->HaloFinder->getHaloCount()[internalHaloIdx];

    double haloMass = this->Halos->fofMass[internalHaloIdx];

    if ((haloMass < this->MinFOFMass) || (haloSize < this->MinFOFSize))
    {
      continue;
    }

    cosmotk::SODHalo* sod = new cosmotk::SODHalo();
    sod->setParameters(chainMesh, this->SODBins, this->RL, this->NP, this->RhoC, this->SODMass,
      this->RhoC, this->MinRadiusFactor, this->MaxRadiusFactor);
    sod->setParticles(this->Particles->xx.size(), &(this->Particles->xx[0]),
      &(this->Particles->yy[0]), &(this->Particles->zz[0]), &(this->Particles->vx[0]),
      &(this->Particles->vy[0]), &(this->Particles->vz[0]), &(this->Particles->mass[0]),
      &(this->Particles->tag[0]));

    double center[3];
    fofHaloCenters->GetPoint(i, center);
    sod->createSODHalo(this->HaloFinder->getHaloCount()[internalHaloIdx], center[0], center[1],
      center[2], this->Halos->fofXVel[internalHaloIdx], this->Halos->fofYVel[internalHaloIdx],
      this->Halos->fofZVel[internalHaloIdx], this->Halos->fofMass[internalHaloIdx]);

    if (sod->SODHaloSize() > 0)
    {
      POSVEL_T pos[3];
      POSVEL_T cofmass[3];
      POSVEL_T mass;
      POSVEL_T vel[3];
      POSVEL_T disp;
      POSVEL_T radius = sod->SODRadius();

      sod->SODAverageLocation(pos);
      sod->SODCenterOfMass(cofmass);
      sod->SODMass(&mass);
      sod->SODAverageVelocity(vel);
      sod->SODVelocityDispersion(&disp);

      sodPos->SetTuple3(i, pos[0], pos[1], pos[2]);
      sodCofMass->SetTuple3(i, cofmass[0], cofmass[1], cofmass[2]);
      sodMass->SetValue(i, mass);
      sodVelocity->SetTuple3(i, vel[0], vel[1], vel[2]);
      sodDispersion->SetValue(i, disp);
      sodRadius->SetValue(i, radius);
    }

    delete sod;
  } // END for all halos within the PMIN threshold

  // STEP 3: De-allocate Chain mesh
  delete chainMesh;
}

//------------------------------------------------------------------------------
void vtkPLANLHaloFinder::InitializeSODHaloArrays(vtkUnstructuredGrid* haloCenters)
{
  assert("pre: centers is nullptr" && (haloCenters != nullptr));

  // TODO: Again, these arrays should match the type of POSVEL_T

  // STEP 0: Allocate arrays
  vtkDoubleArray* averagePosition = vtkDoubleArray::New();
  averagePosition->SetName("SODAveragePosition");
  averagePosition->SetNumberOfComponents(3);
  averagePosition->SetNumberOfTuples(haloCenters->GetNumberOfPoints());

  vtkDoubleArray* centerOfMass = vtkDoubleArray::New();
  centerOfMass->SetName("SODCenterOfMass");
  centerOfMass->SetNumberOfComponents(3);
  centerOfMass->SetNumberOfTuples(haloCenters->GetNumberOfPoints());

  vtkDoubleArray* mass = vtkDoubleArray::New();
  mass->SetName("SODMass");
  mass->SetNumberOfComponents(1);
  mass->SetNumberOfTuples(haloCenters->GetNumberOfPoints());

  vtkDoubleArray* averageVelocity = vtkDoubleArray::New();
  averageVelocity->SetName("SODAverageVelocity");
  averageVelocity->SetNumberOfComponents(3);
  averageVelocity->SetNumberOfTuples(haloCenters->GetNumberOfPoints());

  vtkDoubleArray* velDispersion = vtkDoubleArray::New();
  velDispersion->SetName("SODVelocityDispersion");
  velDispersion->SetNumberOfComponents(1);
  velDispersion->SetNumberOfTuples(haloCenters->GetNumberOfPoints());

  vtkDoubleArray* radius = vtkDoubleArray::New();
  radius->SetName("SODRadius");
  radius->SetNumberOfComponents(1);
  radius->SetNumberOfTuples(haloCenters->GetNumberOfPoints());

  // STEP 1: Initialize
  for (vtkIdType halo = 0; halo < haloCenters->GetNumberOfPoints(); ++halo)
  {
    averagePosition->SetTuple3(halo, 0.0, 0.0, 0.0);
    centerOfMass->SetTuple3(halo, 0.0, 0.0, 0.0);
    mass->SetValue(halo, 0.0);
    averageVelocity->SetTuple3(halo, 0.0, 0.0, 0.0);
    velDispersion->SetValue(halo, 0.0);
    radius->SetValue(halo, 0.0);
  } // END for all extracted FOF halos

  // STEP 2: Add arrays to halo-centers
  haloCenters->GetPointData()->AddArray(averagePosition);
  averagePosition->Delete();
  haloCenters->GetPointData()->AddArray(centerOfMass);
  centerOfMass->Delete();
  haloCenters->GetPointData()->AddArray(mass);
  mass->Delete();
  haloCenters->GetPointData()->AddArray(averageVelocity);
  averageVelocity->Delete();
  haloCenters->GetPointData()->AddArray(velDispersion);
  velDispersion->Delete();
  haloCenters->GetPointData()->AddArray(radius);
  radius->Delete();
}

//------------------------------------------------------------------------------
void vtkPLANLHaloFinder::VectorizeData(vtkUnstructuredGrid* particles)
{
  assert("pre: input particles mesh is nullptr" && (particles != nullptr));

  // TODO: VTK data arrays must match what POSVEL_T & ID_T are.

  vtkPoints* points = particles->GetPoints();
  assert("pre: points should not be nullptr!" && (points != nullptr));

  vtkFloatArray* velocity =
    vtkFloatArray::SafeDownCast(particles->GetPointData()->GetArray("velocity"));
  assert("pre: velocity should not be nullptr!" && (velocity != nullptr));

  vtkFloatArray* pmass = vtkFloatArray::SafeDownCast(particles->GetPointData()->GetArray("mass"));
  assert("pre: pmass should not be nullptr!" && (pmass != nullptr));

  vtkIdTypeArray* uid = vtkIdTypeArray::SafeDownCast(particles->GetPointData()->GetArray("tag"));
  assert("pre: uid should not be nullptr!" && (uid != nullptr));

  vtkIntArray* owner = vtkIntArray::SafeDownCast(particles->GetPointData()->GetArray("ghost"));
  assert("pre: owner should not be nullptr" && (owner != nullptr));

  vtkIntArray* haloTag = vtkIntArray::New();
  haloTag->SetName("HaloID");
  haloTag->SetNumberOfComponents(1);
  haloTag->SetNumberOfTuples(points->GetNumberOfPoints());
  int* haloTagPtr = static_cast<int*>(haloTag->GetVoidPointer(0));

  vtkIdType numParticles = points->GetNumberOfPoints();
  this->Particles->Resize(numParticles);

  for (vtkIdType idx = 0; idx < numParticles; ++idx)
  {
    // Extract position vector
    this->Particles->xx[idx] = points->GetPoint(idx)[0];
    this->Particles->yy[idx] = points->GetPoint(idx)[1];
    this->Particles->zz[idx] = points->GetPoint(idx)[2];

    // Extract velocity vector
    this->Particles->vx[idx] = velocity->GetComponent(idx, 0);
    this->Particles->vy[idx] = velocity->GetComponent(idx, 1);
    this->Particles->vz[idx] = velocity->GetComponent(idx, 2);

    // Extract the mass
    this->Particles->mass[idx] = pmass->GetValue(idx);

    // Extract global particle ID information & also setup global-to-local map
    this->Particles->tag[idx] = uid->GetValue(idx);

    // Extract status
    this->Particles->status[idx] = owner->GetValue(idx);

    // Initialize all halo IDs to -1, i.e., all particles are not in halos
    haloTagPtr[idx] = -1;
  } // END for all particles

  // Remove the ghost array since the status vector now has that information
  particles->GetPointData()->RemoveArray("ghost");

  // Add halo IDs to the particles
  particles->GetPointData()->AddArray(haloTag);
  haloTag->Delete();
}

//------------------------------------------------------------------------------
void vtkPLANLHaloFinder::ComputeFOFHalos(
  vtkUnstructuredGrid* particles, vtkUnstructuredGrid* haloCenters)
{
  assert("pre: input particles mesh is nullptr" && (particles != nullptr));
  assert("pre: halo-centers data-structure is nullptr" && (haloCenters != nullptr));
  assert("pre: halo-finder is not allocated!" && (this->HaloFinder != nullptr));

  // STEP 0: Vectorize the data
  this->VectorizeData(particles);

  // STEP 1: Allocate potential & mask vectors required by the halo-finder
  this->Particles->potential.resize(particles->GetNumberOfPoints());
  this->Particles->mask.resize(particles->GetNumberOfPoints());

  // STEP 2: Initialize halo-finder parameters
  this->HaloFinder->setParameters("", this->RL, this->Overlap, this->NP, this->PMin, this->BB);
  this->HaloFinder->setParticles(this->Particles->xx.size(), &this->Particles->xx[0],
    &this->Particles->yy[0], &this->Particles->zz[0], &this->Particles->vx[0],
    &this->Particles->vy[0], &this->Particles->vz[0], &this->Particles->potential[0],
    &this->Particles->tag[0], &this->Particles->mask[0], &this->Particles->status[0]);

  // STEP 3: Execute the halo-finder
  this->HaloFinder->executeHaloFinder();
  this->HaloFinder->collectHalos();
  //  this->HaloFinder->mergeHalos();

  // STEP 4: Calculate basic FOF halo properties
  this->ComputeFOFHaloProperties();

  // STEP 5: Filter out halos within the PMin threshold
  int numberOfFOFHalos = this->HaloFinder->getNumberOfHalos();
  int* fofHaloCount = this->HaloFinder->getHaloCount();

  for (int halo = 0; halo < numberOfFOFHalos; ++halo)
  {
    // Disregard halos that do not fall within the PMin threshold
    if (fofHaloCount[halo] >= this->PMin)
    {
      this->Halos->ExtractedHalos.push_back(halo);
    } // END if haloSize is within threshold
  }   // END for all halos

  // STEP 6: Loop through the extracted halos and do the following:
  //          1. Compute the halo-centers
  //          2. Attach halo-properties to each halo-center,e.g.,mass,vel,etc.
  //          3. Mark all particles within each halo
  this->InitializeHaloCenters(haloCenters, this->Halos->ExtractedHalos.size());
  assert("pre: halo mass array not present!" && haloCenters->GetPointData()->HasArray("HaloMass"));
  assert("pre: AverageVelocity array not present!" &&
    haloCenters->GetPointData()->HasArray("AverageVelocity"));
  assert("pre: VelocityDispersion array not present!" &&
    haloCenters->GetPointData()->HasArray("VelocityDispersion"));
  assert("pre: HaloID array not present!" && haloCenters->GetPointData()->HasArray("HaloID"));

  vtkPoints* pnts = haloCenters->GetPoints();
  vtkPointData* PD = haloCenters->GetPointData();
  double* haloMass = static_cast<double*>(PD->GetArray("HaloMass")->GetVoidPointer(0));
  double* haloAverageVel = static_cast<double*>(PD->GetArray("AverageVelocity")->GetVoidPointer(0));
  double* haloVelDisp = static_cast<double*>(PD->GetArray("VelocityDispersion")->GetVoidPointer(0));
  int* haloId = static_cast<int*>(PD->GetArray("HaloID")->GetVoidPointer(0));

  double center[3];
  for (unsigned int halo = 0; halo < this->Halos->ExtractedHalos.size(); ++halo)
  {
    int haloIdx = this->Halos->ExtractedHalos[halo];
    assert("pre: haloIdx is out-of-bounds!" && (haloIdx >= 0) &&
      (haloIdx < static_cast<int>(this->Halos->fofMass.size())));

    this->MarkHaloParticlesAndGetCenter(halo, haloIdx, center, particles);
    pnts->SetPoint(halo, center);

    haloMass[halo] = this->Halos->fofMass[haloIdx];
    haloVelDisp[halo] = this->Halos->fofVelDisp[haloIdx];
    haloAverageVel[halo * 3] = this->Halos->fofXVel[haloIdx];
    haloAverageVel[halo * 3 + 1] = this->Halos->fofYVel[haloIdx];
    haloAverageVel[halo * 3 + 2] = this->Halos->fofZVel[haloIdx];
    haloId[halo] = halo;
  } // END for all extracted halos
}

//------------------------------------------------------------------------------
void vtkPLANLHaloFinder::MarkHaloParticlesAndGetCenter(const unsigned int halo,
  const int internalHaloIdx, double center[3], vtkUnstructuredGrid* particles)
{
  assert("pre: output particles data-structured is nullptr" && (particles != nullptr));
  assert(
    "pre: particles must have a HaloID array" && (particles->GetPointData()->HasArray("HaloID")));

  // STEP 0: Get pointer to the haloTags array
  int* haloTags =
    static_cast<int*>(particles->GetPointData()->GetArray("HaloID")->GetVoidPointer(0));

  // STEP 1: Construct FOF halo properties (TODO: we can modularize this part)
  int numberOfHalos = this->HaloFinder->getNumberOfHalos();
  int* fofHalos = this->HaloFinder->getHalos();
  int* fofHaloCount = this->HaloFinder->getHaloCount();
  int* fofHaloList = this->HaloFinder->getHaloList();

  cosmotk::FOFHaloProperties* fof = new cosmotk::FOFHaloProperties();
  fof->setHalos(numberOfHalos, fofHalos, fofHaloCount, fofHaloList);
  fof->setParameters("", this->RL, this->Overlap, this->BB);
  fof->setParticles(this->Particles->xx.size(), &this->Particles->xx[0], &this->Particles->yy[0],
    &this->Particles->zz[0], &this->Particles->vx[0], &this->Particles->vy[0],
    &this->Particles->vz[0], &this->Particles->mass[0], &this->Particles->potential[0],
    &this->Particles->tag[0], &this->Particles->mask[0], &this->Particles->status[0]);

  // STEP 2: Get the particle halo information for the given halo with the given
  // internal halo idx. The halo finder find halos in [0, N]. However, we
  // filter out the halos that are not within the PMin threshold. This yields
  // halos [0, M] where M < N. The internalHaloIdx is w.r.t. the [0,N] range
  // while the halo is w.r.t. the [0,M] range.
  long size = fofHaloCount[internalHaloIdx];
  POSVEL_T* xLocHalo = new POSVEL_T[size];
  POSVEL_T* yLocHalo = new POSVEL_T[size];
  POSVEL_T* zLocHalo = new POSVEL_T[size];
  POSVEL_T* xVelHalo = new POSVEL_T[size];
  POSVEL_T* yVelHalo = new POSVEL_T[size];
  POSVEL_T* zVelHalo = new POSVEL_T[size];
  POSVEL_T* massHalo = new POSVEL_T[size];
  ID_T* id = new ID_T[size];
  int* actualIdx = new int[size];
  fof->extractInformation(internalHaloIdx, actualIdx, xLocHalo, yLocHalo, zLocHalo, xVelHalo,
    yVelHalo, zVelHalo, massHalo, id);

  // STEP 3: Mark particles within the halo with the the given halo tag/ID.
  for (int haloParticleIdx = 0; haloParticleIdx < size; ++haloParticleIdx)
  {
    haloTags[actualIdx[haloParticleIdx]] = halo;
  } // END for all haloParticles

  // STEP 4: Find the center
  switch (this->CenterFindingMethod)
  {
    case CENTER_OF_MASS:
      assert("pre:center of mass has not been constructed correctly!" &&
        (static_cast<int>(this->Halos->fofXCofMass.size()) == numberOfHalos));
      assert("pre:center of mass has not been constructed correctly!" &&
        (static_cast<int>(this->Halos->fofYCofMass.size()) == numberOfHalos));
      assert("pre:center of mass has not been constructed correctly!" &&
        (static_cast<int>(this->Halos->fofZCofMass.size()) == numberOfHalos));
      center[0] = this->Halos->fofXCofMass[internalHaloIdx];
      center[1] = this->Halos->fofYCofMass[internalHaloIdx];
      center[2] = this->Halos->fofZCofMass[internalHaloIdx];
      break;
    case AVERAGE:
      assert("pre:average halo center has not been constructed correctly!" &&
        (static_cast<int>(this->Halos->fofXPos.size()) == numberOfHalos));
      assert("pre:average halo center has not been constructed correctly!" &&
        (static_cast<int>(this->Halos->fofYPos.size()) == numberOfHalos));
      assert("pre:average halo center has not been constructed correctly!" &&
        (static_cast<int>(this->Halos->fofZPos.size()) == numberOfHalos));
      center[0] = this->Halos->fofXPos[internalHaloIdx];
      center[1] = this->Halos->fofYPos[internalHaloIdx];
      center[2] = this->Halos->fofZPos[internalHaloIdx];
      break;
    case MBP:
    case MCP:
    {
      int centerIndex = -1;
      POTENTIAL_T minPotential;

      cosmotk::HaloCenterFinder centerFinder;
      centerFinder.setParticles(size, xLocHalo, yLocHalo, zLocHalo, massHalo, id);
      // TODO FIXME - find out what the zeroes should be
      centerFinder.setParameters(this->BB, 0.0, this->Overlap, 0.0, 0, 0.0, 0.0, 0.0, 0.0);

      if (this->CenterFindingMethod == MBP)
      {
        centerIndex = (size < cosmotk::MBP_THRESHOLD)
          ? centerFinder.mostBoundParticleN2(&minPotential)
          : centerFinder.mostBoundParticleAStar(&minPotential);
      } // END if MBP
      else
      {
        centerIndex = (size < cosmotk::MCP_THRESHOLD)
          ? centerFinder.mostConnectedParticleN2()
          : centerFinder.mostConnectedParticleChainMesh();
      }

      int pidx = actualIdx[centerIndex];
      assert("pre: particle index is out-of-bounds" && (pidx >= 0) &&
        (pidx < particles->GetNumberOfPoints()));
      particles->GetPoint(pidx, center);
    }
    break;
    default:
      vtkErrorMacro("Undefined center-finding method!");
  }

  delete[] xLocHalo;
  delete[] yLocHalo;
  delete[] zLocHalo;
  delete[] xVelHalo;
  delete[] yVelHalo;
  delete[] zVelHalo;
  delete[] massHalo;
  delete[] id;
  delete[] actualIdx;
  delete fof;
}

//------------------------------------------------------------------------------
void vtkPLANLHaloFinder::InitializeHaloCenters(vtkUnstructuredGrid* haloCenters, unsigned int N)
{
  // TODO: Note the vtkArrays here should match what POSVEL_T, ID_T are set
  haloCenters->Allocate(N, 0);

  vtkPoints* pnts = vtkPoints::New();
  pnts->SetDataTypeToDouble();
  pnts->SetNumberOfPoints(N);

  vtkDoubleArray* haloMass = vtkDoubleArray::New();
  haloMass->SetName("HaloMass");
  haloMass->SetNumberOfComponents(1);
  haloMass->SetNumberOfTuples(N);
  double* haloMassArray = static_cast<double*>(haloMass->GetVoidPointer(0));

  vtkDoubleArray* averageVelocity = vtkDoubleArray::New();
  averageVelocity->SetName("AverageVelocity");
  averageVelocity->SetNumberOfComponents(3);
  averageVelocity->SetNumberOfTuples(N);
  double* velArray = static_cast<double*>(averageVelocity->GetVoidPointer(0));

  vtkDoubleArray* velDispersion = vtkDoubleArray::New();
  velDispersion->SetName("VelocityDispersion");
  velDispersion->SetNumberOfComponents(1);
  velDispersion->SetNumberOfTuples(N);
  double* velDispArray = static_cast<double*>(velDispersion->GetVoidPointer(0));

  vtkIntArray* haloIdx = vtkIntArray::New();
  haloIdx->SetName("HaloID");
  haloIdx->SetNumberOfComponents(1);
  haloIdx->SetNumberOfTuples(N);
  int* haloIdxArray = static_cast<int*>(haloIdx->GetVoidPointer(0));

  for (unsigned int i = 0; i < N; ++i)
  {
    vtkIdType vertexIdx = i;
    haloCenters->InsertNextCell(VTK_VERTEX, 1, &vertexIdx);
    velArray[i * 3] = velArray[i * 3 + 1] = velArray[i * 3 + 2] = velDispArray[i] = 0.0;
    haloIdxArray[i] = -1;
    haloMassArray[i] = 0.0;
  }

  haloCenters->SetPoints(pnts);
  pnts->Delete();
  haloCenters->GetPointData()->AddArray(averageVelocity);
  averageVelocity->Delete();
  haloCenters->GetPointData()->AddArray(velDispersion);
  velDispersion->Delete();
  haloCenters->GetPointData()->AddArray(haloIdx);
  haloIdx->Delete();
  haloCenters->GetPointData()->AddArray(haloMass);
  haloMass->Delete();
}

//------------------------------------------------------------------------------
void vtkPLANLHaloFinder::ComputeFOFHaloProperties()
{
  assert("pre: haloFinder is nullptr!" && (this->HaloFinder != nullptr));

  int numberOfHalos = this->HaloFinder->getNumberOfHalos();
  int* fofHalos = this->HaloFinder->getHalos();
  int* fofHaloCount = this->HaloFinder->getHaloCount();
  int* fofHaloList = this->HaloFinder->getHaloList();

  cosmotk::FOFHaloProperties* fof = new cosmotk::FOFHaloProperties();
  fof->setHalos(numberOfHalos, fofHalos, fofHaloCount, fofHaloList);
  fof->setParameters("", this->RL, this->Overlap, this->BB);
  fof->setParticles(this->Particles->xx.size(), &this->Particles->xx[0], &this->Particles->yy[0],
    &this->Particles->zz[0], &this->Particles->vx[0], &this->Particles->vy[0],
    &this->Particles->vz[0], &this->Particles->mass[0], &this->Particles->potential[0],
    &this->Particles->tag[0], &this->Particles->mask[0], &this->Particles->status[0]);

  // Compute average halo position if that's what will be used as the halo
  // center position
  if (this->CenterFindingMethod == AVERAGE)
  {
    fof->FOFPosition(&this->Halos->fofXPos, &this->Halos->fofYPos, &this->Halos->fofZPos);
  }
  // Compute center of mass of every FOF halo if that's what will be used as
  // the halo center
  else if (this->CenterFindingMethod == CENTER_OF_MASS)
  {
    fof->FOFCenterOfMass(
      &this->Halos->fofXCofMass, &this->Halos->fofYCofMass, &this->Halos->fofZCofMass);
  }

  fof->FOFHaloMass(&this->Halos->fofMass);
  fof->FOFVelocityDispersion(
    &this->Halos->fofXVel, &this->Halos->fofYVel, &this->Halos->fofZVel, &this->Halos->fofVelDisp);

  // Sanity checks!
  assert("post: FOF mass property not correctly computed!" &&
    (static_cast<int>(this->Halos->fofMass.size()) == numberOfHalos));
  assert("post: FOF x-velocity component not correctly computed!" &&
    (static_cast<int>(this->Halos->fofXVel.size()) == numberOfHalos));
  assert("post: FOF y-velocity component not correctly computed!" &&
    (static_cast<int>(this->Halos->fofYVel.size()) == numberOfHalos));
  assert("post: FOF z-velocity component not correctly computed!" &&
    (static_cast<int>(this->Halos->fofZVel.size()) == numberOfHalos));
  assert("post: FOF velocity dispersion not correctly computed!" &&
    (static_cast<int>(this->Halos->fofVelDisp.size()) == numberOfHalos));

  delete fof;
}

//------------------------------------------------------------------------------
void vtkPLANLHaloFinder::ResetHaloFinderInternals()
{
  // input particle information
  this->Particles->xx.resize(0);
  this->Particles->yy.resize(0);
  this->Particles->zz.resize(0);
  this->Particles->vx.resize(0);
  this->Particles->vy.resize(0);
  this->Particles->vz.resize(0);
  this->Particles->mass.resize(0);
  this->Particles->potential.resize(0);
  this->Particles->tag.resize(0);
  this->Particles->status.resize(0);
  this->Particles->mask.resize(0);

  // computed FOF properties
  this->Halos->fofMass.resize(0);
  this->Halos->fofXPos.resize(0);
  this->Halos->fofYPos.resize(0);
  this->Halos->fofZPos.resize(0);
  this->Halos->fofXVel.resize(0);
  this->Halos->fofYVel.resize(0);
  this->Halos->fofZVel.resize(0);
  this->Halos->fofXCofMass.resize(0);
  this->Halos->fofYCofMass.resize(0);
  this->Halos->fofZCofMass.resize(0);
  this->Halos->fofVelDisp.resize(0);
  this->Halos->ExtractedHalos.resize(0);
}

//------------------------------------------------------------------------------
bool vtkPLANLHaloFinder::CheckOutputIntegrity(vtkUnstructuredGrid* outputParticles)
{
  assert("pre: particle mesh is nullptr" && (outputParticles != nullptr));
  if (!outputParticles->GetPointData()->HasArray("velocity") ||
    !outputParticles->GetPointData()->HasArray("mass") ||
    !outputParticles->GetPointData()->HasArray("tag") ||
    !outputParticles->GetPointData()->HasArray("ghost"))
  {
    vtkErrorMacro(<< "The input data does not have one or more of "
                  << "the following point arrays: velocity, mass, tag, or ghost.");
    return false;
  }
  return true;
}
