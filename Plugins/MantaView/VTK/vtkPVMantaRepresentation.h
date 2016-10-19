/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMantaRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkPVMantaRepresentationProxy.h

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
// .NAME vtkPVMantaRepresentation - representation for manta views
// .SECTION Description
// This replaces the GL mapper, actor and property for a display pipline
// with the manta versions of those so that the object can be drawn
// in a manta renderer within a PVMantaView

#ifndef vtkPVMantaRepresentation_h
#define vtkPVMantaRepresentation_h

#include "vtkGeometryRepresentationWithFaces.h"
#include "vtkMantaModule.h"

class VTKMANTA_EXPORT vtkPVMantaRepresentation : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkPVMantaRepresentation* New();
  vtkTypeMacro(vtkPVMantaRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overridden to participate in manta state maintenence
  virtual void SetUpdateTime(double time);

  // Description:
  // control that ray traced rendering characteristics of this object
  void SetMaterialType(char*);
  char* GetMaterialType();
  void SetReflectance(double);
  double GetReflectance();
  void SetThickness(double);
  double GetThickness();
  void SetEta(double);
  double GetEta();
  void SetN(double);
  double GetN();
  void SetNt(double);
  double GetNt();
  void SetAllowDataMaterial(bool);
  bool GetAllowDataMaterial();

protected:
  vtkPVMantaRepresentation();
  ~vtkPVMantaRepresentation();

private:
  vtkPVMantaRepresentation(const vtkPVMantaRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVMantaRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
