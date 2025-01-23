// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Maritime Research Institute Netherlands (MARIN)
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCGNSWriter.h"

#include "vtkAppendDataSets.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkImageToStructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridToPointSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include <vtksys/SystemTools.hxx>

// clang-format off
#include "vtk_cgns.h"
#include VTK_CGNS(cgnslib.h)
// clang-format on

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// macro to check a CGNS operation that can return CG_OK or CG_ERROR
// the macro will set the 'error' (string) variable to the CGNS error
// and return false.
#define cg_check_operation(op)                                                                     \
  if (CG_OK != (op))                                                                               \
  {                                                                                                \
    error = std::string(__FUNCTION__) + ":" + std::to_string(__LINE__) + "> " + cg_get_error();    \
    return false;                                                                                  \
  }

// CGNS starts counting at 1
#define CGNS_COUNTING_OFFSET 1

//------------------------------------------------------------------------------
struct write_info
{
  int F, B, Z, Sol;
  int CellDim;
  bool WritePolygonalZone;
  double TimeStep;
  const char* FileName;
  const char* BaseName;
  const char* ZoneName;
  const char* SolutionName;

  std::map<std::string, int> SolutionNames;

  write_info()
  {
    F = B = Z = Sol = 0;
    CellDim = 3;
    WritePolygonalZone = false;
    TimeStep = 0.0;

    FileName = nullptr;
    BaseName = nullptr;
    ZoneName = nullptr;
    SolutionName = nullptr;
  }
};

//------------------------------------------------------------------------------
class vtkCGNSWriter::vtkPrivate
{
public:
  // write a single data set to file
  static bool WriteStructuredGrid(
    vtkStructuredGrid* structuredGrid, write_info& info, std::string& error);
  static bool WritePointSet(vtkPointSet* grid, write_info& info, std::string& error);

  // write a composite dataset to file
  static bool WriteComposite(vtkCompositeDataSet* composite, write_info& info, std::string& error);

  static void Flatten(vtkCompositeDataSet* composite,
    std::vector<vtkSmartPointer<vtkDataObject>>& objects2D,
    std::vector<vtkSmartPointer<vtkDataObject>>& objects3d, int zoneOffset, std::string& name);
  static int DetermineCellDimension(vtkPointSet* pointSet);
  static void SetNameToLength(
    std::string& name, const std::vector<vtkSmartPointer<vtkDataObject>>& objects);
  static bool WriteGridsToBase(std::vector<vtkSmartPointer<vtkDataObject>> blocks,
    std::vector<vtkSmartPointer<vtkDataObject>> otherBlocks, write_info& info, std::string& error);

protected:
  // open and initialize a CGNS file
  static bool InitCGNSFile(write_info& info, std::string& error);
  static bool WriteBase(write_info& info, std::string& error);

  static bool WriteComposite(write_info& info, vtkCompositeDataSet*, std::string& error);
  static bool WritePoints(write_info& info, vtkPoints* pts, std::string& error);

  static bool WriteFieldArray(write_info& info, CGNS_ENUMT(GridLocation_t) location,
    vtkDataSetAttributes* dsa, std::map<unsigned char, std::vector<vtkIdType>>* cellTypeMap,
    std::string& error);

  static bool WriteStructuredGrid(
    write_info& info, vtkStructuredGrid* structuredGrid, std::string& error);
  static bool WritePointSet(write_info& info, vtkPointSet* grid, std::string& error);
  static bool WritePolygonalZone(write_info& info, vtkPointSet* grid, std::string& error);
  static bool WriteCells(write_info& info, vtkPointSet* grid,
    std::map<unsigned char, std::vector<vtkIdType>>& cellTypeMap, std::string& error);

  static bool WriteBaseTimeInformation(write_info& info, std::string& error);
  static bool WriteZoneTimeInformation(write_info& info, std::string& error);
};

