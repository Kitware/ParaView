/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkCGNSWriter.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
Copyright (c) Maritime Research Institute Netherlands (MARIN)
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkCGNSWriter.h"

#include "vtkArrayIteratorIncludes.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"
#include "vtkThreshold.h"
#include "vtkToolkits.h" // for VTK_USE_PARALLEL
#include "vtkUnstructuredGrid.h"

#include "vtk_cgns.h"

#include <map>
#include <set>
#include <vector>

using namespace std;

// macro to check a CGNS operation that can return CG_OK or CG_ERROR
// the macro will set the 'error' (string) variable to the CGNS error
// and return false.
#define cg_check_operation(op)                                                                     \
  if (CG_OK != op)                                                                                 \
  {                                                                                                \
    error = cg_get_error();                                                                        \
    return false;                                                                                  \
  }

// CGNS starts counting at 1
#define CGNS_COUNTING_OFFSET 1

struct write_info
{
  int F, B, Z, Sol;
  int CellDim;
  bool WritePolygonalZone;

  write_info()
  {
    F = B = Z = Sol = 0;
    CellDim = 3;
    WritePolygonalZone = false;
  }
};

class vtkCGNSWriter::vtkPrivate
{
public:
  // open and initialize a CGNS file
  static bool InitCGNSFile(write_info& info, const char* filename, string& error);
  static bool WriteBase(write_info& info, const char* basename, string& error);

  // write a single data set to
  static bool WriteStructuredGrid(vtkStructuredGrid* sg, const char* file, string& error);
  static bool WritePointSet(vtkPointSet* grid, const char* file, string& error);

  // write a multi-block dataset to
  static bool WriteMultiBlock(vtkMultiBlockDataSet* mb, const char* file, string& error);
  static bool WriteMultiPiece(vtkMultiPieceDataSet* mp, const char* file, string& error);

protected:
  static bool WriteMultiBlock(write_info& info, vtkMultiBlockDataSet*, string& error);
  static bool WritePoints(write_info& info, vtkPoints* pts, string& error);

  static bool WriteFieldArray(write_info& info, const char* solution,
    CGNS_ENUMT(GridLocation_t) location, vtkDataSetAttributes* dsa, string& error);

  static bool WriteStructuredGrid(
    write_info& info, vtkStructuredGrid* sg, const char* zonename, string& error);
  static bool WritePointSet(
    write_info& info, vtkPointSet* grid, const char* zonename, string& error);
  static bool WritePolygonalZone(write_info& info, vtkPointSet* grid, string& error);
  static bool WriteCells(write_info& info, vtkPointSet* grid, string& error);
};

bool vtkCGNSWriter::vtkPrivate::WriteCells(write_info& info, vtkPointSet* grid, string& error)
{
  if (!grid)
  {
    error = "Grid pointer not valid.";
    return false;
  }

  if (info.WritePolygonalZone)
  {
    return WritePolygonalZone(info, grid, error);
  }

  // create a mapping of celltype to a list of cells of that type
  // then, write each cell type into a different section of the zone.
  map<unsigned char, vector<vtkIdType> > cellTypeMap;
  for (vtkIdType i = 0; i < grid->GetNumberOfCells(); ++i)
  {
    unsigned char cellType = grid->GetCellType(i);
    cellTypeMap[cellType].push_back(i);
  }

  cgsize_t nCellsWritten(CGNS_COUNTING_OFFSET);
  for (auto& entry : cellTypeMap)
  {
    unsigned char cellType = entry.first;
    CGNS_ENUMT(ElementType_t) cg_elem(CGNS_ENUMV(ElementTypeNull));
    const char* sectionname(nullptr);
    switch (cellType)
    {
      case VTK_TRIANGLE:
        cg_elem = CGNS_ENUMV(TRI_3);
        sectionname = "Elem_Triangles";
        break;
      case VTK_QUAD:
        cg_elem = CGNS_ENUMV(QUAD_4);
        sectionname = "Elem_Quads";
        break;
      case VTK_PYRAMID:
        cg_elem = CGNS_ENUMV(PYRA_5);
        sectionname = "Elem_Pyramids";
        break;
      case VTK_WEDGE:
        cg_elem = CGNS_ENUMV(PENTA_6);
        sectionname = "Elem_Wedges";
        break;
      case VTK_TETRA:
        cg_elem = CGNS_ENUMV(TETRA_4);
        sectionname = "Elem_Tetras";
        break;
      case VTK_HEXAHEDRON:
        cg_elem = CGNS_ENUMV(HEXA_8);
        sectionname = "Elem_Hexas";
        break;
      default:
        // report error?
        continue;
    }

    const vector<vtkIdType>& cellIdsOfType = entry.second;
    vector<cgsize_t> cellsOfTypeArray;

    for (auto& cellId : cellIdsOfType)
    {
      vtkCell* cell = grid->GetCell(cellId);
      int nIds = cell->GetNumberOfPoints();

      for (int j = 0; j < nIds; ++j)
      {
        cellsOfTypeArray.push_back(
          static_cast<cgsize_t>(cell->GetPointId(j) + CGNS_COUNTING_OFFSET));
      }
    }

    int dummy(0);
    cgsize_t start(nCellsWritten);
    cgsize_t end(static_cast<cgsize_t>(nCellsWritten + cellIdsOfType.size() - 1));
    int nBoundary(0);
    cg_check_operation(cg_section_write(info.F, info.B, info.Z, sectionname, cg_elem, start, end,
      nBoundary, cellsOfTypeArray.data(), &dummy));

    nCellsWritten += cellIdsOfType.size();
  }

  return true;
}

