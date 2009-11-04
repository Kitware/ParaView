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
// .NAME vtkMantaPolyDataMapper - 
// .SECTION Description
//
#include "vtkManta.h"
#include "vtkMantaPolyDataMapper.h"
#include "vtkMantaProperty.h"
#include "vtkMantaRenderer.h"
#include "vtkMantaActor.h"
#include "vtkMantaTexture.h"

#include "vtkAppendPolyData.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkGlyph3D.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTubeFilter.h"
#include "vtkUnsignedCharArray.h"
#include "vtkImageData.h"

#include "vtkPoints.h"
#include "vtkGenericCell.h"

#include <Engine/Control/RTRT.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Groups/Group.h>
#include <Model/Groups/Mesh.h>
#include <Model/Primitives/WaldTriangle.h>
#include <Model/Materials/Flat.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/Transparent.h>
#include <Model/Materials/Phong.h>

#include <Model/Textures/TexCoordTexture.h>
#include <Model/Textures/Constant.h>
#include <math.h>

vtkCxxRevisionMacro(vtkMantaPolyDataMapper, "1.2");
vtkStandardNewMacro(vtkMantaPolyDataMapper);

//----------------------------------------------------------------------------
// Construct empty object.
vtkMantaPolyDataMapper::vtkMantaPolyDataMapper()
{
   this->IsCenterAxes = false;
   this->SphereConfig = NULL;
   this->SphereCenter = NULL;
   this->SphereFilter = NULL;
   this->TubeFilter   = NULL;
   this->PolyStrips   = NULL;
   this->VTKtoManta   = NULL;
   this->InternalColorTexture = NULL;
}

//----------------------------------------------------------------------------
// Destructor (don't call ReleaseGraphicsResources() since it is virtual
vtkMantaPolyDataMapper::~vtkMantaPolyDataMapper()
{
  if ( this->SphereConfig != NULL )
    {
    this->SphereConfig->Delete();
    }
  
  if ( this->SphereCenter != NULL )
    {
    this->SphereCenter->Delete();
    }
  
  if ( this->SphereFilter != NULL )
    {
    this->SphereFilter->Delete();
    }
  
  if ( this->TubeFilter != NULL )
    {
    this->TubeFilter->Delete();
    }
  
  if ( this->PolyStrips != NULL )
    {
    this->PolyStrips->Delete();
    }
  
  if ( this->VTKtoManta != NULL )
    {
    this->VTKtoManta->Delete();
    }
}

//----------------------------------------------------------------------------
// Release the graphics resources used by this mapper.  In this case, release
// the display list if any.
void vtkMantaPolyDataMapper::ReleaseGraphicsResources(vtkWindow *win)
{
}

