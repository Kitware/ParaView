/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkMantaActor.cxx

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
// .NAME vtkMantaActor - 
// .SECTION Description
//

#include "vtkManta.h"
#include "vtkMantaActor.h"
#include "vtkMantaRenderer.h"
#include "vtkMantaProperty.h"
#include "vtkMapper.h"
#include "vtkMantaRenderWindow.h"

#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"

#include <Interface/Scene.h>
#include <Interface/Context.h>
#include <Engine/Control/RTRT.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Groups/Group.h>
#include <Model/Groups/Mesh.h>

vtkCxxRevisionMacro(vtkMantaActor, "1.2");
vtkStandardNewMacro(vtkMantaActor);

//----------------------------------------------------------------------------
vtkMantaActor::vtkMantaActor() : Mesh(0), MantaAS(0), MantaWorldGroup(0),
  IsModified(false), LastVisibility(false), Renderer(0)
{
}

//----------------------------------------------------------------------------
// now some Manta resources, ignored previously, can be de-allocated safely
//
vtkMantaActor::~vtkMantaActor()
{
  // The following code allows the user EITHER to repeatedly create and delete
  // (explicit mode) vtkManta objects through ParaView GUI OR to simply leave
  // them there to close ParaView (implicit deletion), without segfaults.
  if ( this->Mesh    && this->Renderer &&
       this->MantaAS && this->MantaWorldGroup
     )
    {
    // Now that this->Renderer is NOT NULL, we can check if the render window
    // is available too. Debug results indicate that a render window might be
    // NULL while Mesh, MantaAS, Renderer, and MantaWorldGroup still exist. In
    // this case, it is Neither this current renderer Nor this current actor
    // responsible for memory de-allocation. One fact is that there are usually
    // multiple actors corresponding to a single VISIBLE Manta geometry, of
    // which some are always INVISIBLE regardless of the visibility of the
    // geometry in the scene. This partly adds to the complexity of memory
    // de-allocation.
    if ( this->Renderer->GetRenderWindow() )
      {
      // NOTE: ONLY when the render window is still available, can Manta
      // objects be deleted. In fact only renderers on layer #0 are able to
      // reach this line.
      //
      // However, an attempt to use the Manta callback scheme for this goal
      // just does NOT work and incurs segfault problems (just upon explicit
      // deletion) even when we guarantee that all required resources, i.e.,
      // renderer, render window, renderer collection, first renderer, safe
      // downcast, Manta engine, and Manta world group, are NOT NULL upon
      // invoking the Manta callback function. This problem is probably due
      // to the thread-safety issue that occurs just when those so many
      // if-statements are checked. In other words, a TRUE if-statement might
      // be immediately made FALSE due to the interference of other co-existent
      // threads' behaviors.
      //
      this->RemoveObjects( this->Renderer, true );
      }
    }

  // now safe to assign them to NULL
  this->Mesh     = NULL;
  this->MantaAS  = NULL;
  this->Renderer = NULL;
  this->MantaWorldGroup = NULL;
}

//----------------------------------------------------------------------------
vtkProperty *vtkMantaActor::MakeProperty()
{
  return vtkMantaProperty::New();
}

//----------------------------------------------------------------------------
void vtkMantaActor::ReleaseGraphicsResources( vtkWindow * win )
{
  if (!win)
    {
    // called by vtkRenderer::SetRenderWindow() with win==0 at initialization
    return;
    }

  if (!MantaAS)
    {
    return;
    }

  if (vtkRenderWindow * renWin = vtkRenderWindow::SafeDownCast(win))
    {
    this->MantaWorldGroup = 0;

    vtkRendererCollection * renders = renWin->GetRenderers();
    if (vtkMantaRenderer * mantaRenderer = vtkMantaRenderer::SafeDownCast(
        renders->GetFirstRenderer()))
      {
#if 0
      // TODO: We can't remove the object this way, when PV is shutting down,
      // mantaRenderer->MantaEngine is already invalid at this point.
      mantaRenderer->GetMantaEngine()->
      addTransaction(
          "delete geometry",
          Manta::Callback::create( this,
              &vtkMantaActor::RemoveObjects,
              renders->GetFirstRenderer(),
              true
          )
      );
#else
      // TODO: We can't remove the object this way either, when PV is still running and
      // the user deletes the actor, the Manta Engine may be still running and cause
      // thread safety issue.
      //this->RemoveObjects( renders->GetFirstRenderer(), true );
#endif
      }
    }

  //this->Superclass::ReleaseGraphicsResources( win );
}

