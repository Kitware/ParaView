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
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkGlyph3D.h"
#include "vtkImageData.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTubeFilter.h"
#include "vtkUnsignedCharArray.h"

#include <Engine/Control/RTRT.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Groups/Group.h>
#include <Model/Groups/Mesh.h>
#include <Model/Primitives/WaldTriangle.h>
#include <Model/Primitives/Sphere.h>
#include <Model/Primitives/Cylinder.h>
#include <Model/Materials/Flat.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/Transparent.h>
#include <Model/Materials/Phong.h>
#include <Model/Textures/TexCoordTexture.h>
#include <Model/Textures/Constant.h>

#include <math.h>

vtkCxxRevisionMacro(vtkMantaPolyDataMapper, "1.10");
vtkStandardNewMacro(vtkMantaPolyDataMapper);

//----------------------------------------------------------------------------
// Construct empty object.
vtkMantaPolyDataMapper::vtkMantaPolyDataMapper()
{
  //cerr << "MM(" << this << ") CREATE" << endl;
  this->InternalColorTexture = NULL;
  this->MantaManager = NULL;
}

//----------------------------------------------------------------------------
// Destructor (don't call ReleaseGraphicsResources() since it is virtual
vtkMantaPolyDataMapper::~vtkMantaPolyDataMapper()
{
  //cerr << "MM(" << this << ") DESTROY" << endl;
  if (this->InternalColorTexture)
    {
    this->InternalColorTexture->Delete();
    }

  if (this->MantaManager)
    {
    //cerr << "MM(" << this << ") DESTROY " 
    //     << this->MantaManager << " " 
    //     << this->MantaManager->GetReferenceCount() << endl;
    this->MantaManager->Delete();
    }
}

//----------------------------------------------------------------------------
// Release the graphics resources used by this mapper.  In this case, release
// the display list if any.
void vtkMantaPolyDataMapper::ReleaseGraphicsResources(vtkWindow *win)
{
  //cerr << "MM(" << this << ") RELEASE GRAPHICS RESOURCES" << endl;
  this->Superclass::ReleaseGraphicsResources( win );
    
  if (!this->MantaManager)
    {
    return;
    }
  this->MantaManager->GetMantaEngine()->addTransaction
    ("delete polyresources",
     Manta::Callback::create( this,
                              &vtkMantaPolyDataMapper::FreeMantaResources
                              )
     );
}

//----------------------------------------------------------------------------
void vtkMantaPolyDataMapper::FreeMantaResources()
{
  //cerr << "MM(" << this << ") FREE MANTA RESOURCES" << endl;
/*
  TODO: Is this necessary? If so how to make it work?
  if (this->InternalColorTexture)
    {
    this->InternalColorTexture->Delete();
    }
  this->InternalColorTexture = NULL;
*/
}

