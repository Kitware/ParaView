/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaPolyDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkMantaPolyDataMapper.cxx

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkManta.h"
#include "vtkMantaActor.h"
#include "vtkMantaManager.h"
#include "vtkMantaPolyDataMapper.h"
#include "vtkMantaProperty.h"
#include "vtkMantaRenderer.h"
#include "vtkMantaTexture.h"

#include "vtkAppendPolyData.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkGlyph3D.h"
#include "vtkImageData.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkSphereSource.h"
#include "vtkStringArray.h"
#include "vtkTransform.h"
#include "vtkTubeFilter.h"
#include "vtkUnsignedCharArray.h"

#include <Core/Geometry/Vector.h>
#include <Engine/Control/RTRT.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Groups/Group.h>
#include <Model/Groups/Mesh.h>
#include <Model/Materials/Dielectric.h>
#include <Model/Materials/Flat.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/MetalMaterial.h>
#include <Model/Materials/Phong.h>
#include <Model/Materials/ThinDielectric.h>
#include <Model/Materials/Transparent.h>
#include <Model/Primitives/TextureCoordinateCylinder.h>
#include <Model/Primitives/TextureCoordinateSphere.h>
#include <Model/Primitives/WaldTriangle.h>
#include <Model/Textures/Constant.h>
#include <Model/Textures/TexCoordTexture.h>

#include <math.h>
#include <vector>

class vtkMantaPolyDataMapper::Helper
{
public:
  Helper() {}
  ~Helper() {}

  Manta::Material* material;
  std::vector<Manta::Vector> texCoords;
  std::map<int, Manta::Material*> extraMaterials;
  std::map<int, std::string> extrasSpecs;
  std::map<int, int> materialOffsets;

  Manta::Material* LookupMaterial(Manta::Mesh* mesh, vtkDataArray* ma, vtkIdType cellNum)
  {
    int offset = this->LookupMaterialID(ma, cellNum);
    return mesh->materials[offset];
  }

  int LookupMaterialID(vtkDataArray* ma, vtkIdType cellNum)
  {
    // converts material ID to an offset in the mess
    // return 0 - the default material if problematic
    int offset = 0;
    if (ma && this->extraMaterials.size())
    {
      int matID = (int)(*ma->GetTuple(cellNum));
      std::map<int, int>::iterator mit = this->materialOffsets.find(matID);
      if (mit != this->materialOffsets.end())
      {
        offset = mit->second;
      }
    }
    return offset;
  }
};

vtkStandardNewMacro(vtkMantaPolyDataMapper);

//----------------------------------------------------------------------------
// Construct empty object.
vtkMantaPolyDataMapper::vtkMantaPolyDataMapper()
{
  // cerr << "MM(" << this << ") CREATE" << endl;
  this->InternalColorTexture = NULL;
  this->MantaManager = NULL;
  this->PointSize = 1.0;
  this->LineWidth = 1.0;
  this->Representation = VTK_SURFACE;
  this->MyHelper = new Helper();
}

//----------------------------------------------------------------------------
// Destructor (don't call ReleaseGraphicsResources() since it is virtual
vtkMantaPolyDataMapper::~vtkMantaPolyDataMapper()
{
  // cerr << "MM(" << this << ") DESTROY" << endl;
  if (this->InternalColorTexture)
  {
    this->InternalColorTexture->Delete();
  }

  std::map<int, Manta::Material*>::iterator it;
  for (it = this->MyHelper->extraMaterials.begin(); it != this->MyHelper->extraMaterials.end();
       it++)
  {
    delete it->second;
  }

  if (this->MantaManager)
  {
    // cerr << "MM(" << this << ") DESTROY "
    //     << this->MantaManager << " "
    //     << this->MantaManager->GetReferenceCount() << endl;
    this->MantaManager->Delete();
  }

  delete this->MyHelper;
}

//----------------------------------------------------------------------------
// Release the graphics resources used by this mapper.  In this case, release
// the display list if any.
void vtkMantaPolyDataMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  // cerr << "MM(" << this << ") RELEASE GRAPHICS RESOURCES" << endl;
  this->Superclass::ReleaseGraphicsResources(win);

  if (this->InternalColorTexture)
  {
    this->InternalColorTexture->Delete();
  }
  this->InternalColorTexture = NULL;
}

