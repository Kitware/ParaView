/*=========================================================================
  
  Program:   ParaView
  Module:    vtkPVLODActor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPVLODActor - an actor that supports multiple levels of detail
// .SECTION Description
// vtkPVLODActor  is a very simple version of vtkLODActor.  Both
// vtkLODActor and vtkLODProp3D can get confused, and substitute
// LOD mappers when they are not needed.  This just has two mappers:
// full res and LOD, and this actor knows which is which.

// .SECTION see also
// vtkActor vtkRenderer vtkLODProp3D vtkLODActor

#ifndef __vtkPVLODActor_h
#define __vtkPVLODActor_h

#include "vtkActor.h"

class vtkMapper;

class VTK_EXPORT vtkPVLODActor : public vtkActor
{
public:
  vtkTypeMacro(vtkPVLODActor,vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPVLODActor *New();

  // Description:
  // This causes the actor to be rendered. It, in turn, will render the actor's
  // property and then mapper.  
  virtual void Render(vtkRenderer *, vtkMapper *);

  // Description:
  // This method is used internally by the rendering process.
  // We overide the superclass method to properly set the estimated render time.
  int RenderOpaqueGeometry(vtkViewport *viewport);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // This sets the low res input.
  virtual void SetLODMapper(vtkMapper*);
  vtkGetObjectMacro(LODMapper, vtkMapper);

  // Description:
  // This is a bit of a hack.  This returns the last mapper used to render.
  // It does this so that compositing can descide if anything was actually renderered.
  vtkMapper *GetMapper() {return this->SelectMapper();}

  // Description:
  // When this objects gets modified, this method also modifies the object.
  void Modified();
  
  // Description:
  // Shallow copy of an LOD actor. Overloads the virtual vtkProp method.
  void ShallowCopy(vtkProp *prop);

  // Description:
  // Get the bounds of the current mapper.
  float *GetBounds();

protected:
  vtkPVLODActor();
  ~vtkPVLODActor();
  vtkPVLODActor(const vtkPVLODActor&);
  void operator=(const vtkPVLODActor&);

  vtkActor            *Device;
  vtkMapper           *LODMapper;

  vtkMapper *SelectMapper();
};

#endif


