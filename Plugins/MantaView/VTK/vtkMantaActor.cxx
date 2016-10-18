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

#include "vtkManta.h"
#include "vtkMantaActor.h"
#include "vtkMantaCompositeMapper.h"
#include "vtkMantaManager.h"
#include "vtkMantaProperty.h"
#include "vtkMantaRenderer.h"
#include "vtkMantaTexture.h"

#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"
#include "vtkTimerLog.h"

#include <Engine/Control/RTRT.h>
#include <Interface/Context.h>
#include <Interface/Scene.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Groups/Group.h>
#include <Model/Groups/RecursiveGrid.h>

//============================================================================

// This is a helper that exists just to hold on to manta side resources
// long enough for the manta thread to destroy them, whenever that
// threads gets around to it (in a callback)
class vtkMantaActorThreadCache
{
public:
  vtkMantaActorThreadCache(Manta::Group* w, Manta::AccelerationStructure* a, Manta::Group* g)
    : MantaWorldGroup(w)
    , MantaAS(a)
    , MantaGeom(g)
  {
    this->DebugCntr = vtkMantaActorThreadCache::GlobalCntr++;
    // cerr << "MAPR( " << this << ") " << this->DebugCntr << endl;
    // cerr << " AS: " << this->MantaAS << endl;
    // cerr << " WG: " << this->MantaWorldGroup << endl;
    // cerr << " MG: " << this->MantaGeom << endl;
  }

  void FreeMantaResources()
  {
    // cerr << "MAPR(" << this << ") FREE MANTA RESOURCES "
    //    << this->DebugCntr << endl;
    // cerr << " AS: " << this->MantaAS << endl;
    if (this->MantaAS)
    {
      this->MantaAS->setGroup(NULL);
    }
    // cerr << " WG: " << this->MantaWorldGroup << endl;
    if (this->MantaWorldGroup)
    {
      this->MantaWorldGroup->remove(this->MantaAS, false);
    }
    delete this->MantaAS;
    // cerr << " MG: " << this->MantaGeom << endl;
    if (this->MantaGeom)
    {
      this->MantaGeom->shrinkTo(0, true);
    }
    delete this->MantaGeom;

    // WARNING: this class must never be instantiated on the stack.
    // Therefore, it has private unimplemented copy/contructors.
    delete this;
  }

  Manta::Group* MantaWorldGroup;
  Manta::AccelerationStructure* MantaAS;
  Manta::Group* MantaGeom;
  int DebugCntr;
  static int GlobalCntr;

private:
  vtkMantaActorThreadCache(const vtkMantaActorThreadCache&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMantaActorThreadCache&) VTK_DELETE_FUNCTION;
};

int vtkMantaActorThreadCache::GlobalCntr = 0;

//===========================================================================

vtkStandardNewMacro(vtkMantaActor);

//----------------------------------------------------------------------------
vtkMantaActor::vtkMantaActor()
  : Group(0)
  , MantaAS(0)
{
  // cerr << "MA(" << this << ") CREATE" << endl;
  this->MantaManager = NULL;
  this->SortType = DYNBVH;
  this->MantaTexture = NULL;
}

