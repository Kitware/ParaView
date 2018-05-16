#include "FEDataStructures.h"

#include <iostream>
#include <mpi.h>
#include <stdlib.h>

#include <vtkCPInputDataDescription.h>
#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkDoubleArray.h>
#include <vtkExtentTranslator.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnstructuredGrid.h>

Grid::Grid(const unsigned int numberOfPoints[3], bool outputImageData, int numberOfGhostLevels)
{
  if (numberOfPoints[0] == 0 || numberOfPoints[1] == 0 || numberOfPoints[2] == 0)
  {
    std::cerr << "Must have a non-zero amount of points in each direction.\n";
  }
  int wholeExtent[6];
  for (int i = 0; i < 3; i++)
  {
    wholeExtent[2 * i] = 0;
    wholeExtent[2 * i + 1] = numberOfPoints[i] - 1;
  }
  // we use the ExtentTranslator to compute the partitioning
  int mpiSize = 1;
  int mpiRank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

  vtkNew<vtkExtentTranslator> extentTranslator;
  int extent[6];
  extentTranslator->PieceToExtentThreadSafe(
    mpiRank, mpiSize, numberOfGhostLevels, wholeExtent, extent, 3, 0);
  // create the grid
  if (outputImageData)
  {
    this->CreateImageData(extent);
  }
  else
  {
    this->CreateUnstructuredGrid(extent);
  }
  // set ghost level information...
  vtkNew<vtkUnsignedCharArray> ghostCells;
  ghostCells->SetNumberOfTuples(this->VTKGrid->GetNumberOfCells());
  ghostCells->SetName(vtkDataSetAttributes::GhostArrayName());
  ghostCells->Fill(0);
  this->VTKGrid->GetCellData()->AddArray(ghostCells);
  if (numberOfGhostLevels || mpiSize > 1)
  {
    // get the extent if we didn't have ghost levels so that we can compare
    // which cells are ghost cells
    int nonGhostLevelExtent[6];
    extentTranslator->PieceToExtentThreadSafe(
      mpiRank, mpiSize, 0, wholeExtent, nonGhostLevelExtent, 3, 0);
    vtkIdType counter = 0;
    for (int k = extent[4]; k < extent[5]; k++)
    {
      bool zGhost = k < nonGhostLevelExtent[4] || k >= nonGhostLevelExtent[5];
      for (int j = extent[2]; j < extent[3]; j++)
      {
        bool yGhost = j < nonGhostLevelExtent[2] || j >= nonGhostLevelExtent[3];
        for (int i = extent[0]; i < extent[1]; i++)
        {
          bool xGhost = i < nonGhostLevelExtent[0] || i >= nonGhostLevelExtent[1];
          if (xGhost || yGhost || zGhost)
          {
            unsigned char value = vtkDataSetAttributes::DUPLICATECELL;
            ghostCells->SetTypedTuple(counter, &value);
          }
          counter++;
        }
      }
    }
  }
}

vtkDataSet* Grid::GetVTKGrid()
{
  return this->VTKGrid;
}

void Grid::CreateImageData(int extent[6])
{
  vtkNew<vtkImageData> imageData;
  imageData->SetExtent(extent);
  this->VTKGrid = imageData;
}

void Grid::CreateUnstructuredGrid(int extent[6])
{
  // create the points -- slowest in the x and fastest in the z directions
  double coord[3] = { 0, 0, 0 };
  vtkNew<vtkPoints> points;
  for (int i = extent[0]; i <= extent[1]; i++)
  {
    coord[0] = static_cast<double>(i);
    for (int j = extent[2]; j <= extent[3]; j++)
    {
      coord[1] = static_cast<double>(j);
      for (int k = extent[4]; k <= extent[5]; k++)
      {
        coord[2] = static_cast<double>(k);
        points->InsertNextPoint(coord);
      }
    }
  }
  vtkNew<vtkUnstructuredGrid> unstructuredGrid;
  unstructuredGrid->Initialize();
  unstructuredGrid->SetPoints(points);
  // create the hex cells
  vtkIdType cellPoints[8];
  int numPoints[3] = { extent[1] - extent[0] + 1, extent[3] - extent[2] + 1,
    extent[5] - extent[4] + 1 };
  unstructuredGrid->Allocate((numPoints[0] - 1) * (numPoints[1] - 1) * (numPoints[2] - 1));
  for (int k = 0; k < numPoints[2] - 1; k++)
  {
    for (int j = 0; j < numPoints[1] - 1; j++)
    {
      for (int i = 0; i < numPoints[0] - 1; i++)
      {
        cellPoints[0] = i * numPoints[1] * numPoints[2] + j * numPoints[2] + k;
        cellPoints[1] = (i + 1) * numPoints[1] * numPoints[2] + j * numPoints[2] + k;
        cellPoints[2] = (i + 1) * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k;
        cellPoints[3] = i * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k;
        cellPoints[4] = i * numPoints[1] * numPoints[2] + j * numPoints[2] + k + 1;
        cellPoints[5] = (i + 1) * numPoints[1] * numPoints[2] + j * numPoints[2] + k + 1;
        cellPoints[6] = (i + 1) * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k + 1;
        cellPoints[7] = i * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k + 1;
        unstructuredGrid->InsertNextCell(VTK_HEXAHEDRON, 8, cellPoints);
      }
    }
  }
  this->VTKGrid = unstructuredGrid;
}

void Grid::UpdateField(double time, vtkCPInputDataDescription* inputDataDescription)
{
  if (inputDataDescription->IsFieldNeeded("Scalar", vtkDataObject::POINT))
  {
    vtkDoubleArray* scalar =
      vtkDoubleArray::FastDownCast(this->VTKGrid->GetPointData()->GetArray("Scalar"));
    if (scalar == nullptr)
    {
      scalar = vtkDoubleArray::New();
      scalar->SetNumberOfTuples(this->VTKGrid->GetNumberOfPoints());
      scalar->SetName("Scalar");
      this->VTKGrid->GetPointData()->AddArray(scalar);
      scalar->Delete(); // ok since VTKGrid is keeping a reference to this array
    }
    vtkIdType numPoints = this->VTKGrid->GetNumberOfPoints();
    for (vtkIdType pt = 0; pt < numPoints; pt++)
    {
      double coord[3];
      this->VTKGrid->GetPoint(pt, coord);
      double value = coord[1] * time;
      scalar->SetTypedTuple(pt, &value);
    }
  }
}