bool vtkCGNSWriter::vtkPrivate::WritePolygonalZone(
  write_info& info, vtkPointSet* grid, string& error)
{
  if (!grid)
  {
    error = "Grid pointer not valid.";
    return false;
  }

  // write all cells in the grid as polyhedra. One polyhedron consists of
  // multiple faces. The faces are written to the NGON_n Element_t array
  // and each cell references a face. Note that this does not have the
  // concept of shared faces!

  // if a cell is not a volume cell, write it as an NGON_n entry only.

  vector<cgsize_t> cellDataArr;
  vector<cgsize_t> faceDataArr;
  vector<cgsize_t> cellDataIdx;
  vector<cgsize_t> faceDataIdx;
  cellDataIdx.push_back(0);
  faceDataIdx.push_back(0);

  cgsize_t ngons(0), ncells(0);
  for (vtkIdType i = 0; i < grid->GetNumberOfCells(); ++i)
  {
    vtkCell* cell = grid->GetCell(i);
    int nFaces = cell->GetNumberOfFaces();
    if (nFaces == 0) // this is a tri, quad or polygon and has no faces. Yes it has 1 face, but 0
                     // are reported.
    {
      cgsize_t nPts = static_cast<cgsize_t>(cell->GetNumberOfPoints());
#if CGNS_VERSION >= 3400
      faceDataIdx.push_back(nPts);
#else
      faceDataArr.push_back(nPts);
#endif
      for (int p = 0; p < nPts; ++p)
      {
        faceDataArr.push_back(static_cast<cgsize_t>(CGNS_COUNTING_OFFSET + cell->GetPointId(p)));
      }
      ++ngons;
      continue;
    }

#if CGNS_VERSION >= 3400
    cellDataIdx.push_back(nFaces);
#else
    cellDataArr.push_back(nFaces);
#endif
    for (int f = 0; f < nFaces; ++f)
    {
      vtkCell* face = cell->GetFace(f);
      cgsize_t nPts = static_cast<cgsize_t>(face->GetNumberOfPoints());

      // NFACE_n references an ngon in the NGON_n array
      cellDataArr.push_back(CGNS_COUNTING_OFFSET + ngons); // ngons start counting at 0
#if CGNS_VERSION >= 3400
      faceDataIdx.push_back(nPts);
#else
      faceDataArr.push_back(nPts);
#endif
      for (int p = 0; p < nPts; ++p)
      {
        faceDataArr.push_back(static_cast<cgsize_t>(CGNS_COUNTING_OFFSET + face->GetPointId(p)));
      }
      ++ngons;
    }
    ++ncells;
  }

#if CGNS_VERSION >= 3400
  // update offsets for faces and cells
  for (size_t idx = 1; idx < faceDataIdx.size(); ++idx)
  {
    faceDataIdx[idx] += faceDataIdx[idx - 1];
  }
  for (size_t idx = 1; idx < cellDataIdx.size(); ++idx)
  {
    cellDataIdx[idx] += cellDataIdx[idx - 1];
  }
#endif

  int dummy(0);
  int nBoundary(0);
  if (ncells > 0)
  {
#if CGNS_VERSION >= 3400
    cg_check_operation(
      cg_poly_section_write(info.F, info.B, info.Z, "Elem_NFACE_n", CGNS_ENUMV(NFACE_n), 1 + ngons,
        ncells + ngons, nBoundary, cellDataArr.data(), cellDataIdx.data(), &dummy));
#else
    cg_check_operation(cg_section_write(info.F, info.B, info.Z, "Elem_NFACE_n", CGNS_ENUMV(NFACE_n),
      1 + ngons, ncells + ngons, nBoundary, cellDataArr.data(), &dummy));
#endif
  }

  if (ngons > 0)
  {
#if CGNS_VERSION >= 3400
    cg_check_operation(cg_poly_section_write(info.F, info.B, info.Z, "Elem_NGON_n",
      CGNS_ENUMV(NGON_n), 1, ngons, nBoundary, faceDataArr.data(), faceDataIdx.data(), &dummy));
#else
    cg_check_operation(cg_section_write(info.F, info.B, info.Z, "Elem_NGON_n", CGNS_ENUMV(NGON_n),
      1, ngons, nBoundary, faceDataArr.data(), &dummy));
#endif
  }

  return true;
}

