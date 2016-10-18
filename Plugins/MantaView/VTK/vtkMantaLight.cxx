/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaLight.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkMantaLight.cxx

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
#include "vtkMantaLight.h"
#include "vtkMantaManager.h"

#include "vtkMantaRenderer.h"

#include "vtkObjectFactory.h"

#include <Engine/Control/RTRT.h>
#include <Interface/Context.h>
#include <Interface/Light.h>
#include <Interface/LightSet.h>
#include <Model/Lights/DirectionalLight.h>
#include <Model/Lights/PointLight.h>

#include <math.h>

vtkStandardNewMacro(vtkMantaLight);

//----------------------------------------------------------------------------
vtkMantaLight::vtkMantaLight()
  : MantaLight(0)
{
  // cerr << "ML(" << this << ") CREATE" << endl;
  this->MantaLight = NULL;
  this->MantaManager = NULL;
  this->MantaPosition[0] = 0.0;
  this->MantaPosition[1] = 0.0;
  this->MantaPosition[2] = 0.0;
}

//----------------------------------------------------------------------------
vtkMantaLight::~vtkMantaLight()
{
  // cerr << "ML(" << this << ") DESTROY" << endl;
  delete this->MantaLight;
  if (this->MantaManager)
  {
    // cerr << "ML(" << this << ") DESTROY " << this->MantaManager << " "
    //     << this->MantaManager->GetReferenceCount() << endl;
    this->MantaManager->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkMantaLight::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkMantaLight::Render(vtkRenderer* ren, int /* not used */)
{
  vtkMantaRenderer* mantaRenderer = vtkMantaRenderer::SafeDownCast(ren);
  if (!mantaRenderer)
  {
    return;
  }

  if (!this->MantaLight)
  {
    mantaRenderer->GetMantaEngine()->addTransaction(
      "create light", Manta::Callback::create(this, &vtkMantaLight::CreateMantaLight, ren));
  }
  else
  {
    mantaRenderer->GetMantaEngine()->addTransaction(
      "update light", Manta::Callback::create(this, &vtkMantaLight::UpdateMantaLight, ren));
  }
}

//----------------------------------------------------------------------------
// called in Transaction context, it is safe to modify the engine state here
void vtkMantaLight::CreateMantaLight(vtkRenderer* ren)
{
  vtkMantaRenderer* mantaRenderer = vtkMantaRenderer::SafeDownCast(ren);
  if (!mantaRenderer)
  {
    return;
  }

  double *color, *position, *focal, direction[3];

  // Manta Lights only have one "color"
  color = this->GetDiffuseColor();
  position = this->GetTransformedPosition();
  focal = this->GetTransformedFocalPoint();

  if (this->GetPositional())
  {
    this->MantaLight = new Manta::PointLight(Manta::Vector(position[0], position[1], position[2]),
      Manta::Color(Manta::RGBColor(color[0], color[1], color[2])));
  }
  else
  {
    // "direction" in Manta means the direction toward light source rather than the
    // direction of rays originate from light source
    direction[0] = position[0] - focal[0];
    direction[1] = position[1] - focal[1];
    direction[2] = position[2] - focal[2];
    this->MantaLight =
      new Manta::DirectionalLight(Manta::Vector(direction[0], direction[1], direction[2]),
        Manta::Color(Manta::RGBColor(color[0], color[1], color[2])));
  }
  mantaRenderer->GetMantaLightSet()->add(this->MantaLight);
  if (!this->MantaManager)
  {
    this->MantaManager = mantaRenderer->GetMantaManager();
    // cerr << "ML(" << this << ") REGISTER " << this->MantaManager << " "
    //     << this->MantaManager->GetReferenceCount() << endl;
    this->MantaManager->Register(this);
  }
}

//------------------------------------------------------------------------------
// called in Transaction context, it is safe to modify the engine state here
void vtkMantaLight::UpdateMantaLight(vtkRenderer* ren)
{
  if (!this->MantaLight)
  {
    return;
  }
  double *color, *position, *focal, direction[3];
  double intens = this->GetIntensity();
  double on = (this->GetSwitch() ? 1.0 : 0.0);

  // Manta Lights only have one "color"
  color = this->GetDiffuseColor();
  position = this->GetTransformedPosition();
  focal = this->GetTransformedFocalPoint();

  double lcolor[3]; // factor in intensity and on/off state for manta API
  lcolor[0] = color[0] * intens * on;
  lcolor[1] = color[1] * intens * on;
  lcolor[2] = color[2] * intens * on;

  if (this->GetPositional())
  {
    Manta::PointLight* pointLight = dynamic_cast<Manta::PointLight*>(this->MantaLight);
    if (pointLight)
    {
      pointLight->setPosition(Manta::Vector(position[0], position[1], position[2]));
      pointLight->setColor(Manta::Color(Manta::RGBColor(lcolor[0], lcolor[1], lcolor[2])));
    }
    else
    {
      vtkWarningMacro(
        << "Changing from Directional to Positional light is not supported by vtkManta");
    }
  }
  else
  {
    Manta::DirectionalLight* dirLight = dynamic_cast<Manta::DirectionalLight*>(this->MantaLight);
    if (dirLight)
    {
      // "direction" in Manta means the direction toward light source rather than the
      // direction of rays originate from light source
      direction[0] = position[0] - focal[0];
      direction[1] = position[1] - focal[1];
      direction[2] = position[2] - focal[2];
      dirLight->setDirection(Manta::Vector(direction[0], direction[1], direction[2]));
      dirLight->setColor(Manta::Color(Manta::RGBColor(lcolor[0], lcolor[1], lcolor[2])));
    }
    else
    {
      vtkWarningMacro(
        << "Changing from Positional to Directional light is not supported by vtkManta");
    }
  }
}

//------------------------------------------------------------------------------
void vtkMantaLight::SetLightType(int type)
{
  if (type == this->GetLightType())
  {
    return;
  }
  this->SetTransformMatrix(NULL);
  this->SetPosition(this->GetMantaPosition());
  this->Superclass::SetLightType(type);
}

//----------------------------------------------------------------------------
void vtkMantaLight::SetMantaPosition(double x, double y, double z)
{
  if (x == this->MantaPosition[0] && y == this->MantaPosition[1] && z == this->MantaPosition[2])
  {
    return;
  }
  this->MantaPosition[0] = x;
  this->MantaPosition[1] = y;
  this->MantaPosition[2] = z;
  this->SetTransformMatrix(NULL);
  this->SetPosition(this->GetMantaPosition());
}
