/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExplicitStructuredGridGeneratorSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExplicitStructuredGridGeneratorSource.h"

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkElevationFilter.h>
#include <vtkExplicitStructuredGrid.h>
#include <vtkExtentTranslator.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkIntArray.h>
#include <vtkMath.h>
#include <vtkMultiProcessController.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkUnsignedShortArray.h>

#include <sstream>

namespace
{
void PillarGridGenerator(int, int wholeExtent[6], int extent[6], vtkExplicitStructuredGrid*);
void DiscontinuousGridGenerator(int, int wholeExtent[6], int extent[6], vtkExplicitStructuredGrid*);
void ContinuousGridGenerator(int, int wholeExtent[6], int extent[6], vtkExplicitStructuredGrid*);
void StepsGridGenerator(int, int wholeExtent[6], int extent[6], vtkExplicitStructuredGrid*, int);
void PyramidGridGenerator(int, int wholeExtent[6], int extent[6], vtkExplicitStructuredGrid*, int);
}

vtkStandardNewMacro(vtkExplicitStructuredGridGeneratorSource);

//----------------------------------------------------------------------------
vtkExplicitStructuredGridGeneratorSource::vtkExplicitStructuredGridGeneratorSource()
{
  this->SetDataExtent(this->DataExtent);
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkExplicitStructuredGridGeneratorSource::~vtkExplicitStructuredGridGeneratorSource()
{
  if (this->Cache)
  {
    this->Cache->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGridGeneratorSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "GeneratorMode: " << this->GeneratorMode << std::endl;
  os << "PyramidStepSize: " << this->PyramidStepSize << std::endl;
  os << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << std::endl;
  os << "DataExtent: " << this->DataExtent[0] << ":" << this->DataExtent[1] << " "
     << this->DataExtent[2] << ":" << this->DataExtent[3] << " " << this->DataExtent[4] << ":"
     << this->DataExtent[5] << " " << std::endl;
}

//----------------------------------------------------------------------------
int vtkExplicitStructuredGridGeneratorSource::RequestInformation(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (this->Cache)
  {
    this->Cache->Delete();
    this->Cache = nullptr;
  }

  if (this->DataExtent[0] >= this->DataExtent[1] || this->DataExtent[2] >= this->DataExtent[3] ||
    this->DataExtent[4] >= this->DataExtent[5])
  {
    vtkErrorMacro("the provided DataExtent : "
      << this->DataExtent[0] << " " << this->DataExtent[1] << " " << this->DataExtent[2] << " "
      << this->DataExtent[3] << " " << this->DataExtent[4] << " " << this->DataExtent[5]
      << " is invalid. Aborting.");
    return 0;
  }

  // Our source can produce sub extent
  outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->DataExtent, 6);

  int numTimesteps = this->GetNumberOfTimeSteps();
  if (numTimesteps > 0)
  {
    std::vector<double> timeSteps(numTimesteps);
    for (int i = 0; i < numTimesteps; i++)
    {
      timeSteps[i] = i;
    }
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeSteps[0], numTimesteps);
    double timeRange[2];
    timeRange[0] = timeSteps[0];
    timeRange[1] = timeSteps[numTimesteps - 1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkExplicitStructuredGridGeneratorSource::AddTemporalData(
  double time, vtkExplicitStructuredGrid* grid)
{
  vtkNew<vtkElevationFilter> elev;
  elev->SetInputData(grid);
  elev->SetLowPoint(0, 0, 0);
  elev->SetHighPoint(50, 50, 75);
  elev->SetScalarRange(-100, 100);
  elev->Update();

  vtkFloatArray* scalars =
    vtkFloatArray::SafeDownCast(elev->GetOutput()->GetPointData()->GetScalars());

  vtkIdType nvals = scalars->GetNumberOfTuples();
  for (vtkIdType i = 0; i < nvals; i++)
  {
    scalars->SetValue(i, scalars->GetValue(i) * cos(time / 10));
  }

  grid->GetPointData()->AddArray(scalars);
}

//----------------------------------------------------------------------------
int vtkExplicitStructuredGridGeneratorSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkExplicitStructuredGrid* grid =
    vtkExplicitStructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int rank = 0;
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if (controller && controller->GetNumberOfProcesses() > 1)
  {
    rank = controller->GetLocalProcessId();
  }

  if (this->DataExtent[0] >= this->DataExtent[1] || this->DataExtent[2] >= this->DataExtent[3] ||
    this->DataExtent[4] >= this->DataExtent[5])
  {
    vtkErrorMacro("the provided DataExtent : "
      << this->DataExtent[0] << " " << this->DataExtent[1] << " " << this->DataExtent[2] << " "
      << this->DataExtent[3] << " " << this->DataExtent[4] << " " << this->DataExtent[5]
      << " is invalid. Aborting.");
    return 0;
  }

  int extent[6];
  int* updateExtent = extent;
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent);

  if (updateExtent[0] >= updateExtent[1] || updateExtent[2] >= updateExtent[3] ||
    updateExtent[4] >= updateExtent[5])
  {
    vtkWarningMacro("the UPDATE_EXTENT() requested by the pipeline : "
      << updateExtent[0] << " " << updateExtent[1] << " " << updateExtent[2] << " "
      << updateExtent[3] << " " << updateExtent[4] << " " << updateExtent[5] << " is invalid."
      << " Using the provided DataExtent instead.");
    updateExtent = this->DataExtent;
  }

  double updateTime = 0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    updateTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  if (this->PyramidStepSize <= 0)
  {
    vtkErrorMacro("Invalid PyramidStepSize : " << this->PyramidStepSize << " It should be > 0");
    return 0;
  }

  std::ostringstream stamp;
  stamp << updateExtent[0] << "," << updateExtent[1] << "," << updateExtent[2] << ","
        << updateExtent[3] << "," << updateExtent[4] << "," << updateExtent[5] << ","
        << this->DataExtent[0] << "," << this->DataExtent[1] << "," << this->DataExtent[2] << ","
        << this->DataExtent[3] << "," << this->DataExtent[4] << "," << this->DataExtent[5];

  if (this->Cache && this->CacheStamp == stamp.str())
  {
    grid->ShallowCopy(this->Cache);
  }
  else if (this->Cache)
  {
    this->Cache->Delete();
    this->Cache = nullptr;
  }

  if (!this->Cache)
  {
    this->Cache = vtkExplicitStructuredGrid::New();
    this->CacheStamp = stamp.str();
    switch (this->GeneratorMode)
    {
      case GENERATOR_PILLAR:
        ::PillarGridGenerator(rank, this->DataExtent, updateExtent, this->Cache);
        break;
      case GENERATOR_DISCONTINOUS:
        ::DiscontinuousGridGenerator(rank, this->DataExtent, updateExtent, this->Cache);
        break;
      case GENERATOR_STEPS:
        ::StepsGridGenerator(
          rank, this->DataExtent, updateExtent, this->Cache, this->PyramidStepSize);
        break;
      case GENERATOR_PYRAMID:
        ::PyramidGridGenerator(
          rank, this->DataExtent, updateExtent, this->Cache, this->PyramidStepSize);
        break;
      case GENERATOR_CONTINUOUS:
      default:
        ::ContinuousGridGenerator(rank, this->DataExtent, updateExtent, this->Cache);
        break;
    }
    vtkIdType ncells = this->Cache->GetNumberOfCells();
    vtkNew<vtkIntArray> objectId;
    objectId->SetName("ObjectId");
    objectId->SetNumberOfTuples(ncells);
    objectId->FillComponent(0, 0);
    this->Cache->GetCellData()->AddArray(objectId);
  }

  grid->ShallowCopy(this->Cache);
  this->AddTemporalData(updateTime, grid);

  return 1;
}

//-----------------------------------------------------------------------------
namespace
{
//-----------------------------------------------------------------------------
void PillarGridGenerator(
  int rank, int wholeExtent[6], int extent[6], vtkExplicitStructuredGrid* grid)
{
  // Set the extent and use the generated cells
  grid->SetExtent(extent);

  double nx = wholeExtent[1] - wholeExtent[0];
  double nz = wholeExtent[5] - wholeExtent[4];

  int lnx = extent[1] - extent[0];
  int lny = extent[3] - extent[2];
  int lnz = extent[5] - extent[4];
  vtkIdType expectedCells = lnx * lny * lnz;

  vtkNew<vtkPoints> points;
  points->Allocate(expectedCells * 8);

  vtkNew<vtkFloatArray> dist_array;
  dist_array->SetName("Distance2Origin");
  dist_array->SetNumberOfTuples(expectedCells);

  vtkNew<vtkUnsignedShortArray> rank_array;
  rank_array->SetName("Rank");
  rank_array->SetNumberOfTuples(expectedCells);

  vtkNew<vtkUnsignedShortArray> coords_array;
  coords_array->SetName("Coordinates");
  coords_array->SetNumberOfComponents(3);
  coords_array->SetNumberOfTuples(expectedCells);

  vtkIdType p_index_p4 = 0;
  vtkIdType p_index_p5 = 0;
  vtkIdType p_index_p6 = 0;
  vtkIdType p_index_p7 = 0;

  double coef_noise = 0.2;

#define GetZShift(_k, _shift)                                                                      \
  (shift_faille + _shift + (_k) +                                                                  \
    ((k == extent[4] || k >= extent[5] - 2) ? 0. : (coef_noise * rand() / RAND_MAX)))
#define GetZShift1(_k) GetZShift(_k, shift_z1)
#define GetZShift2(_k) GetZShift(_k, shift_z2)

  for (int i = extent[0]; i < extent[1]; i++)
  {
    double shift_z1 = nz * 0.5 * sin((static_cast<double>(i) + 1.) * vtkMath::Pi() / nx);
    double shift_z2 = nz * 0.5 * sin((static_cast<double>(i) + 2.) * vtkMath::Pi() / nx);
    double shift_faille = (i > nx / 2) ? 0. : nz * 0.3333;

    for (int j = extent[2]; j < extent[3]; j++)
    {
      for (int k = extent[4]; k < extent[5]; k++)
      {
        vtkIdType cellId = grid->ComputeCellId(i, j, k);
        vtkIdType* indice = grid->GetCellPoints(cellId);
        if (k > extent[4])
        {
          indice[0] = p_index_p4;
          indice[1] = p_index_p5;
          indice[2] = p_index_p6;
          indice[3] = p_index_p7;
        }
        else
        {
          indice[0] = points->InsertNextPoint(i - 0.5, j - 0.5, GetZShift1(k - 0.5));
          indice[1] = points->InsertNextPoint(i + 0.5, j - 0.5, GetZShift2(k - 0.5));
          indice[2] = points->InsertNextPoint(i + 0.5, j + 0.5, GetZShift2(k - 0.5));
          indice[3] = points->InsertNextPoint(i - 0.5, j + 0.5, GetZShift1(k - 0.5));
        }
        indice[4] = points->InsertNextPoint(i - 0.5, j - 0.5, GetZShift1(k + 0.5));
        indice[5] = points->InsertNextPoint(i + 0.5, j - 0.5, GetZShift2(k + 0.5));
        indice[6] = points->InsertNextPoint(i + 0.5, j + 0.5, GetZShift2(k + 0.5));
        indice[7] = points->InsertNextPoint(i - 0.5, j + 0.5, GetZShift1(k + 0.5));

        p_index_p4 = indice[4];
        p_index_p5 = indice[5];
        p_index_p6 = indice[6];
        p_index_p7 = indice[7];

        double dist = sqrt(static_cast<double>(i * i + j * j + k * k));
        dist_array->SetValue(cellId, dist);
        rank_array->SetValue(cellId, rank);
        coords_array->SetTuple3(cellId, i, j, k);
      }
    }
  }
  grid->SetPoints(points);
  grid->GetCellData()->AddArray(dist_array);
  grid->GetCellData()->AddArray(rank_array);
  grid->GetCellData()->AddArray(coords_array);
  grid->CheckAndReorderFaces();
  grid->ComputeFacesConnectivityFlagsArray();
}

//-----------------------------------------------------------------------------
void DiscontinuousGridGenerator(
  int rank, int wholeExtent[6], int extent[6], vtkExplicitStructuredGrid* grid)
{
  srand(1);
  grid->SetExtent(extent);

  double nx = wholeExtent[1] - wholeExtent[0];
  double nz = wholeExtent[5] - wholeExtent[4];

  int lnx = extent[1] - extent[0];
  int lny = extent[3] - extent[2];
  int lnz = extent[5] - extent[4];
  vtkIdType expectedCells = lnx * lny * lnz;

  vtkNew<vtkPoints> points;
  points->Allocate(expectedCells * 8);

  vtkNew<vtkFloatArray> dist_array;
  dist_array->SetName("Distance2Origin");
  dist_array->SetNumberOfValues(expectedCells);

  vtkNew<vtkUnsignedShortArray> rank_array;
  rank_array->SetName("Rank");
  rank_array->SetNumberOfValues(expectedCells);

  vtkNew<vtkUnsignedShortArray> coords_array;
  coords_array->SetName("Coordinates");
  coords_array->SetNumberOfComponents(3);
  coords_array->SetNumberOfTuples(expectedCells);

  for (vtkIdType i = 0; i < expectedCells; i++)
  {
    // Blank after copying the cell data to ensure it is not overwrited
    grid->BlankCell(i);
  }

  double coef_noise = 0.2;

  for (int i = extent[0]; i < extent[1]; i++)
  {
    double shift_z1 = nz * 0.5 * sin(((double)i + 1.) * vtkMath::Pi() / nx);
    double shift_z2 = nz * 0.5 * sin(((double)i + 2.) * vtkMath::Pi() / nx);
    double shift_faille = (i > nx / 2) ? 0. : nz * 0.3333;

    for (int j = extent[2]; j < extent[3]; j++)
    {
      for (int k = extent[4]; k < extent[5]; k++)
      {
        vtkIdType cellId = grid->ComputeCellId(i, j, k);
        vtkIdType* indice = grid->GetCellPoints(cellId);
        indice[0] = points->InsertNextPoint(i - 0.5, j - 0.5, GetZShift1(k - 0.5));
        indice[1] = points->InsertNextPoint(i + 0.5, j - 0.5, GetZShift2(k - 0.5));
        indice[2] = points->InsertNextPoint(i + 0.5, j + 0.5, GetZShift2(k - 0.5));
        indice[3] = points->InsertNextPoint(i - 0.5, j + 0.5, GetZShift1(k - 0.5));
        indice[4] = points->InsertNextPoint(i - 0.5, j - 0.5, GetZShift1(k + 0.5));
        indice[5] = points->InsertNextPoint(i + 0.5, j - 0.5, GetZShift2(k + 0.5));
        indice[6] = points->InsertNextPoint(i + 0.5, j + 0.5, GetZShift2(k + 0.5));
        indice[7] = points->InsertNextPoint(i - 0.5, j + 0.5, GetZShift1(k + 0.5));

        double dist = sqrt(static_cast<double>(i * i + j * j + k * k));
        dist_array->SetValue(cellId, dist);
        rank_array->SetValue(cellId, rank);
        coords_array->SetTuple3(cellId, i, j, k);
        grid->UnBlankCell(cellId);
      }
    }
  }
  grid->SetPoints(points);
  grid->GetCellData()->AddArray(dist_array);
  grid->GetCellData()->AddArray(rank_array);
  grid->GetCellData()->AddArray(coords_array);
  grid->CheckAndReorderFaces();
  grid->ComputeFacesConnectivityFlagsArray();
}

//-----------------------------------------------------------------------------
void ContinuousGridGenerator(
  int rank, int wholeExtent[6], int extent[6], vtkExplicitStructuredGrid* grid)
{
  grid->SetExtent(extent);

  double nx = wholeExtent[1] - wholeExtent[0];
  double nz = wholeExtent[5] - wholeExtent[4];

  int lnx = extent[1] - extent[0];
  int lny = extent[3] - extent[2];
  int lnz = extent[5] - extent[4];
  vtkIdType expectedCells = lnx * lny * lnz;

  int npx = lnx + 1;
  int npy = lny + 1;
  int npz = lnz + 1;

  vtkNew<vtkPoints> points;
  points->Allocate(npx * npy * npz);

  vtkNew<vtkFloatArray> dist_array;
  dist_array->SetName("Distance2Origin");
  dist_array->SetNumberOfValues(expectedCells);

  vtkNew<vtkUnsignedShortArray> rank_array;
  rank_array->SetName("Rank");
  rank_array->SetNumberOfValues(expectedCells);

  vtkNew<vtkUnsignedShortArray> coords_array;
  coords_array->SetName("Coordinates");
  coords_array->SetNumberOfComponents(3);
  coords_array->SetNumberOfTuples(expectedCells);

  for (vtkIdType i = 0; i < expectedCells; i++)
  {
    // Blank after copying the cell data to ensure it is not overwrited
    grid->BlankCell(i);
  }

  for (int i = extent[0]; i < extent[1] + 1; i++)
  {
    double shift_z = nz * 0.5 * sin(((double)i + 1.) * vtkMath::Pi() / nx);

    for (int j = extent[2]; j < extent[3] + 1; j++)
    {
      for (int k = extent[4]; k < extent[5] + 1; k++)
      {
        double z = k + shift_z;
        points->InsertNextPoint(i, j, z);
      }
    }
  }

  for (int i = extent[0], ii = 0; i < extent[1]; i++, ii++)
  {
    for (int j = extent[2], jj = 0; j < extent[3]; j++, jj++)
    {
      for (int k = extent[4], kk = 0; k < extent[5]; k++, kk++)
      {
        vtkIdType cellId = grid->ComputeCellId(i, j, k);
        vtkIdType* indice = grid->GetCellPoints(cellId);
        indice[0] = (kk) + (jj) * (lnz + 1) + (ii) * (lnz + 1) * (lny + 1);
        indice[1] = (kk + 1) + (jj) * (lnz + 1) + (ii) * (lnz + 1) * (lny + 1);
        indice[2] = (kk + 1) + (jj + 1) * (lnz + 1) + (ii) * (lnz + 1) * (lny + 1);
        indice[3] = (kk) + (jj + 1) * (lnz + 1) + (ii) * (lnz + 1) * (lny + 1);
        indice[4] = (kk) + (jj) * (lnz + 1) + (ii + 1) * (lnz + 1) * (lny + 1);
        indice[5] = (kk + 1) + (jj) * (lnz + 1) + (ii + 1) * (lnz + 1) * (lny + 1);
        indice[6] = (kk + 1) + (jj + 1) * (lnz + 1) + (ii + 1) * (lnz + 1) * (lny + 1);
        indice[7] = (kk) + (jj + 1) * (lnz + 1) + (ii + 1) * (lnz + 1) * (lny + 1);
        assert(cellId < expectedCells);
        double dist = sqrt(static_cast<double>(i * i + j * j + k * k));
        dist_array->SetValue(cellId, dist);
        rank_array->SetValue(cellId, rank);
        coords_array->SetTuple3(cellId, i, j, k);
        grid->UnBlankCell(cellId);
      }
    }
  }
  grid->SetPoints(points);
  grid->GetCellData()->AddArray(dist_array);
  grid->GetCellData()->AddArray(rank_array);
  grid->GetCellData()->AddArray(coords_array);
  grid->CheckAndReorderFaces();
  grid->ComputeFacesConnectivityFlagsArray();
}

//-----------------------------------------------------------------------------
void StepsGridGenerator(
  int rank, int wholeExtent[6], int extent[6], vtkExplicitStructuredGrid* grid, int nsteps)
{
  // Set the extent and use the generated cells
  grid->SetExtent(extent);
  srand(1);

  double nx = wholeExtent[1] - wholeExtent[0];
  double nz = wholeExtent[5] - wholeExtent[4];

  int lnx = extent[1] - extent[0];
  int lny = extent[3] - extent[2];
  int lnz = extent[5] - extent[4];
  vtkIdType expectedCells = lnx * lny * lnz;

  int lstep = (nx / nsteps);

  vtkNew<vtkPoints> points;
  points->Allocate(expectedCells * 8);

  vtkNew<vtkFloatArray> dist_array;
  dist_array->SetName("Distance2Origin");
  dist_array->SetNumberOfValues(expectedCells);

  vtkNew<vtkUnsignedShortArray> rank_array;
  rank_array->SetName("Rank");
  rank_array->SetNumberOfValues(expectedCells);

  vtkNew<vtkUnsignedShortArray> coords_array;
  coords_array->SetName("Coordinates");
  coords_array->SetNumberOfComponents(3);
  coords_array->SetNumberOfTuples(expectedCells);

  for (vtkIdType i = 0; i < expectedCells; i++)
  {
    // Blank after copying the cell data to ensure it is not overwrited
    grid->BlankCell(i);
  }

  // Index cells
  int icmin = 0;
  int icmax = 0;
  // Index (1D cooordinate) point
  int ipmin = -1;
  int ipmax = -1;

  double coef_noise = 0.1111;
  double coef_fault = 0.3333;

  int firstipmin = -1;

  for (int istep = 0; istep < nsteps; istep++)
  {
    double shift_step = nz * coef_noise * rand() / RAND_MAX;
    double shift_fault = 0.;

    if (istep > nsteps / 2)
    {
      shift_fault = nz * coef_fault;
    }

    if (istep == nsteps - 1)
    {
      // last steps
      icmax = nx - 1;
    }
    else
    {
      icmax = icmin + lstep - 1;
    }

    ipmin = ipmax + 1;
    ipmax = ipmin + (icmax - icmin) + 1;

    int licmin = std::max(icmin, extent[0]);
    int licmax = std::min(icmax, extent[1] - 1);
    if (licmin <= licmax)
    {
      int lipmin = ipmin + (licmin - icmin);
      int lipmax = ipmax - (licmax - icmax);
      if (firstipmin == -1)
      {
        firstipmin = lipmin;
      }

      for (int i = lipmin; i <= lipmax; i++)
      {
        for (int j = extent[2]; j < extent[3] + 1; j++)
        {
          for (int k = extent[4]; k < extent[5] + 1; k++)
          {
            double x = i - istep;
            double y = j;
            double shift_bf = nz * 0.5 * sin((x + 1.) * vtkMath::Pi() / nx);
            double z = k + shift_bf + shift_step + shift_fault;
            shift_bf = 0.;
            points->InsertNextPoint(x, y, z);
          }
        }
      }

      for (int i = licmin; i <= licmax; i++)
      {
        for (int j = extent[2], jj = 0; j < extent[3]; j++, jj++)
        {
          for (int k = extent[4], kk = 0; k < extent[5]; k++, kk++)
          {
            int ii = (lipmin - firstipmin) + i - licmin;
            vtkIdType cellId = grid->ComputeCellId(i, j, k);
            vtkIdType* indice = grid->GetCellPoints(cellId);
            indice[0] = (kk) + (jj) * (lnz + 1) + (ii) * (lnz + 1) * (lny + 1);
            indice[1] = (kk + 1) + (jj) * (lnz + 1) + (ii) * (lnz + 1) * (lny + 1);
            indice[2] = (kk + 1) + (jj + 1) * (lnz + 1) + (ii) * (lnz + 1) * (lny + 1);
            indice[3] = (kk) + (jj + 1) * (lnz + 1) + (ii) * (lnz + 1) * (lny + 1);
            indice[4] = (kk) + (jj) * (lnz + 1) + (ii + 1) * (lnz + 1) * (lny + 1);
            indice[5] = (kk + 1) + (jj) * (lnz + 1) + (ii + 1) * (lnz + 1) * (lny + 1);
            indice[6] = (kk + 1) + (jj + 1) * (lnz + 1) + (ii + 1) * (lnz + 1) * (lny + 1);
            indice[7] = (kk) + (jj + 1) * (lnz + 1) + (ii + 1) * (lnz + 1) * (lny + 1);
            double dist = sqrt(static_cast<double>(i * i + j * j + k * k));
            assert(cellId < expectedCells);
            dist_array->SetValue(cellId, dist);
            rank_array->SetValue(cellId, rank);
            coords_array->SetTuple3(cellId, i, j, k);
            grid->UnBlankCell(cellId);
          }
        }
      }
    }

    icmin += lstep;
  }

  points->Squeeze();
  grid->SetPoints(points);
  grid->GetCellData()->AddArray(dist_array);
  grid->GetCellData()->AddArray(rank_array);
  grid->GetCellData()->AddArray(coords_array);
  grid->CheckAndReorderFaces();
  grid->ComputeFacesConnectivityFlagsArray();
}

//-----------------------------------------------------------------------------
void PyramidGridGenerator(
  int rank, int wholeExtent[6], int extent[6], vtkExplicitStructuredGrid* grid, int size)
{
  // Set the extent and use the generated cells
  grid->SetExtent(extent);

  double nx = wholeExtent[1] - wholeExtent[0];
  double nz = wholeExtent[5] - wholeExtent[4];

  int lnx = extent[1] - extent[0];
  int lny = extent[3] - extent[2];
  int lnz = extent[5] - extent[4];
  vtkIdType expectedCells = lnx * lny * lnz;

  vtkNew<vtkPoints> points;
  points->Allocate(expectedCells * 8);

  vtkNew<vtkFloatArray> dist_array;
  dist_array->SetName("Distance2Origin");
  dist_array->SetNumberOfValues(expectedCells);
  dist_array->FillValue(0);

  vtkNew<vtkUnsignedShortArray> rank_array;
  rank_array->SetName("Rank");
  rank_array->SetNumberOfValues(expectedCells);
  rank_array->FillValue(rank);

  vtkNew<vtkUnsignedShortArray> coords_array;
  coords_array->SetName("Coordinates");
  coords_array->SetNumberOfComponents(3);
  coords_array->SetNumberOfTuples(expectedCells);

  for (vtkIdType i = 0; i < expectedCells; i++)
  {
    // Blank after copying the cell data to ensure it is not overwrited
    grid->BlankCell(i);
  }

  for (int i = extent[0]; i < extent[1] + 1; i++)
  {
    double shift_z = nz * 0.5 * sin(((double)i + 1.) * vtkMath::Pi() / nx);

    for (int j = extent[2]; j < extent[3] + 1; j++)
    {
      for (int k = extent[4]; k < extent[5] + 1; k++)
      {
        double z = -(double)k + shift_z;
        points->InsertNextPoint(i, j, z);
      }
    }
  }

  for (int i = extent[0], ii = 0; i < extent[1]; i++, ii++)
  {
    for (int j = extent[2], jj = 0; j < extent[3]; j++, jj++)
    {
      for (int k = extent[4], kk = 0; k < extent[5]; k++, kk++)
      {

        bool insert = false;
        if ((i + j) <= size)
        {
          if ((k >= i) || (k >= j))
          {
            insert = true;
          }
        }
        else if (((i + j) > size) && (i < size) && (j < size))
        {
          if ((k >= (size - i)) || (k >= (size - j)))
          {
            insert = true;
          }
        }
        else
        {
          insert = true;
        }

        vtkIdType cellId = grid->ComputeCellId(i, j, k);
        if (insert)
        {
          vtkIdType* indice = grid->GetCellPoints(cellId);
          indice[0] = (kk) + (jj) * (lnz + 1) + (ii) * (lnz + 1) * (lny + 1);
          indice[1] = (kk + 1) + (jj) * (lnz + 1) + (ii) * (lnz + 1) * (lny + 1);
          indice[2] = (kk + 1) + (jj + 1) * (lnz + 1) + (ii) * (lnz + 1) * (lny + 1);
          indice[3] = (kk) + (jj + 1) * (lnz + 1) + (ii) * (lnz + 1) * (lny + 1);
          indice[4] = (kk) + (jj) * (lnz + 1) + (ii + 1) * (lnz + 1) * (lny + 1);
          indice[5] = (kk + 1) + (jj) * (lnz + 1) + (ii + 1) * (lnz + 1) * (lny + 1);
          indice[6] = (kk + 1) + (jj + 1) * (lnz + 1) + (ii + 1) * (lnz + 1) * (lny + 1);
          indice[7] = (kk) + (jj + 1) * (lnz + 1) + (ii + 1) * (lnz + 1) * (lny + 1);

          double dist = sqrt(static_cast<double>(i * i + j * j + k * k));
          dist_array->SetValue(cellId, dist);
          grid->UnBlankCell(cellId);
        }
        rank_array->SetValue(cellId, rank);
        coords_array->SetTuple3(cellId, i, j, k);
      }
    }
  }

  points->Squeeze();
  grid->SetPoints(points);
  grid->GetCellData()->AddArray(dist_array);
  grid->GetCellData()->AddArray(rank_array);
  grid->GetCellData()->AddArray(coords_array);
  grid->CheckAndReorderFaces();
  grid->ComputeFacesConnectivityFlagsArray();
}
}
