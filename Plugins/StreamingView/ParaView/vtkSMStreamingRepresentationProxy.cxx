/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkSMStreamingRepresentationProxy.cxx

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

#include "vtkSMStreamingRepresentationProxy.h"

#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMOutputPort.h"

vtkStandardNewMacro(vtkSMStreamingRepresentationProxy);

//-----------------------------------------------------------------------------
vtkSMStreamingRepresentationProxy::vtkSMStreamingRepresentationProxy()
{
  this->PieceCache = NULL;
  this->Harness = NULL;
}

//-----------------------------------------------------------------------------
vtkSMStreamingRepresentationProxy::~vtkSMStreamingRepresentationProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMStreamingRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkSMStreamingRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  this->PieceCache =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PieceCache"));

  this->Harness =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("Harness"));

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream
    << vtkClientServerStream::Invoke
    << this->GetID()
    << "SetPieceCache"
    << this->PieceCache->GetID()
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->GetID()
    << "SetHarness"
    << this->Harness->GetID()
    << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::SERVERS,
                 stream);
}

//------------------------------------------------------------------------------
void vtkSMStreamingRepresentationProxy::AddInput
  (unsigned int inputPort,
   vtkSMSourceProxy *input,
   unsigned int outputPort,
   const char* method)
{
  if (!this->ObjectsCreated)
    {
    this->CreateVTKObjects();
    }
  this->PieceCache->AddInput(inputPort, input, outputPort, method);
  this->Harness->AddInput(0, this->PieceCache, 0, "SetInputConnection");
  this->Superclass::AddInput(0, this->Harness, 0, "SetInputConnection");
}
