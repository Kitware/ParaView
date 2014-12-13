/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPANLHaloFinder.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPANLHaloFinder.h"

#include "vtkCellType.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkLongLongArray.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

#include "CosmoHaloFinderP.h"
#include "FOFHaloProperties.h"
#include "ParticleDistribute.h"
#include "ParticleExchange.h"
#include "Partition.h"
#include "SubHaloFinder.h"
#include "HaloCenterFinder.h"

#include <cassert>
#include <vector>

namespace {
// magic number taken from BasicDefinition.h in the halo finder code
static const double GRAVITY_C = 43.015e-10;
static const ID_T   MBP_THRESHOLD = 100;
static const ID_T   MCP_THRESHOLD = 100;

class ExtractHalo
{
public:
  ExtractHalo(int numHalos, int* haloCounts, cosmotk::FOFHaloProperties* fof)
  {
    this->size = 0;
    this->counts = haloCounts;
    this->fofProperties = fof;
    int maxNumParticles = 0;
    for (int i = 0; i < numHalos; ++i)
      {
      if (haloCounts[i] > maxNumParticles)
        {
        maxNumParticles = haloCounts[i];
        }
      }
    this->actualIndex.resize(maxNumParticles);
    this->xLoc.resize(maxNumParticles);
    this->yLoc.resize(maxNumParticles);
    this->zLoc.resize(maxNumParticles);
    this->xVel.resize(maxNumParticles);
    this->yVel.resize(maxNumParticles);
    this->zVel.resize(maxNumParticles);
    this->mass.resize(maxNumParticles);
    this->id.resize(maxNumParticles);
  }

  void SetCurrentHalo(int haloIdx)
  {
    this->size = this->counts[haloIdx];

    fofProperties->extractInformation(haloIdx,&this->actualIndex[0],
        &this->xLoc[0],&this->yLoc[0],&this->zLoc[0],&this->xVel[0],
        &this->yVel[0],&this->zVel[0],&this->mass[0],&this->id[0]);
  }

  void SetParticles(cosmotk::SubHaloFinder& subFinder)
  {
    subFinder.setParticles(
          this->size,&this->xLoc[0],&this->yLoc[0],&this->zLoc[0],
        &this->xVel[0],&this->yVel[0],&this->zVel[0],&this->mass[0],&this->id[0]);
  }

  void SetParticles(cosmotk::FOFHaloProperties& properties)
  {
    properties.setParticles(
          this->size,&this->xLoc[0],&this->yLoc[0],&this->zLoc[0],
        &this->xVel[0],&this->yVel[0],&this->zVel[0],&this->mass[0],&this->id[0]);
  }

  void SetParticles(cosmotk::HaloCenterFinder& centerFinder)
  {
    centerFinder.setParticles(this->size,&this->xLoc[0],&this->yLoc[0],
        &this->zLoc[0],&this->mass[0],&this->id[0]);
  }

  int GetActualIndex(size_t i)
  {
    return this->actualIndex[i];
  }

  int GetNumberOfParticlesInCurrentHalo() { return this->size; }

private:
  int* counts;
  cosmotk::FOFHaloProperties* fofProperties;

  int size;
  std::vector< int > actualIndex;
  std::vector< POSVEL_T > xLoc;
  std::vector< POSVEL_T > yLoc;
  std::vector< POSVEL_T > zLoc;
  std::vector< POSVEL_T > xVel;
  std::vector< POSVEL_T > yVel;
  std::vector< POSVEL_T > zVel;
  std::vector< POSVEL_T > mass;
  std::vector< ID_T > id;
};
}

class vtkPANLHaloFinder::vtkInternals
{
public:
  cosmotk::CosmoHaloFinderP* haloFinder;
  // halo finder input arrays
  std::vector< POSVEL_T > xx;
  std::vector< POSVEL_T > yy;
  std::vector< POSVEL_T > zz;
  std::vector< POSVEL_T > vx;
  std::vector< POSVEL_T > vy;
  std::vector< POSVEL_T > vz;
  std::vector< POSVEL_T > mass;
  std::vector< ID_T > tag;
  std::vector< MASK_T > mask;
  std::vector< POTENTIAL_T > potential;
  std::vector< STATUS_T > status;