//----------------------------------------------------------------------------
// Receives from Actor -> maps data to primitives
// called by Mapper->Render() (which is called by Actor->Render())
void vtkMantaPolyDataMapper::RenderPiece(vtkRenderer *ren, vtkActor *act)
{
  vtkMantaRenderer* mantaRenderer =
    vtkMantaRenderer::SafeDownCast(ren);
  if (!mantaRenderer)
    {
    return;
    }
  if (!this->MantaManager)
    {
    this->MantaManager = mantaRenderer->GetMantaManager();
    //cerr << "MM(" << this << ") REGISTER " << this->MantaManager << " " 
    //     << this->MantaManager->GetReferenceCount() << endl;
    this->MantaManager->Register(this);
    }

  // write geometry, first ask the pipeline to update data
  vtkPolyData *input = this->GetInput();
  if (input == NULL)
    {
    vtkErrorMacro(<< "No input to vtkMantaPolyDataMapper!");
    return;
    }
  else
    {
    this->InvokeEvent( vtkCommand::StartEvent, NULL );
    
    // Static = 1:  this mapper does NOT need to propagate updates to other mappers
    // down the pipeline and therefore saves the time that would be otherwise taken
    if ( !this->Static )
      {
      input->Update();
      }
    
    this->InvokeEvent( vtkCommand::EndEvent, NULL );
    
    vtkIdType numPts = input->GetNumberOfPoints();
    if ( numPts == 0 )
      {
      vtkDebugMacro(<< "No points from the input to vtkMantaPolyDataMapper!");
      input = NULL;
      return;
      }
    }
  
  if ( this->LookupTable == NULL )
    {
    this->CreateDefaultLookupTable();
    }
  
  // TODO: vtkOpenGLPolyDataMapper uses OpenGL clip planes here
  
  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  this->MapScalars( act->GetProperty()->GetOpacity() );
  
  if ( this->ColorTextureMap )
    {
    if (!this->InternalColorTexture)
      {
      this->InternalColorTexture = vtkMantaTexture::New();
      this->InternalColorTexture->RepeatOff();
      }
    this->InternalColorTexture->SetInput(this->ColorTextureMap);
    }
  
  // if something has changed, regenerate Manta primitives if required
  if ( this->GetMTime()  > this->BuildTime ||
       input->GetMTime() > this->BuildTime ||
       // TODO: Actor is always modified in ParaView by vtkPVLODActor,
       // it calls vtkActor::SetProperty every time its Render() is called.
       //act->GetMTime()   > this->BuildTime ||
       act->GetProperty()->GetMTime() > this->BuildTime
       )
    {
    //this->ReleaseGraphicsResources( ren->GetRenderWindow() );

    // If we are coloring by texture, then load the texture map.
    // Use Map as indicator, because texture hangs around.
    if (this->ColorTextureMap)
      {
      this->InternalColorTexture->Load(ren);
      }
    
    this->Draw(ren, act);
    this->BuildTime.Modified();
    }
  
  input = NULL;
  // TODO: deal with timer ??
}

