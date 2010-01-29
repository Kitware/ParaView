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
#include "vtkMantaManager.h"
#include "vtkMantaProperty.h"
#include "vtkMantaRenderer.h"
#include "vtkMantaRenderWindow.h"
#include "vtkMapper.h"

#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"

#include <Interface/Scene.h>
#include <Interface/Context.h>
#include <Engine/Control/RTRT.h>
#include <Model/Groups/DynBVH.h>
#include <Model/Groups/Group.h>

vtkCxxRevisionMacro(vtkMantaActor, "1.9");
vtkStandardNewMacro(vtkMantaActor);

//----------------------------------------------------------------------------
vtkMantaActor::vtkMantaActor() : Group(0), MantaAS(0)
{
  cerr << "MA(" << this << ") CREATE" << endl;
  this->MantaManager = NULL;
}

//----------------------------------------------------------------------------
// now some Manta resources, ignored previously, can be de-allocated safely
//
vtkMantaActor::~vtkMantaActor()
{
  cerr << "MA(" << this << ") DESTROY" << endl;
  if (this->MantaManager)
    {
    this->MantaManager->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkMantaActor::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//----------------------------------------------------------------------------
vtkProperty *vtkMantaActor::MakeProperty()
{
  return vtkMantaProperty::New();
}

//----------------------------------------------------------------------------
void vtkMantaActor::ReleaseGraphicsResources( vtkWindow * win )
{
  cerr << "MA(" << this << ") RELEASE GRAPHICS RESOURCES" << endl;
  this->Superclass::ReleaseGraphicsResources( win );

  if (!this->MantaManager)
    {
    return;
    }
  this->MantaManager->GetMantaEngine()->
    addTransaction(
                   "delete geometry",
                   Manta::Callback::create( this,
                                            &vtkMantaActor::RemoveObjects,
                                            true
                                            )
                   );
}

//----------------------------------------------------------------------------
void vtkMantaActor::Render( vtkRenderer * ren, vtkMapper * mapper )
{
  if ( vtkMantaRenderer * mantaRenderer = vtkMantaRenderer::SafeDownCast( ren ) )
    {
    if (!this->MantaManager)
      {
      this->MantaManager = mantaRenderer->GetMantaManager();
      this->MantaManager->Register(this);
      }

    // TODO: be smarter on update or create rather than create every time
    // build transformation (with AffineTransfrom and Instance?)

    // TODO: the way "real FLAT" shading is done right now (by not supplying vertex
    // normals), changing from FLAT to Gouraud shading needs to create a new mesh.

    //check if anything that affect appearence has changed, if so, rebuild manta
    //object so we see it. Don't do it every frame, since it is costly.
    if (mapper->GetInput()->GetMTime() > this->MeshMTime ||
        mapper->GetMTime() > this->MeshMTime ||
        this->GetProperty()->GetMTime() > this->MeshMTime ||
        this->GetMTime() > this->MeshMTime)
      {
      // update pipeline to get up to date data to show
      mapper->Render(ren, this);

      this->MeshMTime.Modified();

      this->MantaManager->GetMantaEngine()->addTransaction
        ("update geometry",
         Manta::Callback::create(this, &vtkMantaActor::UpdateObjects, ren));
      }
    }
}

//----------------------------------------------------------------------------
void vtkMantaActor::SetVisibility(int newval)
{
  if (newval == this->GetVisibility())
    {
    return;
    }
  if (this->MantaManager && !newval)
    {
    //this is necessary since Render (and thus UpdateObjects) is not 
    //called when visibility is off.
    this->MantaManager->GetMantaEngine()->addTransaction
      ( "detach geometry",
        Manta::Callback::create( this,
                                 &vtkMantaActor::RemoveObjects,
                                 false
                               )
      );
    }
  this->Superclass::SetVisibility(newval);
}

//----------------------------------------------------------------------------
void vtkMantaActor::RemoveObjects(bool deleteMesh )
{  
  cerr << "MA(" << this << ") REMOVE OBJECTS" << endl;
  if (!this->MantaManager)
    {
    return;
    }

  if (this->MantaAS)
    {
    //TODO: I think the old group can and should be deleted too
    this->MantaAS->setGroup( NULL ); 

    this->MantaManager->GetMantaWorldGroup()->remove( this->MantaAS, true );
    //delete this->MantaAS; //TODO: does remove above to this? This is double delete F.S.R.
    this->MantaAS = NULL;
    }

  if (deleteMesh)
    {    
    if (this->Group)
      {
      //delete all geometry
      this->Group->shrinkTo(0, true);
      delete this->Group;
      this->Group = NULL;
      }
    }
}

//----------------------------------------------------------------------------
void vtkMantaActor::UpdateObjects( vtkRenderer * ren )
{
  cerr << "MA(" << this << ") UPDATE" << endl;

  vtkMantaRenderer * mantaRenderer =
    vtkMantaRenderer::SafeDownCast( ren );
  if (!mantaRenderer)
    {
    return;
    }

  //TODO:
  //We are using Manta's DynBVH, but we never use it Dyn-amically.
  //Instead we delete the old and rebuild a new AS every time something changes,
  //We should either ask the DynBVH to update itself,
  //or try different acceleration structures. Those might be faster - either 
  //during sort or during search.

  //Remove whatever we used to show in the scene
  this->RemoveObjects(false);

  if (this->Group)
    {
    //Add what we are now supposed to show
    //Create an acceleration structure for the data and add it to the scene

    //We have to nest to make an AS for each inner group
    //Is there a Manta call we can make to simply recurse while making the AS?
    this->MantaAS = new Manta::DynBVH();
    Manta::Group *group = new Manta::Group();
    for (unsigned int i = 0; i < this->Group->size(); i++)
      {
      Manta::DynBVH *innerBVH = new Manta::DynBVH();
      Manta::Group * innerGroup = static_cast<Manta::Group *>
        (this->Group->get(i));
      if (innerGroup)
        {
        innerBVH->setGroup(innerGroup);
        group->add(innerBVH);
        }
      else
        {
        delete innerBVH;
        group->add(this->Group->get(i));
        }
      }
    this->MantaAS->setGroup(group);
    Manta::Group* mantaWorldGroup = this->MantaManager->GetMantaWorldGroup();
    mantaWorldGroup->add(static_cast<Manta::Object *> (this->MantaAS));

    Manta::PreprocessContext context(this->MantaManager->GetMantaEngine(), 0, 1,
                                     this->MantaManager->GetMantaLightSet());
    mantaWorldGroup->preprocess(context);
    }
}