  cosmotk::FOFHaloProperties* fof;
  // fof halo properties output arrays
  std::vector< int > center;
  std::vector< POSVEL_T > fofMass;
  std::vector< POSVEL_T > fofXPos;
  std::vector< POSVEL_T > fofYPos;
  std::vector< POSVEL_T > fofZPos;
  std::vector< POSVEL_T > fofXCofMass;
  std::vector< POSVEL_T > fofYCofMass;
  std::vector< POSVEL_T > fofZCofMass;
  std::vector< POSVEL_T > fofXVel;
  std::vector< POSVEL_T > fofYVel;
  std::vector< POSVEL_T > fofZVel;
  std::vector< POSVEL_T > fofVelDisp;

  vtkInternals()
  {
    this->fof = NULL;
    this->haloFinder = NULL;
  }

  ~vtkInternals()
  {
    if (this->fof)
      {
      delete this->fof;
      }
    if (this->haloFinder)
      {
      delete this->haloFinder;
      }
  }

  void reserveForInputData(vtkIdType numPts)
  {
    this->xx.resize(numPts);
    this->yy.resize(numPts);
    this->zz.resize(numPts);
    this->vx.resize(numPts);
    this->vy.resize(numPts);
    this->vz.resize(numPts);
    this->tag.resize(numPts);
  }
  void clear()
  {
    this->xx.clear();
    this->yy.clear();
    this->zz.clear();
    this->vx.clear();
    this->vy.clear();
    this->vz.clear();
    this->tag.clear();
  }
};

vtkStandardNewMacro(vtkPANLHaloFinder)

vtkPANLHaloFinder::vtkPANLHaloFinder()
{
  this->Internal = new vtkPANLHaloFinder::vtkInternals;
  this->Controller = vtkMultiProcessController::GetGlobalController();
  this->SetNumberOfOutputPorts(3);
  this->RunSubHaloFinder = false;
  this->RL = 256;
  this->DistanceConvertFactor = 1.0;
  this->MassConvertFactor = 1.0;
  this->DeadSize = 8;
  this->ParticleMass = 1.307087181e+09;
  this->BB = 0.1679999998;
  this->AlphaFactor = 1.0;
  this->BetaFactor = 0.0;
  this->NP = 1024;
  this->NMin = 1;
  this->PMin = 10000;
  this->MinFOFSubhaloSize = 10000;
  this->MinCandidateSize = 200;
  this->NumSPHNeighbors = 64;
  this->NumNeighbors = 20;
}

vtkPANLHaloFinder::~vtkPANLHaloFinder()
{
  delete this->Internal;
}

void vtkPANLHaloFinder::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkPANLHaloFinder::RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *)
{
  return 1;
}

int vtkPANLHaloFinder::RequestData(vtkInformation *, vtkInformationVector **inVector, vtkInformationVector *outVector)
{
  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::GetData(inVector[0],0);
  assert(grid);
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outVector,0);
  assert(output);
  vtkUnstructuredGrid* fofProperties = vtkUnstructuredGrid::GetData(outVector,1);
  assert(fofProperties);
  vtkUnstructuredGrid* subFofProperties = vtkUnstructuredGrid::GetData(outVector,2);
  assert(subFofProperties);

  cosmotk::Partition::initialize();

  this->DistributeInput(grid);
  this->CreateGhostParticles();
  this->ExecuteHaloFinder(output,fofProperties);
  this->FindCenters(fofProperties);
  if (this->RunSubHaloFinder)
    {
    this->ExecuteSubHaloFinder(output,subFofProperties);
    }
  return 1;
}

