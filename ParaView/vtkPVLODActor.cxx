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
#include <stdlib.h>
#include <math.h>
#include "vtkPVLODActor.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkPVLODActor* vtkPVLODActor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVLODActor");
  if(ret)
    {
    return (vtkPVLODActor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVLODActor;
}

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
  this->TimePerPoint = this->LODTimePerPoint = 0.00001;
}

//----------------------------------------------------------------------------
vtkPVLODActor::~vtkPVLODActor()
{
  this->SetLODMapper(NULL);
  this->Device->Delete();
  this->Device = NULL;
}


//----------------------------------------------------------------------------
void vtkPVLODActor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor::PrintSelf(os,indent);

  
}

//----------------------------------------------------------------------------
// We use points as the size of the data, because cells cqan mislead.
// A good example is verts.  One cell can contain any number of verticies.
vtkMapper *vtkPVLODActor::SelectMapper()
{
  float myTime, timePerPoint;

  if (this->Mapper == NULL || this->Mapper->GetInput() == NULL)
    {
    return this->LODMapper;
    }
  if (this->LODMapper == NULL || this->LODMapper->GetInput() == NULL)
    {
    return this->Mapper;
    }

  // Figure out how much time we have to render.
  myTime = this->AllocatedRenderTime;

  // Choose the smallest time constant.
  timePerPoint = this->TimePerPoint;
  if (this->LODTimePerPoint < timePerPoint)
    {
    timePerPoint = this->LODTimePerPoint;
    }

  // Will the high res take too long?
  if ( (timePerPoint * this->Mapper->GetInput()->GetNumberOfPoints()) > myTime)
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

  // Now since We cannot use the mappers "TimeToDraw" before the render,
  // we need to save the information.
  if (mapper == this->Mapper)
    {
    this->TimePerPoint = mapper->GetTimeToDraw() / mapper->GetInput()->GetNumberOfPoints();
    }
  else if (mapper == this->LODMapper)
    {
    this->LODTimePerPoint = mapper->GetTimeToDraw() / mapper->GetInput()->GetNumberOfPoints();
    }
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