bool vtkCGNSWriter::vtkPrivate::WritePointSet(
  write_info& info, vtkPointSet* grid, const char* zonename, string& error)
{
  cgsize_t nPts = static_cast<cgsize_t>(grid->GetNumberOfPoints());
  cgsize_t nCells = static_cast<cgsize_t>(grid->GetNumberOfCells());
  cgsize_t dim[3] = { nPts, nCells, 0 };

  bool isPolygonal(false);
  for (int i = 0; i < grid->GetNumberOfCells(); ++i)
  {
    vtkCell* cell = grid->GetCell(i);
    unsigned char cellType = cell->GetCellType();

    isPolygonal |= cellType == VTK_POLYHEDRON;
    isPolygonal |= cellType == VTK_POLYGON;
  }
  info.WritePolygonalZone = isPolygonal;

  cg_check_operation(
    cg_zone_write(info.F, info.B, zonename, dim, CGNS_ENUMV(Unstructured), &(info.Z)));

  vtkPoints* pts = grid->GetPoints();

  if (!WritePoints(info, pts, error))
  {
    return false;
  }

  if (!WriteCells(info, grid, error))
  {
    return false;
  }

  if (!WriteFieldArray(info, "PointData", CGNS_ENUMV(Vertex), grid->GetPointData(), error))
  {
    return false;
  }

  if (!WriteFieldArray(info, "CellData", CGNS_ENUMV(CellCenter), grid->GetCellData(), error))
  {
    return false;
  }

  return true;
}

bool vtkCGNSWriter::vtkPrivate::WritePointSet(vtkPointSet* grid, const char* file, string& error)
{
  write_info info;
  if (grid->IsA("vtkPolyData"))
  {
    info.CellDim = 2;
  }
  else if (grid->IsA("vtkUnstructuredGrid"))
  {
    info.CellDim = 1;
    for (int i = 0; i < grid->GetNumberOfCells(); ++i)
    {
      vtkCell* cell = grid->GetCell(i);
      int cellDim = cell->GetCellDimension();
      if (info.CellDim < cellDim)
      {
        info.CellDim = cellDim;
      }
    }
  }

  if (!InitCGNSFile(info, file, error))
  {
    return false;
  }
  if (!WriteBase(info, "Base", error))
  {
    return false;
  }

  bool rc = WritePointSet(info, grid, "Zone 1", error);
  cg_check_operation(cg_close(info.F)) return rc;
}

