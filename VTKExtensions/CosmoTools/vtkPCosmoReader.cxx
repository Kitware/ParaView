// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2009 Los Alamos National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-USGov
#include "vtkPCosmoReader.h"

#include "vtkCellArray.h"
#include "vtkDataObject.h"
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

#include <vector>

using namespace std;

// RRU stuff
#include "ParticleDistribute.h"
#include "ParticleExchange.h"
#include "Partition.h"

using namespace cosmotk;

vtkStandardNewMacro(vtkPCosmoReader);
vtkCxxSetObjectMacro(vtkPCosmoReader, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkPCosmoReader::vtkPCosmoReader()
{
  this->SetNumberOfInputPorts(0);

  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  if (!this->Controller)
  {
    vtkSmartPointer<vtkDummyController> controller = vtkSmartPointer<vtkDummyController>::New();
    this->SetController(controller);
  }

  this->FileName = nullptr;
  this->RL = 100;
  this->Overlap = 5;
  this->ReadMode = 1;
  this->CosmoFormat = 1;
  //  this->ByteSwap = 0;
}

//----------------------------------------------------------------------------
vtkPCosmoReader::~vtkPCosmoReader()
{
  if (this->FileName)
  {
    delete[] this->FileName;
  }
  this->SetController(0);
}

//----------------------------------------------------------------------------
void vtkPCosmoReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Controller)
  {
    os << indent << "Controller: " << this->Controller << endl;
  }
  else
  {
    os << indent << "Controller: (null)\n";
  }

  os << indent << "FileName: " << (this->FileName != nullptr ? this->FileName : "") << endl;
  os << indent << "rL: " << this->RL << endl;
  os << indent << "Overlap: " << this->Overlap << endl;
  os << indent << "ReadMode: " << this->ReadMode << endl;
  os << indent << "CosmoFormat: " << this->CosmoFormat << endl;
}