//----------------------------------------------------------------------------
void vtkMantaPolyDataMapper::DrawPolygons(vtkPolyData *polys, Manta::Mesh *mesh)
{
  int numtriangles = 0;
  vtkCellArray *cells = polys->GetPolys();
  vtkIdType npts = 0, *index = 0, cellNum = 0;
  bool cellScalar = false;

  // implement color by point field data
  if (this->Colors || this->ColorCoordinates)
    {
    if (( this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA ||
          this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA ||
          !polys->GetPointData()->GetScalars()
          )
        && this->ScalarMode != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA
        )
      {
      cellScalar = true;
      }
    }
  // write polygons with on the fly triangulation, assuming polygons are simple and
  // can be triangulated into "fans"
  for ( cells->InitTraversal(); cells->GetNextCell(npts, index); cellNum++ )
    {
    int triangle[3];
    
    // the first triangle
    triangle[0] = index[0];
    triangle[1] = index[1];
    triangle[2] = index[2];
    mesh->vertex_indices.push_back(triangle[0]);
    mesh->vertex_indices.push_back(triangle[1]);
    mesh->vertex_indices.push_back(triangle[2]);
    mesh->face_material.push_back(0);
    
    if ( !mesh->vertexNormals.empty() )
      {
      mesh->normal_indices.push_back(triangle[0]);
      mesh->normal_indices.push_back(triangle[1]);
      mesh->normal_indices.push_back(triangle[2]);
      }
    
    if ( !mesh->texCoords.empty() )
      {
      if (cellScalar)
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
    numtriangles ++;
    
    // the remaining triangles, of which
    // each introduces a triangle after extraction
    for ( int i = 3; i < npts; i ++ )
      {
      triangle[1] = triangle[2];
      triangle[2] = index[i];
      mesh->vertex_indices.push_back(triangle[0]);
      mesh->vertex_indices.push_back(triangle[1]);
      mesh->vertex_indices.push_back(triangle[2]);
      mesh->face_material.push_back(0);
      
      if ( !mesh->vertexNormals.empty() )
        {
        mesh->normal_indices.push_back(triangle[0]);
        mesh->normal_indices.push_back(triangle[1]);
        mesh->normal_indices.push_back(triangle[2]);
        }
      
      if ( !mesh->texCoords.empty() )
        {
        if (cellScalar)
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
      numtriangles ++;
      }
    }
  
  // It is always good to assign unused pointers to NULL
  // to avoid wild ones that may cause problems on some occasions.
  // 'index' must NOT be 'delete'-d here.
  index = NULL;
  cells = NULL;
  
  //cerr << "polygons: # of triangles = " << numtriangles << endl;
  
  // TODO: memory leak, wald_triangle is not deleted
  Manta::WaldTriangle *wald_triangle = new Manta::WaldTriangle[numtriangles];
  for ( int i = 0; i < numtriangles; i ++ )
    {
    mesh->addTriangle( &wald_triangle[i] );
    }
}

//----------------------------------------------------------------------------
void vtkMantaPolyDataMapper::DrawTStrips(vtkPolyData *polys, Manta::Mesh *mesh)
{
  // total number of triangles
  int total_triangles = 0;
  
  vtkCellArray *cells = polys->GetStrips();
  vtkIdType npts = 0, *index = 0, cellNum = 0;;
  bool cellScalar = false;
  
  // implement color by point field data
  if (( this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA ||
        this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA ||
        this->ScalarMode == VTK_SCALAR_MODE_USE_FIELD_DATA ||
        !polys->GetPointData()->GetScalars()
        )
      && this->ScalarMode != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA
      )
    {
    cellScalar = true;
    }
  
  for ( cells->InitTraversal(); cells->GetNextCell(npts, index); cellNum++ )
    {
    // count of the i-th triangle in a strip
    int numtriangles2 = 0;
    
    int triangle[3];
    // the first triangle
    triangle[0] = index[0];
    triangle[1] = index[1];
    triangle[2] = index[2];
    mesh->vertex_indices.push_back( triangle[0] );
    mesh->vertex_indices.push_back( triangle[1] );
    mesh->vertex_indices.push_back( triangle[2] );
    mesh->face_material.push_back(0);
    
    if ( !mesh->vertexNormals.empty() )
      {
      mesh->normal_indices.push_back( triangle[0] );
      mesh->normal_indices.push_back( triangle[1] );
      mesh->normal_indices.push_back( triangle[2] );
      }
    
    if ( !mesh->texCoords.empty() )
      {
      if ( cellScalar )
        {
        mesh->texture_indices.push_back(cellNum);
        mesh->texture_indices.push_back(cellNum);
        mesh->texture_indices.push_back(cellNum);
        }
      else
        {
        mesh->texture_indices.push_back( triangle[0] );
        mesh->texture_indices.push_back( triangle[1] );
        mesh->texture_indices.push_back( triangle[2] );
        }
      }
    
    total_triangles ++;
    numtriangles2 ++;
    
    // the rest of triangles
    for ( int i = 3; i < npts; i ++ )
      {
      int tmp[3];
      if ( numtriangles2 % 2 == 1 )
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
      
      mesh->vertex_indices.push_back( triangle[0] );
      mesh->vertex_indices.push_back( triangle[1] );
      mesh->vertex_indices.push_back( triangle[2] );
      mesh->face_material.push_back(0);
      
      if ( !mesh->vertexNormals.empty() )
        {
        mesh->normal_indices.push_back( triangle[0] );
        mesh->normal_indices.push_back( triangle[1] );
        mesh->normal_indices.push_back( triangle[2] );
        }
      
      if ( !mesh->texCoords.empty() )
        {
        if ( cellScalar )
          {
          mesh->texture_indices.push_back(cellNum);
          mesh->texture_indices.push_back(cellNum);
          mesh->texture_indices.push_back(cellNum);
          }
        else
          {
          mesh->texture_indices.push_back( triangle[0] );
          mesh->texture_indices.push_back( triangle[1] );
          mesh->texture_indices.push_back( triangle[2] );
          }
        }
      
      total_triangles ++;
      numtriangles2 ++;
      }
    }
  
  //cerr << "strips: # of triangles = " << total_triangles << endl;
  
  // TODO: memory leak, wald_triangle is not deleted
  Manta::WaldTriangle *wald_triangle = new Manta::WaldTriangle[total_triangles];
  for ( int i = 0; i < total_triangles; i++ )
    {
    mesh->addTriangle( &wald_triangle[i] );
    }
}

//----------------------------------------------------------------------------
// Draw method for Manta.
void vtkMantaPolyDataMapper::Draw(vtkRenderer *renderer, vtkActor *actor)
{
  vtkMantaActor    *mantaActor    = vtkMantaActor::SafeDownCast(actor);
  vtkMantaProperty *mantaProperty = vtkMantaProperty::SafeDownCast( mantaActor->GetProperty() );
  
  vtkPolyData *input = this->GetInput();

  // obtain the OpenGL-based point size and line width
  // that are specified through vtkProperty
  double    pointSize = mantaProperty->GetPointSize();
  double    lineWidth = mantaProperty->GetLineWidth();
  pointSize = (pointSize < 1.0) ? 1.0 : pointSize;
  lineWidth = (lineWidth < 1.0) ? 1.0 : lineWidth;
  pointSize = sqrt(pointSize);
  lineWidth = sqrt(lineWidth);
  
  //convert VTK_VERTEX cells to manta spheres
  Manta::Group *sphereGroup = NULL;
  Manta::Group *tubeGroup = NULL;

  //TODO: Does not respect view transform
  if ( input->GetNumberOfVerts() > 0 )
    {
    sphereGroup = new Manta::Group();
    //TODO: color each point correctly
    Manta::Flat *sphereMat = new Manta::Flat(Manta::Color(Manta::RGBColor(0.0,0.5,0.0)));
    double sphereRadius = pointSize * 0.010;

    vtkCellArray *ca = input->GetVerts();
    ca->InitTraversal();
    vtkIdType npts;
    vtkIdType *pts;
    vtkPoints *ptarray = input->GetPoints();
    double coord[3];
    while (ca->GetNextCell(npts, pts))
      {
      ptarray->GetPoint(pts[0], coord);
      Manta::Sphere *sphere = new Manta::Sphere
        (sphereMat, 
         Manta::Vector(coord[0], coord[1], coord[2]), 
         sphereRadius);
      sphereGroup->add(sphere);
      }
    }
  
  //TODO: Does not respect view transform
  //convert VTK_LINE type cells to manta cylinders
  if ( input->GetNumberOfLines() > 0 )
    {
    tubeGroup = new Manta::Group();
    //TODO: color each segment correctly
    Manta::Flat *tubeMat = new Manta::Flat(Manta::Color(Manta::RGBColor(0.0,0.0,0.5)));
    double tubeDiameter = lineWidth * 0.005;

    vtkCellArray *ca = input->GetLines();
    ca->InitTraversal();
    vtkIdType npts;
    vtkIdType *pts;
    vtkPoints *ptarray = input->GetPoints();
    double coord0[3];
    double coord1[3];
    while (ca->GetNextCell(npts, pts))
      {
      ptarray->GetPoint(pts[0], coord0);
      for (vtkIdType i = 1; i < npts; i++)
        {
        ptarray->GetPoint(pts[i], coord1);
        Manta::Cylinder *segment = new Manta::Cylinder
          (tubeMat,
           Manta::Vector(coord0[0], coord0[1], coord0[2]), 
           Manta::Vector(coord1[0], coord1[1], coord1[2]), 
           tubeDiameter);
        tubeGroup->add(segment);
        coord0[0] = coord1[0];
        coord0[1] = coord1[2];
        coord0[2] = coord1[3];
        }
      }    
    }

  //TODO: Use manta instancing to transform instead of doing it brute force here
  // transform point coordinates and normal vectors
  vtkTransform *transform = vtkTransform::New();
  transform->SetMatrix( actor->GetMatrix() );
  
  // Create Manta::Mesh for the PolyData Actor
  Manta::Mesh *mesh = new Manta::Mesh();
  
  // write point coordinates to 'points' and then to 'mesh'
  vtkPoints *points = vtkPoints::New();
  transform->TransformPoints( input->GetPoints(), points );
  for ( int i = 0; i < points->GetNumberOfPoints(); i++ )
    {
    double *pos = points->GetPoint(i);
    mesh->vertices.push_back( Manta::Vector(pos[0], pos[1], pos[2]) );
    }
  points->Delete();
  
  // write vertex normals only when VTK_GOURAND or VTK_PHONG
  // TODO: the way "real FLAT" shading is done right now (by not supplying vertex
  // normals), changing from FLAT to Gouraud shading needs to create a new mesh.
  if ( mantaProperty->GetInterpolation() != VTK_FLAT )
    {
    //cerr << "GETTING NORMALS" << endl;
    vtkPointData *pointData = input->GetPointData();
    if ( pointData->GetNormals() )
      {
      vtkDataArray *normals = vtkFloatArray::New();
      normals->SetNumberOfComponents(3);
      transform->TransformNormals( pointData->GetNormals(), normals );      
      for ( int i = 0; i < normals->GetNumberOfTuples(); i ++ )
        {
        double *normal = normals->GetTuple(i);
        mesh->vertexNormals.push_back( Manta::Vector(normal[0], normal[1], normal[2]) );
        }
      normals->Delete();
      }
    }
  
  transform->Delete();
  
  if ( this->Colors )
    {
    //cerr << "HAS COLORS" << endl;
    // vertex color, easily supported by Manta with TexCoordTexture
    // TODO: color by Cell v.s. Point data
    // TODO: memory leak, delete material and texture at some point
    // TODO: how should we deal with interpolated alpha?
    // TODO: make this a factory method
    //cerr << "scalar colormap with mapper Colors array\n";
    Manta::Material *material = NULL;
    Manta::Texture<Manta::Color> *texture = new Manta::TexCoordTexture();
    
    if ( actor->GetProperty()->GetInterpolation() == VTK_FLAT )
      {
      //cerr << "FLAT" << endl;
      material = new Manta::Flat(texture);
      }
    else
      if ( actor->GetProperty()->GetOpacity() < 1.0 )
        {
        //cerr << "TRANSLUCENT" << endl;
        material = new Manta::Transparent(texture, 
                                          actor->GetProperty()->GetOpacity());
        }
      else
        if ( actor->GetProperty()->GetSpecular() == 0 )
          {
          //cerr << "LAMBERTIAN" << endl;
          material = new Manta::Lambertian(texture);
          }
        else
          {
          //cerr << "PHONG" << endl;
          //TODO: Phong shading on colormap??
          double *specular = actor->GetProperty()->GetSpecularColor();
          Manta::Texture<Manta::Color> *specularTexture =
            new Manta::Constant<Manta::Color>
            (Manta::Color(Manta::RGBColor(specular[0],
                                          specular[1],
                                          specular[2])));
          material =
            new Manta::Phong
            (texture,
             specularTexture,
             static_cast<int> (actor->GetProperty()->GetSpecularPower()),
             NULL);
          }
    
    if ( material == NULL )
      {
      delete texture;
      }
    else
      {
      mesh->materials.push_back(material);
      }
    
    for ( int i = 0; i < this->Colors->GetNumberOfTuples(); i ++ )
      {
      unsigned char *color = this->Colors->GetPointer(4 * i);
      mesh->texCoords.push_back
        (Manta::Vector(color[0]/255.0, color[1]/255.0, color[2]/255.0) );
      }
    }
  else
    if (this->InterpolateScalarsBeforeMapping && this->ColorCoordinates)
      {
      //cerr << "INTERP AND COLORCOORDS" << endl;
      // TODO: drawing of lines creates triangle strip with possibly undefined
      // tex coordinates scalar color with texture
      vtkDataArray * tcoords = this->ColorCoordinates;
      if ( tcoords )
        {
        //cerr << "HAS TCCORDS" << endl;
        for (int i = 0; i < tcoords->GetNumberOfTuples(); i++)
          {
          double *tcoord = tcoords->GetTuple(i);
          mesh->texCoords.push_back( Manta::Vector(tcoord[0], 0, 0) );
          }
        }
      
      // TODO: memory leak, delete material at some point
      if (this->InternalColorTexture)
        {
        //cerr << "INTERNAL COLOR TEXTURE" << endl;
        Manta::Texture<Manta::Color> *texture = 
          this->InternalColorTexture->GetMantaTexture();
        Manta::Material *material = new Manta::Lambertian(texture);
        mesh->materials.push_back( material );
        //cerr << "scalar color map with texture\n";
        }
      else
        {
        //cerr << "texture coordinate provided but no texture\n";
        }
      }
    else
      if (input->GetPointData()->GetTCoords() && actor->GetTexture() )
        {
        //cerr << "USE VTKTEXTURE" << endl;
        // use vtkTexture
        // write texture coordinates
        vtkDataArray *tcoords = input->GetPointData()->GetTCoords();
        for (int i = 0; i < tcoords->GetNumberOfTuples(); i++)
          {
          double *tcoord = tcoords->GetTuple(i);
          mesh->texCoords.push_back
            ( Manta::Vector(tcoord[0], tcoord[1], tcoord[2]) );
          }
        
        // TODO: memory leak, delete material at some point
        // get Manta texture from Actor->Texture
        vtkMantaTexture *mantaTexture = 
          vtkMantaTexture::SafeDownCast(actor->GetTexture());
        if (mantaTexture)
          {
          //cerr << "MANTA TEXTURE" << endl;
          Manta::Texture<Manta::Color> *texture = 
            mantaTexture->GetMantaTexture();
          Manta::Material *material = new Manta::Lambertian(texture);
          mesh->materials.push_back( material );
          }
        else
          {
          cerr << "texture coordinate provided but no texture\n";
          }
        }
      else
        {
        //cerr << "using actor's property" << endl;
        // no colormap or texture, use the Actor's property
        Manta::Material *mmat = mantaProperty->GetMantaMaterial();
        if(!mmat)
          {
          mantaProperty->CreateMantaProperty();
          mmat = mantaProperty->GetMantaMaterial();
          }
        mesh->materials.push_back( mmat );
        }
  
  //TODO: Respect wireframe and point mode
  // write polygons
  bool hadsome = false;
  if ( input->GetNumberOfPolys() > 0 )
    {
    hadsome = true;
    this->DrawPolygons(input, mesh);
    }
  
  //TODO: Respect wireframe and point mode
  // write triangle strips
  if ( input->GetNumberOfStrips() > 0 )
    {
    hadsome = true;
    this->DrawTStrips(input, mesh);
    }

  Manta::Group *group = new Manta::Group();
  //cerr << "MM(" << this << ") Create Group " << group << endl;
  if(sphereGroup)
    {
    //cerr << "MM(" << this << ")   points " << sphereGroup << endl;
    group->add(sphereGroup);
    }
  if(tubeGroup)
    {
    //cerr << "MM(" << this << ")   lines " << tubeGroup << endl;
    group->add(tubeGroup);
    }
  if (hadsome)
    {
    //cerr << "MM(" << this << ")   polygons " << mesh << endl;
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
    //cerr << "NOTHING TO SEE" << endl;
    }
}
