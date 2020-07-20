/*=========================================================================

  Program:   ParaView
  Module:    vtkConvertPolyhedraFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2020 Menno Deij - van Rijswijk (MARIN)
-------------------------------------------------------------------------*/

#include "vtkConvertPolyhedraFilter.h"

#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGridBase.h"

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <vector>

vtkStandardNewMacro(vtkConvertPolyhedraFilter);

//------------------------------------------------------------------------------
void vtkConvertPolyhedraFilter::InsertNextPolyhedralCell(
  vtkUnstructuredGridBase* grid, vtkIdList* faces) const
{
  if (grid == nullptr || faces == nullptr)
  {
    vtkErrorMacro("Grid or faces list undefined.");
    return;
  }

  const vtkIdType nFaces = faces->GetId(0);
  switch (nFaces)
  {
    case 1:
    case 2:
    case 3:
    {
      vtkErrorMacro("Cell with < 4 faces not supported.");
      break;
    }
    case 4:
    {
      // VTK_TETRAHEDRON OR VTK_POLYHEDRON
      bool allTri(true);
      for (vtkIdType n = 1; n < faces->GetNumberOfIds(); n += 4)
      {
        const vtkIdType numberOfFaceVertices = faces->GetId(n);
        if (numberOfFaceVertices != 3)
        {
          allTri = false;
          break;
        }
      }
      if (!allTri)
      {
        grid->InsertNextCell(VTK_POLYHEDRON, faces);
      }
      else
      {
        std::set<vtkIdType> ids;
        vtkIdType offset(2);
        for (vtkIdType fi = 0; fi < 3; ++fi)
        {
          for (vtkIdType vi = 0; vi < 3; ++vi)
          {
            ids.insert(faces->GetId(offset + vi));
          }
          offset += 4; // 1 counter + 3 indices
        }
        if (ids.size() != 4)
        {
          vtkErrorMacro("4-sided cell with all triangles, but not 4 unique vertex indices.");
          return;
        }

        std::vector<vtkIdType> a(ids.begin(), ids.end());
        grid->InsertNextCell(VTK_TETRA, 4, a.data());
      }
      break;
    }

    case 5:
    case 6:
    {
      // get all the triangular, quadrangular and polyhedral faces
      std::vector<vtkIdType> triFaces, quadFaces, polyFaces;
      std::vector<std::vector<vtkIdType> > faceVertices;
      std::set<vtkIdType> cellVertices;
      std::map<vtkIdType, int> vertexFaceConnectivity;

      vtkIdType offset(1);
      for (vtkIdType fi = 0; fi < nFaces; ++fi)
      {
        const vtkIdType nVertices = faces->GetId(offset);
        if (nVertices == 3)
        {
          triFaces.push_back(fi);
        }
        else if (nVertices == 4)
        {
          quadFaces.push_back(fi);
        }
        else if (nVertices > 4)
        {
          polyFaces.push_back(fi);
          break; // for-loop.
        }

        std::vector<vtkIdType> face;
        ++offset; // first vertex index
        for (vtkIdType vi = 0; vi < nVertices; ++vi)
        {
          vtkIdType vertex = faces->GetId(offset + vi);
          cellVertices.emplace(vertex);
          vertexFaceConnectivity[vertex]++;
          face.emplace_back(vertex);
        }

        faceVertices.emplace_back(face);
        offset += nVertices;
      }

      if (!polyFaces.empty())
      {
        grid->InsertNextCell(VTK_POLYHEDRON, faces);
        break; // outer-switch
      }

      switch (nFaces) // inner-switch
      {
        case 5:
        {
          if (triFaces.size() == 2 && quadFaces.size() == 3)
          {
            // VTK_WEDGE
            auto& bottomFace = faceVertices[triFaces[0]];
            auto& topFace = faceVertices[triFaces[1]];

            std::array<std::vector<vtkIdType>*, 3> actualSides = { &faceVertices[quadFaces[0]],
              &faceVertices[quadFaces[1]], &faceVertices[quadFaces[2]] };

            // the next bit is tried two times, the second time with the top face reversed
            // if the first time the correct orientation was not found.
            bool ok(false);
            vtkIdType topFaceCandidate[3];
            for (int twoTimes = 0; twoTimes < 2 && !ok; ++twoTimes)
            {
              std::array<std::array<vtkIdType, 4>, 3> sides = { { { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
                { 0, 0, 0, 0 } } };

              for (offset = 0; offset < 3 && !ok; ++offset)
              {
                topFaceCandidate[0] = topFace[(0 + offset) % 3];
                topFaceCandidate[1] = topFace[(1 + offset) % 3];
                topFaceCandidate[2] = topFace[(2 + offset) % 3];

                for (int i = 0; i < 3; ++i)
                {
                  sides[i][0] = bottomFace[(0 + i) % 3];
                  sides[i][1] = bottomFace[(1 + i) % 3];
                  sides[i][3] = topFaceCandidate[(1 + i) % 3];
                  sides[i][2] = topFaceCandidate[(0 + i) % 3];
                }

                // next bit is: all of the sides must match at least one of the actual sides
                ok = std::all_of(
                  sides.begin(), sides.end(), [&actualSides](std::array<vtkIdType, 4>& side) {
                    return std::any_of(actualSides.begin(), actualSides.end(),
                      [&side](std::vector<vtkIdType>* actualSide) {
                        return std::all_of(actualSide->begin(), actualSide->end(),
                          [&side](const vtkIdType actualSideVertex) {
                            return std::find(side.begin(), side.end(), actualSideVertex) !=
                              side.end();
                          });
                      });
                  });
              }

              if (!ok)
              {
                // reverse topFace and try again
                std::reverse(topFace.begin(), topFace.end());
              }
            }

            if (ok)
            {
              vtkIdType wedge[6] = { bottomFace[0], bottomFace[1], bottomFace[2],
                topFaceCandidate[0], topFaceCandidate[1], topFaceCandidate[2] };

              grid->InsertNextCell(VTK_WEDGE, 6, wedge);
              break; // inner switch
            }
          }

          else if (triFaces.size() == 4 && quadFaces.size() == 1)
          {
            // VTK_PYRAMID
            std::vector<vtkIdType> quadFaceVerts = faceVertices[*quadFaces.begin()];

            auto topVertex = std::find_if(cellVertices.begin(), cellVertices.end(),
              [&vertexFaceConnectivity](const vtkIdType vi) {
                return vertexFaceConnectivity[vi] == 4;
              });

            if (topVertex == cellVertices.end())
            {
              vtkErrorMacro("Failed to find VTK_PYRAMID top vertex");
            }
            else
            {
              vtkIdType pyramid[5] = { quadFaceVerts[0], quadFaceVerts[1], quadFaceVerts[2],
                quadFaceVerts[3], *topVertex };

              grid->InsertNextCell(VTK_PYRAMID, 5, pyramid);
              break;
            }
          }

          grid->InsertNextCell(VTK_POLYHEDRON, faces);
          break;
        }

        case 6:
        {
          // VTK_HEXAHEDRON OR VTK_POLYHEDRON

          if (polyFaces.empty() && triFaces.empty() && cellVertices.size() == 8)
          {
            bool allThreeFaces =
              std::all_of(vertexFaceConnectivity.begin(), vertexFaceConnectivity.end(),
                [](std::pair<const vtkIdType, int>& t) { return t.second == 3; });

            if (allThreeFaces)
            {
              // ok, we have 6 quad faces, with 8 vertices and each vertex connected to exactly 3
              // faces this is a hexahedron. But how are vertices connected?

              // start with assigning the first face as the bottom face
              auto& bottomFace = faceVertices[quadFaces[0]];

              // look for the top face, which has no vertices in common with the bottom face
              vtkIdType topFaceIndex = 1;
              bool noCommonVertices(false);
              for (; topFaceIndex < 6; ++topFaceIndex)
              {
                auto& candidateTopFace = faceVertices[quadFaces[topFaceIndex]];
                noCommonVertices = std::all_of(candidateTopFace.begin(), candidateTopFace.end(),
                  [&bottomFace](const vtkIdType vi) {
                    return std::find(bottomFace.begin(), bottomFace.end(), vi) == bottomFace.end();
                  });
                if (noCommonVertices)
                {
                  // found the top face
                  break; // for
                }
              }

              if (noCommonVertices)
              {
                auto& topFace = faceVertices[quadFaces[topFaceIndex]];

                // populate the (actual) side faces
                std::array<std::vector<vtkIdType>*, 4> actualSides = { nullptr, nullptr, nullptr,
                  nullptr };

                for (int n = 1, i = 0; n < 6; ++n)
                {
                  if (n == topFaceIndex)
                  {
                    continue;
                  }
                  actualSides[i++] = &faceVertices[quadFaces[n]];
                }

                // find out where the top-face should start so as to be aligned
                // with the bottom face. This means determining the offset in the
                // topFace array AND trying it clock-wise and anti-clockwise

                vtkIdType topFaceCandidate[4];

                bool ok(false);
                for (int twoTimes = 0; twoTimes < 2 && !ok; ++twoTimes)
                {
                  for (offset = 0; offset < 4 && !ok; ++offset)
                  {
                    topFaceCandidate[0] = topFace[(0 + offset) % 4];
                    topFaceCandidate[1] = topFace[(1 + offset) % 4];
                    topFaceCandidate[2] = topFace[(2 + offset) % 4];
                    topFaceCandidate[3] = topFace[(3 + offset) % 4];

                    std::array<std::array<vtkIdType, 4>, 4> sides = { { { 0, 0, 0, 0 },
                      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } } };

                    for (int i = 0; i < 4; ++i)
                    {
                      sides[i][0] = bottomFace[(0 + i) % 4];
                      sides[i][1] = bottomFace[(1 + i) % 4];
                      sides[i][2] = topFaceCandidate[(0 + i) % 4];
                      sides[i][3] = topFaceCandidate[(1 + i) % 4];
                    }

                    // next bit is: all of the sides must match at least one of the actual sides
                    ok = std::all_of(
                      sides.begin(), sides.end(), [&actualSides](std::array<vtkIdType, 4>& side) {
                        return std::any_of(actualSides.begin(), actualSides.end(),
                          [&side](std::vector<vtkIdType>* actualSide) {
                            return std::all_of(actualSide->begin(), actualSide->end(),
                              [&side](vtkIdType actualSideVertex) {
                                return std::find(side.begin(), side.end(), actualSideVertex) !=
                                  side.end();
                              });
                          });
                      });
                  }

                  if (!ok)
                  {
                    std::reverse(topFace.begin(), topFace.end());
                  }
                }

                if (ok)
                {
                  vtkIdType hex[8] = { bottomFace[0], bottomFace[1], bottomFace[2], bottomFace[3],
                    topFaceCandidate[0], topFaceCandidate[1], topFaceCandidate[2],
                    topFaceCandidate[3] };

                  grid->InsertNextCell(VTK_HEXAHEDRON, 8, hex);
                  break; // inner switch
                }
              }
            }
          }

          grid->InsertNextCell(VTK_POLYHEDRON, faces);
          break; // inner switch
        }
        default:
          // it is not possible to get here because
          // this switch statement is nested inside the cases 5 and 6
          // which are both handled before the default: statement.
          // this code is only here to pacify the compiler
          // that would otherwise issue a warning
          break; // inner switch
      }
      break; // outer switch
    }

    default:
    {
      grid->InsertNextCell(VTK_POLYHEDRON, faces);
      break;
    }
  }
}
void vtkConvertPolyhedraFilter::InsertNextPolygonalCell(
  vtkUnstructuredGridBase* grid, vtkIdList* vertices) const
{
  if (grid == nullptr || vertices == nullptr)
  {
    vtkErrorMacro("Grid or vertex list undefined.");
    return;
  }

  const vtkIdType nVertices = vertices->GetNumberOfIds();
  if (nVertices < 3)
  {
    vtkErrorMacro("Polygonal cell with < 3 vertices not supported.");
    return;
  }

  switch (nVertices)
  {
    case 3:
    {
      grid->InsertNextCell(VTK_TRIANGLE, vertices);
      break;
    }
    case 4:
    {
      grid->InsertNextCell(VTK_QUAD, vertices);
      break;
    }
    default:
    {
      grid->InsertNextCell(VTK_POLYGON, vertices);
      break;
    }
  }
}

