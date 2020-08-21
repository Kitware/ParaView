#include "FEDataStructures.h"

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

void InitializeGrid(Grid* grid, const unsigned int numPoints[3], const double spacing[3])
{
  grid->NumberOfPoints = 0;
  grid->Points = 0;
  grid->NumberOfCells = 0;
  grid->Cells = 0;
  if (numPoints[0] == 0 || numPoints[1] == 0 || numPoints[2] == 0)
  {
    printf("Must have a non-zero amount of points in each direction.\n");
  }
  // in parallel, we do a simple partitioning in the x-direction.
  int mpiSize = 1;
  int mpiRank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

  unsigned int startXPoint = mpiRank * numPoints[0] / mpiSize;
  unsigned int endXPoint = (mpiRank + 1) * numPoints[0] / mpiSize;
  if (mpiSize != mpiRank + 1)
  {
    endXPoint++;
  }

  // create the points -- slowest in the x and fastest in the z directions
  if (grid->Points != 0)
  {
    free(grid->Points);
  }
  unsigned int numXPoints = endXPoint - startXPoint;
  grid->Points = (double*)malloc(3 * sizeof(double) * numPoints[1] * numPoints[2] * numXPoints);
  unsigned int counter = 0;
  unsigned int i, j, k;
  for (i = startXPoint; i < endXPoint; i++)
  {
    for (j = 0; j < numPoints[1]; j++)
    {
      for (k = 0; k < numPoints[2]; k++)
      {
        grid->Points[counter] = i * spacing[0];
        grid->Points[counter + 1] = j * spacing[1];
        grid->Points[counter + 2] = k * spacing[2];
        counter += 3;
      }
    }
  }
  grid->NumberOfPoints = numPoints[1] * numPoints[2] * numXPoints;
  // create the hex cells
  if (grid->Cells != 0)
  {
    free(grid->Cells);
  }
  grid->Cells = (int64_t*)malloc(
    8 * sizeof(int64_t) * (numPoints[1] - 1) * (numPoints[2] - 1) * (numXPoints - 1));
  counter = 0;
  for (i = 0; i < numXPoints - 1; i++)
  {
    for (j = 0; j < numPoints[1] - 1; j++)
    {
      for (k = 0; k < numPoints[2] - 1; k++)
      {
        grid->Cells[counter] = i * numPoints[1] * numPoints[2] + j * numPoints[2] + k;
        grid->Cells[counter + 1] = (i + 1) * numPoints[1] * numPoints[2] + j * numPoints[2] + k;
        grid->Cells[counter + 2] =
          (i + 1) * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k;
        grid->Cells[counter + 3] = i * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k;
        grid->Cells[counter + 4] = i * numPoints[1] * numPoints[2] + j * numPoints[2] + k + 1;
        grid->Cells[counter + 5] = (i + 1) * numPoints[1] * numPoints[2] + j * numPoints[2] + k + 1;
        grid->Cells[counter + 6] =
          (i + 1) * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k + 1;
        grid->Cells[counter + 7] = i * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k + 1;
        counter += 8;
      }
    }
  }
  grid->NumberOfCells = (numPoints[1] - 1) * (numPoints[2] - 1) * (numXPoints - 1);
}

void FinalizeGrid(Grid* grid)
{
  if (grid->Points)
  {
    free(grid->Points);
    grid->Points = 0;
  }
  if (grid->Cells)
  {
    free(grid->Cells);
    grid->Cells = 0;
  }
  grid->NumberOfPoints = 0;
  grid->NumberOfCells = 0;
}

void InitializeAttributes(Attributes* attributes, Grid* grid)
{
  attributes->GridPtr = grid;
  attributes->Velocity = 0;
  attributes->Pressure = 0;
}

void UpdateFields(Attributes* attributes, double time)
{
  unsigned int numPoints = attributes->GridPtr->NumberOfPoints;
  if (attributes->Velocity != 0)
  {
    free(attributes->Velocity);
  }
  attributes->Velocity = (double*)malloc(sizeof(double) * numPoints * 3);
  unsigned int i;
  for (i = 0; i < numPoints; i++)
  {
    attributes->Velocity[i] = 0;
    attributes->Velocity[i + numPoints] = attributes->GridPtr->Points[i * 3 + 1] * time;
    attributes->Velocity[i + 2 * numPoints] = 0;
  }
  unsigned int numCells = attributes->GridPtr->NumberOfCells;
  if (attributes->Pressure != 0)
  {
    free(attributes->Pressure);
  }
  attributes->Pressure = (float*)malloc(sizeof(float) * numCells);
  for (i = 0; i < numCells; i++)
  {
    attributes->Pressure[i] = (float)time / 2.0;
  }
}