//----------------------------------------------------------------------------
// Receives from Actor -> maps data to primitives
// called by Mapper->Render() (which is called by Actor->Render())
void vtkMantaPolyDataMapper::RenderPiece(vtkRenderer* ren, vtkActor* act)
{
  vtkMantaRenderer* mantaRenderer = vtkMantaRenderer::SafeDownCast(ren);
  if (!mantaRenderer)
  {
    return;
  }
  if (!this->MantaManager)
  {
    this->MantaManager = mantaRenderer->GetMantaManager();
    // cerr << "MM(" << this << ") REGISTER " << this->MantaManager << " "
    //     << this->MantaManager->GetReferenceCount() << endl;
    this->MantaManager->Register(this);
  }

  // write geometry, first ask the pipeline to update data
  vtkPolyData* input = this->GetInput();
  if (input == NULL)
  {
    vtkErrorMacro(<< "No input to vtkMantaPolyDataMapper!");
    return;
  }
  else
  {
    this->InvokeEvent(vtkCommand::StartEvent, NULL);

    // Static = 1:  this mapper does NOT need to propagate updates to other mappers
    // down the pipeline and therefore saves the time that would be otherwise taken
    if (!this->Static)
    {
      this->Update();
    }

    this->InvokeEvent(vtkCommand::EndEvent, NULL);

    vtkIdType numPts = input->GetNumberOfPoints();
    if (numPts == 0)
    {
      vtkDebugMacro(<< "No points from the input to vtkMantaPolyDataMapper!");
      input = NULL;
      return;
    }
  }

  if (this->LookupTable == NULL)
  {
    this->CreateDefaultLookupTable();
  }

  // TODO: vtkOpenGLPolyDataMapper uses OpenGL clip planes here

  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  this->MapScalars(act->GetProperty()->GetOpacity());

  if (this->ColorTextureMap)
  {
    if (!this->InternalColorTexture)
    {
      this->InternalColorTexture = vtkMantaTexture::New();
      this->InternalColorTexture->RepeatOff();
    }
    this->InternalColorTexture->SetInputData(this->ColorTextureMap);
  }

  // if the data defines any, make sure we have specific materials handy
  vtkMantaProperty* prop = vtkMantaProperty::SafeDownCast(act->GetProperty());
  this->MakeMantaProperties(input, prop->GetAllowDataMaterial());

  // if something has changed, regenerate Manta primitives if required
  if (this->GetMTime() > this->BuildTime || input->GetMTime() > this->BuildTime ||
    // act->GetMTime()   > this->BuildTime ||
    act->GetProperty()->GetMTime() > this->BuildTime ||
    act->GetMatrix()->GetMTime() > this->BuildTime)
  {
    // this->ReleaseGraphicsResources( ren->GetRenderWindow() );

    // If we are coloring by texture, then load the texture map.
    // Use Map as indicator, because texture hangs around.
    if (this->ColorTextureMap)
    {
      this->InternalColorTexture->Load(ren);
    }
    vtkMantaActor* mantaActor = vtkMantaActor::SafeDownCast(act);
    if (mantaActor->GetMantaTexture())
    {
      mantaActor->GetMantaTexture()->Load(ren);
    }

    this->Draw(ren, act);
    this->BuildTime.Modified();
  }

  input = NULL;
  // TODO: deal with timer ??
}

