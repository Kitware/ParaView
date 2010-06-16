/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMantaClientServerViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkSMMantaClientServerViewProxy.h

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
// .NAME vtkMantaClientServerViewProxy - parallel view setup for vtkManta
// .SECTION Description
// A parallel View that sets up the parallel display pipeline so that it
// works with manta.

#ifndef __vtkSMMantaClientServerViewProxy_h
#define __vtkSMMantaClientServerViewProxy_h

#include "vtkSMClientServerRenderViewProxy.h"

class vtkSMRepresentationProxy;

class vtkSMMantaClientServerViewProxy : public vtkSMClientServerRenderViewProxy
{
public:
  static vtkSMMantaClientServerViewProxy* New();
  vtkTypeMacro(vtkSMMantaClientServerViewProxy, vtkSMClientServerRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  //Controls number of render threads.
  virtual void SetThreads(int val);
  vtkGetMacro(Threads, int);

  // Description:
  // Parameters that controls ray tracing quality
  // Defaults are for minimal quality and maximal speed.
  virtual void SetEnableShadows(int val);
  vtkGetMacro(EnableShadows, int);
  virtual void SetSamples(int val);
  vtkGetMacro(Samples, int);
  virtual void SetMaxDepth(int val);
  vtkGetMacro(MaxDepth, int);

  // Description:
  // Create a default representation for the given output port of source proxy.
  // Returns a new proxy.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(
    vtkSMProxy*, int opport);


  // Description:
  // Checks if color depth is sufficient to support selection.
  // If not, will return 0 and any calls to SelectVisibleCells will 
  // quietly return an empty selection.
  virtual bool IsSelectionAvailable() { return false;}

  // Description:
  // Overridden to prevent client side rendering, which isn't ray traced yet, and
  // because switching back and forth has issues.
  virtual void SetRemoteRenderThreshold(double ) 
  {
    this->RemoteRenderThreshold = 0.0;
  }

  // Description:
  // Overridden to prevent LOD, since that causes rebuilds of the accel structure on every frame.
  virtual void SetLODThreshold(double)
  {
    this->LODThreshold = 100000.0;
  }

//BTX
protected:
  vtkSMMantaClientServerViewProxy();
  ~vtkSMMantaClientServerViewProxy();

  // Description:
  // Called at the start of CreateVTKObjects().
  virtual bool BeginCreateVTKObjects();
  virtual void EndCreateVTKObjects();

  // Description:
  // Overridden to prevent screen space downsampling until it works with manta.
  virtual void SetImageReductionFactorInternal(int factor) { return; }

  int EnableShadows;
  int Threads;
  int Samples;
  int MaxDepth;

private:

  vtkSMMantaClientServerViewProxy(const vtkSMMantaClientServerViewProxy&); // Not implemented.
  void operator=(const vtkSMMantaClientServerViewProxy&); // Not implemented.

//ETX
};


#endif

