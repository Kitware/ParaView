/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPANLSubhaloFinder.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPANLSubhaloFinder.h"

#include "vtkCellType.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTypeInt64Array.h"
#include "vtkUnstructuredGrid.h"

#include "FOFHaloProperties.h"
#include "SubHaloFinder.h"

#include <map>
#include <set>
#include <vector>

namespace
{
// magic number taken from BasicDefinition.h in the halo finder code
static const double GRAVITY_C = 43.015e-10;
}

class vtkPANLSubhaloFinder::vtkInternals
{
public:
  vtkInternals() {}
  std::map<vtkIdType, std::vector<vtkIdType> > haloIndices;

  std::vector<POSVEL_T> xx;
  std::vector<POSVEL_T> yy;
  std::vector<POSVEL_T> zz;
  std::vector<POSVEL_T> vx;
  std::vector<POSVEL_T> vy;
  std::vector<POSVEL_T> vz;
  std::vector<POSVEL_T> mass;
  std::vector<ID_T> id;
  std::vector<ID_T> actualIndex;

  void ReadHalos(vtkDataArray* haloTag, vtkIdList* halos)
  {
    haloIndices.clear();
    for (int i = 0; i < halos->GetNumberOfIds(); ++i)
    {
      haloIndices[halos->GetId(i)] = std::vector<vtkIdType>();
    }
    for (int j = 0; j < haloTag->GetNumberOfTuples(); ++j)
    {
      for (int i = 0; i < halos->GetNumberOfIds(); ++i)
      {
        if (halos->GetId(i) == static_cast<vtkIdType>(haloTag->GetTuple1(j)))
        {
          haloIndices[halos->GetId(i)].push_back(j);
        }
      }
    }
  }

  void LoadHalo(vtkIdType haloId, double particleMass, vtkUnstructuredGrid* input)
  {
    vtkPointData* pd = input->GetPointData();
    assert(pd);
    vtkDataArray* vx_array = pd->GetArray("vx");
    assert(vx_array);
    vtkDataArray* vy_array = pd->GetArray("vy");
    assert(vy_array);
    vtkDataArray* vz_array = pd->GetArray("vz");
    assert(vz_array);
    vtkDataArray* id_array = pd->GetArray("id");
    assert(id_array);
    double point[3];
    std::vector<vtkIdType>& haloIdxs = haloIndices[haloId];
    this->xx.resize(haloIdxs.size());
    this->yy.resize(haloIdxs.size());
    this->zz.resize(haloIdxs.size());
    this->vx.resize(haloIdxs.size());
    this->vy.resize(haloIdxs.size());
    this->vz.resize(haloIdxs.size());
    this->mass.resize(haloIdxs.size());
    this->id.resize(haloIdxs.size());
    this->actualIndex.resize(haloIdxs.size());
    for (size_t i = 0; i < haloIdxs.size(); ++i)
    {
      vtkIdType idx = haloIdxs[i];
      input->GetPoint(idx, point);
      this->xx[i] = point[0];
      this->yy[i] = point[1];
      this->zz[i] = point[2];
      this->vx[i] = vx_array->GetTuple1(idx);
      this->vy[i] = vy_array->GetTuple1(idx);
      this->vz[i] = vz_array->GetTuple1(idx);
      this->mass[i] = particleMass;
      this->id[i] = id_array->GetTuple1(idx);
      this->actualIndex[i] = idx;
    }
  }
};

vtkPANLSubhaloFinder::vtkPANLSubhaloFinder()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
  this->Internal = new vtkPANLSubhaloFinder::vtkInternals;
  this->SetNumberOfOutputPorts(2);

  this->Mode = ALL_HALOS;
  this->SizeThreshold = 1000;

  this->RL = 256;
  this->DeadSize = 8;
  this->ParticleMass = 1.307087181e+09;
  this->BB = 0.1679999998;
  this->AlphaFactor = 1.0;
  this->BetaFactor = 0.0;
  this->MinCandidateSize = 200;
  this->NumSPHNeighbors = 64;
  this->NumNeighbors = 20;
}

vtkPANLSubhaloFinder::~vtkPANLSubhaloFinder()
{
  delete this->Internal;
}

vtkStandardNewMacro(vtkPANLSubhaloFinder);

void vtkPANLSubhaloFinder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkPANLSubhaloFinder::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  return 1;
}

int vtkPANLSubhaloFinder::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  return 1;
}

