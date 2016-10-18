/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMantaRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkPVMantaRepresentation.cxx

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
#include "vtkPVMantaRepresentation.h"

#include "vtkCompositePolyDataMapper2.h"
#include "vtkInformation.h"
#include "vtkMantaCompositeMapper.h"
#include "vtkMantaLODActor.h"
#include "vtkMantaPolyDataMapper.h"
#include "vtkMantaProperty.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMantaRepresentation);

//-----------------------------------------------------------------------------
vtkPVMantaRepresentation::vtkPVMantaRepresentation()
{
  this->Mapper->Delete();
  this->Mapper = vtkMantaCompositeMapper::New();
  this->LODMapper->Delete();
  this->LODMapper = vtkMantaCompositeMapper::New();
  this->Actor->Delete();
  this->Actor = vtkMantaLODActor::New();
  this->Property->Delete();
  this->Property = vtkMantaProperty::New();

  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetLODMapper(this->LODMapper);
  this->Actor->SetProperty(this->Property);

  vtkInformation* keys = vtkInformation::New();
  this->Actor->SetPropertyKeys(keys);
  keys->Delete();
}

//-----------------------------------------------------------------------------
vtkPVMantaRepresentation::~vtkPVMantaRepresentation()
{
}

//-----------------------------------------------------------------------------
void vtkPVMantaRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVMantaRepresentation::SetMaterialType(char* newval)
{
  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(this->Property);
  mantaProperty->SetMaterialType(newval);
}

//----------------------------------------------------------------------------
char* vtkPVMantaRepresentation::GetMaterialType()
{
  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(this->Property);
  return mantaProperty->GetMaterialType();
}

//----------------------------------------------------------------------------
void vtkPVMantaRepresentation::SetReflectance(double newval)
{
  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(this->Property);
  mantaProperty->SetReflectance(newval);
}

//----------------------------------------------------------------------------
double vtkPVMantaRepresentation::GetReflectance()
{
  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(this->Property);
  return mantaProperty->GetReflectance();
}

//----------------------------------------------------------------------------
void vtkPVMantaRepresentation::SetThickness(double newval)
{
  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(this->Property);
  mantaProperty->SetThickness(newval);
}

//----------------------------------------------------------------------------
double vtkPVMantaRepresentation::GetThickness()
{
  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(this->Property);
  return mantaProperty->GetThickness();
}

//----------------------------------------------------------------------------
void vtkPVMantaRepresentation::SetEta(double newval)
{
  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(this->Property);
  mantaProperty->SetEta(newval);
}

//----------------------------------------------------------------------------
double vtkPVMantaRepresentation::GetEta()
{
  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(this->Property);
  return mantaProperty->GetEta();
}

//----------------------------------------------------------------------------
void vtkPVMantaRepresentation::SetN(double newval)
{
  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(this->Property);
  mantaProperty->SetN(newval);
}

//----------------------------------------------------------------------------
double vtkPVMantaRepresentation::GetN()
{
  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(this->Property);
  return mantaProperty->GetN();
}

//----------------------------------------------------------------------------
void vtkPVMantaRepresentation::SetNt(double newval)
{
  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(this->Property);
  mantaProperty->SetNt(newval);
}

//----------------------------------------------------------------------------
double vtkPVMantaRepresentation::GetNt()
{
  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(this->Property);
  return mantaProperty->GetNt();
}

//----------------------------------------------------------------------------
void vtkPVMantaRepresentation::SetUpdateTime(double t)
{
  if (t != this->UpdateTime)
  {
    this->Mapper->Modified();
    this->Superclass::SetUpdateTime(t);
  }
}

//----------------------------------------------------------------------------
void vtkPVMantaRepresentation::SetAllowDataMaterial(bool newval)
{
  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(this->Property);
  mantaProperty->SetAllowDataMaterial(newval);
}

//----------------------------------------------------------------------------
bool vtkPVMantaRepresentation::GetAllowDataMaterial()
{
  vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(this->Property);
  return mantaProperty->GetAllowDataMaterial();
}
