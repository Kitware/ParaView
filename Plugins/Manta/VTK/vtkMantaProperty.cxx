/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkMantaProperty.cxx

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
#include "vtkMantaManager.h"
#include "vtkMantaProperty.h"
#include "vtkMantaRenderer.h"

#include "vtkObjectFactory.h"

#include <Core/Color/RGBColor.h>
#include <Engine/Control/RTRT.h>
//#include <Model/Materials/AmbientOcclusion.h>
#include <Model/Materials/Dielectric.h>
#include <Model/Materials/Flat.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/MetalMaterial.h>
#include <Model/Materials/OrenNayar.h>
#include <Model/Materials/Phong.h>
#include <Model/Materials/ThinDielectric.h>
#include <Model/Materials/Transparent.h>
#include <Model/Textures/Constant.h>

#include <cstring>

//============================================================================
//This is a helper that exists just to hold on to manta side resources
//long enough for the manta thread to destroy them, whenever that
//threads gets around to it (in a callback)
class vtkMantaPropertyThreadCache
{
public:
  vtkMantaPropertyThreadCache(Manta::Material *m,
      Manta::Texture<Manta::Color> *dT,
      Manta::Texture<Manta::Color> *sT )
    : MantaMaterial(m), DiffuseTexture(dT), SpecularTexture(sT)
  {
    this->DebugCntr = vtkMantaPropertyThreadCache::GlobalCntr++;
    //cerr << "MPPR( " << this << ") " << this->DebugCntr << endl;
  }
  
  void FreeMantaResources()
  {
    //cerr << "MPPR(" << this << ") FREE MANTA RESOURCES " 
    //<< this->DebugCntr << endl;
    delete this->MantaMaterial;
    delete this->DiffuseTexture;
    delete this->SpecularTexture;

    //WARNING: this class must never be instantiated on the stack.
    //Therefore, it has private unimplemented copy/contructors.
    delete this; 
  }
    
  Manta::Material *MantaMaterial;
  Manta::Texture<Manta::Color> *DiffuseTexture;
  Manta::Texture<Manta::Color> *SpecularTexture;
  int DebugCntr;
  static int GlobalCntr;

private:
  vtkMantaPropertyThreadCache(const vtkMantaPropertyThreadCache&);  // Not implemented.
  void operator=(const vtkMantaPropertyThreadCache&);  // Not implemented.
};

int vtkMantaPropertyThreadCache::GlobalCntr = 0;

//===========================================================================

vtkStandardNewMacro(vtkMantaProperty);

//----------------------------------------------------------------------------
vtkMantaProperty::vtkMantaProperty()
  : MantaMaterial(0), DiffuseTexture(0), SpecularTexture(0),
    Reflectance(0.0), Eta(1.52), Thickness(1.0), N(1.0), Nt(1.0)
{
  //cerr << "MP(" << this << ") CREATE" << endl;
  this->MaterialType = NULL;
  this->SetMaterialType("default");
  this->MantaManager = NULL;
}

//----------------------------------------------------------------------------
vtkMantaProperty::~vtkMantaProperty()
{
  if (this->MantaManager)
    {
    //cerr << "MP(" << this << ") DESTROY " << this->MantaManager << " " 
    //     << this->MantaManager->GetReferenceCount() << endl;   
    this->MantaManager->Delete();
    }
  delete[] this->MaterialType;
}

//----------------------------------------------------------------------------
void vtkMantaProperty::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkMantaProperty::ReleaseGraphicsResources(vtkWindow *win)
{
  //cerr << "MP(" << this << ") RELEASE GRAPHICS RESOURCES" << endl;
  this->Superclass::ReleaseGraphicsResources(win);
  if (!this->MantaManager)
    {
    return;
    }

  //save off the pointers for the manta thread
  vtkMantaPropertyThreadCache *R =
    new vtkMantaPropertyThreadCache(this->MantaMaterial,
                                    this->DiffuseTexture,
                                    this->SpecularTexture);
  //make no further references to them in this thread
  this->MantaMaterial = NULL;
  this->DiffuseTexture = NULL;
  this->SpecularTexture = NULL;
  
  //ask the manta thread to free them when it can
  this->MantaManager->GetMantaEngine()->addTransaction
    ( "cleanup property",
      Manta::Callback::create
      (R, &vtkMantaPropertyThreadCache::FreeMantaResources));
}

//----------------------------------------------------------------------------
void vtkMantaProperty::Render( vtkActor *vtkNotUsed(anActor),
                               vtkRenderer * ren)
{
  vtkMantaRenderer * mantaRenderer = vtkMantaRenderer::SafeDownCast( ren );
  if (!mantaRenderer)
    {
    return;
    }
  if (!this->MantaManager)
    {
    this->MantaManager = mantaRenderer->GetMantaManager();
    //cerr << "MP(" << this << ") REGISTER " << this->MantaManager << " " 
    //     << this->MantaManager->GetReferenceCount() << endl;
    this->MantaManager->Register(this);
    }

  return;
/*
  if ( this->GetMTime() > this->MantaMaterialMTime )
    {
    //TODO: this doesn't actually have to be a transaction, other than 
    //the deletions
    //TODO: Create should happen before now, whenever the prop is 
    //changes actually (see how MantaPolyDataMapper creates it)
    this->MantaManager->GetMantaEngine()->addTransaction
      ( "set property",
        Manta::Callback::create(this, &vtkMantaProperty::CreateMantaProperty));

    this->MantaMaterialMTime.Modified();
    }
*/
}