//------------------------------------------------------------------------------
bool vtkCGNSWriter::vtkPrivate::WriteCells(write_info& info, vtkPointSet* grid,
  std::map<unsigned char, std::vector<vtkIdType>>& cellTypeMap, std::string& error)
{
  if (!grid)
  {
    error = "Grid pointer not valid.";
    return false;
  }

  if (info.WritePolygonalZone)
  {
    return vtkPrivate::WritePolygonalZone(info, grid, error);
  }

  for (vtkIdType i = 0; i < grid->GetNumberOfCells(); ++i)
  {
    unsigned char cellType = grid->GetCellType(i);
    cellTypeMap[cellType].push_back(i);
  }

  cgsize_t nCellsWritten(CGNS_COUNTING_OFFSET);
  for (auto& entry : cellTypeMap)
  {
    const unsigned char cellType = entry.first;
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

    const std::vector<vtkIdType>& cellIdsOfType = entry.second;
    std::vector<cgsize_t> cellsOfTypeArray;

    for (auto& cellId : cellIdsOfType)
    {
      vtkCell* cell = grid->GetCell(cellId);
      const int nIds = cell->GetNumberOfPoints();

      for (int j = 0; j < nIds; ++j)
      {
        cellsOfTypeArray.push_back(
          static_cast<cgsize_t>(cell->GetPointId(j) + CGNS_COUNTING_OFFSET));
      }
    }

    int dummy(0);
    const cgsize_t start(nCellsWritten);
    const cgsize_t end(static_cast<cgsize_t>(nCellsWritten + cellIdsOfType.size() - 1));
    const int nBoundary(0);
    cg_check_operation(cg_section_write(info.F, info.B, info.Z, sectionname, cg_elem, start, end,
      nBoundary, cellsOfTypeArray.data(), &dummy));

    nCellsWritten += cellIdsOfType.size();
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkCGNSWriter::vtkPrivate::WritePolygonalZone(
  write_info& info, vtkPointSet* grid, std::string& error)
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

  std::vector<cgsize_t> faceDataArr;
  std::vector<cgsize_t> faceDataIdx;
  faceDataIdx.push_back(0);

  cgsize_t ncells(0), ngons(0);
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

    for (int f = 0; f < nFaces; ++f)
    {
      vtkCell* face = cell->GetFace(f);
      cgsize_t nPts = static_cast<cgsize_t>(face->GetNumberOfPoints());

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

  std::vector<cgsize_t> cellDataArr;
  std::vector<cgsize_t> cellDataIdx;
  cellDataIdx.push_back(0);
  cgsize_t ngon(0);
  for (vtkIdType i = 0; i < grid->GetNumberOfCells(); ++i)
  {
    vtkCell* cell = grid->GetCell(i);
    int nFaces = cell->GetNumberOfFaces();
    if (nFaces == 0)
    {
      ++ngon;
      continue;
    }

#if CGNS_VERSION >= 3400
    cellDataIdx.push_back(nFaces);
#else
    cellDataArr.push_back(nFaces);
#endif
    for (int f = 0; f < nFaces; ++f)
    {
      // NFACE_n references an ngon in the NGON_n array
      // that array starts with offset 'ncells' so take that into account
      cellDataArr.push_back(CGNS_COUNTING_OFFSET + ncells + ngon); // ngon start counting at 0
      ++ngon;
    }
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
  if (ngons > 0)
  {
#if CGNS_VERSION >= 3400
    cg_check_operation(
      cg_poly_section_write(info.F, info.B, info.Z, "Elem_NGON_n", CGNS_ENUMV(NGON_n), 1 + ncells,
        ncells + ngons, nBoundary, faceDataArr.data(), faceDataIdx.data(), &dummy));
#else
    cg_check_operation(cg_section_write(info.F, info.B, info.Z, "Elem_NGON_n", CGNS_ENUMV(NGON_n),
      1 + ncells, ncells + ngons, nBoundary, faceDataArr.data(), &dummy));
#endif
  }

  if (ncells > 0)
  {
#if CGNS_VERSION >= 3400
    cg_check_operation(cg_poly_section_write(info.F, info.B, info.Z, "Elem_NFACE_n",
      CGNS_ENUMV(NFACE_n), 1, ncells, nBoundary, cellDataArr.data(), cellDataIdx.data(), &dummy));
#else
    cg_check_operation(cg_section_write(info.F, info.B, info.Z, "Elem_NFACE_n", CGNS_ENUMV(NFACE_n),
      1, ncells, nBoundary, cellDataArr.data(), &dummy));
#endif
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkCGNSWriter::vtkPrivate::WritePointSet(
  write_info& info, vtkPointSet* grid, std::string& error)
{
  const cgsize_t nPts = static_cast<cgsize_t>(grid->GetNumberOfPoints());
  const cgsize_t nCells = static_cast<cgsize_t>(grid->GetNumberOfCells());
  if (nPts == 0 && nCells == 0)
  {
    // don't write anything
    return true;
  }
  cgsize_t dim[3] = { nPts, nCells, 0 };

  bool isPolygonal(false);
  for (vtkIdType i = 0; i < grid->GetNumberOfCells(); ++i)
  {
    vtkCell* cell = grid->GetCell(i);
    const unsigned char cellType = cell->GetCellType();

    isPolygonal |= cellType == VTK_POLYHEDRON;
    isPolygonal |= cellType == VTK_POLYGON;
  }
  info.WritePolygonalZone = isPolygonal;

  cg_check_operation(
    cg_zone_write(info.F, info.B, info.ZoneName, dim, CGNS_ENUMV(Unstructured), &(info.Z)));

  vtkPoints* pts = grid->GetPoints();

  if (!vtkPrivate::WritePoints(info, pts, error))
  {
    return false;
  }
  std::map<unsigned char, std::vector<vtkIdType>> cellTypeMap;
  if (!vtkPrivate::WriteCells(info, grid, cellTypeMap, error))
  {
    return false;
  }

  info.SolutionName = "CellData";
  auto ptr = (isPolygonal ? nullptr : &cellTypeMap);
  if (!vtkPrivate::WriteFieldArray(info, CGNS_ENUMV(CellCenter), grid->GetCellData(), ptr, error))
  {
    return false;
  }

  info.SolutionName = "PointData";
  if (!vtkPrivate::WriteFieldArray(info, CGNS_ENUMV(Vertex), grid->GetPointData(), nullptr, error))
  {
    return false;
  }

  return vtkPrivate::WriteZoneTimeInformation(info, error);
}

//------------------------------------------------------------------------------
bool vtkCGNSWriter::vtkPrivate::WriteZoneTimeInformation(write_info& info, std::string& error)
{
  if (info.SolutionNames.empty())
  {
    return true;
  }

  auto at = info.SolutionNames.find("CellData");
  bool hasCellData = at != info.SolutionNames.end();
  int cellDataS = hasCellData ? at->second : -1;
  at = info.SolutionNames.find("PointData");
  bool hasVertData = at != info.SolutionNames.end();
  int vertDataS = hasVertData ? at->second : -1;

  cgsize_t dim[2] = { 32, 1 };
  if (!hasCellData && !hasVertData)
  {
    error = "No cell data or vert data found, but solution names not empty.";
    return false;
  }

  if (hasCellData || hasVertData)
  {
    cg_check_operation(cg_ziter_write(info.F, info.B, info.Z, "ZoneIterativeData_t"));
    cg_check_operation(cg_goto(info.F, info.B, "Zone_t", info.Z, "ZoneIterativeData_t", 1, "end"));

    if (hasCellData)
    {
      int sol[1] = { cellDataS };
      const char* timeStepNames = "CellData\0                       ";
      cg_check_operation(
        cg_array_write("FlowSolutionCellPointers", CGNS_ENUMV(Character), 2, dim, timeStepNames));
      cg_check_operation(cg_array_write("CellCenterIndices", CGNS_ENUMV(Integer), 1, &dim[1], sol));
      cg_check_operation(cg_descriptor_write("CellCenterPrefix", "CellCenter"));
    }

    if (hasVertData)
    {
      int sol[1] = { vertDataS };
      const char* timeStepNames = "PointData\0                      ";
      cg_check_operation(
        cg_array_write("FlowSolutionVertexPointers", CGNS_ENUMV(Character), 2, dim, timeStepNames));
      cg_check_operation(
        cg_array_write("VertexSolutionIndices", CGNS_ENUMV(Integer), 1, &dim[1], sol));
      cg_check_operation(cg_descriptor_write("VertexPrefix", "Vertex"));
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkCGNSWriter::vtkPrivate::WriteBaseTimeInformation(write_info& info, std::string& error)
{
  double time[1] = { info.TimeStep };

  cg_check_operation(cg_biter_write(info.F, info.B, "TimeIterValues", 1));
  cg_check_operation(cg_goto(info.F, info.B, "BaseIterativeData_t", 1, "end"));

  cgsize_t dimTimeValues[1] = { 1 };
  cg_check_operation(cg_array_write("TimeValues", CGNS_ENUMV(RealDouble), 1, dimTimeValues, time));

  cg_check_operation(cg_simulation_type_write(info.F, info.B, CGNS_ENUMV(TimeAccurate)));
  return true;
}

//------------------------------------------------------------------------------
bool vtkCGNSWriter::vtkPrivate::WritePointSet(
  vtkPointSet* grid, write_info& info, std::string& error)
{
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
      info.CellDim = std::max(info.CellDim, cellDim);
    }
  }

  if (!vtkPrivate::InitCGNSFile(info, error))
  {
    return false;
  }
  info.BaseName = "Base";
  if (!vtkPrivate::WriteBase(info, error))
  {
    return false;
  }

  info.ZoneName = "Zone 1";
  const bool rc = WritePointSet(info, grid, error);
  cg_check_operation(cg_close(info.F));
  return rc;
}

//------------------------------------------------------------------------------
namespace
{
/**
 * This function assigns the correct order of data elements for cases where
 * multiple element types are written in different sections in a CGNS zone.
 * Because all similar cells are grouped in the CGNS file, this means that
 * the order of the cells is different in the file compared to the data arrays.
 * To compensate for that, the data is reordered to follow the cell order in the file.
 */
void ReorderData(
  std::vector<double>& temp, std::map<unsigned char, std::vector<vtkIdType>>* cellTypeMap)
{
  std::vector<double> reordered(temp.size());

  size_t i(0);
  for (auto& entry : *cellTypeMap)
  {
    const std::vector<vtkIdType>& cellIdsOfType = entry.second;
    for (size_t j = 0; j < cellIdsOfType.size(); ++j)
    {
      reordered[i++] = temp[cellIdsOfType[j]];
    }
  }

  for (i = 0; i < temp.size(); ++i)
  {
    temp[i] = reordered[i];
  }
}
}

//------------------------------------------------------------------------------
// writes a field array to a new solution
bool vtkCGNSWriter::vtkPrivate::WriteFieldArray(write_info& info,
  CGNS_ENUMT(GridLocation_t) location, vtkDataSetAttributes* dsa,
  std::map<unsigned char, std::vector<vtkIdType>>* cellTypeMap, std::string& error)
{
  if (!dsa)
  {
    error = "No valid pointer to dataset attributes.";
    return false;
  }

  const int nArr = dsa->GetNumberOfArrays();
  if (nArr > 0)
  {
    std::vector<double> temp;

    int dummy(0);
    cg_check_operation(
      cg_sol_write(info.F, info.B, info.Z, info.SolutionName, location, &(info.Sol)));
    info.SolutionNames.emplace(info.SolutionName, info.Sol);

    for (int i = 0; i < nArr; ++i)
    {
      vtkDataArray* da = dsa->GetArray(i);
      if (!da)
        continue;

      temp.reserve(da->GetNumberOfTuples());
      if (da->GetNumberOfComponents() != 1)
      {
        const std::string fieldName = da->GetName();
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

            if (location == CGNS_ENUMV(CellCenter) && cellTypeMap)
            {
              ::ReorderData(temp, cellTypeMap);
            }

            std::string fieldComponentName = fieldName + components[idx];

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

        if (location == CGNS_ENUMV(CellCenter) && cellTypeMap)
        {
          ::ReorderData(temp, cellTypeMap);
        }

        cg_check_operation(cg_field_write(info.F, info.B, info.Z, info.Sol, CGNS_ENUMV(RealDouble),
          da->GetName(), temp.data(), &dummy));

        temp.clear();
      }
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkCGNSWriter::vtkPrivate::WritePoints(write_info& info, vtkPoints* pts, std::string& error)
{
  // is there a better way to do this, other than creating temp array and striping X, Y an Z into
  // separate arrays?
  // maybe using the low-level API where I think a stride can be given while writing.
  const char* names[3] = { "CoordinateX", "CoordinateY", "CoordinateZ" };

  double* temp = new (std::nothrow) double[pts->GetNumberOfPoints()];
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

//------------------------------------------------------------------------------
bool vtkCGNSWriter::vtkPrivate::InitCGNSFile(write_info& info, std::string& error)
{
  if (!info.FileName)
  {
    error = "File name not defined.";
    return false;
  }

  cg_check_operation(cg_open(info.FileName, CG_MODE_WRITE, &(info.F)));
  return true;
}

//------------------------------------------------------------------------------
bool vtkCGNSWriter::vtkPrivate::WriteBase(write_info& info, std::string& error)
{
  if (!info.BaseName)
  {
    error = "Base name not defined.";
    return false;
  }

  cg_check_operation(cg_base_write(info.F, info.BaseName, info.CellDim, 3, &(info.B)));
  return vtkPrivate::WriteBaseTimeInformation(info, error);
}

//------------------------------------------------------------------------------
bool vtkCGNSWriter::vtkPrivate::WriteStructuredGrid(
  write_info& info, vtkStructuredGrid* structuredGrid, std::string& error)
{
  if (structuredGrid->GetNumberOfCells() == 0 && structuredGrid->GetNumberOfPoints() == 0)
  {
    // don't write anything
    return true;
  }
  if (!info.ZoneName)
  {
    error = "Zone name not defined.";
    return false;
  }

  cgsize_t dim[9];

  // set the dimensions
  int pointDims[3] = { -1 };
  structuredGrid->GetDimensions(pointDims);
  int cellDims[3];
  int j;

  structuredGrid->GetCellDims(cellDims);

  if ((pointDims[0] < 0) || (pointDims[1] < 0) || (pointDims[2] < 0))
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
    cg_zone_write(info.F, info.B, info.ZoneName, dim, CGNS_ENUMV(Structured), &(info.Z)));

  vtkPoints* pts = structuredGrid->GetPoints();

  if (!vtkPrivate::WritePoints(info, pts, error))
  {
    return false;
  }

  info.SolutionName = "PointData";
  if (!vtkPrivate::WriteFieldArray(
        info, CGNS_ENUMV(Vertex), structuredGrid->GetPointData(), nullptr, error))
  {
    return false;
  }

  info.SolutionName = "CellData";
  if (!vtkPrivate::WriteFieldArray(
        info, CGNS_ENUMV(CellCenter), structuredGrid->GetCellData(), nullptr, error))
  {
    return false;
  }

  return vtkPrivate::WriteZoneTimeInformation(info, error);
}

//------------------------------------------------------------------------------
bool vtkCGNSWriter::vtkPrivate::WriteStructuredGrid(
  vtkStructuredGrid* structuredGrid, write_info& info, std::string& error)
{
  // get the structured grid IJK dimensions
  // and set CellDim to the correct value.
  int dims[3] = { 0 };
  structuredGrid->GetDimensions(dims);
  info.CellDim = 0;
  for (int n = 0; n < 3; n++)
  {
    if (dims[n] > 1)
    {
      info.CellDim += 1;
    }
  }

  info.BaseName = "Base";
  if (!vtkPrivate::InitCGNSFile(info, error) || !WriteBase(info, error))
  {
    return false;
  }

  info.ZoneName = "Zone 1";
  const bool rc = vtkPrivate::WriteStructuredGrid(info, structuredGrid, error);
  cg_check_operation(cg_close(info.F));
  return rc;
}

//------------------------------------------------------------------------------
int vtkCGNSWriter::vtkPrivate::DetermineCellDimension(vtkPointSet* pointSet)
{
  int CellDim = 0;
  vtkStructuredGrid* structuredGrid = vtkStructuredGrid::SafeDownCast(pointSet);
  if (structuredGrid)
  {
    int dims[3] = { 0 };
    structuredGrid->GetDimensions(dims);
    CellDim = 0;
    for (int n = 0; n < 3; n++)
    {
      if (dims[n] > 1)
      {
        CellDim += 1;
      }
    }
  }
  else
  {
    vtkUnstructuredGrid* unstructuredGrid = vtkUnstructuredGrid::SafeDownCast(pointSet);
    if (unstructuredGrid)
    {
      CellDim = 1;
      for (vtkIdType n = 0; n < unstructuredGrid->GetNumberOfCells(); ++n)
      {
        vtkCell* cell = unstructuredGrid->GetCell(n);
        int curCellDim = cell->GetCellDimension();
        CellDim = std::max(CellDim, curCellDim);
      }
    }
  }
  return CellDim;
}

//------------------------------------------------------------------------------
void vtkCGNSWriter::vtkPrivate::SetNameToLength(
  std::string& name, const std::vector<vtkSmartPointer<vtkDataObject>>& objects)
{
  if (name.length() > 32)
  {
    const std::string oldname(name);
    name = name.substr(0, 32);
    for (auto& e : objects)
    {
      int j = 1;
      const char* objectName(nullptr);
      vtkInformation* info = e->GetInformation();
      if (info && info->Has(vtkCompositeDataSet::NAME()))
      {
        objectName = info->Get(vtkCompositeDataSet::NAME());
      }
      while (objectName && objectName == name && j < 100)
      {
        name = name.substr(0, j < 10 ? 31 : 30) + std::to_string(j);
        ++j;
      }
      // if there are 100 duplicate zones after truncation, give up.
      // an error will be given by CGNS that a duplicate name has been found.
    }

    vtkWarningWithObjectMacro(nullptr, << "Zone name '" << oldname << "' has been truncated to '"
                                       << name
                                       << " to conform to 32-character limit on names in CGNS.");
  }
}

//------------------------------------------------------------------------------
void vtkCGNSWriter::vtkPrivate::Flatten(vtkCompositeDataSet* composite,
  std::vector<vtkSmartPointer<vtkDataObject>>& o2d,
  std::vector<vtkSmartPointer<vtkDataObject>>& o3d, int zoneOffset, std::string& name)
{
  vtkPartitionedDataSet* partitioned = vtkPartitionedDataSet::SafeDownCast(composite);
  if (partitioned)
  {
    if (partitioned->GetNumberOfPartitions() == 0)
    {
      return;
    }
    vtkNew<vtkAppendDataSets> append;
    append->SetMergePoints(true);
    if (name.empty())
    {
      name = "Zone " + std::to_string(zoneOffset);
    }

    for (unsigned i = 0; i < partitioned->GetNumberOfPartitions(); ++i)
    {
      vtkDataObject* partition = partitioned->GetPartitionAsDataObject(i);
      if (partition)
      {
        append->AddInputDataObject(partition);
        if (partitioned->HasMetaData(i) &&
          partitioned->GetMetaData(i)->Has(vtkCompositeDataSet::NAME()))
        {
          name = partitioned->GetMetaData(i)->Get(vtkCompositeDataSet::NAME());
        }
      }
    }
    append->Update();
    vtkDataObject* result = append->GetOutputDataObject(0);
    vtkPointSet* pointSet = vtkPointSet::SafeDownCast(result);
    if (pointSet)
    {
      pointSet->GetInformation()->Set(vtkCompositeDataSet::NAME(), name.c_str());
      const int cellDim = vtkPrivate::DetermineCellDimension(pointSet);
      if (cellDim == 3)
      {
        o3d.emplace_back(pointSet);
      }
      else
      {
        o2d.emplace_back(pointSet);
      }
    }
    return;
  }

  vtkSmartPointer<vtkDataObjectTreeIterator> iter;
  iter.TakeReference(vtkDataObjectTree::SafeDownCast(composite)->NewTreeIterator());
  iter->VisitOnlyLeavesOff();
  iter->TraverseSubTreeOff();
  iter->SkipEmptyNodesOff();
  int i(0);
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), ++i)
  {
    name = "Zone " + std::to_string(i + zoneOffset);
    if (iter->HasCurrentMetaData() && iter->GetCurrentMetaData()->Has(vtkCompositeDataSet::NAME()))
    {
      name = iter->GetCurrentMetaData()->Get(vtkCompositeDataSet::NAME());
    }
    vtkDataObject* curDO = iter->GetCurrentDataObject();
    if (!curDO)
    {
      continue;
    }

    vtkCompositeDataSet* subComposite = vtkCompositeDataSet::SafeDownCast(curDO);
    if (subComposite)
    {
      vtkPrivate::Flatten(subComposite, o2d, o3d, ++zoneOffset, name);
    }
    vtkPointSet* pointSet = vtkPointSet::SafeDownCast(curDO);
    if (pointSet)
    {
      if (!name.empty())
      {
        pointSet->GetInformation()->Set(vtkCompositeDataSet::NAME(), name);
      }

      const int cellDim = vtkPrivate::DetermineCellDimension(pointSet);
      if (cellDim == 3)
      {
        o3d.emplace_back(pointSet);
      }
      else
      {
        o2d.emplace_back(pointSet);
      }
    }
  }
}

//------------------------------------------------------------------------------
bool vtkCGNSWriter::vtkPrivate::WriteGridsToBase(std::vector<vtkSmartPointer<vtkDataObject>> blocks,
  std::vector<vtkSmartPointer<vtkDataObject>> otherBlocks, write_info& info, std::string& error)
{
  for (auto& block : blocks)
  {
    std::string name = block->GetInformation()->Get(vtkCompositeDataSet::NAME());
    vtkPrivate::SetNameToLength(name, blocks);
    vtkPrivate::SetNameToLength(name, otherBlocks);
    info.ZoneName = name.c_str();

    vtkStructuredGrid* structuredGrid = vtkStructuredGrid::SafeDownCast(block);
    vtkPointSet* pointSet = vtkPointSet::SafeDownCast(block);

    // test for structured grid first. it is also a vtkPointSet
    // but it needs to be written structured.
    if (structuredGrid)
    {
      if (!vtkPrivate::WriteStructuredGrid(info, structuredGrid, error))
      {
        return false;
      }
    }
    else if (pointSet)
    {
      if (!vtkPrivate::WritePointSet(info, pointSet, error))
      {
        return false;
      }
    }
    else if (block)
    {
      std::stringstream ss;
      ss << "Writing of block type '" << block->GetClassName() << "' not supported.";
      error = ss.str();
      return false;
    }
    else
    {
      vtkWarningWithObjectMacro(nullptr, << "Writing of empty block skipped.");
    }
    info.SolutionNames.clear();
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkCGNSWriter::vtkPrivate::WriteComposite(
  write_info& info, vtkCompositeDataSet* composite, std::string& error)
{
  std::vector<vtkSmartPointer<vtkDataObject>> surfaceBlocks, volumeBlocks;
  if (composite->GetNumberOfCells() == 0 && composite->GetNumberOfPoints() == 0)
  {
    // don't write anything
    return true;
  }

  std::string name;
  vtkPrivate::Flatten(composite, surfaceBlocks, volumeBlocks, 0, name);

  if (!volumeBlocks.empty())
  {
    info.CellDim = 3;
    info.BaseName = "Base_Volume_Elements";
    if (!vtkPrivate::WriteBase(info, error))
    {
      return false;
    }

    if (!vtkPrivate::WriteGridsToBase(volumeBlocks, surfaceBlocks, info, error))
    {
      return false;
    }
  }

  if (!surfaceBlocks.empty())
  {
    info.CellDim = 2;
    info.BaseName = "Base_Surface_Elements";
    if (!vtkPrivate::WriteBase(info, error))
    {
      return false;
    }
    if (!vtkPrivate::WriteGridsToBase(surfaceBlocks, volumeBlocks, info, error))
    {
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkCGNSWriter::vtkPrivate::WriteComposite(
  vtkCompositeDataSet* composite, write_info& info, std::string& error)
{
  if (!vtkPrivate::InitCGNSFile(info, error))
  {
    return false;
  }

  const bool rc = vtkPrivate::WriteComposite(info, composite, error);
  cg_check_operation(cg_close(info.F));
  return rc;
}

//------------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkCGNSWriter);

//------------------------------------------------------------------------------
vtkCGNSWriter::vtkCGNSWriter()
{
  // use the method, this will call the corresponding library method.
  this->SetUseHDF5(this->UseHDF5);
}

//------------------------------------------------------------------------------
void vtkCGNSWriter::SetUseHDF5(bool value)
{
  this->UseHDF5 = value;
  cg_set_file_type(value ? CG_FILE_HDF5 : CG_FILE_ADF);
}

//------------------------------------------------------------------------------
vtkCGNSWriter::~vtkCGNSWriter()
{
  this->SetFileName(nullptr);
  this->SetFileNameSuffix(nullptr);
  if (this->OriginalInput)
  {
    this->OriginalInput->UnRegister(this);
    this->OriginalInput = nullptr;
  }

  if (this->TimeValues)
  {
    this->TimeValues->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkCGNSWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "UseHDF5 " << (this->UseHDF5 ? "On" : "Off") << endl;
  os << indent << "WriteAllTimeSteps " << (this->WriteAllTimeSteps ? "On" : "Off") << endl;
  os << indent << "NumberOfTimeSteps " << this->NumberOfTimeSteps << endl;
  os << indent << "CurrentTimeIndex " << this->CurrentTimeIndex << endl;
  os << indent << "TimeValues " << (this->TimeValues ? this->TimeValues->GetName() : "(none)")
     << endl;
  os << indent << "OriginalInput "
     << (this->OriginalInput ? this->OriginalInput->GetClassName() : "(none)") << endl;
  os << indent << "WasWritingSuccessful " << (this->WasWritingSuccessful ? "Yes" : "No") << endl;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
int vtkCGNSWriter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->NumberOfTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  else
  {
    this->NumberOfTimeSteps = 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkCGNSWriter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->TimeValues)
  {
    this->TimeValues = vtkDoubleArray::New();
    vtkInformation* info = inputVector[0]->GetInformationObject(0);
    double* data = info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int len = info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->TimeValues->SetNumberOfValues(len);
    if (data)
    {
      std::copy(data, data + len, this->TimeValues->GetPointer(0));
    }
  }
  if (this->TimeValues && this->WriteAllTimeSteps)
  {
    if (this->TimeValues->GetPointer(0))
    {
      double timeReq = this->TimeValues->GetValue(this->CurrentTimeIndex);
      inputVector[0]->GetInformationObject(0)->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkCGNSWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkCGNSWriter::RequestData(vtkInformation* request, vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->FileName)
  {
    return 1;
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (this->OriginalInput)
  {
    this->OriginalInput->UnRegister(this);
  }

  this->OriginalInput = vtkDataObject::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (this->OriginalInput)
  {
    this->OriginalInput->Register(this);
  }

  // is this the first request
  if (this->CurrentTimeIndex == 0 && this->WriteAllTimeSteps)
  {
    // Tell the pipeline to start looping.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }

  this->WriteData();
  if (!this->WasWritingSuccessful)
  {
    this->SetErrorCode(1L);
  }

  this->CurrentTimeIndex++;
  if (this->CurrentTimeIndex >= this->NumberOfTimeSteps)
  {
    this->CurrentTimeIndex = 0;
    if (this->WriteAllTimeSteps)
    {
      // Tell the pipeline to stop looping.
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 0);
    }
  }

  return this->WasWritingSuccessful ? 1 : 0;
}

//-----------------------------------------------------------------------------
static bool SuffixValidation(char* fileNameSuffix)
{
  std::string suffix(fileNameSuffix);
  // Only allow this format: ABC%.Xd
  // ABC is an arbitrary string which may or may not exist
  // % and d must exist and d must be the last char
  // . and X may or may not exist, X must be an integer if it exists
  if (suffix.empty() || suffix[suffix.size() - 1] != 'd')
  {
    return false;
  }
  std::string::size_type lastPercentage = suffix.find_last_of('%');
  if (lastPercentage == std::string::npos)
  {
    return false;
  }
  if (suffix.size() - lastPercentage > 2 && !isdigit(suffix[lastPercentage + 1]) &&
    suffix[lastPercentage + 1] != '.')
  {
    return false;
  }
  for (std::string::size_type i = lastPercentage + 2; i < suffix.size() - 1; ++i)
  {
    if (!isdigit(suffix[i]))
    {
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkCGNSWriter::WriteData()
{
  this->WasWritingSuccessful = false;
  if (!this->FileName || !this->OriginalInput)
  {
    return;
  }

  write_info info;
  std::stringstream fileNameWithTimeStep;

  // formattedFileName string must be on outer context
  // such that the c_str() pointer is kept alive
  // while writing with it stored in info.FileName
  std::string formattedFileName;
  if (this->TimeValues && this->CurrentTimeIndex < this->TimeValues->GetNumberOfValues())
  {
    if (this->WriteAllTimeSteps && this->TimeValues->GetNumberOfValues() > 1)
    {
      const std::string fileNamePath = vtksys::SystemTools::GetFilenamePath(this->FileName);
      const std::string filenameNoExt =
        vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);
      const std::string extension = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
      if (this->FileNameSuffix && SuffixValidation(this->FileNameSuffix))
      {
        char suffix[100];
        snprintf(suffix, 100, this->FileNameSuffix, this->CurrentTimeIndex);
        if (!fileNamePath.empty())
        {
          fileNameWithTimeStep << fileNamePath << "/";
        }
        fileNameWithTimeStep << filenameNoExt << suffix << extension;
        formattedFileName = fileNameWithTimeStep.str();
        info.FileName = formattedFileName.c_str();
        info.TimeStep = this->TimeValues->GetValue(this->CurrentTimeIndex);
      }
      else
      {
        vtkErrorMacro(
          "Invalid file suffix:" << (this->FileNameSuffix ? this->FileNameSuffix : "null")
                                 << ". Expected valid % format specifiers!");
        return;
      }
    }
    else
    {
      if (this->OriginalInput->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()))
      {
        info.TimeStep = this->OriginalInput->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
      }
      else
      {
        info.TimeStep = 0.0;
      }
      info.FileName = this->FileName;
    }
  }
  else
  {
    info.FileName = this->FileName;
    info.TimeStep = 0.0;
  }

  std::string error;
  if (this->OriginalInput->IsA("vtkCompositeDataSet"))
  {
    vtkCompositeDataSet* composite = vtkCompositeDataSet::SafeDownCast(this->OriginalInput);
    this->WasWritingSuccessful = vtkCGNSWriter::vtkPrivate::WriteComposite(composite, info, error);
  }
  else if (this->OriginalInput->IsA("vtkDataSet"))
  {
    if (this->OriginalInput->IsA("vtkStructuredGrid"))
    {
      vtkStructuredGrid* structuredGrid = vtkStructuredGrid::SafeDownCast(this->OriginalInput);
      this->WasWritingSuccessful =
        vtkCGNSWriter::vtkPrivate::WriteStructuredGrid(structuredGrid, info, error);
    }
    else if (this->OriginalInput->IsA("vtkPointSet"))
    {
      vtkPointSet* unstructuredGrid = vtkPointSet::SafeDownCast(this->OriginalInput);
      this->WasWritingSuccessful =
        vtkCGNSWriter::vtkPrivate::WritePointSet(unstructuredGrid, info, error);
    }
    else if (this->OriginalInput->IsA("vtkRectilinearGrid"))
    {
      vtkRectilinearGrid* rectilinearGrid = vtkRectilinearGrid::SafeDownCast(this->OriginalInput);
      vtkNew<vtkRectilinearGridToPointSet> conv;
      conv->SetInputData(rectilinearGrid);
      conv->Update();
      vtkStructuredGrid* sg = conv->GetOutput();
      this->WasWritingSuccessful = vtkCGNSWriter::vtkPrivate::WriteStructuredGrid(sg, info, error);
    }
    else if (this->OriginalInput->IsA("vtkImageData"))
    {
      vtkImageData* cartesianGrid = vtkImageData::SafeDownCast(this->OriginalInput);
      vtkNew<vtkImageToStructuredGrid> conv;
      conv->SetInputData(cartesianGrid);
      conv->Update();
      vtkStructuredGrid* sg = conv->GetOutput();
      this->WasWritingSuccessful = vtkCGNSWriter::vtkPrivate::WriteStructuredGrid(sg, info, error);
    }
    else
    {
      error = std::string("Unsupported class type '") + this->OriginalInput->GetClassName() +
        "' on input.\nSupported types are vtkImageData, vtkRectilinearGrid, "
        "vtkStructuredGrid, vtkPointSet, their subclasses and composite "
        "datasets of said classes.";
    }
  }
  else
  {
    vtkErrorMacro(<< "Unsupported class type '" << this->OriginalInput->GetClassName()
                  << "' on input.\nSupported types are vtkImageData, vtkRectilinearGrid, "
                     "vtkStructuredGrid, vtkPointSet, their subclasses and composite "
                     "datasets of said classes.");
  }

  // the writer can be used for multiple timesteps
  // and the array is re-created at each use.
  // except when writing multiple timesteps
  if (!this->WriteAllTimeSteps && this->TimeValues)
  {
    this->TimeValues->Delete();
    this->TimeValues = nullptr;
  }

  if (!this->WasWritingSuccessful)
  {
    vtkErrorMacro(<< " Writing failed: " << error);
  }
}
