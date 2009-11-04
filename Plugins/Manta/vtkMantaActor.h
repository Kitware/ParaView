/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkMantaActor.h

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
// .NAME vtkMantaActor - Manta Raytracer actor
// .SECTION Description
// vtkMantaActor is a concrete implementation of the abstract class vtkActor.
// vtkMantaActor interfaces to the Manta Raytracer library.

#ifndef __vtkMantaActor_h
#define __vtkMantaActor_h

#include "vtkMantaConfigure.h"
#include "vtkActor.h"

//BTX
namespace Manta {
class Group;
class Mesh;
class AccelerationStructure;
class Object;
};
//ETX

class vtkTimeStamp;
class vtkMantaProperty;
class vtkMantaRenderer;

class VTK_vtkManta_EXPORT vtkMantaActor : public vtkActor
{
public:
  static vtkMantaActor *New();
  vtkTypeRevisionMacro(vtkMantaActor,vtkActor);
  virtual void PrintSelf(ostream& os, vtkIndent indent);
  
  virtual void SetVisibility(int);
  
  vtkProperty * MakeProperty();
  
  // Description:
  // This causes the actor to be rendered. It in turn will render the actor's
  // property, texture map and then mapper. If a property hasn't been
  // assigned, then the actor will create one automatically. Note that a side
  // effect of this method is that the pipeline will be updated.
  void Render(vtkRenderer *ren, vtkMapper *mapper);
  
  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);
  void RemoveObjects(vtkRenderer * renderer, bool deleteMesh );
  
  //BTX
  // Regular non-vtkSetGet functions are used here as Manta does NOT
  // adopt reference-counting to manage objects. Otherwise it would
  // complicate the memory management issue due to the inconsistency.
  void SetMesh( Manta::Mesh * mesh ) { this->Mesh = mesh; }
  void SetMantaAS( Manta::AccelerationStructure * mantaAS )
  { this->MantaAS = mantaAS; }
  void SetMantaWorldGroup( Manta::Group * mantaWorldGroup )
  { this->MantaWorldGroup = mantaWorldGroup; }
  Manta::Mesh * GetMesh() { return this->Mesh; }
  Manta::AccelerationStructure * GetMantaAS() { return this->MantaAS; }
  Manta::Group * GetMantaWorldGroup() { return this->MantaWorldGroup; }
  //ETX
  
  vtkSetMacro(IsModified, bool);
  vtkGetMacro(IsModified, bool);
  
  // Description:
  // Set / Get the visibility of the actor for the LAST frame / time.
  // This PREVIOUS status is used in comparison with the visibility
  // for the CURRENT frame / time to determine whether or not
  // this->SetIsModified(true) and hence vtkMantaActor::UpdateObjects()
  // are to be invoked in support of dynamically turning on/off vtkManta
  // objects including the center of rotation axes widget.
  //
  // These two functions are called in vtkMantaRenderer::
  // UpdateActorsForVisibility() to determine whether the geometry,
  // specifically, the acceleration structure, of an actor needs to
  // be added to or removed from the host renderer's world group.
  vtkSetMacro(LastVisibility, bool);
  vtkGetMacro(LastVisibility, bool);
  void DetachFromMantaRenderEngine( vtkMantaRenderer * renderer );
  
  // Description:
  // Set/Get the renderer that this vtkMantaActor is added to.
  //
  // 'Set' is called in vtkMantaRenderer::UpdateActorsForVisibility()
  // such that this vtkMantaActor obtains a handle to the associated
  // vtkMantaRenderer for memory de-allocation upon destruction when
  // a vtkManta object is deleted, either explicitly through ParaView
  // pipeline browser or implicitly via closing ParaView.
  void SetRenderer( vtkMantaRenderer * renderer )
  { this->Renderer = renderer; }
  vtkMantaRenderer * GetRenderer() { return this->Renderer; }
  
 protected:
  vtkMantaActor();
  ~vtkMantaActor();
  
 private:
  vtkMantaActor(const vtkMantaActor&);  // Not implemented.
  void operator=(const vtkMantaActor&);  // Not implemented.
  
  vtkMantaRenderer * Renderer;
  bool IsModified;
  bool LastVisibility; // visibility for the LAST frame / time
  void UpdateObjects(vtkRenderer *);
  vtkTimeStamp MeshMTime;
    
  // for the moment, each Actor is assumed to be "single piece" and is
  // represented by one Manta::Mesh, one Manta::AccelerationStructure
  // is used to accelerate the raytracing process.
  //BTX
  Manta::Mesh * Mesh;
  Manta::AccelerationStructure * MantaAS;
  Manta::Group * MantaWorldGroup;
  //ETX
};

#endif
