/*=========================================================================

  Module:    vtkMooseXfemClip.cxx
  Copyright (c) 2017 Battelle Energy Alliance, LLC

  Based on Visualization Toolkit
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMooseXfemClip.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkImplicitFunction.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNonMergingPointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkMooseXfemClip);

//----------------------------------------------------------------------------
vtkMooseXfemClip::vtkMooseXfemClip()
{
  this->OutputPointsPrecision = DEFAULT_PRECISION;
  this->Locator = nullptr;
}

//----------------------------------------------------------------------------
vtkMooseXfemClip::~vtkMooseXfemClip()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkMooseXfemClip::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkMooseXfemClip::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    // The vtkNonMergingPointLocator does not merge coincident points on
    // opposing sides of the cutting plane, which is essential for proper
    // visualization of fields that are discontinuous across that interface.
    this->Locator = vtkNonMergingPointLocator::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//----------------------------------------------------------------------------
int vtkMooseXfemClip::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* realInput = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  // We have to create a copy of the input because clip requires being
  // able to InterpolateAllocate point data from the input that is
  // exactly the same as output. If the input arrays and output arrays
  // are different vtkCell3D's Clip will fail. By calling InterpolateAllocate
  // here, we make sure that the output will look exactly like the output
  // (unwanted arrays will be eliminated in InterpolateAllocate). The
  // last argument of InterpolateAllocate makes sure that arrays are shallow
  // copied from realInput to input.
  vtkSmartPointer<vtkDataSet> input;
  input.TakeReference(realInput->NewInstance());
  input->CopyStructure(realInput);
  input->GetCellData()->PassData(realInput->GetCellData());
  input->GetPointData()->InterpolateAllocate(realInput->GetPointData(), 0, 0, 1);

  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  vtkPointData *inPD = input->GetPointData(), *outPD = output->GetPointData();
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  vtkPoints* newPoints;
  vtkFloatArray* cellScalars;
  vtkImplicitFunction* ClipFunction = NULL;
  vtkPlane* ClipPlane = nullptr;

  vtkDataArray* XFEMCutOriginArray = input->GetCellData()->GetArray("xfem_cut_origin_");
  vtkDataArray* XFEMCutNormalArray = input->GetCellData()->GetArray("xfem_cut_normal_");
  if (XFEMCutOriginArray && XFEMCutNormalArray)
  {
    ClipFunction = vtkPlane::New();
    ClipPlane = vtkPlane::SafeDownCast(ClipFunction);
  }
  else
  {
    std::cout << "Must provide xfem_cut_origin_[0-2] and xfem_cut_normal_[0-2] data to cut by plane"
              << std::endl;
    return 1;
  }

  vtkPoints* cellPts;
  double s;
  vtkIdType npts;
  const vtkIdType* pts;
  int cellType = 0;
  vtkIdType i;
  int j;
  vtkIdType estimatedSize;

  vtkDebugMacro(<< "Clipping dataset");

  // Initialize self; create output objects
  //
  if (numPts < 1)
  {
    vtkDebugMacro(<< "No data to clip");
    return 1;
  }

  // allocate the output and associated helper classes
  estimatedSize = numCells;
  estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }
  cellScalars = vtkFloatArray::New();
  cellScalars->Allocate(VTK_CELL_SIZE);
  vtkCellArray* conn = vtkCellArray::New();
  conn->Allocate(estimatedSize, estimatedSize / 2);
  conn->InitTraversal();
  vtkUnsignedCharArray* types = vtkUnsignedCharArray::New();
  types->Allocate(estimatedSize, estimatedSize / 2);
  vtkIdTypeArray* locs = vtkIdTypeArray::New();
  locs->Allocate(estimatedSize, estimatedSize / 2);
  newPoints = vtkPoints::New();

  // set precision for the points in the output
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    vtkPointSet* inputPointSet = vtkPointSet::SafeDownCast(input);
    if (inputPointSet)
    {
      newPoints->SetDataType(inputPointSet->GetPoints()->GetDataType());
    }
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPoints->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(VTK_DOUBLE);
  }

  newPoints->Allocate(numPts, numPts / 2);

  // locator used to merge potentially duplicate points
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPoints, input->GetBounds());

  vtkDataSetAttributes* tempDSA = vtkDataSetAttributes::New();
  tempDSA->InterpolateAllocate(inPD, 1, 2);
  outPD->InterpolateAllocate(inPD, estimatedSize, estimatedSize / 2);
  tempDSA->Delete();
  outCD = output->GetCellData();
  outCD->CopyAllocate(inCD, estimatedSize, estimatedSize / 2);

  // Process all cells and clip each in turn
  //
  int abort = 0;
  vtkIdType updateTime = numCells / 20 + 1; // update roughly every 5%
  vtkGenericCell* cell = vtkGenericCell::New();
  int num = 0;
  int numNew = 0;
  for (vtkIdType cellId = 0; cellId < numCells && !abort; cellId++)
  {
    if (!(cellId % updateTime))
    {
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
    }

    input->GetCell(cellId, cell);
    cellPts = cell->GetPoints();
    npts = cellPts->GetNumberOfPoints();

    double* Origin;
    Origin = XFEMCutOriginArray->GetTuple3(cellId);
    ClipPlane->SetOrigin(Origin);

    double* Normal;
    Normal = XFEMCutNormalArray->GetTuple3(cellId);
    double len = Normal[0] * Normal[0] + Normal[1] * Normal[1] + Normal[2] * Normal[2];
    len = sqrt(len);
    double tol = 1.e-15;
    if (len > tol)
    {
      // Normals are output from XFEM code facing outward from cut plane.
      // Needs to face into cut plane for vtk clip plane, so mult by -1.
      Normal[0] = -Normal[0] / len;
      Normal[1] = -Normal[1] / len;
      Normal[2] = -Normal[2] / len;
      ClipPlane->SetNormal(Normal);

      // evaluate plane cutting function
      for (i = 0; i < npts; i++)
      {
        cellPts = cell->GetPoints();
        s = ClipFunction->FunctionValue(cellPts->GetPoint(i));
        cellScalars->InsertTuple(i, &s);
      }
    }
    else // Don't cut this element
    {
      for (i = 0; i < npts; i++)
      {
        s = 1.0;
        cellScalars->InsertTuple(i, &s);
      }
    }

    double value = 0.0;
    int InsideOut = 0;

    // perform the clipping
    cell->Clip(
      value, cellScalars, this->Locator, conn, inPD, outPD, inCD, cellId, outCD, InsideOut);
    numNew = conn->GetNumberOfCells() - num;
    num = conn->GetNumberOfCells();

    for (j = 0; j < numNew; j++)
    {
      if (cell->GetCellType() == VTK_POLYHEDRON)
      {
        // Polyhedron cells have a special cell connectivity format
        //(nCell0Faces, nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...).
        // But we don't need to deal with it here. The special case is handled
        // by vtkUnstructuredGrid::SetCells(), which will be called next.
        types->InsertNextValue(VTK_POLYHEDRON);
      }
      else
      {
        locs->InsertNextValue(conn->GetTraversalLocation());
        conn->GetNextCell(npts, pts);

        // For each new cell added, got to set the type of the cell
        switch (cell->GetCellDimension())
        {
          case 0: // points are generated--------------------------------
            cellType = (npts > 1 ? VTK_POLY_VERTEX : VTK_VERTEX);
            break;

          case 1: // lines are generated---------------------------------
            cellType = (npts > 2 ? VTK_POLY_LINE : VTK_LINE);
            break;

          case 2: // polygons are generated------------------------------
            cellType = (npts == 3 ? VTK_TRIANGLE : (npts == 4 ? VTK_QUAD : VTK_POLYGON));
            break;

          case 3: // tetrahedra or wedges are generated------------------
            cellType = (npts == 4 ? VTK_TETRA : VTK_WEDGE);
            break;
        } // switch

        types->InsertNextValue(cellType);
      }
    } // for each new cell
  }   // for each cell

  cell->Delete();
  cellScalars->Delete();

  if (ClipFunction)
  {
    ClipFunction->Delete();
  }

  output->SetPoints(newPoints);
  output->SetCells(types, locs, conn);
  conn->Delete();
  types->Delete();
  locs->Delete();

  newPoints->Delete();
  this->Locator->Initialize(); // release any extra memory
  output->Squeeze();

  return 1;
}
