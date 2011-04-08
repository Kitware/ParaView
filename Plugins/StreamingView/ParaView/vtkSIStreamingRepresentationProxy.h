/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkSIStreamingRepresentationProxy.h

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
// .NAME vtkStreamingRepresentationProxy - representation that renders in
// a streaming fashion
// .SECTION Description
// A Representation that sets up the display pipeline so that it iterates
// to render. At no time is the entire dataset resident in memory. Different
// configurations of this view render in sequential, prioritized, and
// multiresolution progressions.

#ifndef __vtkSIStreamingRepresentationProxy_h
#define __vtkSIStreamingRepresentationProxy_h

#include "vtkSIPVRepresentationProxy.h"

class VTK_EXPORT vtkSIStreamingRepresentationProxy :
  public vtkSIPVRepresentationProxy
{
public:
  static vtkSIStreamingRepresentationProxy* New();
  vtkTypeMacro(vtkSIStreamingRepresentationProxy,
               vtkSIPVRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These methods are called to add/remove input connections by
  // vtkSIInputProperty. This indirection makes it possible for subclasses to
  // insert VTK-algorithms in the input pipeline.
  virtual void AddInput( int input_port, vtkAlgorithmOutput* connection,
                         const char* method);

//BTX
protected:
  vtkSIStreamingRepresentationProxy();
  ~vtkSIStreamingRepresentationProxy();

  // Description:
  // Creates the VTK objects. This also parses  the xml definition for proxy to
  // create property-handlers.
  // Returns true if object are (or have been) created successfully.
  // \c message is used to obtain information about what proxy helper this is if
  // the objects need to be created.
  virtual bool CreateVTKObjects(vtkSMMessage* message);



private:
  vtkSIStreamingRepresentationProxy
    (const vtkSIStreamingRepresentationProxy&); // Not implemented.
  void operator=(const vtkSIStreamingRepresentationProxy&); // Not implemented.

//ETX
};


#endif
