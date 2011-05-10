/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkSMStreamingViewProxy.h

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
// .NAME vtkSMStreamingViewProxy - paraview control of a view that renders
// in a streaming fashion
// .SECTION Description
// vtkSMStreamingViewProxy controls vtkPVStreamingView instances which
// do the actual multi-pass rendering.

#ifndef __vtkSMStreamingViewProxy_h
#define __vtkSMStreamingViewProxy_h

#include "vtkSMRenderViewProxy.h"

class vtkSMRepresentationProxy;

class VTK_EXPORT vtkSMStreamingViewProxy : public vtkSMRenderViewProxy
{
public:
  static vtkSMStreamingViewProxy* New();
  vtkTypeMacro(vtkSMStreamingViewProxy, vtkSMRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overrridded to create streaming representations and to give the PV
  // Views access to their StreamingDrivers.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(
    vtkSMProxy*, int opport);

  // Description:
  // Disable surface selection since it conflicts with multipass rendering.
  virtual bool IsSelectionAvailable() { return false; }

  // Description:
  // Ask the PVViews if multipass rendering has finished, if not we mark
  // them modified so that the next render executes their pipelines fully.
  bool IsDisplayDone();

  // Description:
  // Allow direct access to the driver proxy to avoid messy exposed
  // properties in the XML.
  vtkSMProxy *GetDriver() { return this->Driver; };

//BTX
protected:
  vtkSMStreamingViewProxy();
  ~vtkSMStreamingViewProxy();

  // Description:
  // Overridden to tun through the full set of passes and to grab the front buffer
  // so the saved image corresponds to what is shown on screen.
  virtual void CaptureWindowInternalRender();

  // Description:
  // Overridden to initialize the CS wrapped VTK streaming library and
  // expose the streaming driver proxy.
  virtual void CreateVTKObjects();

  // Description:
  // The entity that drives streaming.
  vtkSMProxy *Driver;

private:

  vtkSMStreamingViewProxy(const vtkSMStreamingViewProxy&); // Not implemented.
  void operator=(const vtkSMStreamingViewProxy&); // Not implemented.

//ETX
};


#endif