//----------------------------------------------------------------------------
void vtkMantaPolyDataMapper::DrawPolygons(vtkPolyData* polys, vtkPoints* ptarray, Manta::Mesh* mesh,
  Manta::Group* points, Manta::Group* lines, vtkMantaTexture* mantaTexture)
{
  std::vector<Manta::Vector>& texCoords = this->MyHelper->texCoords;
  Manta::Material* material = NULL;
  vtkDataArray* ma = polys->GetCellData()->GetArray("material");
  int total_triangles = 0;
  vtkCellArray* cells = polys->GetPolys();
  vtkIdType npts = 0, *index = 0, cellNum = 0;

  switch (this->Representation)
  {
    case VTK_POINTS:
    {
      for (cells->InitTraversal(); cells->GetNextCell(npts, index); cellNum++)
      {
        material = this->MyHelper->LookupMaterial(mesh, ma, cellNum);
        double coord[3];
        Manta::Vector noTC(0.0, 0.0, 0.0);
        for (int i = 0; i < npts; i++)
        {
          // TODO: Make option to scale pointsize by scalar
          ptarray->GetPoint(index[i], coord);
          Manta::TextureCoordinateSphere* sphere = new Manta::TextureCoordinateSphere(material,
            Manta::Vector(coord[0], coord[1], coord[2]), this->PointSize,
            (texCoords.size() ? texCoords[(this->CellScalarColor ? cellNum : index[i])] : noTC));
          points->add(sphere);
        }
        total_triangles++;
      }
      // cerr << "polygons: # of triangles = " << total_triangles << endl;
    } // VTK_POINTS;
    break;
    case VTK_WIREFRAME:
    {
      double coord0[3];
      double coord1[3];
      Manta::Vector noTC(0.0, 0.0, 0.0);
      Manta::TextureCoordinateCylinder* segment;
      for (cells->InitTraversal(); cells->GetNextCell(npts, index); cellNum++)
      {
        material = this->MyHelper->LookupMaterial(mesh, ma, cellNum);
        ptarray->GetPoint(index[0], coord0);
        for (vtkIdType i = 1; i < npts; i++)
        {
          // TODO: Make option to scale linewidth by scalar
          ptarray->GetPoint(index[i], coord1);
          segment = new Manta::TextureCoordinateCylinder(material,
            Manta::Vector(coord0[0], coord0[1], coord0[2]),
            Manta::Vector(coord1[0], coord1[1], coord1[2]), this->LineWidth,
            (texCoords.size() ? texCoords[(this->CellScalarColor ? cellNum : index[i - 1])] : noTC),
            (texCoords.size() ? texCoords[(this->CellScalarColor ? cellNum : index[i])] : noTC));
          lines->add(segment);
          coord0[0] = coord1[0];
          coord0[1] = coord1[1];
          coord0[2] = coord1[2];
        }
        ptarray->GetPoint(index[0], coord1);
        segment = new Manta::TextureCoordinateCylinder(material,
          Manta::Vector(coord0[0], coord0[1], coord0[2]),
          Manta::Vector(coord1[0], coord1[1], coord1[2]), this->LineWidth,
          (texCoords.size() ? texCoords[(this->CellScalarColor ? cellNum : index[npts - 1])]
                            : noTC),
          (texCoords.size() ? texCoords[(this->CellScalarColor ? cellNum : index[0])] : noTC));
        lines->add(segment);
      }
    } // VTK_WIREFRAME:
    break;
    case VTK_SURFACE:
    default:
    {

      // write polygons with on the fly triangulation, assuming polygons are simple and
      // can be triangulated into "fans"
      for (cells->InitTraversal(); cells->GetNextCell(npts, index); cellNum++)
      {
        int triangle[3];

        // the first triangle
        triangle[0] = index[0];
        triangle[1] = index[1];
        triangle[2] = index[2];
        mesh->vertex_indices.push_back(triangle[0]);
        mesh->vertex_indices.push_back(triangle[1]);
        mesh->vertex_indices.push_back(triangle[2]);
        mesh->face_material.push_back(this->MyHelper->LookupMaterialID(ma, cellNum));
        if (!mesh->vertexNormals.empty())
        {
          mesh->normal_indices.push_back(triangle[0]);
          mesh->normal_indices.push_back(triangle[1]);
          mesh->normal_indices.push_back(triangle[2]);
        }

        if (!mesh->texCoords.empty())
        {
          if (this->CellScalarColor && !mantaTexture)
          {
            mesh->texture_indices.push_back(cellNum);
            mesh->texture_indices.push_back(cellNum);
            mesh->texture_indices.push_back(cellNum);
          }
          else
          {
            mesh->texture_indices.push_back(triangle[0]);
            mesh->texture_indices.push_back(triangle[1]);
            mesh->texture_indices.push_back(triangle[2]);
          }
        }
        total_triangles++;

        // the remaining triangles, of which
        // each introduces a triangle after extraction
        for (int i = 3; i < npts; i++)
        {
          triangle[1] = triangle[2];
          triangle[2] = index[i];
          mesh->vertex_indices.push_back(triangle[0]);
          mesh->vertex_indices.push_back(triangle[1]);
          mesh->vertex_indices.push_back(triangle[2]);
          mesh->face_material.push_back(this->MyHelper->LookupMaterialID(ma, cellNum));

          if (!mesh->vertexNormals.empty())
          {
            mesh->normal_indices.push_back(triangle[0]);
            mesh->normal_indices.push_back(triangle[1]);
            mesh->normal_indices.push_back(triangle[2]);
          }

          if (!mesh->texCoords.empty())
          {
            if (this->CellScalarColor && !mantaTexture)
            {
              mesh->texture_indices.push_back(cellNum);
              mesh->texture_indices.push_back(cellNum);
              mesh->texture_indices.push_back(cellNum);
            }
            else
            {
              mesh->texture_indices.push_back(triangle[0]);
              mesh->texture_indices.push_back(triangle[1]);
              mesh->texture_indices.push_back(triangle[2]);
            }
          }
          total_triangles++;
        }
      }
      // cerr << "polygons: # of triangles = " << total_triangles << endl;

      for (int i = 0; i < total_triangles; i++)
      {
        mesh->addTriangle(new Manta::WaldTriangle);
      }
    } // VTK_SURFACE
    break;
  }
}

