/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStreamingRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkPVStreamingRepresentation.cxx

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
// .NAME vtkPVStreamingRepresentation - a representation that causes its
// child representations to stream
// .SECTION Description
// The servermanager makes sure that a vtkPVStreamingRepresentation
// has a piece cache filter and a streaming harness before its own pipeline.
// This class makes sure that those get connected to the streaming driver
// in the View that shows this representation so that the driver can
// stream it.

#ifndef __vtkPVStreamingRepresentation_h
#define __vtkPVStreamingRepresentation_h

#include "vtkPVCompositeRepresentation.h"

class vtkPieceCacheFilter;
class vtkStreamingHarness;

class VTK_EXPORT vtkPVStreamingRepresentation
  : public vtkPVCompositeRepresentation
{
  //*****************************************************************
public:
  static vtkPVStreamingRepresentation* New();
  vtkTypeMacro(vtkPVStreamingRepresentation, vtkPVCompositeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //Access to the immediately upstream pipeline wrangling filters.
  void SetPieceCache(vtkPieceCacheFilter *);
  vtkGetObjectMacro(PieceCache, vtkPieceCacheFilter);
  void SetHarness(vtkStreamingHarness *);
  vtkGetObjectMacro(Harness, vtkStreamingHarness);

  // Description:
  // Overridden to make sure that the upstream filters flow when we do
  void MarkModified();

  // Description:
  // Overridden to take the streaming harness out of consideration when the rep is
  // not shown.
  virtual void SetVisibility(bool val);

  // Description:
  // Overridden to disable selection representation.
  // ID selection won't work with multires, and the way PV sets up this proxy
  // precludes adding the harness in front of it in order to force streamed updates.
  virtual void SetSelectionVisibility(bool visible);

//BTX
protected:
  vtkPVStreamingRepresentation();
  ~vtkPVStreamingRepresentation();

  // Description:
  // Overridden to associate the streaming view's driver with this
  // represention's harness.
  virtual bool AddToView(vtkView*);

  // Description:
  // Overridded to release the association from the streaming view's driver
  // and this represention's harness.
  virtual bool RemoveFromView(vtkView*);

  vtkStreamingHarness *Harness;
  vtkPieceCacheFilter *PieceCache;

private:
  vtkPVStreamingRepresentation
    (const vtkPVStreamingRepresentation&); // Not implemented
  void operator=(const vtkPVStreamingRepresentation&); // Not implemented
//ETX
};

#endif
