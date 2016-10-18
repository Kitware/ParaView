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
// .NAME vtkMantaActor - vtkActor for Manta Ray traced scenes
// .SECTION Description
// vtkMantaActor is a concrete implementation of the abstract class vtkActor.
// vtkMantaActor interfaces to the Manta Raytracer library.

#ifndef vtkMantaActor_h
#define vtkMantaActor_h

#include "vtkActor.h"
#include "vtkMantaModule.h"

namespace Manta
{
class Group;
class AccelerationStructure;
class Object;
};

class vtkMantaManager;
class vtkMantaTexture;
class vtkTimeStamp;

class VTKMANTA_EXPORT vtkMantaActor : public vtkActor
{
public:
  static vtkMantaActor* New();
  vtkTypeMacro(vtkMantaActor, vtkActor);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overriden to help ensure that a Manta compatible class is created.
  vtkProperty* MakeProperty();

  // Description:
  // This causes the actor to be rendered. It in turn will render the actor's
  // property, texture map and then mapper. If a property hasn't been
  // assigned, then the actor will create one automatically. Note that a side
  // effect of this method is that the pipeline will be updated.
  void Render(vtkRenderer* ren, vtkMapper* mapper);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow*);

  // Description:
  // Overridden to schedule a transaction to hide the object
  virtual void SetVisibility(int);

  // Description:
  // Transaction callback that hides the object
  void RemoveObjects();

  // Description:
  // Overridden to swap in a manta texture
  virtual void SetTexture(vtkTexture*);

  // TODO: This leaks whatever was there, but must schedule its
  // deletion because of threading
  void SetGroup(Manta::Group* group);
  Manta::Group* GetGroup() { return this->Group; }
  Manta::AccelerationStructure* GetMantaAS() { return this->MantaAS; }

  // Description:
  // Lets you choose the manta space sorting (acceleration) structure
  // type used internally. Default is 0=DYNBVH
  vtkSetMacro(SortType, int);
  vtkGetMacro(SortType, int);

  // Description:
  // Accessor to the manta texture that backs the vtk texture
  virtual void SetMantaTexture(vtkMantaTexture*);
  vtkGetObjectMacro(MantaTexture, vtkMantaTexture);

protected:
  vtkMantaActor();
  ~vtkMantaActor();

private:
  vtkMantaActor(const vtkMantaActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMantaActor&) VTK_DELETE_FUNCTION;

  void UpdateObjects(vtkRenderer*);
  vtkTimeStamp MeshMTime;

  int SortType;

  enum
  {
    DYNBVH,
    RECURSIVEGRID3
  };
  Manta::Group* Group;                   // geometry
  Manta::AccelerationStructure* MantaAS; // acceleration structure for that geometry

  vtkMantaTexture* MantaTexture;
  vtkMantaManager* MantaManager;
};

#endif