//----------------------------------------------------------------------------
void vtkMantaPolyDataMapper::DrawTStrips(vtkPolyData* polys, vtkPoints* ptarray, Manta::Mesh* mesh,
  Manta::Group* points, Manta::Group* lines, vtkMantaTexture* mantaTexture)
{
  // cerr << "TSTRIPS" << endl;
  Manta::Material* material = this->MyHelper->material;
  std::vector<Manta::Vector>& texCoords = this->MyHelper->texCoords;

  // total number of triangles
  int total_triangles = 0;

  vtkCellArray* cells = polys->GetStrips();
  vtkIdType npts = 0, *index = 0, cellNum = 0;
  ;

  switch (this->Representation)
  {
    case VTK_POINTS:
    {
      for (cells->InitTraversal(); cells->GetNextCell(npts, index); cellNum++)
      {
        double coord[3];
        Manta::Vector noTC(0.0, 0.0, 0.0);
        for (int i = 0; i < npts; i++)
        {
          // TODO: Make option to scale pointsize by scalar
          ptarray->GetPoint(index[i], coord);
          Manta::TextureCoordinateSphere* sphere = new Manta::TextureCoordinateSphere(material,
            Manta::Vector(coord[0], coord[1], coord[2]), this->PointSize,
            (texCoords.size() ? texCoords[(this->CellScalarColor ? cellNum : index[i])] : noTC));
          points->add(sphere);
        }
        total_triangles++;
      }
      // cerr << "polygons: # of triangles = " << total_triangles << endl;
    } // VTK_POINTS;
    break;
    case VTK_WIREFRAME:
    {
      double coord0[3];
      double coord1[3];
      double coord2[3];
      Manta::Vector noTC(0.0, 0.0, 0.0);
      Manta::TextureCoordinateCylinder* segment;
      for (cells->InitTraversal(); cells->GetNextCell(npts, index); cellNum++)
      {
        // TODO: Make option to scale linewidth by scalar
        ptarray->GetPoint(index[0], coord0);
        ptarray->GetPoint(index[1], coord1);
        segment = new Manta::TextureCoordinateCylinder(material,
          Manta::Vector(coord0[0], coord0[1], coord0[2]),
          Manta::Vector(coord1[0], coord1[1], coord1[2]), this->LineWidth,
          (texCoords.size() ? texCoords[(this->CellScalarColor ? cellNum : index[0])] : noTC),
          (texCoords.size() ? texCoords[(this->CellScalarColor ? cellNum : index[1])] : noTC));
        lines->add(segment);
        for (vtkIdType i = 2; i < npts; i++)
        {
          ptarray->GetPoint(index[i], coord2);
          segment = new Manta::TextureCoordinateCylinder(material,
            Manta::Vector(coord1[0], coord1[1], coord1[2]),
            Manta::Vector(coord2[0], coord2[1], coord2[2]), this->LineWidth,
            (texCoords.size() ? texCoords[(this->CellScalarColor ? cellNum : index[i - 1])] : noTC),
            (texCoords.size() ? texCoords[(this->CellScalarColor ? cellNum : index[i])] : noTC));
          lines->add(segment);
          segment = new Manta::TextureCoordinateCylinder(material,
            Manta::Vector(coord2[0], coord2[1], coord2[2]),
            Manta::Vector(coord0[0], coord0[1], coord0[2]), this->LineWidth,
            (texCoords.size() ? texCoords[(this->CellScalarColor ? cellNum : index[i])] : noTC),
            (texCoords.size() ? texCoords[(this->CellScalarColor ? cellNum : index[i - 2])]
                              : noTC));
          lines->add(segment);
          coord0[0] = coord1[0];
          coord0[1] = coord1[1];
          coord0[2] = coord1[2];
          coord1[0] = coord2[0];
          coord1[1] = coord2[1];
          coord1[2] = coord2[2];
        }
      }
    } // VTK_WIREFRAME:
    break;
    case VTK_SURFACE:
    default:
    {
      for (cells->InitTraversal(); cells->GetNextCell(npts, index); cellNum++)
      {
        // count of the i-th triangle in a strip
        int numtriangles2 = 0;

        int triangle[3];
        // the first triangle
        triangle[0] = index[0];
        triangle[1] = index[1];
        triangle[2] = index[2];
        mesh->vertex_indices.push_back(triangle[0]);
        mesh->vertex_indices.push_back(triangle[1]);
        mesh->vertex_indices.push_back(triangle[2]);
        mesh->face_material.push_back(0);

        if (!mesh->vertexNormals.empty())
        {
          mesh->normal_indices.push_back(triangle[0]);
          mesh->normal_indices.push_back(triangle[1]);
          mesh->normal_indices.push_back(triangle[2]);
        }

        if (!mesh->texCoords.empty())
        {
          if (this->CellScalarColor && !mantaTexture)
          {
            mesh->texture_indices.push_back(cellNum);
            mesh->texture_indices.push_back(cellNum);
            mesh->texture_indices.push_back(cellNum);
          }
          else
          {
            mesh->texture_indices.push_back(triangle[0]);
            mesh->texture_indices.push_back(triangle[1]);
            mesh->texture_indices.push_back(triangle[2]);
          }
        }

        total_triangles++;
        numtriangles2++;

        // the rest of triangles
        for (int i = 3; i < npts; i++)
        {
          int tmp[3];
          if (numtriangles2 % 2 == 1)
          {
            // an odd triangle
            tmp[0] = triangle[1];
            tmp[1] = triangle[2];
            tmp[2] = index[i];

            triangle[0] = tmp[0];
            triangle[1] = tmp[2];
            triangle[2] = tmp[1];
          }
          else
          {
            // an even triangle
            tmp[0] = triangle[1];
            tmp[1] = triangle[2];
            tmp[2] = index[i];

            triangle[0] = tmp[1];
            triangle[1] = tmp[0];
            triangle[2] = tmp[2];
          }

          mesh->vertex_indices.push_back(triangle[0]);
          mesh->vertex_indices.push_back(triangle[1]);
          mesh->vertex_indices.push_back(triangle[2]);
          mesh->face_material.push_back(0);

          if (!mesh->vertexNormals.empty())
          {
            mesh->normal_indices.push_back(triangle[0]);
            mesh->normal_indices.push_back(triangle[1]);
            mesh->normal_indices.push_back(triangle[2]);
          }

          if (!mesh->texCoords.empty())
          {
            if (this->CellScalarColor && !mantaTexture)
            {
              mesh->texture_indices.push_back(cellNum);
              mesh->texture_indices.push_back(cellNum);
              mesh->texture_indices.push_back(cellNum);
            }
            else
            {
              mesh->texture_indices.push_back(triangle[0]);
              mesh->texture_indices.push_back(triangle[1]);
              mesh->texture_indices.push_back(triangle[2]);
            }
          }

          total_triangles++;
          numtriangles2++;
        }
      }

      // cerr << "strips: # of triangles = " << total_triangles << endl;
      for (int i = 0; i < total_triangles; i++)
      {
        mesh->addTriangle(new Manta::WaldTriangle);
      }
    }
  }
}