int vtkPANLSubhaloFinder::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::GetData(inputVector[0], 0);
  vtkMultiBlockDataSet* multiBlockIn = vtkMultiBlockDataSet::GetData(inputVector[0], 0);
  vtkUnstructuredGrid* allParticlesOutput = vtkUnstructuredGrid::GetData(outputVector, 0);
  vtkMultiBlockDataSet* multiBlockAllParticles = vtkMultiBlockDataSet::GetData(outputVector, 0);
  vtkUnstructuredGrid* subFofProperties = vtkUnstructuredGrid::GetData(outputVector, 1);
  vtkMultiBlockDataSet* multiBlockSubProperties = vtkMultiBlockDataSet::GetData(outputVector, 1);

  if (grid != nullptr)
  {
    assert(allParticlesOutput);
    assert(subFofProperties);
    allParticlesOutput->ShallowCopy(grid);
    this->ExecuteSubHaloFinder(grid, allParticlesOutput, subFofProperties);
  }
  else if (multiBlockIn)
  {
    assert(multiBlockAllParticles);
    assert(multiBlockSubProperties);
    vtkIdType numberOfInputBlocks = multiBlockIn->GetNumberOfBlocks();
    multiBlockAllParticles->SetNumberOfBlocks(numberOfInputBlocks);
    multiBlockSubProperties->SetNumberOfBlocks(numberOfInputBlocks);
    for (vtkIdType i = 0; i < numberOfInputBlocks; ++i)
    {
      vtkUnstructuredGrid* block = vtkUnstructuredGrid::SafeDownCast(multiBlockIn->GetBlock(i));
      if (block != nullptr)
      {
        vtkNew<vtkUnstructuredGrid> blockAllParticles;
        vtkNew<vtkUnstructuredGrid> blockSubProperties;
        blockAllParticles->ShallowCopy(block);
        this->ExecuteSubHaloFinder(
          block, blockAllParticles.GetPointer(), blockSubProperties.GetPointer());
        multiBlockAllParticles->SetBlock(i, blockAllParticles.GetPointer());
        multiBlockSubProperties->SetBlock(i, blockSubProperties.GetPointer());
      }
    }
  }
  else
  {
    vtkErrorMacro("No Input!");
    return 0;
  }
  return 1;
}

vtkIdType vtkPANLSubhaloFinder::GetHaloToProcess(vtkIdType idx)
{
  assert(idx >= 0 && idx < this->HalosToProcess->GetNumberOfIds());
  return this->HalosToProcess->GetId(idx);
}

void vtkPANLSubhaloFinder::AddHaloToProcess(vtkIdType haloId)
{
  this->HalosToProcess->InsertNextId(haloId);
  this->Modified();
}

void vtkPANLSubhaloFinder::SetHaloToProcess(vtkIdType idx, vtkIdType haloId)
{
  assert(idx >= 0 && idx < this->HalosToProcess->GetNumberOfIds());
  if (this->HalosToProcess->GetId(idx) != haloId)
  {
    this->HalosToProcess->SetId(idx, haloId);
    this->Modified();
  }
}

void vtkPANLSubhaloFinder::SetNumberOfHalosToProcess(vtkIdType num)
{
  if (num != this->HalosToProcess->GetNumberOfIds())
  {
    this->HalosToProcess->SetNumberOfIds(num);
    this->Modified();
  }
}

vtkIdType vtkPANLSubhaloFinder::GetNumberOfHalosToProcess()
{
  return this->HalosToProcess->GetNumberOfIds();
}

void vtkPANLSubhaloFinder::ClearHalosToProcess()
{
  if (this->HalosToProcess->GetNumberOfIds() > 0)
  {
    this->HalosToProcess->Reset();
    this->Modified();
  }
}