//----------------------------------------------------------------------------
// now some Manta resources, ignored previously, can be de-allocated safely
//
vtkMantaActor::~vtkMantaActor()
{
  // cerr << "MA(" << this << ") DESTROY" << endl;
  this->SetMantaTexture(NULL);

  if (this->MantaManager)
  {
    this->ReleaseGraphicsResources(NULL);
    // cerr << "MA(" << this << " DESTROY " << this->MantaManager << " "
    //     << this->MantaManager->GetReferenceCount() << endl;
    this->MantaManager->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkMantaActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkProperty* vtkMantaActor::MakeProperty()
{
  return vtkMantaProperty::New();
}

//----------------------------------------------------------------------------
void vtkMantaActor::ReleaseGraphicsResources(vtkWindow* win)
{
  // cerr << "MA(" << this << ") RELEASE GRAPHICS RESOURCES" << endl;
  if (win)
  {
    this->Superclass::ReleaseGraphicsResources(win);
  }
  if (!this->MantaManager)
  {
    // cerr << "MA(" << this << ") NO MGR" << endl;
    return;
  }

  // save off the pointers for the manta thread
  vtkMantaActorThreadCache* R = new vtkMantaActorThreadCache(
    this->MantaManager->GetMantaWorldGroup(), this->MantaAS, this->Group);
  // cerr << "MA(" << this << ") handed off to MAPR(" << R << ")" << endl;
  // make no further references to them in this thread
  this->MantaAS = NULL;
  this->Group = NULL;

  // ask the manta thread to free them when it can
  this->MantaManager->GetMantaEngine()->addTransaction(
    "cleanup actor", Manta::Callback::create(R, &vtkMantaActorThreadCache::FreeMantaResources));
}

//----------------------------------------------------------------------------
void vtkMantaActor::Render(vtkRenderer* ren, vtkMapper* mapper)
{
  // cerr << "MA(" << this << ") RENDER" << endl;
  if (vtkMantaRenderer* mantaRenderer = vtkMantaRenderer::SafeDownCast(ren))
  {
    if (!this->MantaManager)
    {
      this->MantaManager = mantaRenderer->GetMantaManager();
      // cerr << "MA(" << this << " REGISTER " << this->MantaManager << " "
      //     << this->MantaManager->GetReferenceCount() << endl;
      this->MantaManager->Register(this);
    }

    // TODO: be smarter on update or create rather than create every time
    // build transformation (with AffineTransfrom and Instance?)

    // TODO: the way "real FLAT" shading is done right now (by not supplying vertex
    // normals), changing from FLAT to Gouraud shading needs to create a new mesh.

    // check if anything that affect appearence has changed, if so, rebuild manta
    // object so we see it. Don't do it every frame, since it is costly.
    unsigned long itime = 0;
    if (mapper->GetInput())
    {
      itime = mapper->GetInput()->GetMTime();
    }
    else
    {
      vtkMantaCompositeMapper* mcMapper = vtkMantaCompositeMapper::SafeDownCast(mapper);
      if (mcMapper)
      {
        itime = mcMapper->GetInputTime();
      }
    }

    if ((itime > this->MeshMTime) || (mapper->GetMTime() > this->MeshMTime) ||
      (this->GetProperty()->GetMTime() > this->MeshMTime) || (this->GetMTime() > this->MeshMTime))
    {
      // update pipeline to get up to date data to show
      mapper->Render(ren, this);

      this->MeshMTime.Modified();

      this->MantaManager->GetMantaEngine()->addTransaction(
        "update geometry", Manta::Callback::create(this, &vtkMantaActor::UpdateObjects, ren));
    }
  }
}

//----------------------------------------------------------------------------
void vtkMantaActor::SetVisibility(int newval)
{
  // cerr << "MA(" << this << ") SET VISIBILITY " << newval << endl;
  if (newval == this->GetVisibility())
  {
    return;
  }
  if (this->MantaManager && !newval)
  {
    // this is necessary since Render (and thus UpdateObjects) is not
    // called when visibility is off.
    this->MantaManager->GetMantaEngine()->addTransaction(
      "detach geometry", Manta::Callback::create(this, &vtkMantaActor::RemoveObjects));
  }
  this->Superclass::SetVisibility(newval);
}

//----------------------------------------------------------------------------
void vtkMantaActor::RemoveObjects()
{
  // cerr << "MA(" << this << ") REMOVE OBJECTS" << endl;
  if (!this->MantaManager)
  {
    return;
  }

  if (this->MantaAS)
  {
    // cerr << " AS: " << this->MantaAS << endl;
    // cerr << " WG: " << this->MantaManager->GetMantaWorldGroup() << endl;
    Manta::Group* grp = this->MantaAS->getGroup();
    for (unsigned int i = 0; i < grp->size(); i++)
    {
      Manta::Group* ig = dynamic_cast<Manta::Group*>(grp->get(i));
      if (ig)
      {
        ig->shrinkTo(0, true);
        // delete ig;
      }
    }
    grp->shrinkTo(0, true);
    // delete grp;
    this->MantaAS->setGroup(NULL);
    this->MantaManager->GetMantaWorldGroup()->remove(this->MantaAS, false);
    delete this->MantaAS;
    this->MantaAS = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkMantaActor::UpdateObjects(vtkRenderer* ren)
{
  // cerr << "MA(" << this << ") UPDATE OBJ" << endl;
  vtkMantaRenderer* mantaRenderer = vtkMantaRenderer::SafeDownCast(ren);
  if (!mantaRenderer)
  {
    return;
  }

  if (!this->MantaManager)
  {
    return;
  }

  // TODO:
  // We are using Manta's DynBVH, but we never use it Dyn-amically.
  // Instead we delete the old and rebuild a new AS every time something changes,
  // We should either ask the DynBVH to update itself,
  // or try different acceleration structures. Those might be faster - either
  // during sort or during search.

  // Remove whatever we used to show in the scene
  this->RemoveObjects();

  // Add what we are now supposed to show.
  if (this->Group)
  {
    vtkTimerLog::MarkStartEvent("Execute AccelStructBuild ");
    // Create an acceleration structure for the data and add it to the scene

    // We have to nest to make an AS for each inner group
    // Is there a Manta call we can make to simply recurse while making the AS?
    this->MantaAS = new Manta::DynBVH(false);
    // cerr << "MA(" << this << ") CREATE AS " << this->MantaAS << endl;
    Manta::Group* group = new Manta::Group();
    for (unsigned int i = 0; i < this->Group->size(); i++)
    {
      Manta::AccelerationStructure* innerAS = NULL;
      switch (this->SortType)
      {
        case DYNBVH:
        default:
          innerAS = new Manta::DynBVH(false);
          break;
        case RECURSIVEGRID3:
          innerAS = new Manta::RecursiveGrid(3);
          break;
      }

      Manta::Group* innerGroup = dynamic_cast<Manta::Group*>(this->Group->get(i));
      if (innerGroup)
      {
        // cerr << "MA(" << this << ") BVH FOR " << i << " " << innerGroup << endl;
        innerAS->setGroup(innerGroup);
        group->add(innerAS);
        innerAS->rebuild();
      }
      else
      {
        // cerr << "MA(" << this << ") SIMPLE " << i << " " << innerGroup << endl;
        delete innerAS;
        group->add(this->Group->get(i));
      }
    }

    // cerr << "MA(" << this << ") PREPROCESS" << endl;
    this->MantaAS->setGroup(group);
    Manta::Group* mantaWorldGroup = this->MantaManager->GetMantaWorldGroup();
    mantaWorldGroup->add(static_cast<Manta::Object*>(this->MantaAS));
    // cerr << "ME = " << this->MantaManager->GetMantaEngine() << endl;
    // cerr << "LS = " << this->MantaManager->GetMantaLightSet() << endl;
    Manta::PreprocessContext context(
      this->MantaManager->GetMantaEngine(), 0, 1, this->MantaManager->GetMantaLightSet());
    mantaWorldGroup->preprocess(context);
    // cerr << "PREP DONE" << endl;

    vtkTimerLog::MarkEndEvent("Execute AccelStructBuild ");
  }
}

//----------------------------------------------------------------------------
void vtkMantaActor::SetGroup(Manta::Group* group)
{
  // cerr << "MA(" << this << ") SET GROUP"
  //     << " WAS " << this->Group
  //     << " NOW " << group << endl;
  if (!this->Group)
  {
    this->Group = group;
    return;
  }

  // save off the pointers for the manta thread
  vtkMantaActorThreadCache* R = new vtkMantaActorThreadCache(NULL, NULL, this->Group);

  this->Group = group;
  // ask the manta thread to free them when it can
  this->MantaManager->GetMantaEngine()->addTransaction(
    "change geometry", Manta::Callback::create(R, &vtkMantaActorThreadCache::FreeMantaResources));
}

//----------------------------------------------------------------------------
void vtkMantaActor::SetTexture(vtkTexture* texture)
{
  if (texture == this->GetTexture())
  {
    return;
  }
  this->Superclass::SetTexture(texture);
  if (texture)
  {
    vtkMantaTexture* mt = vtkMantaTexture::New();
    mt->SetInputData(this->GetTexture()->GetInput());
    this->SetMantaTexture(mt);
    mt->Delete();
  }
  else
  {
    this->SetMantaTexture(NULL);
  }
}

//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkMantaActor, MantaTexture, vtkMantaTexture);