//----------------------------------------------------------------------------
int vtkPCosmoReader::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // set the pieces as the number of processes
  outputVector->GetInformationObject(0)->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  outputVector->GetInformationObject(0)->Set(
    vtkDataObject::DATA_NUMBER_OF_PIECES(), this->Controller->GetNumberOfProcesses());

  // set the ghost levels
  outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkPCosmoReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // check that the piece number is correct
  int updatePiece = 0;
  int updateTotal = 1;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
  {
    updatePiece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  }
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
  {
    updateTotal = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  }

  if (updatePiece != this->Controller->GetLocalProcessId() ||
    updateTotal != this->Controller->GetNumberOfProcesses())
  {
    vtkErrorMacro(<< "Piece number does not match process number.");
    return 0;
  }

  if (this->FileName == nullptr || this->FileName[0] == '\0')
  {
    vtkErrorMacro(<< "No FileName specified!");
    return 0;
  }

  // RRU code
  // Initialize the partitioner which uses MPI Cartesian Topology
  Partition::initialize();

  // Construct the particle distributor, exchanger and halo finder
  ParticleDistribute distribute;
  ParticleExchange exchange;

  // Initialize classes for reading, exchanging and calculating
  if (this->CosmoFormat)
  {
    distribute.setParameters(this->FileName, this->RL, "RECORD");
  }
  else
  {
    distribute.setParameters(this->FileName, this->RL, "BLOCK");
  }

  //  if( this->ByteSwap )
  //    {
  //    distribute.setByteSwap(true);
  //    }
  //  else
  //    {
  //    distribute.setByteSwap(false);
  //    }

  exchange.setParameters(this->RL, this->Overlap);

  distribute.initialize();
  exchange.initialize();

  // Read alive particles only from files
  // In ROUND_ROBIN all files are read and particles are passed round robin
  // to every other processor so that every processor chooses its own
  // In ONE_TO_ONE every processor reads its own processor in the topology
  // which has already been populated with the correct alive particles
  vector<POSVEL_T>* xx = new vector<POSVEL_T>;
  vector<POSVEL_T>* yy = new vector<POSVEL_T>;
  vector<POSVEL_T>* zz = new vector<POSVEL_T>;
  vector<POSVEL_T>* vx = new vector<POSVEL_T>;
  vector<POSVEL_T>* vy = new vector<POSVEL_T>;
  vector<POSVEL_T>* vz = new vector<POSVEL_T>;
  vector<POSVEL_T>* mass = new vector<POSVEL_T>;
  vector<ID_T>* tag = new vector<ID_T>;
  vector<STATUS_T>* status = new vector<STATUS_T>;

  distribute.setParticles(xx, yy, zz, vx, vy, vz, mass, tag);
  if (this->ReadMode)
  {
    distribute.readParticlesRoundRobin();
  }
  else
  {
    distribute.readParticlesOneToOne();
  }

  // Create the mask and potential vectors which will be filled in elsewhere
  int numberOfParticles = (int)xx->size();
  vector<POTENTIAL_T>* potential = new vector<POTENTIAL_T>(numberOfParticles);
  vector<MASK_T>* mask = new vector<MASK_T>(numberOfParticles);

  // Exchange particles adds dead particles to all the vectors
  exchange.setParticles(xx, yy, zz, vx, vy, vz, mass, potential, tag, mask, status);
  exchange.exchangeParticles();

  // create VTK structures
  numberOfParticles = (int)xx->size();
  potential->clear();
  mask->clear();

  vtkPoints* points = vtkPoints::New();
  points->SetDataTypeToFloat();
  points->Allocate(numberOfParticles);
  vtkCellArray* cells = vtkCellArray::New();
  cells->Allocate(cells->EstimateSize(numberOfParticles, 1));

  vtkFloatArray* vel = vtkFloatArray::New();
  vel->SetName("velocity");
  vel->SetNumberOfComponents(DIMENSION);
  vel->Allocate(numberOfParticles);
  vtkFloatArray* m = vtkFloatArray::New();
  m->SetName("mass");
  m->Allocate(numberOfParticles);
  vtkIdTypeArray* uid = vtkIdTypeArray::New();
  uid->SetName("tag");
  uid->Allocate(numberOfParticles);
  vtkIntArray* owner = vtkIntArray::New();
  owner->SetName("ghost");
  owner->Allocate(numberOfParticles);
  vtkUnsignedCharArray* ghost = vtkUnsignedCharArray::New();
  ghost->SetName(vtkDataSetAttributes::GhostArrayName());
  ghost->Allocate(numberOfParticles);

  // put it into the correct VTK structure
  for (vtkIdType i = 0; i < numberOfParticles; i = i + 1)
  {
    float pt[DIMENSION];

    // insert point and cell
    pt[0] = xx->back();
    xx->pop_back();
    pt[1] = yy->back();
    yy->pop_back();
    pt[2] = zz->back();
    zz->pop_back();

    vtkIdType pid = points->InsertNextPoint(pt);
    cells->InsertNextCell(1, &pid);

    // insert velocity
    pt[0] = vx->back();
    vx->pop_back();
    pt[1] = vy->back();
    vy->pop_back();
    pt[2] = vz->back();
    vz->pop_back();

    vel->InsertNextTuple(pt);

    // insert mass
    pt[0] = mass->back();
    mass->pop_back();

    m->InsertNextValue(pt[0]);

    // insert tag
    vtkTypeInt64 particle = tag->back();
    tag->pop_back();

    uid->InsertNextValue(particle);

    // insert ghost status
    int neighbor = status->back();
    unsigned char level = neighbor < 0 ? 0 : 1;
    status->pop_back();

    owner->InsertNextValue(neighbor);
    ghost->InsertNextValue((level > 0) ? vtkDataSetAttributes::DUPLICATEPOINT : 0);
  }

  // cleanup
  output->SetPoints(points);
  output->SetCells(1, cells);
  output->GetPointData()->AddArray(vel);
  output->GetPointData()->AddArray(m);
  output->GetPointData()->AddArray(uid);
  output->GetPointData()->AddArray(owner);
  output->GetPointData()->AddArray(ghost);

  output->Squeeze();

  points->Delete();
  cells->Delete();
  vel->Delete();
  m->Delete();
  uid->Delete();
  owner->Delete();
  ghost->Delete();

  delete xx;
  delete yy;
  delete zz;
  delete vx;
  delete vy;
  delete vz;
  delete mass;
  delete tag;
  delete status;
  delete potential;
  delete mask;

  return 1;
}