void vtkPANLHaloFinder::DistributeInput(vtkUnstructuredGrid *input)
{
  vtkPointData* pd = input->GetPointData();
  assert(pd);
  vtkDataArray* vx = pd->GetArray("vx");
  assert(vx);
  vtkDataArray* vy = pd->GetArray("vy");
  assert(vy);
  vtkDataArray* vz = pd->GetArray("vz");
  assert(vz);
  vtkDataArray* id = pd->GetArray("id");
  assert(id);
  const vtkIdType numParticlesBefore = input->GetNumberOfPoints();
  this->Internal->reserveForInputData(numParticlesBefore);
  double point[3];
  for (vtkIdType i = 0; i < numParticlesBefore; ++i)
    {
    input->GetPoint(i,point);
    this->Internal->xx[i] = point[0];
    this->Internal->yy[i] = point[1];
    this->Internal->zz[i] = point[2];
    this->Internal->vx[i] = vx->GetTuple1(i);
    this->Internal->vy[i] = vy->GetTuple1(i);
    this->Internal->vz[i] = vz->GetTuple1(i);
    this->Internal->tag[i] = id->GetTuple1(i);
    }

  cosmotk::ParticleDistribute particleDistribute;
  particleDistribute.setParameters("",this->RL,"RECORD");
  particleDistribute.setConvertParameters(this->DistanceConvertFactor,this->MassConvertFactor);
  particleDistribute.initialize();
  particleDistribute.setParticles(&this->Internal->xx,&this->Internal->yy,
      &this->Internal->zz,&this->Internal->vx,&this->Internal->vy,
      &this->Internal->vz,&this->Internal->mass,&this->Internal->tag);
  particleDistribute.distributeGIOParticles();
  // with particles redistriubuted, number of them on current process may change
  const vtkIdType numParticlesAfter = this->Internal->xx.size();
  for (vtkIdType i = 0; i < numParticlesAfter; ++i)
    {
    this->Internal->mass[i] = this->ParticleMass;
    }
  this->Internal->potential.resize(numParticlesAfter);
  this->Internal->mask.resize(numParticlesAfter);
}

void vtkPANLHaloFinder::CreateGhostParticles()
{
  cosmotk::ParticleExchange particleExchange;
  particleExchange.setParameters(this->RL,this->DeadSize);
  particleExchange.initialize();
  particleExchange.setParticles(
        &this->Internal->xx,&this->Internal->yy,&this->Internal->zz,
        &this->Internal->vx,&this->Internal->vy,&this->Internal->vz,
        &this->Internal->mass,&this->Internal->potential,&this->Internal->tag,
        &this->Internal->mask,&this->Internal->status);
  particleExchange.exchangeParticles();
  const vtkIdType numParticles = this->Internal->xx.size();
  for (vtkIdType i = 0; i < numParticles; ++i)
    {
    // see if we even need this loop
    assert(this->Internal->mass[i] == this->ParticleMass);
    this->Internal->mass[i] = this->ParticleMass;
    }
}