//----------------------------------------------------------------------------
// This function invokes vtkMantaActor::RemoveObjects( . ) through the Manta::
// Callback::create( ... ) scheme for THREAD SAFETY. **PLEASE NOTE**: A DIRECT
// call to vtkMantaActor::RemoveObjects( . ) would cause segfault problems. It
// could be only the first renderer of a render window that is to be used to
// detach the geometry, specifically remove the acceleration structure, from
// the world group of the scene to turn off the visibility of the actor.

//
// called by vtkMantaRenderer::UpdateActorsForVisibility()
void vtkMantaActor::DetachFromMantaRenderEngine( vtkMantaRenderer * renderer )
{
  vtkRendererCollection * renders = renderer->GetRenderWindow()->GetRenderers();

  if ( vtkMantaRenderer *render0 =
       vtkMantaRenderer::SafeDownCast( renders->GetFirstRenderer() ) )
    {
    render0->GetMantaEngine()->addTransaction
      ( "detach geometry",
        Manta::Callback::create( this,
                                 &vtkMantaActor::RemoveObjects,
                                 renders->GetFirstRenderer(),
                                 false
                               )
      );
    }
}

//----------------------------------------------------------------------------
// DETACH the 'old' mesh from the acceleration structure (this->MantaAS) and
// DETACH and DEEP-delete the acceleration structure (this->MantaAS) from the
// host renderer's Manta world group (i.e., this->MantaWorldGroup)'s
// vector<Manta::Object *>.
//
// NOTE: this function supports BOTH visibility toggling' of vtkManta objects
//       (including the center of rotation axes widget) AND deleting vtkManta
//       objects. For the former, it does NOT delete the 'old' mesh because
//       the 'old' mesh might be assigned to a new acceleration structure upon
//       visitility on later. The mesh, as the underlying geometry, will NOT
//       be deleted until the corresponding vtkManta object is totally deleted
//       , i.e., in the latter case, either explicitly via ParaView's pipeline
//       browser or implicitly by closing ParaView.
//
//       The argument, 'renderer', is usually the FIRST renderer of a window.
//
//       Simply DETACHING (without deep-deleting) this->MantaAS would affect
//       the switch between various color map modes.
//
// RELATED: vtkMantaRenderer::UpdateActorsForVisibility()
void vtkMantaActor::RemoveObjects( vtkRenderer * renderer, bool deleteMesh )
{
  // this->MantaAS is an acceleration structure attached to a vtkMantaActor
  // object (the 'current' acceleration structure)
  if ( this->MantaAS )
    {
    // Detach the 'old' mesh, i.e, the group (Manta::DynBVH::currGroup)
    // assigned to the current acceleration structure, from the latter. It is
    // this->UpdataObjects( ... ) that sets the 'old' mesh as the 'currGroup'
    // of this->MantaAS.
    //
    // NOTE: we might NOT delete the 'old' mesh here as it might be exactly
    //       the same as the one (this->Mesh) to be assigned LATER to a new
    //       acceleration structure, as is the case with visibility toggling.
    this->MantaAS->setGroup( NULL );

    // ONLY when the corresponding vtkManta object is totally deleted, either
    // explicitly through ParaView's pipeline browser or implicitly by closing
    // ParaView, is the mesh actually deleted as follows
    if ( deleteMesh == true && this->Mesh != NULL )
      {
      delete this->Mesh;
      this->Mesh = NULL;
      }

    // Remove the pointer to the current acceleration structure (this->MantaAS)
    // from the host renderer's world group (i.e., this->MantaWorldGroup)'s
    // vector<Manta::Object *> and actually delete this acceleration structure
    // (true means DEEP delete) that is of type Manta::Object.
    if ( vtkMantaRenderer * mantaRenderer =
         vtkMantaRenderer::SafeDownCast( renderer ) )
      {
      mantaRenderer->GetMantaWorldGroup()->remove( this->MantaAS, true );
      }

    // this->MantaAS was DEEP-deleted while this->Mesh may or may not exist
    this->MantaAS = NULL;
    }

  // TODO: we should invalidate the already rendered image in the pipeline
}

