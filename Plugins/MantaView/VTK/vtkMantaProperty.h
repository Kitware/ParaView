/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkMantaProperty.h

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
// .NAME vtkOpenProperty - Manta property
// .SECTION Description
// vtkMantaProperty is a concrete implementation of the abstract class
// vtkProperty. vtkMantaProperty interfaces to the Manta Raytracer library.

#ifndef vtkMantaProperty_h
#define vtkMantaProperty_h

#include "Interface/Texture.h"
#include "vtkMantaModule.h"
#include "vtkProperty.h"

#include <string>

namespace Manta
{
class Material;
// TODO: what should we do to deal with template+namespace?
// class Texture<Color>;
}

class vtkMantaRenderer;
class vtkMantaManager;

class VTKMANTA_EXPORT vtkMantaProperty : public vtkProperty
{
public:
  static vtkMantaProperty* New();
  vtkTypeMacro(vtkMantaProperty, vtkProperty);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Render(vtkActor* a, vtkRenderer* ren);

  // Description:
  // Implement base class method.
  void BackfaceRender(vtkActor* a, vtkRenderer* ren);

  // Description:
  // Release any graphics resources that are being consumed by this
  // property. The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow* win);

  // functions that change parameters of various materials
  vtkSetStringMacro(MaterialType);
  vtkGetStringMacro(MaterialType);
  vtkSetMacro(Reflectance, float);
  vtkGetMacro(Reflectance, float);
  vtkSetMacro(Eta, float);
  vtkGetMacro(Eta, float);
  vtkSetMacro(Thickness, float);
  vtkGetMacro(Thickness, float);
  vtkSetMacro(N, float);
  vtkGetMacro(N, float);
  vtkSetMacro(Nt, float);
  vtkGetMacro(Nt, float);

  vtkSetMacro(MantaMaterial, Manta::Material*);
  vtkGetMacro(MantaMaterial, Manta::Material*);
  vtkSetMacro(DiffuseTexture, Manta::Texture<Manta::Color>*);
  vtkGetMacro(DiffuseTexture, Manta::Texture<Manta::Color>*);
  vtkSetMacro(SpecularTexture, Manta::Texture<Manta::Color>*);
  vtkGetMacro(SpecularTexture, Manta::Texture<Manta::Color>*);

  // Description:
  // Internal callbacks for manta thread use.
  // Do not call them directly.
  void CreateMantaProperty();

  // Description:
  // Given a string specification with name and parameters for a
  // material constructor, makes and returns one. Returns null on
  // error.
  static Manta::Material* ManufactureMaterial(std::string specification);

  // Description:
  // Given two materials, return a material that makes sense as the
  // interface between them.
  static Manta::Material* CombineMaterials(std::string m, std::string d);

  // Description:
  // When true (the default) material properties specified in the data
  // control the appearance. These specification consists of a field associated
  // a field associated string array named "mantaMaterials" that define a
  // some number of materials and a cell associated int array named "mantaMaterial"
  // that chooses from those numbered materials for each cell,
  vtkSetMacro(AllowDataMaterial, bool);
  vtkGetMacro(AllowDataMaterial, bool);

protected:
  vtkMantaProperty();
  ~vtkMantaProperty();

private:
  vtkMantaProperty(const vtkMantaProperty&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMantaProperty&) VTK_DELETE_FUNCTION;

  // the last time MantaMaterial is modified
  vtkTimeStamp MantaMaterialMTime;

  Manta::Material* MantaMaterial;
  Manta::Texture<Manta::Color>* DiffuseTexture;
  Manta::Texture<Manta::Color>* SpecularTexture;

  // type of material to use. possible values are: "lambertian", "phong",
  // "transparent", "thindielectric", "dielectric", "metal", "orennayer"
  char* MaterialType;

  // amount of reflection to use. should be between 0.0 and 1.0
  float Reflectance;

  // the index of refraction for a material. used with the thin dielectric
  // material
  float Eta;

  // how thick the material is. used with the thin dielectric material
  float Thickness;

  // index of refraction for outside material. used in dielectric material
  float N;

  // index of refraction for inside material (transmissive). used in
  // dielectric material
  float Nt;

  vtkMantaManager* MantaManager;

  bool AllowDataMaterial;
};

#endif