//----------------------------------------------------------------------------
// Implement base class method.
void vtkMantaProperty::BackfaceRender( vtkActor * vtkNotUsed( anActor ),
                                       vtkRenderer * vtkNotUsed( ren ) )
{
  // NOT supported by Manta
  //TODO: Do something about it.
  cerr
    << "vtkMantaProperty::BackfaceRender(), backface rendering "
    << "is not supported by Manta"
    << endl;
}


//----------------------------------------------------------------------------
void vtkMantaProperty::CreateMantaProperty()
{
  //cerr << "MP(" << this << ") CREATE MANTA PROPERTY" << endl;

  double * diffuse  = this->GetDiffuseColor();
  double * specular = this->GetSpecularColor();

  //this only happens in a manta thread callback, so this is safe to do - not true

  /*
  if (this->MantaMaterial)
    {
    cerr << "DELETING " << this->MantaMaterial << endl;
    }
  */
  delete this->MantaMaterial;
  delete this->DiffuseTexture;
  delete this->SpecularTexture;

  this->DiffuseTexture  = new Manta::Constant<Manta::Color>
    (  Manta::Color( Manta::RGBColor( diffuse[0],  diffuse[1],  diffuse[2]  ) )  );

  this->SpecularTexture = new Manta::Constant<Manta::Color>
    (  Manta::Color( Manta::RGBColor( specular[0], specular[1], specular[2] ) )  );

  // A note on Manta Materials and shading model:
  // 1. Surface normal is computed at each hit point, if the primitive
  // has curvature (like sphere or triangle with vertex normal), it will
  // be different at each hit point.
  // 2. The Flat material takes account only the surface normal and the
  // color texture of the material. The color texture acts like the emissive
  // color in OpenGL. It is multiplied by the dot product of surface normal
  // and ray direction to get the result color. Since the dot product is
  // different at each point on the primitive the result looks more like
  // Gouraud shading without specular highlight in OpenGL.
  // 3. To get "real" FLAT shading, we have to not to supply vertex normal when
  // we create the mesh.
  // TODO: replace this whole thing with a factory
  if ( strcmp( this->MaterialType, "default" ) == 0 )
    {
    // if lighting is disabled, use EmitMaterial
    if ( this->Interpolation == VTK_FLAT )
      {
      this->MantaMaterial = new Manta::Flat( this->DiffuseTexture );
      }
    else
      if ( this->GetOpacity() < 1.0 )
        {
        this->MantaMaterial =
          new Manta::Transparent( this->DiffuseTexture, this->GetOpacity() );
        }
      else
        if ( this->GetSpecular() == 0 )
          {
          this->MantaMaterial = new Manta::Lambertian( this->DiffuseTexture );
          }
        else
          {
          this->MantaMaterial =
            new Manta::Phong( this->DiffuseTexture,
                              this->SpecularTexture,
                              static_cast<int> ( this->GetSpecularPower() ),
                              NULL );
          }
    }
  else
    {
    if ( strcmp( this->MaterialType,  "lambertian" ) == 0 )
      {
      this->MantaMaterial = new Manta::Lambertian( this->DiffuseTexture );
      }
    else
      if ( strcmp( this->MaterialType, "phong" ) == 0 )
        {
        this->MantaMaterial =
          new Manta::Phong( this->DiffuseTexture,
                            this->SpecularTexture,
                            static_cast<int> ( this->GetSpecularPower() ),
                            new Manta::Constant<Manta::ColorComponent>
                            ( this->Reflectance ) );
        }
      else
        if ( strcmp( this->MaterialType, "transparent" ) == 0 )
          {
          this->MantaMaterial
            = new Manta::Transparent( this->DiffuseTexture, this->GetOpacity() );
          }
        else
          if ( strcmp( this->MaterialType, "thindielectric" ) == 0 )
            {
            this->MantaMaterial = new Manta::ThinDielectric
              (
               new Manta::Constant<Manta::Real>( this->Eta ),
               this->DiffuseTexture, this->Thickness, 1 );
            }
          else
            if ( strcmp( this->MaterialType, "dielectric" ) == 0 )
              {
              this->MantaMaterial
                = new Manta::Dielectric( new Manta::Constant<Manta::Real>( this->N  ),
                                         new Manta::Constant<Manta::Real>( this->Nt ),
                                         this->DiffuseTexture );
              }
            else
              if ( strcmp( this->MaterialType, "metal" ) == 0 )
                {
                this->MantaMaterial = new Manta::MetalMaterial( this->DiffuseTexture );
                }
              else
                if ( strcmp( this->MaterialType, "orennayer" ) == 0 )
                  {
                  this->MantaMaterial = new Manta::OrenNayar( this->DiffuseTexture );
                  }
                else
                  {
                  // just default to phong
                  this->MantaMaterial
                    = new Manta::Phong( this->DiffuseTexture,
                                        this->SpecularTexture,
                                        static_cast<int> ( this->GetSpecularPower() ),
                                        new Manta::Constant<Manta::ColorComponent>
                                        (
                                         this->Reflectance) );
                  }
    }

  //cerr << "CREATED " << this->MantaMaterial << endl;
}