void vtkPANLHaloFinder::ExecuteHaloFinder(vtkUnstructuredGrid *allParticles,
                                          vtkUnstructuredGrid *fofProperties)
{
  this->Internal->haloFinder = new cosmotk::CosmoHaloFinderP();
  this->Internal->haloFinder->setParameters("",this->RL,this->DeadSize,this->NP,this->PMin,this->BB,this->NMin);
  this->Internal->haloFinder->setParticles(
      this->Internal->xx.size(),&this->Internal->xx[0],&this->Internal->yy[0],
      &this->Internal->zz[0],&this->Internal->vx[0],&this->Internal->vy[0],
      &this->Internal->vz[0],&this->Internal->potential[0],
      &this->Internal->tag[0],&this->Internal->mask[0],
      &this->Internal->status[0]);
  this->Internal->haloFinder->executeHaloFinder();
  this->Internal->haloFinder->collectHalos(false);
  this->Internal->fof = new cosmotk::FOFHaloProperties();
  int numberOfFOFHalos = this->Internal->haloFinder->getNumberOfHalos();
  int* fofHalos = this->Internal->haloFinder->getHalos();
  int* fofHaloCount = this->Internal->haloFinder->getHaloCount();
  int* fofHaloList = this->Internal->haloFinder->getHaloList();
  this->Internal->fof->setHalos(numberOfFOFHalos,fofHalos,fofHaloCount,fofHaloList);
  this->Internal->fof->setParameters("",this->RL,this->DeadSize,this->BB);
  this->Internal->fof->setParticles(
      this->Internal->xx.size(),&this->Internal->xx[0],&this->Internal->yy[0],
      &this->Internal->zz[0],&this->Internal->vx[0],&this->Internal->vy[0],
      &this->Internal->vz[0],&this->Internal->mass[0],&this->Internal->potential[0],
      &this->Internal->tag[0],&this->Internal->mask[0],
      &this->Internal->status[0]);
  this->Internal->fof->FOFHaloMass(&this->Internal->fofMass);
  this->Internal->fof->FOFPosition(&this->Internal->fofXPos,&this->Internal->fofYPos,&this->Internal->fofZPos);
  this->Internal->fof->FOFCenterOfMass(
        &this->Internal->fofXCofMass,&this->Internal->fofYCofMass,&this->Internal->fofZCofMass);
  this->Internal->fof->FOFVelocity(&this->Internal->fofXVel,&this->Internal->fofYVel,&this->Internal->fofZVel);
  this->Internal->fof->FOFVelocityDispersion(&this->Internal->fofXVel,&this->Internal->fofYVel,&this->Internal->fofZVel,
                            &this->Internal->fofVelDisp);
  vtkNew< vtkPoints > points;
  allParticles->SetPoints(points.GetPointer());
  vtkNew< vtkFloatArray > velocityX;
  velocityX->SetName("vx");
  velocityX->SetNumberOfTuples(this->Internal->xx.size());
  vtkNew< vtkFloatArray > velocityY;
  velocityY->SetName("vy");
  velocityY->SetNumberOfTuples(this->Internal->xx.size());
  vtkNew< vtkFloatArray > velocityZ;
  velocityZ->SetName("vz");
  velocityZ->SetNumberOfTuples(this->Internal->xx.size());
  vtkNew< vtkLongLongArray > particleId;
  particleId->SetName("id");
  particleId->SetNumberOfTuples(this->Internal->xx.size());
  vtkNew< vtkLongLongArray > haloTags;
  haloTags->SetName("fof_halo_tag");
  haloTags->SetNumberOfTuples(this->Internal->xx.size());

  allParticles->Allocate(this->Internal->xx.size());
  float v[3];
  for (vtkIdType i = 0; i < this->Internal->xx.size(); ++i)
    {
    points->InsertNextPoint(this->Internal->xx[i],
                            this->Internal->yy[i],
                            this->Internal->zz[i]);
    velocityX->SetValue(i,this->Internal->vx[i]);
    velocityY->SetValue(i,this->Internal->vy[i]);
    velocityZ->SetValue(i,this->Internal->vz[i]);
    particleId->SetValue(i,this->Internal->tag[i]);
    haloTags->SetValue(i,this->Internal->haloFinder->getHaloIDForParticle(i));
    allParticles->InsertNextCell(VTK_VERTEX,1,&i);
    }
  allParticles->GetPointData()->AddArray(velocityX.GetPointer());
  allParticles->GetPointData()->AddArray(velocityY.GetPointer());
  allParticles->GetPointData()->AddArray(velocityZ.GetPointer());
  allParticles->GetPointData()->AddArray(particleId.GetPointer());
  allParticles->GetPointData()->AddArray(haloTags.GetPointer());

  vtkNew< vtkPoints > haloCenter;
  haloCenter->SetNumberOfPoints(numberOfFOFHalos);
  fofProperties->SetPoints(haloCenter.GetPointer());

  vtkPointData* pointData = fofProperties->GetPointData();
  vtkNew< vtkFloatArray > fofCenterOfMass;
  fofCenterOfMass->SetName("fof_halo_com");
  fofCenterOfMass->SetNumberOfComponents(3);
  fofCenterOfMass->SetNumberOfTuples(numberOfFOFHalos);
  pointData->AddArray(fofCenterOfMass.GetPointer());
  vtkNew< vtkFloatArray > haloVelocity;
  haloVelocity->SetName("fof_halo_mean_velocity");
  haloVelocity->SetNumberOfComponents(3);
  haloVelocity->SetNumberOfTuples(numberOfFOFHalos);
  pointData->AddArray(haloVelocity.GetPointer());
  vtkNew< vtkFloatArray > velocityDispersion;
  velocityDispersion->SetName("fof_velocity_dispersion");
  velocityDispersion->SetNumberOfTuples(numberOfFOFHalos);
  pointData->AddArray(velocityDispersion.GetPointer());
  vtkNew< vtkFloatArray > mass;
  mass->SetName("fof_halo_mass");
  mass->SetNumberOfTuples(numberOfFOFHalos);
  pointData->AddArray(mass.GetPointer());
  vtkNew< vtkIntArray > count;
  count->SetName("fof_halo_count");
  count->SetNumberOfTuples(numberOfFOFHalos);
  pointData->AddArray(count.GetPointer());
  vtkNew< vtkLongLongArray > tag;
  tag->SetName("fof_halo_tag");
  tag->SetNumberOfTuples(numberOfFOFHalos);
  pointData->AddArray(tag.GetPointer());
  // TODO --- center finding algorithms...

  fofProperties->Allocate(numberOfFOFHalos);
  float com[3], vel[3];
  for (vtkIdType i = 0; i < numberOfFOFHalos; ++i)
    {
    haloCenter->SetPoint(i,this->Internal->fofXPos[i],
                     this->Internal->fofYPos[i],
                     this->Internal->fofZPos[i]);
    com[0] = this->Internal->fofXCofMass[i];
    com[1] = this->Internal->fofYCofMass[i];
    com[2] = this->Internal->fofZCofMass[i];
    vel[0] = this->Internal->fofXVel[i];
    vel[1] = this->Internal->fofYVel[i];
    vel[2] = this->Internal->fofZVel[i];
    fofCenterOfMass->SetTupleValue(i,com);
    haloVelocity->SetTupleValue(i,vel);
    velocityDispersion->SetValue(i,this->Internal->fofVelDisp[i]);
    mass->SetValue(i,this->Internal->fofMass[i]);
    count->SetValue(i,fofHaloCount[i]);
    tag->SetValue(i,this->Internal->haloFinder->getHaloID(i));
    fofProperties->InsertNextCell(VTK_VERTEX,1,&i);
    }
}