// writes a field array to a new solution
bool vtkCGNSWriter::vtkPrivate::WriteFieldArray(write_info& info, const char* solution,
  CGNS_ENUMT(GridLocation_t) location, vtkDataSetAttributes* dsa, string& error)
{
  if (!dsa)
  {
    error = "No valid pointer to dataset attributes.";
    return false;
  }

  int nArr = dsa->GetNumberOfArrays();
  if (nArr > 0)
  {
    vector<double> temp;

    int dummy(0);
    cg_check_operation(cg_sol_write(info.F, info.B, info.Z, solution, location, &(info.Sol)));
    for (int i = 0; i < nArr; ++i)
    {
      vtkDataArray* da = dsa->GetArray(i);
      if (!da)
        continue;

      temp.reserve(da->GetNumberOfTuples());
      if (da->GetNumberOfComponents() != 1)
      {
        string fieldName = da->GetName();
        if (da->GetNumberOfComponents() == 3)
        {
          // here we have to stripe the XYZ values, same as with the vertices.
          const char* const components[3] = { "X", "Y", "Z" };

          for (int idx = 0; idx < 3; ++idx)
          {
            for (vtkIdType t = 0; t < da->GetNumberOfTuples(); ++t)
            {
              double* tpl = da->GetTuple(t);
              temp.push_back(tpl[idx]);
            }

            string fieldComponentName = fieldName + components[idx];

            cg_check_operation(cg_field_write(info.F, info.B, info.Z, info.Sol,
              CGNS_ENUMV(RealDouble), fieldComponentName.c_str(), temp.data(), &dummy));

            temp.clear();
          }
        }
        else
        {
          vtkWarningWithObjectMacro(nullptr, << " Field " << da->GetName() << " has "
                                             << da->GetNumberOfComponents()
                                             << " components, which is not supported. Skipping...");
        }
      }
      else // 1-component field data.
      {
        // force to double precision, even if data type is single precision,
        // see https://gitlab.kitware.com/paraview/paraview/-/issues/18827
        for (vtkIdType t = 0; t < da->GetNumberOfTuples(); ++t)
        {
          double* tpl = da->GetTuple(t);
          temp.push_back(*tpl);
        }

        cg_check_operation(cg_field_write(info.F, info.B, info.Z, info.Sol, CGNS_ENUMV(RealDouble),
          da->GetName(), temp.data(), &dummy));

        temp.clear();
      }
    }
  }
  return true;
}

bool vtkCGNSWriter::vtkPrivate::WritePoints(write_info& info, vtkPoints* pts, string& error)
{
  // is there a better way to do this, other than creating temp array and striping X, Y an Z into
  // separate arrays?
  // maybe using the low-level API where I think a stride can be given while writing.
  const char* names[3] = { "CoordinateX", "CoordinateY", "CoordinateZ" };

  double* temp = new (nothrow) double[pts->GetNumberOfPoints()];
  if (!temp)
  {
    error = "Failed to allocate temporary array";
    return false;
  }

  for (int idx = 0; idx < 3; ++idx)
  {
    for (vtkIdType i = 0; i < pts->GetNumberOfPoints(); ++i)
    {
      double* xyz = pts->GetPoint(i);
      temp[i] = xyz[idx];
    }
    int dummy(0);
    if (CG_OK !=
      cg_coord_write(info.F, info.B, info.Z, CGNS_ENUMV(RealDouble), names[idx], temp, &dummy))
    {
      delete[] temp; // don't leak
      error = cg_get_error();
      return false;
    }
  }

  delete[] temp; // don't leak
  return true;
}

bool vtkCGNSWriter::vtkPrivate::InitCGNSFile(write_info& info, const char* file, string& error)
{
  cg_check_operation(cg_open(file, CG_MODE_WRITE, &(info.F)));
  return true;
}

bool vtkCGNSWriter::vtkPrivate::WriteBase(write_info& info, const char* basename, string& error)
{
  cg_check_operation(cg_base_write(info.F, basename, info.CellDim, 3, &(info.B)));
  return true;
}