//----------------------------------------------------------------------------
// Draw method for Manta.
void vtkMantaPolyDataMapper::Draw(vtkRenderer* renderer, vtkActor* actor)
{
  vtkMantaActor* mantaActor = vtkMantaActor::SafeDownCast(actor);
  if (!mantaActor)
  {
    return;
  }
  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(mantaActor->GetProperty());
  if (!mantaProperty)
  {
    return;
  }
  vtkMantaTexture* mantaTexture = NULL;
  vtkPolyData* input = this->GetInput();

  // Compute we need to for color
  this->Representation = mantaProperty->GetRepresentation();

  this->CellScalarColor = false;
  if ((this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA ||
        this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA ||
        this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA ||
        !input->GetPointData()->GetScalars()) &&
    this->ScalarMode != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
  {
    this->CellScalarColor = true;
  }

  this->MyHelper->material = NULL;
  this->MyHelper->texCoords.clear();
  Manta::Material*& material = this->MyHelper->material;
  std::vector<Manta::Vector>& texCoords = this->MyHelper->texCoords;

  if (input->GetPointData()->GetTCoords() && mantaActor->GetTexture())
  {
    // cerr << "color using actor's texture" << endl;
    mantaTexture = vtkMantaTexture::SafeDownCast(mantaActor->GetMantaTexture());
    Manta::Texture<Manta::Color>* texture = mantaTexture->GetMantaTexture();
    material = new Manta::Lambertian(texture);

    // convert texture coordinates to manta format
    vtkDataArray* tcoords = input->GetPointData()->GetTCoords();
    for (int i = 0; i < tcoords->GetNumberOfTuples(); i++)
    {
      double* tcoord = tcoords->GetTuple(i);
      texCoords.push_back(Manta::Vector(tcoord[0], tcoord[1], 0.0));
    }
  }
  else if (!this->ScalarVisibility || (!this->Colors && !this->ColorCoordinates))
  {
    // cerr << "Solid color from actor's property" << endl;
    material = mantaProperty->GetMantaMaterial();
    if (!material)
    {
      mantaProperty->CreateMantaProperty();
      material = mantaProperty->GetMantaMaterial();

      mantaProperty->SetMantaMaterial(NULL);
      mantaProperty->SetSpecularTexture(NULL);
      mantaProperty->SetDiffuseTexture(NULL);
    }
  }
  else if (this->Colors)
  {
    // cerr << "Color scalar values directly (interpolation in color space)" << endl;
    Manta::Texture<Manta::Color>* texture = new Manta::TexCoordTexture();
    if (mantaProperty->GetInterpolation() == VTK_FLAT)
    {
      // cerr << "Flat" << endl;
      material = new Manta::Flat(texture);
    }
    else
    {
      if (mantaProperty->GetOpacity() < 1.0)
      {
        // cerr << "Translucent" << endl;
        material = new Manta::Transparent(texture, mantaProperty->GetOpacity());
      }
      else
      {
        if (mantaProperty->GetSpecular() == 0)
        {
          // cerr << "non specular" << endl;
          material = new Manta::Lambertian(texture);
        }
        else
        {
          // cerr << "phong" << endl;
          double* specular = mantaProperty->GetSpecularColor();
          Manta::Texture<Manta::Color>* specularTexture = new Manta::Constant<Manta::Color>(
            Manta::Color(Manta::RGBColor(specular[0], specular[1], specular[2])));
          material = new Manta::Phong(
            texture, specularTexture, static_cast<int>(mantaProperty->GetSpecularPower()), NULL);
        }
      }
    }

    // this table has one RGBA for every point (or cell) in object
    for (int i = 0; i < this->Colors->GetNumberOfTuples(); i++)
    {
      unsigned char* color = this->Colors->GetPointer(4 * i);
      texCoords.push_back(Manta::Vector(color[0] / 255.0, color[1] / 255.0, color[2] / 255.0));
    }
  }
  else if (this->ColorCoordinates)
  {
    // cerr << "interpolate in data space, then color map each pixel" << endl;
    Manta::Texture<Manta::Color>* texture = this->InternalColorTexture->GetMantaTexture();
    material = new Manta::Lambertian(texture);

    // this table is a color transfer function with colors that cover the scalar range
    // I think
    for (int i = 0; i < this->ColorCoordinates->GetNumberOfTuples(); i++)
    {
      double* tcoord = this->ColorCoordinates->GetTuple(i);
      texCoords.push_back(Manta::Vector(tcoord[0], 0, 0));
    }
  }

  // transform point coordinates according to actor's transformation matrix
  // TODO: Use manta instancing to transform instead of doing it brute force here
  // to reduce number of copies
  vtkTransform* transform = vtkTransform::New();
  transform->SetMatrix(actor->GetMatrix());
  vtkPoints* points = vtkPoints::New();
  transform->TransformPoints(input->GetPoints(), points);

  // obtain the OpenGL-based point size and line width
  // that are specified through vtkProperty
  this->PointSize = mantaProperty->GetPointSize();
  this->LineWidth = mantaProperty->GetLineWidth();
  if (this->PointSize < 1.0)
  {
    this->PointSize = 1.0;
  }
  if (this->LineWidth < 1.0)
  {
    this->LineWidth = 1.0;
  }
  this->PointSize = sqrt(this->PointSize) * 0.010;
  this->LineWidth = sqrt(this->LineWidth) * 0.005;

  // containers for the manta primitives we are going to produce
  Manta::Group* sphereGroup = new Manta::Group();
  Manta::Group* tubeGroup = new Manta::Group();
  Manta::Mesh* mesh = new Manta::Mesh();

  // convert VTK_VERTEX cells to manta spheres
  if (input->GetNumberOfVerts() > 0)
  {
    vtkCellArray* ca = input->GetVerts();
    ca->InitTraversal();
    vtkIdType npts;
    vtkIdType* pts;
    vtkPoints* ptarray = points;
    double coord[3];
    vtkIdType cell;
    Manta::Vector noTC(0.0, 0.0, 0.0);
    while ((cell = ca->GetNextCell(npts, pts)))
    {
      // TODO: Make option to scale pointsize by scalar

      ptarray->GetPoint(pts[0], coord);
      Manta::TextureCoordinateSphere* sphere = new Manta::TextureCoordinateSphere(material,
        Manta::Vector(coord[0], coord[1], coord[2]), this->PointSize,
        (texCoords.size() ? texCoords[(this->CellScalarColor ? cell : pts[0])] : noTC));
      sphereGroup->add(sphere);
    }
  }

  // convert VTK_LINE type cells to manta cylinders
  if (input->GetNumberOfLines() > 0)
  {
    vtkCellArray* ca = input->GetLines();
    ca->InitTraversal();
    vtkIdType npts;
    vtkIdType* pts;
    vtkPoints* ptarray = points;
    double coord0[3];
    double coord1[3];
    vtkIdType cell;
    Manta::Vector noTC(0.0, 0.0, 0.0);
    while ((cell = ca->GetNextCell(npts, pts)))
    {
      ptarray->GetPoint(pts[0], coord0);
      for (vtkIdType i = 1; i < npts; i++)
      {
        // TODO: Make option to scale linewidth by scalar
        ptarray->GetPoint(pts[i], coord1);
        Manta::TextureCoordinateCylinder* segment = new Manta::TextureCoordinateCylinder(material,
          Manta::Vector(coord0[0], coord0[1], coord0[2]),
          Manta::Vector(coord1[0], coord1[1], coord1[2]), this->LineWidth,
          (texCoords.size() ? texCoords[(this->CellScalarColor ? cell : pts[0])] : noTC),
          (texCoords.size() ? texCoords[(this->CellScalarColor ? cell : pts[1])] : noTC));
        tubeGroup->add(segment);
        coord0[0] = coord1[0];
        coord0[1] = coord1[1];
        coord0[2] = coord1[2];
      }
    }
  }

  // convert coordinates to manta format
  // TODO: eliminate the copy
  for (int i = 0; i < points->GetNumberOfPoints(); i++)
  {
    double* pos = points->GetPoint(i);
    mesh->vertices.push_back(Manta::Vector(pos[0], pos[1], pos[2]));
  }

  // Do flat shading by not supplying vertex normals to manta
  if (mantaProperty->GetInterpolation() != VTK_FLAT)
  {
    vtkPointData* pointData = input->GetPointData();
    if (pointData->GetNormals())
    {
      vtkDataArray* normals = vtkFloatArray::New();
      normals->SetNumberOfComponents(3);
      transform->TransformNormals(pointData->GetNormals(), normals);
      for (int i = 0; i < normals->GetNumberOfTuples(); i++)
      {
        double* normal = normals->GetTuple(i);
        mesh->vertexNormals.push_back(Manta::Vector(normal[0], normal[1], normal[2]));
      }
      normals->Delete();
    }
  }

  // give our materials to the mesh
  int i = 0;
  mesh->materials.push_back(material);
  std::map<int, Manta::Material*>::iterator it;
  for (it = this->MyHelper->extraMaterials.begin(); it != this->MyHelper->extraMaterials.end();
       it++)
  {
    i++;
    if (!it->second)
    {
      // user asked for manual, or was a problem with spec
      // in either case, use manual
      mesh->materials.push_back(material);
    }
    else
    {
      mesh->materials.push_back(it->second);
    }
    // polys refer to materials by offset not ID
    // keep track of material IDs to location mapping
    this->MyHelper->materialOffsets[it->first] = i;
  }

  mesh->texCoords = texCoords;
  texCoords.clear();

  // convert polygons to manta format
  if (input->GetNumberOfPolys() > 0)
  {
    this->DrawPolygons(input, points, mesh, sphereGroup, tubeGroup, mantaTexture);
  }

  // convert triangle strips to manta format
  if (input->GetNumberOfStrips() > 0)
  {
    this->DrawTStrips(input, points, mesh, sphereGroup, tubeGroup, mantaTexture);
  }

  // delete transformed point coordinates
  transform->Delete();
  points->Delete();

  // put everything together into one group
  Manta::Group* group = new Manta::Group();
  if (sphereGroup->size())
  {
    // cerr << "MM(" << this << ")   points " << sphereGroup->size() << endl;
    group->add(sphereGroup);
  }
  else
  {
    delete sphereGroup;
  }
  if (tubeGroup->size())
  {
    // cerr << "MM(" << this << ")   lines " << tubeGroup->size() << endl;
    group->add(tubeGroup);
  }
  else
  {
    delete tubeGroup;
  }
  if (mesh->size())
  {
    // cerr << "MM(" << this << ")   polygons " << mesh->size() << endl;
    group->add(mesh);
  }
  else
  {
    delete mesh;
  }

  if (group->size())
  {
    mantaActor->SetGroup(group);
  }
  else
  {
    mantaActor->SetGroup(NULL);
    delete group;
    // cerr << "NOTHING TO SEE" << endl;
  }
}

//----------------------------------------------------------------------------
void vtkMantaPolyDataMapper::MakeMantaProperties(vtkPolyData* input, bool allow)
{
  if (!allow)
  {
    // remove anything we used to have
    std::map<int, Manta::Material*>::iterator it;
    for (it = this->MyHelper->extraMaterials.begin(); it != this->MyHelper->extraMaterials.end();
         it++)
    {
      delete it->second;
    }
    this->MyHelper->extraMaterials.clear();
    return;
  }

  vtkStringArray* sa =
    vtkStringArray::SafeDownCast(input->GetFieldData()->GetAbstractArray("material_properties"));
  vtkDataArray* ida = vtkDataArray::SafeDownCast(input->GetFieldData()->GetArray("material_ids"));
  vtkDataArray* ipa = input->GetFieldData()->GetArray("material_ancestors");
  if (sa)
  {
    std::vector<int> missed_interfaces;
    Manta::Material* newMat = NULL;

    for (int i = 0; i < sa->GetNumberOfTuples(); i++)
    {
      int idx = i;
      if (ida)
      {
        idx = (int)(*ida->GetTuple(i));
      }
      std::string spec = sa->GetValue(i);
      std::map<int, std::string>::iterator sit;
      std::map<int, Manta::Material*>::iterator mit;
      sit = this->MyHelper->extrasSpecs.find(idx);
      if (sit != this->MyHelper->extrasSpecs.end())
      {
        // had something, check if it needs updating
        std::string s = sit->second;
        if (s != spec)
        {
          // yes, delete the old content
          mit = this->MyHelper->extraMaterials.find(idx);
          delete mit->second;
          this->MyHelper->extraMaterials.erase(mit);
          this->MyHelper->extrasSpecs.erase(sit);
          sit = this->MyHelper->extrasSpecs.end();
        }
      }
      if (sit == this->MyHelper->extrasSpecs.end())
      {
        // new or replacement, make and insert
        if (ipa && spec == "interface")
        {
          missed_interfaces.push_back(idx);
          // use NULL for now but try and fill in below
        }
        else
        {
          newMat = vtkMantaProperty::ManufactureMaterial(spec);
          // may end up with NULL, in which case we use the manually define material
        }
        this->MyHelper->extraMaterials[idx] = newMat;
        this->MyHelper->extrasSpecs[idx] = spec;
        this->Modified();
      }
    }

    // look for user defined interface materials
    std::map<std::pair<int, int>, int> overrides;
    vtkDataArray* inInterfaceIDs = input->GetFieldData()->GetArray("interface_ids");
    vtkStringArray* inInterfaceSpecs =
      vtkStringArray::SafeDownCast(input->GetFieldData()->GetAbstractArray("interface_properties"));
    if (inInterfaceIDs && inInterfaceSpecs)
    {
      int nOverrides = inInterfaceIDs->GetNumberOfTuples();
      for (int i = 0; i < nOverrides; i++)
      {
        double* old = inInterfaceIDs->GetTuple2(i);
        int pid0 = (int)old[0];
        int pid1 = (int)old[1];
        std::pair<int, int> p = std::make_pair(pid0, pid1);
        overrides[p] = i;
      }
    }

    // figure out what to do about interface materials
    for (int i = 0; i < missed_interfaces.size(); i++)
    {
      int idx = missed_interfaces[i];
      double dpids[2];
      int pid0, pid1;
      ipa->GetTuple(idx, dpids);
      pid0 = (int)dpids[0];
      pid1 = (int)dpids[1];

      std::pair<int, int> p = std::make_pair(pid0, pid1);
      if (overrides.find(p) != overrides.end())
      {
        // user told us what to do use that
        int location = overrides[p];
        std::string spec = inInterfaceSpecs->GetValue(location);
        newMat = vtkMantaProperty::ManufactureMaterial(spec);
      }
      else
      {
        // user didn't give us guidance, try to do something sensible
        std::string spec0 = this->MyHelper->extrasSpecs[pid0];
        std::string spec1 = this->MyHelper->extrasSpecs[pid1];
        newMat = vtkMantaProperty::CombineMaterials(spec0, spec1);
      }
      // may be NULL, in which case we use the manually define material
      this->MyHelper->extraMaterials[idx] = newMat;
    }
  }
}