//----------------------------------------------------------------------------
// Receives from Actor -> maps data to primitives
// called by Mapper->Render() (which is called by Actor->Render())
void vtkMantaPolyDataMapper::RenderPiece(vtkRenderer *ren, vtkActor *act)
{
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
  
  // TODO: vtkOpenGLPolyDataMapper uses OpenGL clip planes for some reason
  
  // A strict check is performed here in case of any non-center-axes
  // vtkPolyData that happens to contain a point-based data array called
  // "Axes", too. More info is available in vtkAxes that is used to create
  // this special representation for the center of rotation axes widget.
  this->SetIsCenterAxes( this->GetInput()->GetLines()
                         ->GetNumberOfCells(  ) == 3  &&
                         this->GetInput()->GetPoints()
                         ->GetNumberOfPoints( ) == 6  &&
                         this->GetInput()->GetPointData()
                         ->GetScalars( "Axes" ) != NULL );
  
  // For vertex coloring, this sets this->Colors as side effect.
  // For texture map coloring, this sets ColorCoordinates
  // and ColorTextureMap as a side effect.
  this->MapScalars( act->GetProperty()->GetOpacity() );
  
  if ( this->ColorTextureMap )
    {
    if (this->InternalColorTexture == 0)
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
  
  cerr << "polygons: # of triangles = " << numtriangles << endl;
  
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
  
  cerr << "strips: # of triangles = " << total_triangles << endl;
  
  // TODO: memory leak, wald_triangle is not deleted
  Manta::WaldTriangle *wald_triangle = new Manta::WaldTriangle[total_triangles];
  for ( int i = 0; i < total_triangles; i++ )
    {
    mesh->addTriangle( &wald_triangle[i] );
    }
}

//----------------------------------------------------------------------------
void vtkMantaPolyDataMapper::ExtractVertices(vtkPolyData* srcPoly,  vtkPolyData* outPoly)
{
  if ( srcPoly == NULL || outPoly == NULL )
    {
    vtkErrorMacro(<< "either srcPoly or outPoly is NULL!");
    return;
    }
  
  double          pntCoord[3];
  vtkIdType       srcCellId, outCellId, srcPntId, outPntId;
  
  vtkIdType       numCells  = srcPoly->GetNumberOfVerts();
  vtkPoints*      srcPoints = srcPoly->GetPoints();
  vtkPoints*      outPoints = vtkPoints::New();
  outPoints->Allocate(numCells);
  
  vtkCellData*    srcCD = srcPoly->GetCellData();
  vtkCellData*    outCD = outPoly->GetCellData();
  outCD->CopyAllocate(srcCD, numCells);
  
  vtkPointData*   srcPD = srcPoly->GetPointData();
  vtkPointData*   outPD = outPoly->GetPointData();
  outPD->CopyAllocate(srcPD, numCells);
  
  vtkCellArray*   vt_cells = vtkCellArray::New();
  vtkGenericCell* vertCell = vtkGenericCell::New();
  
  for ( srcCellId = 0;  srcCellId < numCells;  srcCellId ++ )
    {
    srcPoly->GetCell(srcCellId, vertCell);
    srcPntId  = vertCell->GetPointId(0);
    srcPoints->GetPoint(srcPntId, pntCoord);
    outPntId  = outPoints->InsertNextPoint(pntCoord);
    outCellId = vt_cells->InsertNextCell(1);
    vt_cells->InsertCellPoint(outPntId);
    outPD->CopyData(srcPD, srcPntId,  outPntId );
    outCD->CopyData(srcCD, srcCellId, outCellId);
    }
  
  outPoly->SetPoints(outPoints);
  outPoly->SetVerts(vt_cells);
  outPoly->Squeeze();
  
  vertCell->Delete();
  vt_cells->Delete();
  outPoints->Delete();
  srcPoints = NULL;
  srcCD = NULL;
  outCD = NULL;
  srcPD = NULL;
  outPD = NULL;
}

//----------------------------------------------------------------------------
// This function specifies the radius and number of sides for the resulting
// cylinders if the source vtkPolyData is the Center Of Rotation axes widget.
// The COR, with the geometry and attributes generated through source vtkAxes,
// contains a scalar data array called 'Axes', which is then employed in the
// line-to-cylinder convertor, i.e., vtkTubeFilter, to create texture
// coordinates for the vertices of the resulting cylinders. The texture
// coordinates, after correction by FillInTCoordsForCenterAxes(.), are used to
// index into a color texture map to obtain the actual axes colors.
//
// NOTE: This function needs to be called prior to this->TubeFilter->Update(),
// in vtkMantaPolyDataMapper::Draw(vtkRenderer *renderer, vtkActor *actor),
// and after which, needs to be paired with vtkMantaPolyDataMapper::
// FillInTCoordsForCenterAxes(.).
void vtkMantaPolyDataMapper::EnableTCoordsForCenterAxes(
                                                        vtkPolyData * centerAxes )
{
  if ( this->GetIsCenterAxes() &&
       this->ColorCoordinates  &&
       this->InterpolateScalarsBeforeMapping )
    {
    int  arrayIndex = -1;
    centerAxes->GetPointData()->GetArray( "Axes", arrayIndex );
    this->TubeFilter->SetRadius( 3 * 0.005 );
    this->TubeFilter->SetNumberOfSides( 6 );
    this->TubeFilter->SetGenerateTCoordsToUseScalars();
    this->SetInputArrayToProcess( arrayIndex, 0, 0,
                                  vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                  vtkDataSetAttributes::SCALARS );
    }
}

//----------------------------------------------------------------------------
// This function fixes the texture coordinates, created through vtkTubeFilter,
// for the Center Of Rotation axes widget since vtkTubeFilters fails to
// interpolate along a CONSTANT line to produce FIXED texture coordinates that
// are then actually used to index into a color texture map for FLAT-colored
// axes. Given the texture coordinates array allocated via vtkTubeFilter, this
// function updates this array with proper texture (color) coordinates that
// are accessed from this->ColorCoordinates.
//
// NOTE: This function needs to be called after this->TubeFilter->Update(),
// in vtkMantaPolyDataMapper::Draw(vtkRenderer *renderer, vtkActor *actor),
// as a paired function of vtkMantaPolyDataMapper::
// EnableTCoordsForCenterAxes(.).
void vtkMantaPolyDataMapper::FillInTCoordsForCenterAxes(
                                                        vtkPolyData * centerAxes )
{
  if ( this->GetIsCenterAxes() &&
       this->ColorCoordinates  &&
       this->InterpolateScalarsBeforeMapping )
    {
    vtkDataArray * pClrCoords = this->ColorCoordinates;
    vtkDataArray * pTexCoords = this->TubeFilter->GetOutput()
      ->GetPointData()->GetTCoords();
    int      numCCoords = pClrCoords->GetNumberOfTuples();
    int      numTCoords = pTexCoords->GetNumberOfTuples();
    int      numAxisLns = centerAxes->GetLines()->GetNumberOfCells();
    int      ptsPerLine = numTCoords / numAxisLns;
    int      tCordIndex = 0;
    double * colorCoord = NULL;
    if ( numCCoords == numAxisLns * 2 )
      {
      for ( int j = 0; j < numAxisLns; j ++ )
        for ( int i = 0; i < ptsPerLine; i ++, tCordIndex ++ )
          {
          colorCoord = pClrCoords->GetTuple( j << 1 );
          pTexCoords->SetTuple2( tCordIndex, colorCoord[0], 0 );
          }
      }
    pClrCoords = NULL;
    pTexCoords = NULL;
    colorCoord = NULL;
    }
}

//----------------------------------------------------------------------------
// Draw method for Manta.
void vtkMantaPolyDataMapper::Draw(vtkRenderer *renderer, vtkActor *actor)
{
  vtkMantaActor    *mantaActor    = vtkMantaActor::SafeDownCast(actor);
  vtkMantaProperty *mantaProperty = vtkMantaProperty::SafeDownCast( mantaActor->GetProperty() );
  
  vtkPolyData *input = this->GetInput();
  
#if 1
  // obtain the OpenGL-based point size and line width
  // that are specified through vtkProperty
  double    pointSize = mantaProperty->GetPointSize();
  double    lineWidth = mantaProperty->GetLineWidth();
  pointSize = (pointSize < 1.0) ? 1.0 : pointSize;
  lineWidth = (lineWidth < 1.0) ? 1.0 : lineWidth;
  pointSize = sqrt(pointSize);
  lineWidth = sqrt(lineWidth);
  
  // convert points to spheres
  if ( input->GetNumberOfVerts() > 0 )
    {
    if ( this->SphereConfig == NULL)
      {
      this->SphereConfig = vtkSphereSource::New();
      }
    
    if ( this->SphereCenter != NULL )
      {
      this->SphereCenter->Delete();
      }
    this->SphereCenter = vtkPolyData::New();
    
    if ( this->SphereFilter == NULL)
      {
      this->SphereFilter = vtkGlyph3D::New();
      }
    
    // sphere configuration
    int   sphereRes = int(pointSize * 5);
    this->SphereConfig->SetRadius( pointSize * 0.010 );
    this->SphereConfig->SetPhiResolution(sphereRes);
    this->SphereConfig->SetThetaResolution(sphereRes);
    
    // NOTE: here 'input' can NOT be directly used as the input to 'SphereFilter'
    // because input->GetPoints() might include those points introduced by line-type
    // primitives, as is the case with a hybrid vertices+lines vtkPolyData input.
    // Thus only those points introduced by vertex-type primitives may be taken as
    // the input to 'SphereFilter'.
    this->ExtractVertices( input, this->SphereCenter );
    this->SphereFilter->SetInput( this->SphereCenter );
    this->SphereFilter->SetSource( SphereConfig->GetOutput() );
    this->SphereFilter->Update();
    }
  
  // convert lines to tubes
  if ( input->GetNumberOfLines() > 0 )
    {
    if ( this->TubeFilter == NULL)
      {
      this->TubeFilter = vtkTubeFilter::New();
      }
    
    int   tubeNumSides = int(lineWidth * 4);
    this->TubeFilter->SetRadius( lineWidth * 0.005 );
    this->TubeFilter->SetNumberOfSides(tubeNumSides);
    this->TubeFilter->UseDefaultNormalOn();
    this->TubeFilter->SetDefaultNormal(0.577, 0.577, 0.577);
    this->EnableTCoordsForCenterAxes(input);
    this->TubeFilter->SetInput(input);
    this->TubeFilter->Update();
    this->FillInTCoordsForCenterAxes(input);
    }
  
  // merge the original polygons and triangle strips with
  // the constructed spheres and tubes
  if ( this->VTKtoManta != NULL )
    {
    this->VTKtoManta->Delete();
    }
  this->VTKtoManta = vtkAppendPolyData::New();
  
  if ( input->GetNumberOfVerts() > 0 )
    {
    this->VTKtoManta->AddInput( this->SphereFilter->GetOutput() );
    }
  
  if ( input->GetNumberOfLines() > 0 )
    {
    this->VTKtoManta->AddInput( this->TubeFilter->GetOutput() );
    }
  
  // use the original polygons and triangle strips while
  // deactivating the original vertices and lines
  if ( this->PolyStrips != NULL )
    {
    this->PolyStrips->Delete();
    }
  this->PolyStrips = vtkPolyData::New();
  this->PolyStrips->ShallowCopy(input);
  this->PolyStrips->SetVerts(NULL);
  this->PolyStrips->SetLines(NULL);
  this->VTKtoManta->AddInput( this->PolyStrips );
  
  // take the Manta-compliant polydata as the input to Manta::Mesh
  this->VTKtoManta->Update();
  input = this->VTKtoManta->GetOutput();
#endif
  
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
    pos = NULL;
    }
  points->Delete();
  
  // write vertex normals only when VTK_GOURAND or VTK_PHONG
  // TODO: the way "real FLAT" shading is done right now (by not supplying vertex
  // normals), changing from FLAT to Gouraud shading needs to create a new mesh.
  if ( mantaProperty->GetInterpolation() != VTK_FLAT )
    {
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
        normal = NULL;
        }
      normals->Delete();
      }
    pointData = NULL;
    }
  
  transform->Delete();
  
  // make sure the Center Of Rotation axes widget is INsensitive to the
  // change of the color map scheme used by disabling the first switch
  // below IF AND ONLY IF the target actor corresponds to the COR widget
  if ( this->Colors && this->GetIsCenterAxes() == false )
    {
    // vertex color, easily supported by Manta with TexCoordTexture
    // TODO: color by Cell v.s. Point data
    // TODO: memory leak, delete material and texture at some point
    // TODO: how should we deal with interpolated alpha?
    // TODO: make this a factory method
    cerr << "scalar colormap with mapper Colors array\n";
    Manta::Material *material = 0;
    Manta::Texture<Manta::Color> *texture = new Manta::TexCoordTexture();
    
    if ( actor->GetProperty()->GetInterpolation() == VTK_FLAT )
      {
      material = new Manta::Flat(texture);
      }
    else
      if ( actor->GetProperty()->GetOpacity() < 1.0 )
        {
        material = new Manta::Transparent( texture, actor->GetProperty()->GetOpacity() );
        }
      else
        if ( actor->GetProperty()->GetSpecular() == 0 )
          {
          material = new Manta::Lambertian(texture);
          }
        else
          {
          //TODO: Phong shading on colormap??
          double *specular = actor->GetProperty()->GetSpecularColor();
          Manta::Texture<Manta::Color> *specularTexture =
            new Manta::Constant<Manta::Color>(Manta::Color(Manta::RGBColor(specular[0],
                                                                           specular[1],
                                                                           specular[2])));
          material =
            new Manta::Phong(texture,
                             specularTexture,
                             static_cast<int> (actor->GetProperty()->GetSpecularPower()), NULL);
          }
    
    if ( material == NULL )
      {
      delete  texture;
      texture = NULL;
      }
    else
      {
      mesh->materials.push_back(material);
      }
    
    for ( int i = 0; i < this->Colors->GetNumberOfTuples(); i ++ )
      {
      unsigned char *color = this->Colors->GetPointer(4 * i);
      mesh->texCoords.push_back( Manta::Vector(color[0]/255.0, color[1]/255.0, color[2]/255.0) );
      color = NULL;
      }
    }
  else
    // this switch should be always chosen AS LONG AS the target actor
    // is the center of rotation axes widget
    if (this->InterpolateScalarsBeforeMapping && this->ColorCoordinates)
      {
      // TODO: drawing of lines creates triangle strip with possibly undefined tex coordinates
      // scalar color with texture
      vtkDataArray * tcoords = this->ColorCoordinates;
      if ( tcoords )
        {
        // A special treatment is needed here for the center of rotation axes
        // widget due to the fact that the color texture coordinates, stored
        // in 'this->ColorCoordinates', are generated prior to the line-to-
        // cylinder conversion and therefore the number of the tuples is
        // less than that of the actual vertices constituting the cylinders,
        // i.e., the thick axes of the widget.
        //
        // To address this issue, the proper color coordinates are created in
        // the line-to-cylinder process, by turning on a flag through the call
        // to EnableTCoordsForCenterAxes(.), an then corrected (vtkTubeFilter
        // fails to interpolate along a CONSTANT line to produce FIXED texture
        // coordinates that are then used for FLAT coloring) by a call to
        // FillInTCoordsForCenterAxes(.).
        //
        // NOTE: any changes to the color map code, e.g., something related to
        // to this->InterpolateScalarsBeforeMapping, this->Colors,
        // this->ColorCoordinates, and this->InternalColorTexture, might
        // cause new problems with the center of rotation axes wwidget.
        if ( this->GetIsCenterAxes() )
          {
          tcoords = this->TubeFilter->GetOutput()
            ->GetPointData()->GetTCoords();
          }
        
        for (int i = 0; i < tcoords->GetNumberOfTuples(); i++)
          {
          double *tcoord = tcoords->GetTuple(i);
          mesh->texCoords.push_back( Manta::Vector(tcoord[0], 0, 0) );
          }
        }
      
      // TODO: memory leak, delete material at some point
      if (this->InternalColorTexture)
        {
        Manta::Texture<Manta::Color> *texture = this->InternalColorTexture->GetMantaTexture();
        Manta::Material *material = new Manta::Lambertian(texture);
        mesh->materials.push_back( material );
        cerr << "scalar color map with texture\n";
        }
      else
        {
        cerr << "texture coordinate provided but no texture\n";
        }
      }
    else
      if (input->GetPointData()->GetTCoords() && actor->GetTexture() )
        {
        // use vtkTexture
        // write texture coordinates
        vtkDataArray *tcoords = input->GetPointData()->GetTCoords();
        for (int i = 0; i < tcoords->GetNumberOfTuples(); i++)
          {
          double *tcoord = tcoords->GetTuple(i);
          mesh->texCoords.push_back( Manta::Vector(tcoord[0], tcoord[1], tcoord[2]) );
          }
        
        // TODO: memory leak, delete material at some point
        // get Manta texture from Actor->Texture
        vtkMantaTexture *mantaTexture = vtkMantaTexture::SafeDownCast(actor->GetTexture());
        if (mantaTexture)
          {
          Manta::Texture<Manta::Color> *texture = mantaTexture->GetMantaTexture();
          Manta::Material *material = new Manta::Lambertian(texture);
          mesh->materials.push_back( material );
          cerr << "color with texture\n";
          }
        else
          {
          cerr << "texture coordinate provided but no texture\n";
          }
        }
      else
        {
        // no colormap or texture, use the Actor's property
        mesh->materials.push_back( mantaProperty->GetMantaMaterial() );
        }
  
  mantaProperty = NULL;
  
  
  // write polygons
  if ( input->GetNumberOfPolys() > 0 )
    {
    this->DrawPolygons(input, mesh);
    }
  
  // write triangle strips
  if ( input->GetNumberOfStrips() > 0 )
    {
    this->DrawTStrips(input, mesh);
    }
  
  // we delay the deletion of old mesh at transaction time
  mantaActor->SetMesh(mesh);
  mantaActor->SetIsModified(true);
  mantaActor = NULL;
  input      = NULL;
}