bool vtkCGNSWriter::vtkPrivate::WriteStructuredGrid(
  write_info& info, vtkStructuredGrid* sg, const char* zonename, string& error)
{
  cgsize_t dim[9];

  // set the dimensions
  int* pointDims = sg->GetDimensions();
  int cellDims[3];
  int j;

  sg->GetCellDims(cellDims);

  if (!pointDims)
  {
    error = "Failed to get vertex dimensions.";
    return false;
  }

  // init dimensions
  for (int i = 0; i < 3; ++i)
  {
    dim[0 * 3 + i] = 1;
    dim[1 * 3 + i] = 0;
    dim[2 * 3 + i] = 0; // always 0 for structured
  }
  j = 0;
  for (int i = 0; i < 3; ++i)
  {
    // skip unitary index dimension
    if (pointDims[i] == 1)
    {
      continue;
    }
    dim[0 * 3 + j] = pointDims[i];
    dim[1 * 3 + j] = cellDims[i];
    j++;
  }
  // Repacking dimension in case j < 3 because CGNS expects a resized dim matrix
  // For instance if j == 2 then move from 3x3 matrix to 3x2 matrix
  for (int k = 1; (k < 3) && (j < 3); ++k)
  {
    for (int i = 0; i < j; ++i)
    {
      dim[j * k + i] = dim[3 * k + i];
    }
  }

  // create the structured zone. Cells are implicit
  cg_check_operation(
    cg_zone_write(info.F, info.B, zonename, dim, CGNS_ENUMV(Structured), &(info.Z)));

  vtkPoints* pts = sg->GetPoints();

  if (!WritePoints(info, pts, error))
  {
    return false;
  }

  if (!WriteFieldArray(info, "PointData", CGNS_ENUMV(Vertex), sg->GetPointData(), error))
  {
    return false;
  }

  if (!WriteFieldArray(info, "CellData", CGNS_ENUMV(CellCenter), sg->GetCellData(), error))
  {
    return false;
  }
  return true;
}

bool vtkCGNSWriter::vtkPrivate::WriteStructuredGrid(
  vtkStructuredGrid* sg, const char* file, string& error)
{
  write_info info;
  int* dims = sg->GetDimensions();
  info.CellDim = 0;
  for (int n = 0; n < 3; n++)
  {
    if (dims[n] > 1)
    {
      info.CellDim += 1;
    }
  }
  if (!InitCGNSFile(info, file, error) || !WriteBase(info, "Base", error))
  {
    return false;
  }
  bool rc = WriteStructuredGrid(info, sg, "Zone 1", error);
  cg_check_operation(cg_close(info.F));
  return rc;
}

struct entry
{
  vtkDataObject* obj;
  string name;
  entry(vtkDataObject* o, string objectName)
  {
    obj = o;
    name = objectName;
  }
};

void Flatten(vtkMultiBlockDataSet* mb, vector<entry>& o2d, vector<entry>& o3d, int zoneOffset)
{
  for (unsigned int i = 0; i < mb->GetNumberOfBlocks(); ++i)
  {
    string zonename = "Zone " + to_string(i + zoneOffset);

    vtkInformation* md(nullptr);
    if (mb->HasMetaData(i))
    {
      md = mb->GetMetaData(i);
      if (md->Has(vtkCompositeDataSet::NAME()))
      {
        zonename = md->Get(vtkCompositeDataSet::NAME());
        if (zonename.length() > 32)
        {
          string oldname(zonename);
          zonename = zonename.substr(0, 32);
          for (auto& e : o2d)
          {
            int j = 1;
            while (e.name == zonename && j < 100)
            {
              zonename = zonename.substr(0, j < 10 ? 31 : 30) + to_string(j);
              ++j;
            }
            // if there are 100 duplicate zones after truncation, give up.
            // an error will be given by CGNS that a duplicate name has been found.
          }
          for (auto& e : o3d)
          {
            int j = 1;
            while (e.name == zonename && j < 100)
            {
              zonename = zonename.substr(0, j < 10 ? 31 : 30) + to_string(j);
              ++j;
            }
            // if there are 100 duplicate zones after truncation, give up.
            // an error will be given by CGNS that a duplicate name has been found.
          }

          vtkWarningWithObjectMacro(
            nullptr, << "Zone name '" << oldname << "' has been truncated to '" << zonename
                     << " to conform to 32-character limit on names in CGNS.");
        }
      }
    }

    vtkDataObject* block = mb->GetBlock(i);
    auto nested = vtkMultiBlockDataSet::SafeDownCast(block);
    auto polydata = vtkPolyData::SafeDownCast(block);
    if (polydata)
    {
      o2d.push_back(entry(block, zonename));
    }
    else if (nested)
    {
      Flatten(nested, o2d, o3d, zoneOffset + 1);
    }
    else if (block)
    {
      int CellDim = 3;
      if (block->IsA("vtkStructuredGrid"))
      {
        vtkStructuredGrid* sg = vtkStructuredGrid::SafeDownCast(block);
        int* dims = sg->GetDimensions();
        CellDim = 0;
        for (int n = 0; n < 3; n++)
        {
          if (dims[n] > 1)
          {
            CellDim += 1;
          }
        }
      }
      else if (block->IsA("vtkUnstructuredGrid"))
      {
        vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(block);
        CellDim = 1;
        for (int n = 0; n < ug->GetNumberOfCells(); ++n)
        {
          vtkCell* cell = ug->GetCell(n);
          int curCellDim = cell->GetCellDimension();
          if (CellDim < curCellDim)
          {
            CellDim = curCellDim;
          }
        }
      }
      if (CellDim == 3)
      {
        o3d.push_back(entry(block, zonename));
      }
      else
      {
        o2d.push_back(entry(block, zonename));
      }
    }
  }
}