//----------------------------------------------------------------------------
// Update the acceleration structure and the host renderer's world group with
// a 'new' mesh (this->Mesh), which might be the OLD one when switching
// between various color map modes. It is NEW only when a vtkManta (geometric)
// object is created from scratch.
//
// RELATED: vtkMantaRenderer::UpdateActorsForVisibility()
void vtkMantaActor::UpdateObjects( vtkRenderer * ren )
{
  // This line is used for switching between various color map modes and hence
  // the 'old' mesh is actually NOT deleted at all. In fact, the 'old' mesh is
  // detached from this->MantaAS. Then this->MantaAS is detached from the Manta
  // world group and DEEP-deleted.
  //
  // This line is skipped for any vtkManta (geometric) object that is created
  // from scratch. For this case, this->Mesh is indeed a NEW mesh and should be
  // readily available upon the call to this function.
  this->RemoveObjects( ren, false );

  if (vtkMantaRenderer * mantaRenderer = vtkMantaRenderer::SafeDownCast(ren))
    {
    // create a new acceleration structure
    this->MantaAS = new Manta::DynBVH();

    // Set the mesh, this->Mesh, as the 'currGroup' of the acceleration
    // structure, this->MantaAS, and rebuild/update will be done by preprocess()
    // TODO: can we just "update" the mesh instead of setting a new one?
    this->MantaAS->setGroup(this->Mesh);
    this->IsModified = false;

    // add this new acceleration structure to the host renderer's world group
    // (this->MantaWorldGroup)'s vector<Manta::Object *>
    mantaRenderer->GetMantaWorldGroup()->add(
        static_cast<Manta::Object *> (this->MantaAS));

    this->MantaWorldGroup = mantaRenderer->GetMantaWorldGroup();

    // apply preprocessing to the world group of the host renderer
    Manta::PreprocessContext context(mantaRenderer->GetMantaEngine(), 0, 1,
        mantaRenderer->GetMantaLightSet());
    mantaRenderer->GetMantaWorldGroup()->preprocess(context);

    // number of objects in the world group
    cerr << "world group size " << mantaRenderer->GetMantaWorldGroup()->size()
        << endl;
    }
}

//----------------------------------------------------------------------------
// Actual actor render method. called by vtkActor::RenderOpaqueGeometry and
// vtkActor::RenderTranslucentGeometry after vtkProperty::Render() and
// vtkTexture::Renderer() are called
void vtkMantaActor::Render( vtkRenderer * ren, vtkMapper * mapper )
{
  if ( vtkMantaRenderer * mantaRenderer = vtkMantaRenderer::SafeDownCast( ren ) )
    {
    // TODO: be smarter on update or create rather than create every time
    // build transformation (with AffineTransfrom and Instance?)

    // TODO: the way "real FLAT" shading is done right now (by not supplying vertex
    // normals), changing from FLAT to Gouraud shading needs to create a new mesh.


    //check if anything that affect appearence has changed, if so, rebuild manta
    //object so we see it. Don't do it every frame, since it is costly.
    if (mapper->GetInput()->GetMTime() > this->MeshMTime ||
        mapper->GetMTime() > this->MeshMTime ||
        this->GetProperty()->GetMTime() > this->MeshMTime ||
        this->IsModified)
      {
      if (!(mapper->GetScalarVisibility()))
        {
        vtkMantaProperty * mantaProperty =
          vtkMantaProperty::SafeDownCast(this->GetProperty());
        if (!mantaProperty)
          return;

        if (this->Mesh)
          {
          this->Mesh->materials.erase(this->Mesh->materials.begin());
          this->Mesh->materials.push_back(mantaProperty->GetMantaMaterial());
          // do preprocessing for the material since we only update the material
          // and don't call preprocessing for the actor
          Manta::PreprocessContext context
            (mantaRenderer->GetMantaEngine(), 0, 1,
             mantaRenderer->GetMantaLightSet());
          mantaProperty->GetMantaMaterial()->preprocess(context);
          this->MeshMTime.Modified();
          }
        }
      else
        {
        // send a render to the mapper to update pipeline
        mapper->Render(ren, this);

        // remove old and add newly created geometries to the world of objects
        if (this->IsModified)
          {
          this->MeshMTime.Modified();
          mantaRenderer->GetMantaEngine() ->addTransaction("update geometry",
            Manta::Callback::create(this, &vtkMantaActor::UpdateObjects, ren));
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkMantaActor::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//----------------------------------------------------------------------------
void vtkMantaActor::SetVisibility(int newval)
{
  this->Superclass::SetVisibility(newval);
}