void vtkPANLHaloFinder::ExecuteSubHaloFinder(vtkUnstructuredGrid *allParticles,
                                             vtkUnstructuredGrid *subFofProperties)
{
  std::vector< ID_T > parentHaloTag, subHaloTag;
  std::vector< long > parentFOFCount, subCount;
  std::vector< POSVEL_T > subRadius, subMass, subCenterOfMassX, subCenterOfMassY, subCenterOfMassZ,
      subAvgX, subAvgY, subAvgZ, subAvgVX, subAvgVY, subAvgVZ, subVelDisp;

  std::vector< POSVEL_T > shX, shY, shZ, shVX, shVY, shVZ;
  std::vector< ID_T > shTag, shHID, shID;

  vtkNew< vtkLongLongArray > subhaloId;
  subhaloId->SetName("subhalo_tag");
  subhaloId->SetNumberOfTuples(this->Internal->xx.size());
  for (size_t i = 0; i < this->Internal->xx.size(); ++i)
    {
    subhaloId->SetValue(i,-1);
    }

  int numberOfFOFHalos = this->Internal->haloFinder->getNumberOfHalos();
  int* fofHaloCount = this->Internal->haloFinder->getHaloCount();
  ExtractHalo haloData(numberOfFOFHalos,fofHaloCount,this->Internal->fof);

  for (int halo = 0; halo < numberOfFOFHalos; ++halo)
    {
    long particleCount = fofHaloCount[halo];
    if (particleCount > this->MinFOFSubhaloSize)
      {
      haloData.SetCurrentHalo(halo);

      cosmotk::SubHaloFinder subFinder;
      subFinder.setParameters(this->ParticleMass,GRAVITY_C,
                               this->AlphaFactor,this->BetaFactor,
                               this->MinCandidateSize,
                               this->NumSPHNeighbors,this->NumNeighbors);

      haloData.SetParticles(subFinder);
      subFinder.findSubHalos();

      int numberOfSubHalos = subFinder.getNumberOfSubhalos();
      int* fofSubHalos = subFinder.getSubhalos();
      int* fofSubHaloCount = subFinder.getSubhaloCount();
      int* fofSubHaloList = subFinder.getSubhaloList();

      cosmotk::FOFHaloProperties subhaloProperties;
      subhaloProperties.setHalos(numberOfSubHalos,fofSubHalos,
                                 fofSubHaloCount,fofSubHaloList);
      subhaloProperties.setParameters("",this->RL,this->DeadSize,this->BB);
      haloData.SetParticles(subhaloProperties);

      std::vector< POSVEL_T > subhaloMass;
      subhaloProperties.FOFHaloMass(&subhaloMass);

      std::vector< POSVEL_T > subhaloXPos;
      std::vector< POSVEL_T > subhaloYPos;
      std::vector< POSVEL_T > subhaloZPos;
      subhaloProperties.FOFPosition(&subhaloXPos,&subhaloYPos,&subhaloZPos);

      std::vector< POSVEL_T > subhaloXCofMass;
      std::vector< POSVEL_T > subhaloYCofMass;
      std::vector< POSVEL_T > subhaloZCofMass;
      subhaloProperties.FOFCenterOfMass(&subhaloXCofMass,&subhaloYCofMass,
                                        &subhaloZCofMass);

      std::vector< POSVEL_T > subhaloXVel;
      std::vector< POSVEL_T > subhaloYVel;
      std::vector< POSVEL_T > subhaloZVel;
      subhaloProperties.FOFVelocity(&subhaloXVel,&subhaloYVel,&subhaloZVel);

      std::vector< POSVEL_T > subhaloVelDisp;
      subhaloProperties.FOFVelocityDispersion(&subhaloXVel,&subhaloYVel,
                                              &subhaloZVel,&subhaloVelDisp);

      for (int sidx = 0; sidx < numberOfSubHalos; ++sidx)
        {
        parentHaloTag.push_back(this->Internal->haloFinder->getHaloID(halo));
        parentFOFCount.push_back(particleCount);
        subHaloTag.push_back(sidx);
        subCount.push_back(fofSubHaloCount[sidx]);
        subMass.push_back(subhaloMass[sidx]);
        subCenterOfMassX.push_back(subhaloXCofMass[sidx]);
        subCenterOfMassY.push_back(subhaloYCofMass[sidx]);
        subCenterOfMassZ.push_back(subhaloZCofMass[sidx]);
        subAvgX.push_back(subhaloXPos[sidx]);
        subAvgY.push_back(subhaloYPos[sidx]);
        subAvgZ.push_back(subhaloZPos[sidx]);
        subAvgVX.push_back(subhaloXVel[sidx]);
        subAvgVY.push_back(subhaloYVel[sidx]);
        subAvgVZ.push_back(subhaloZVel[sidx]);
        subVelDisp.push_back(subhaloVelDisp[sidx]);
        }

      size_t pointsBefore = shX.size();
      subFinder.getSubhaloCosmoData(this->Internal->haloFinder->getHaloID(halo),
                                    shX,shY,shZ,shVX,shVY,shVZ,shTag,
                                    shHID,shID);

      for (size_t i = 0; pointsBefore + i < shX.size(); ++i)
        {
        subhaloId->SetValue(haloData.GetActualIndex(i),shID[pointsBefore+i]);
        }

      }
    }

  allParticles->GetPointData()->AddArray(subhaloId.GetPointer());

  // make subhaloproperties file....
  vtkNew< vtkPoints > points;
  points->SetNumberOfPoints(subMass.size());
  subFofProperties->SetPoints(points.GetPointer());

  vtkPointData* pointData = subFofProperties->GetPointData();
  vtkNew< vtkFloatArray > fofCenterOfMass;
  fofCenterOfMass->SetName("subhalo_com");
  fofCenterOfMass->SetNumberOfComponents(3);
  fofCenterOfMass->SetNumberOfTuples(subMass.size());
  pointData->AddArray(fofCenterOfMass.GetPointer());
  vtkNew< vtkFloatArray > haloVelocity;
  haloVelocity->SetName("subhalo_mean_velocity");
  haloVelocity->SetNumberOfComponents(3);
  haloVelocity->SetNumberOfTuples(subMass.size());
  pointData->AddArray(haloVelocity.GetPointer());
  vtkNew< vtkFloatArray > velocityDispersion;
  velocityDispersion->SetName("subhalo_velocity_dispersion");
  velocityDispersion->SetNumberOfTuples(subMass.size());
  pointData->AddArray(velocityDispersion.GetPointer());
  vtkNew< vtkFloatArray > mass;
  mass->SetName("subhalo_mass");
  mass->SetNumberOfTuples(subMass.size());
  pointData->AddArray(mass.GetPointer());
  vtkNew< vtkIntArray > count;
  count->SetName("fof_halo_count");
  count->SetNumberOfTuples(subMass.size());
  pointData->AddArray(count.GetPointer());
  vtkNew< vtkIntArray > subhaloCount;
  subhaloCount->SetName("subhalo_count");
  subhaloCount->SetNumberOfTuples(subMass.size());
  pointData->AddArray(subhaloCount.GetPointer());
  vtkNew< vtkLongLongArray > tag;
  tag->SetName("fof_halo_tag");
  tag->SetNumberOfTuples(subMass.size());
  pointData->AddArray(tag.GetPointer());
  vtkNew< vtkLongLongArray > subtag;
  subtag->SetName("subhalo_tag");
  subtag->SetNumberOfTuples(subMass.size());
  pointData->AddArray(subtag.GetPointer());

  subFofProperties->Allocate(subMass.size());
  float com[3], vel[3];
  for (vtkIdType i = 0; i < subMass.size(); ++i)
    {
    points->SetPoint(i,subAvgX[i],subAvgY[i],subAvgZ[i]);
    com[0] = subCenterOfMassX[i];
    com[1] = subCenterOfMassY[i];
    com[2] = subCenterOfMassZ[i];
    vel[0] = subAvgVX[i];
    vel[1] = subAvgVY[i];
    vel[2] = subAvgVZ[i];
    fofCenterOfMass->SetTupleValue(i,com);
    haloVelocity->SetTupleValue(i,vel);
    velocityDispersion->SetValue(i,subVelDisp[i]);
    mass->SetValue(i,subMass[i]);
    count->SetValue(i,parentFOFCount[i]);
    tag->SetValue(i,parentHaloTag[i]);
    subhaloCount->SetValue(i,subCount[i]);
    subtag->SetValue(i,subHaloTag[i]);
    subFofProperties->InsertNextCell(VTK_VERTEX,1,&i);
    }

}