bool vtkCGNSWriter::vtkPrivate::WriteMultiBlock(
  write_info& info, vtkMultiBlockDataSet* mb, string& error)
{
  vector<entry> surfaceBlocks, volumeBlocks;
  Flatten(mb, surfaceBlocks, volumeBlocks, 0);

  if (volumeBlocks.size() > 0)
  {
    info.CellDim = 3;
    if (!WriteBase(info, "Base_Volume_Elements", error))
      return false;

    for (auto& e : volumeBlocks)
    {
      vtkStructuredGrid* sg = vtkStructuredGrid::SafeDownCast(e.obj);
      if (sg)
      {
        if (!WriteStructuredGrid(info, sg, e.name.c_str(), error))
        {
          return false;
        }
        continue;
      }
      vtkPointSet* ps = vtkPointSet::SafeDownCast(e.obj);
      if (ps)
      {
        if (!WritePointSet(info, ps, e.name.c_str(), error))
        {
          return false;
        }
        continue;
      }

      if (e.obj)
      {
        vtkErrorWithObjectMacro(
          nullptr, << "Writing of block type '" << e.obj->GetClassName() << "' not supported.");
      }
      else
      {
        vtkWarningWithObjectMacro(nullptr, << "Writing of unsupported block type skipped.");
      }
    }
  }

  if (surfaceBlocks.size() > 0)
  {
    info.CellDim = 2;
    if (!WriteBase(info, "Base_Surface_Elements", error))
      return false;

    for (auto& e : surfaceBlocks)
    {
      vtkStructuredGrid* sg = vtkStructuredGrid::SafeDownCast(e.obj);
      if (sg)
      {
        if (!WriteStructuredGrid(info, sg, e.name.c_str(), error))
        {
          return false;
        }
        continue;
      }
      vtkPointSet* ps = vtkPointSet::SafeDownCast(e.obj);
      if (ps)
      {
        if (!WritePointSet(info, ps, e.name.c_str(), error))
        {
          return false;
        }
      }
    }
  }

  return true;
}

bool vtkCGNSWriter::vtkPrivate::WriteMultiBlock(
  vtkMultiBlockDataSet* mb, const char* file, string& error)
{
  write_info info;
  if (!InitCGNSFile(info, file, error))
  {
    return false;
  }

  bool rc = WriteMultiBlock(info, mb, error);
  cg_check_operation(cg_close(info.F));
  return rc;
}

bool vtkCGNSWriter::vtkPrivate::WriteMultiPiece(
  vtkMultiPieceDataSet* vtkNotUsed(mp), const char* vtkNotUsed(file), string& error)
{
  // todo: multi-piece writing to a single zone. Requires extensive rework, but may be done together
  // with parallel writing implementation?
  error = "Not implemented.";
  return false;
}

