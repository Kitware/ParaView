/*=========================================================================
  
  Program:   ParaView
  Module:    vtkPVLODActor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVLODActor.h"

#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"

#include <math.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLODActor);

vtkCxxSetObjectMacro(vtkPVLODActor, LODMapper, vtkMapper);

//----------------------------------------------------------------------------
vtkPVLODActor::vtkPVLODActor()
{
  vtkMatrix4x4 *m;
  
  // get a hardware dependent actor and mappers
  this->Device = vtkActor::New();
  m = vtkMatrix4x4::New();
  this->Device->SetUserMatrix(m);
  m->Delete();
  
  this->LODMapper = NULL;

  this->EnableLOD = 1;
}

//----------------------------------------------------------------------------
vtkPVLODActor::~vtkPVLODActor()
{
  this->SetLODMapper(NULL);
  this->Device->Delete();
  this->Device = NULL;
}

//----------------------------------------------------------------------------
// We use points as the size of the data, because cells cqan mislead.
// A good example is verts.  One cell can contain any number of verticies.
vtkMapper *vtkPVLODActor::SelectMapper()
{
  if (this->Mapper == NULL || this->Mapper->GetInput() == NULL)
    {
    return this->LODMapper;
    }
  if (this->LODMapper == NULL || this->LODMapper->GetInput() == NULL)
    {
    return this->Mapper;
    }

  if (this->EnableLOD && vtkPVApplication::GetGlobalLODFlag())
    {
    return this->LODMapper;
    }

  return this->Mapper;
}






//----------------------------------------------------------------------------
void vtkPVLODActor::Render(vtkRenderer *ren, vtkMapper *vtkNotUsed(m))
{
  vtkMatrix4x4 *matrix;
  vtkMapper *mapper;
  
  if (this->Mapper == NULL)
    {
    vtkErrorMacro("No mapper for actor.");
    return;
    }
  
  mapper = this->SelectMapper();

  if (mapper == NULL)
    {
    return;
    }
    
  /* render the property */
  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }
  this->Property->Render(this, ren);
  if (this->BackfaceProperty)
    {
    this->BackfaceProperty->BackfaceRender(this, ren);
    this->Device->SetBackfaceProperty(this->BackfaceProperty);
    }
  this->Device->SetProperty(this->Property);
  
  
  /* render the texture */
  if (this->Texture)
    {
    this->Texture->Render(ren);
    }
  
  // make sure the device has the same matrix
  matrix = this->Device->GetUserMatrix();
  this->GetMatrix(matrix);
  
  // Store information on time it takes to render.
  // We might want to estimate time from the number of polygons in mapper.
  this->Device->Render(ren,mapper);
  this->EstimatedRenderTime = mapper->GetTimeToDraw();

}

int vtkPVLODActor::RenderOpaqueGeometry(vtkViewport *vp)
{
  int          renderedSomething = 0; 
  vtkRenderer  *ren = (vtkRenderer *)vp;

  if ( ! this->Mapper )
    {
    return 0;
    }

  // make sure we have a property
  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }

  // is this actor opaque ?
  if (this->GetIsOpaque())
    {
    this->Property->Render(this, ren);

    // render the backface property
    if (this->BackfaceProperty)
      {
      this->BackfaceProperty->BackfaceRender(this, ren);
      }
    
    // render the texture 
    if (this->Texture)
      {
      this->Texture->Render(ren);
      }
    this->Render(ren,this->Mapper);

    renderedSomething = 1;
    }

  return renderedSomething;
}

void vtkPVLODActor::ReleaseGraphicsResources(vtkWindow *renWin)
{
  vtkActor::ReleaseGraphicsResources(renWin);
  
  // broadcast the message down to the individual LOD mappers
  this->LODMapper->ReleaseGraphicsResources(renWin);
}


// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
float *vtkPVLODActor::GetBounds()
{
  int i,n;
  float *bounds, bbox[24], *fptr;
  vtkMapper *mapper = this->GetMapper();

  vtkDebugMacro( << "Getting Bounds" );

  // get the bounds of the Mapper if we have one
  if (!mapper)
    {
    return this->Bounds;
    }

  bounds = mapper->GetBounds();
  // Check for the special case when the mapper's bounds are unknown
  if (!bounds)
    {
    return bounds;
    }

  // Check for the special case when the actor is empty.
  if (bounds[0] > bounds[1])
    { 
    memcpy( this->MapperBounds, bounds, 6*sizeof(float) );
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_LARGE_FLOAT;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
    this->BoundsMTime.Modified();
    return this->Bounds;
    }

  // Check if we have cached values for these bounds - we cache the
  // values returned by this->Mapper->GetBounds() and we store the time
  // of caching. If the values returned this time are different, or
  // the modified time of this class is newer than the cached time,
  // then we need to rebuild.
  if ( ( memcmp( this->MapperBounds, bounds, 6*sizeof(float) ) != 0 ) ||
       ( this->GetMTime() > this->BoundsMTime ) )
    {
    vtkDebugMacro( << "Recomputing bounds..." );

    memcpy( this->MapperBounds, bounds, 6*sizeof(float) );

    // fill out vertices of a bounding box
    bbox[ 0] = bounds[1]; bbox[ 1] = bounds[3]; bbox[ 2] = bounds[5];
    bbox[ 3] = bounds[1]; bbox[ 4] = bounds[2]; bbox[ 5] = bounds[5];
    bbox[ 6] = bounds[0]; bbox[ 7] = bounds[2]; bbox[ 8] = bounds[5];
    bbox[ 9] = bounds[0]; bbox[10] = bounds[3]; bbox[11] = bounds[5];
    bbox[12] = bounds[1]; bbox[13] = bounds[3]; bbox[14] = bounds[4];
    bbox[15] = bounds[1]; bbox[16] = bounds[2]; bbox[17] = bounds[4];
    bbox[18] = bounds[0]; bbox[19] = bounds[2]; bbox[20] = bounds[4];
    bbox[21] = bounds[0]; bbox[22] = bounds[3]; bbox[23] = bounds[4];
  
    // save the old transform
    this->Transform->Push(); 
    this->Transform->SetMatrix(this->GetMatrix());

    // and transform into actors coordinates
    fptr = bbox;
    for (n = 0; n < 8; n++) 
      {
      this->Transform->TransformPoint(fptr,fptr);
      fptr += 3;
      }
  
    this->Transform->Pop();  
  
    // now calc the new bounds
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_LARGE_FLOAT;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
    for (i = 0; i < 8; i++)
      {
      for (n = 0; n < 3; n++)
        {
        if (bbox[i*3+n] < this->Bounds[n*2]) 
          {
          this->Bounds[n*2] = bbox[i*3+n];
          }
        if (bbox[i*3+n] > this->Bounds[n*2+1]) 
          {
          this->Bounds[n*2+1] = bbox[i*3+n];
          }
        }
      }
    this->BoundsMTime.Modified();
    }

  return this->Bounds;
}


//----------------------------------------------------------------------------
void vtkPVLODActor::Modified()
{
  this->Device->Modified();
  this->vtkActor::Modified();
}

void vtkPVLODActor::ShallowCopy(vtkProp *prop)
{
  vtkPVLODActor *a = vtkPVLODActor::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetLODMapper(a->GetLODMapper());
    }

  // Now do superclass
  this->vtkActor::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void vtkPVLODActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "LODMapper: " << this->GetLODMapper() << endl;
}