void vtkPANLHaloFinder::FindCenters(vtkUnstructuredGrid *fofProperties)
{
  if (this->CenterFindingMode == vtkPANLHaloFinder::NONE)
    {
    return;
    }
  int numberOfFOFHalos = this->Internal->haloFinder->getNumberOfHalos();
  int* fofHaloCount = this->Internal->haloFinder->getHaloCount();

  vtkNew< vtkFloatArray > centers;
  centers->SetName("fof_center");
  centers->SetNumberOfComponents(3);
  centers->SetNumberOfTuples(numberOfFOFHalos);

  ExtractHalo haloData(numberOfFOFHalos,fofHaloCount,this->Internal->fof);
  for (int halo = 0; halo < numberOfFOFHalos; ++halo)
    {
    haloData.SetCurrentHalo(halo);
    cosmotk::HaloCenterFinder centerFinder;
    haloData.SetParticles(centerFinder);
    centerFinder.setParameters(this->BB,this->SmoothingLength,
                               this->DistanceConvertFactor,this->RL,
                               this->NP,this->OmegaMatter,this->OmegaCB,
                               this->Hubble,this->RedShift);
    int centerIndex = -1;
    if (this->CenterFindingMode == MOST_BOUND_PARTICLE)
      {
      float minPotential;
      if (haloData.GetNumberOfParticlesInCurrentHalo() < MBP_THRESHOLD)
        {
        centerIndex = centerFinder.mostBoundParticleN2(&minPotential);
        }
      else
        {
        centerIndex = centerFinder.mostBoundParticleAStar(&minPotential);
        }
      }
    else if (this->CenterFindingMode == MOST_CONNECTED_PARTICLE)
      {
      if (haloData.GetNumberOfParticlesInCurrentHalo() < MCP_THRESHOLD)
        {
        centerIndex = centerFinder.mostConnectedParticleN2();
        }
      else
        {
        centerIndex = centerFinder.mostConnectedParticleChainMesh();
        }
      }
    else if (this->CenterFindingMode == HIST_CENTER_FINDING)
      {
      centerIndex = centerFinder.mostConnectedParticleHist();
      }
    else
      {
      return;
      }
    float center[] = { 0.0, 0.0, 0.0 };
    if (centerIndex >= 0)
      {
      double* point = fofProperties->GetPoint(haloData.GetActualIndex(centerIndex));
      center[0] = point[0];
      center[1] = point[1];
      center[2] = point[2];
      }
    centers->SetTupleValue(halo,center);
    }
  fofProperties->GetPointData()->AddArray(centers.GetPointer());
}