//------------------------------------------------------------------------------
int vtkConvertPolyhedraFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkUnstructuredGridBase* input =
    vtkUnstructuredGridBase::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGridBase* output =
    vtkUnstructuredGridBase::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input == nullptr || output == nullptr)
  {
    return 0;
  }

  // allocate
  output->Allocate(input->GetNumberOfCells());

  // re-use the point-data, cell-data and vertices
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());
  output->SetPoints(input->GetPoints());

  // get progress step
  const vtkIdType progressStep =
    std::max(static_cast<vtkIdType>(1), input->GetNumberOfCells() / 100);

  int n = 0;
  auto it = input->NewCellIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextCell())
  {
    const int cellType = it->GetCellType();

    if (cellType == VTK_POLYHEDRON)
    {
      InsertNextPolyhedralCell(output, it->GetFaces());
    }
    else if (cellType == VTK_POLYGON)
    {
      InsertNextPolygonalCell(output, it->GetPointIds());
    }
    else
    {
      output->InsertNextCell(cellType, it->GetPointIds());
    }

    // update progress
    if (n % progressStep == 0)
    {
      const double fraction = static_cast<double>(n) / (progressStep * 100);
      this->UpdateProgress(fraction);
    }
    ++n;
  }
  it->Delete();

  return 1;
}

//------------------------------------------------------------------------------
void vtkConvertPolyhedraFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