vtkStandardNewMacro(vtkCGNSWriter);

vtkCGNSWriter::vtkCGNSWriter()
{
  this->FileName = (nullptr);
  this->OriginalInput = (nullptr);
  this->SetUseHDF5(true); // use the method, this will call the corresponding library method.
}

vtkCGNSWriter::~vtkCGNSWriter()
{
  delete[] this->FileName;
  if (this->OriginalInput)
  {
    this->OriginalInput->UnRegister(this);
  }
}

void vtkCGNSWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName " << (this->FileName ? this->FileName : "(none)") << endl;
}

int vtkCGNSWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }
  else if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }
  // generate the data
  else if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int vtkCGNSWriter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  // todo: support writing time steps
  // vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  // if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  //{
  //  this->NumberOfTimeSteps =
  //    inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  //}
  // else
  //{
  //  this->NumberOfTimeSteps = 0;
  //}

  return 1;
}

int vtkCGNSWriter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  // todo: support writing time steps
  // vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  // if (this->WriteAllTimeSteps &&
  //  inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  //{
  //  double* timeSteps =
  //    inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  //  double timeReq = timeSteps[this->CurrentTimeIndex];
  //  inputVector[0]->GetInformationObject(0)->Set
  //  (vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
  //}
  return 1;
}

int vtkCGNSWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

int vtkCGNSWriter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->FileName)
  {
    return 1;
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  this->OriginalInput = vtkDataObject::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (this->OriginalInput)
  {
    this->OriginalInput->Register(this);
  }

  this->WriteData();
  if (!WasWritingSuccessful)
    SetErrorCode(1L);

  return WasWritingSuccessful ? 1 : 0;
}

void vtkCGNSWriter::SetUseHDF5(bool value)
{
  this->UseHDF5 = value;
  cg_set_file_type(value ? CG_FILE_HDF5 : CG_FILE_ADF);
}

void vtkCGNSWriter::WriteData()
{
  WasWritingSuccessful = false;
  if (!this->FileName || !this->OriginalInput)
    return;

  string error;
  if (this->OriginalInput->IsA("vtkMultiBlockDataSet"))
  {
    vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(this->OriginalInput);
    WasWritingSuccessful = vtkCGNSWriter::vtkPrivate::WriteMultiBlock(mb, this->FileName, error);
  }
  else if (this->OriginalInput->IsA("vtkMultiPieceDataSet"))
  {
    vtkMultiPieceDataSet* ds = vtkMultiPieceDataSet::SafeDownCast(this->OriginalInput);
    WasWritingSuccessful = vtkCGNSWriter::vtkPrivate::WriteMultiPiece(ds, this->FileName, error);
  }
  else if (this->OriginalInput->IsA("vtkDataSet"))
  {
    if (this->OriginalInput->IsA("vtkStructuredGrid"))
    {
      vtkStructuredGrid* sg = vtkStructuredGrid::SafeDownCast(this->OriginalInput);
      WasWritingSuccessful =
        vtkCGNSWriter::vtkPrivate::WriteStructuredGrid(sg, this->FileName, error);
    }
    else if (this->OriginalInput->IsA("vtkPointSet"))
    {
      vtkPointSet* ug = vtkPointSet::SafeDownCast(this->OriginalInput);
      WasWritingSuccessful = vtkCGNSWriter::vtkPrivate::WritePointSet(ug, this->FileName, error);
    }
    else
    {
      error = string("Unsupported class type '") + this->OriginalInput->GetClassName() +
        "' on input.\nSupported types are vtkStructuredGrid, vtkPointSet, their subclasses and "
        "multi-block datasets of said classes.";
    }
  }
  else
  {
    vtkErrorMacro(<< "Unsupported class type '" << this->OriginalInput->GetClassName()
                  << "' on input.\nSupported types are vtkStructuredGrid, vtkPointSet, their "
                     "subclasses and multi-block datasets of said classes.");
  }
  if (!WasWritingSuccessful)
  {
    vtkErrorMacro(<< " Writing failed: " << error);
  }
}
