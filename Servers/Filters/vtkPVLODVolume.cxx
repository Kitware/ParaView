/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODVolume.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLODVolume.h"

#include "vtkAbstractVolumeMapper.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkTimerLog.h"
#include "vtkTransform.h"
#include "vtkPVProcessModule.h"

#include <math.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLODVolume);
vtkCxxRevisionMacro(vtkPVLODVolume, "1.2");

vtkCxxSetObjectMacro(vtkPVLODVolume, LODMapper, vtkAbstractVolumeMapper);

//----------------------------------------------------------------------------
vtkPVLODVolume::vtkPVLODVolume()
{
  this->LODMapper = NULL;
  this->MapperBounds[0] = this->MapperBounds[1] = this->MapperBounds[2] = 0;
  this->MapperBounds[3] = this->MapperBounds[4] = this->MapperBounds[5] = 0;
}

//----------------------------------------------------------------------------
vtkPVLODVolume::~vtkPVLODVolume()
{
  this->SetLODMapper(NULL);
}

//----------------------------------------------------------------------------
// We use points as the size of the data, because cells cqan mislead.
// A good example is verts.  One cell can contain any number of verticies.
vtkAbstractVolumeMapper *vtkPVLODVolume::SelectMapper()
{
  if (this->Mapper == NULL || this->Mapper->GetDataSetInput() == NULL)
    {
    return this->LODMapper;
    }
  if (this->LODMapper == NULL || this->LODMapper->GetDataSetInput() == NULL)
    {
    return this->Mapper;
    }

  if (vtkPVProcessModule::GetGlobalLODFlag())
    {
    return this->LODMapper;
    }

  return this->Mapper;
}






//-----------------------------------------------------------------------------
int vtkPVLODVolume::RenderTranslucentGeometry(vtkViewport *vp)
{
  vtkRenderer             *ren = static_cast<vtkRenderer*>(vp);
  vtkAbstractVolumeMapper *mapper = this->SelectMapper();

  if ( !mapper )
    {
    vtkErrorMacro(<< "You must specify a mapper!\n");
    return 0;
    }

  // If we don't have any input, return silently.
  if (!mapper->GetDataSetInput())
    {
    return 0;
    }

  // make sure we have a property
  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }

  mapper->Render(ren, this);
  this->EstimatedRenderTime += mapper->GetTimeToDraw();

  return 1;
}

void vtkPVLODVolume::ReleaseGraphicsResources(vtkWindow *renWin)
{
  this->Superclass::ReleaseGraphicsResources(renWin);
  
  // broadcast the message down to the individual LOD mappers
  if (this->LODMapper)
    {
    this->LODMapper->ReleaseGraphicsResources(renWin);
    }
}


// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkPVLODVolume::GetBounds()
{
  int i,n;
  double *bounds, bbox[24], *fptr;
  vtkAbstractVolumeMapper *mapper = this->GetMapper();

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
    memcpy( this->MapperBounds, bounds, 6*sizeof(double) );
    vtkMath::UninitializeBounds(this->Bounds);
    this->BoundsMTime.Modified();
    return this->Bounds;
    }

  // Check if we have cached values for these bounds - we cache the
  // values returned by this->Mapper->GetBounds() and we store the time
  // of caching. If the values returned this time are different, or
  // the modified time of this class is newer than the cached time,
  // then we need to rebuild.
  if ( ( memcmp( this->MapperBounds, bounds, 6*sizeof(double) ) != 0 ) ||
       ( this->GetMTime() > this->BoundsMTime ) )
    {
    vtkDebugMacro( << "Recomputing bounds..." );

    memcpy( this->MapperBounds, bounds, 6*sizeof(double) );

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
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
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


//-----------------------------------------------------------------------------
void vtkPVLODVolume::ShallowCopy(vtkProp *prop)
{
  vtkPVLODVolume *a = vtkPVLODVolume::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetLODMapper(a->GetLODMapper());
    }

  // Now do superclass
  this->vtkVolume::ShallowCopy(prop);
}


//----------------------------------------------------------------------------
void vtkPVLODVolume::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if (this->LODMapper)
    {
    os << indent << "LODMapper: " << this->GetLODMapper() << endl;
    }
}