void vtkPANLSubhaloFinder::ExecuteSubHaloFinder(vtkUnstructuredGrid* input,
  vtkUnstructuredGrid* allParticles, vtkUnstructuredGrid* subFofProperties)
{
  std::vector<ID_T> parentHaloTag, subHaloTag;
  std::vector<long> parentFOFCount, subCount;
  std::vector<POSVEL_T> subMass, subCenterOfMassX, subCenterOfMassY, subCenterOfMassZ, subAvgX,
    subAvgY, subAvgZ, subAvgVX, subAvgVY, subAvgVZ, subVelDisp;

  std::vector<POSVEL_T> shX, shY, shZ, shVX, shVY, shVZ;
  std::vector<ID_T> shTag, shHID, shID;

  vtkNew<vtkTypeInt64Array> subhaloId;
  subhaloId->SetName("subhalo_tag");
  subhaloId->SetNumberOfTuples(input->GetNumberOfPoints());
  subhaloId->FillComponent(0, -1);

  vtkDataArray* haloTagArray = input->GetPointData()->GetArray("fof_halo_tag");
  std::set<vtkIdType> uniqueIds;
  for (vtkIdType i = 0; i < haloTagArray->GetNumberOfTuples(); ++i)
  {
    vtkIdType halo = static_cast<vtkIdType>(haloTagArray->GetTuple1(i));
    if (halo >= 0)
    {
      uniqueIds.insert(halo);
    }
  }
  vtkNew<vtkIdList> allHaloIds;
  for (std::set<vtkIdType>::iterator itr = uniqueIds.begin(); itr != uniqueIds.end(); ++itr)
  {
    allHaloIds->InsertNextId(*itr);
  }

  vtkNew<vtkIdList> finalHalosToProcess;
  finalHalosToProcess->DeepCopy(this->Mode == ONLY_SELECTED_HALOS
      ? this->HalosToProcess.GetPointer()
      : allHaloIds.GetPointer());

  this->Internal->ReadHalos(haloTagArray, finalHalosToProcess.GetPointer());

  if (this->Mode == HALOS_LARGER_THAN_THRESHOLD)
  {
    for (vtkIdType i = finalHalosToProcess->GetNumberOfIds() - 1; i >= 0; --i)
    {
      if (this->Internal->haloIndices[finalHalosToProcess->GetId(i)].size() <
        static_cast<size_t>(this->SizeThreshold))
      {
        finalHalosToProcess->DeleteId(finalHalosToProcess->GetId(i));
        i = std::min(i, finalHalosToProcess->GetNumberOfIds());
      }
    }
  }

  for (int i = 0; i < finalHalosToProcess->GetNumberOfIds(); ++i)
  {
    vtkIdType haloId = finalHalosToProcess->GetId(i);
    vtkDebugMacro(<< "Processing halo: " << haloId);
    this->Internal->LoadHalo(haloId, this->ParticleMass, input);
    long particleCount = this->Internal->xx.size();
    if (particleCount == 0)
    {
      continue;
    }

    cosmotk::SubHaloFinder subFinder;
    subFinder.setParameters(this->ParticleMass, GRAVITY_C, this->AlphaFactor, this->BetaFactor,
      this->MinCandidateSize, this->NumSPHNeighbors, this->NumNeighbors);
    subFinder.setParticles(this->Internal->xx.size(), &this->Internal->xx[0],
      &this->Internal->yy[0], &this->Internal->zz[0], &this->Internal->vx[0],
      &this->Internal->vy[0], &this->Internal->vz[0], &this->Internal->mass[0],
      &this->Internal->id[0]);
    subFinder.findSubHalos();

    int numberOfSubHalos = subFinder.getNumberOfSubhalos();
    int* fofSubHalos = subFinder.getSubhalos();
    int* fofSubHaloCount = subFinder.getSubhaloCount();
    int* fofSubHaloList = subFinder.getSubhaloList();

    cosmotk::FOFHaloProperties subhaloProperties;
    subhaloProperties.setHalos(numberOfSubHalos, fofSubHalos, fofSubHaloCount, fofSubHaloList);
    subhaloProperties.setParameters("", this->RL, this->DeadSize, this->BB);
    subhaloProperties.setParticles(this->Internal->xx.size(), &this->Internal->xx[0],
      &this->Internal->yy[0], &this->Internal->zz[0], &this->Internal->vx[0],
      &this->Internal->vy[0], &this->Internal->vz[0], &this->Internal->mass[0],
      &this->Internal->id[0]);

    std::vector<POSVEL_T> subhaloMass;
    subhaloProperties.FOFHaloMass(&subhaloMass);

    std::vector<POSVEL_T> subhaloXPos;
    std::vector<POSVEL_T> subhaloYPos;
    std::vector<POSVEL_T> subhaloZPos;
    subhaloProperties.FOFPosition(&subhaloXPos, &subhaloYPos, &subhaloZPos);

    std::vector<POSVEL_T> subhaloXCofMass;
    std::vector<POSVEL_T> subhaloYCofMass;
    std::vector<POSVEL_T> subhaloZCofMass;
    subhaloProperties.FOFCenterOfMass(&subhaloXCofMass, &subhaloYCofMass, &subhaloZCofMass);

    std::vector<POSVEL_T> subhaloXVel;
    std::vector<POSVEL_T> subhaloYVel;
    std::vector<POSVEL_T> subhaloZVel;
    subhaloProperties.FOFVelocity(&subhaloXVel, &subhaloYVel, &subhaloZVel);

    std::vector<POSVEL_T> subhaloVelDisp;
    subhaloProperties.FOFVelocityDispersion(
      &subhaloXVel, &subhaloYVel, &subhaloZVel, &subhaloVelDisp);

    for (int sidx = 0; sidx < numberOfSubHalos; ++sidx)
    {
      parentHaloTag.push_back(haloId);
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
    subFinder.getSubhaloCosmoData(haloId, shX, shY, shZ, shVX, shVY, shVZ, shTag, shHID, shID);

    for (size_t j = 0; pointsBefore + j < shX.size(); ++j)
    {
      subhaloId->SetValue(this->Internal->actualIndex[j], shID[pointsBefore + j]);
    }
  }

  allParticles->GetPointData()->AddArray(subhaloId.GetPointer());

  // make subhaloproperties file....
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(subMass.size());
  subFofProperties->SetPoints(points.GetPointer());

  vtkPointData* pointData = subFofProperties->GetPointData();
  vtkNew<vtkFloatArray> fofCenterOfMass;
  fofCenterOfMass->SetName("subhalo_com");
  fofCenterOfMass->SetNumberOfComponents(3);
  fofCenterOfMass->SetNumberOfTuples(subMass.size());
  pointData->AddArray(fofCenterOfMass.GetPointer());
  vtkNew<vtkFloatArray> haloVelocity;
  haloVelocity->SetName("subhalo_mean_velocity");
  haloVelocity->SetNumberOfComponents(3);
  haloVelocity->SetNumberOfTuples(subMass.size());
  pointData->AddArray(haloVelocity.GetPointer());
  vtkNew<vtkFloatArray> velocityDispersion;
  velocityDispersion->SetName("subhalo_velocity_dispersion");
  velocityDispersion->SetNumberOfTuples(subMass.size());
  pointData->AddArray(velocityDispersion.GetPointer());
  vtkNew<vtkFloatArray> mass;
  mass->SetName("subhalo_mass");
  mass->SetNumberOfTuples(subMass.size());
  pointData->AddArray(mass.GetPointer());
  vtkNew<vtkIntArray> count;
  count->SetName("fof_halo_count");
  count->SetNumberOfTuples(subMass.size());
  pointData->AddArray(count.GetPointer());
  vtkNew<vtkIntArray> subhaloCount;
  subhaloCount->SetName("subhalo_count");
  subhaloCount->SetNumberOfTuples(subMass.size());
  pointData->AddArray(subhaloCount.GetPointer());
  vtkNew<vtkTypeInt64Array> tag;
  tag->SetName("fof_halo_tag");
  tag->SetNumberOfTuples(subMass.size());
  pointData->AddArray(tag.GetPointer());
  vtkNew<vtkTypeInt64Array> subtag;
  subtag->SetName("subhalo_tag");
  subtag->SetNumberOfTuples(subMass.size());
  pointData->AddArray(subtag.GetPointer());

  subFofProperties->Allocate(subMass.size());
  float com[3], vel[3];
  for (vtkIdType i = 0; static_cast<size_t>(i) < subMass.size(); ++i)
  {
    points->SetPoint(i, subAvgX[i], subAvgY[i], subAvgZ[i]);
    com[0] = subCenterOfMassX[i];
    com[1] = subCenterOfMassY[i];
    com[2] = subCenterOfMassZ[i];
    vel[0] = subAvgVX[i];
    vel[1] = subAvgVY[i];
    vel[2] = subAvgVZ[i];
    fofCenterOfMass->SetTypedTuple(i, com);
    haloVelocity->SetTypedTuple(i, vel);
    velocityDispersion->SetValue(i, subVelDisp[i]);
    mass->SetValue(i, subMass[i]);
    count->SetValue(i, parentFOFCount[i]);
    tag->SetValue(i, parentHaloTag[i]);
    subhaloCount->SetValue(i, subCount[i]);
    subtag->SetValue(i, subHaloTag[i]);
    subFofProperties->InsertNextCell(VTK_VERTEX, 1, &i);
  }
}
