/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODPartDisplayInformation.cxx
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
#include "vtkPVLODPartDisplayInformation.h"

#include "vtkClientServerStream.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkQuadricClustering.h"

vtkStandardNewMacro(vtkPVLODPartDisplayInformation);
vtkCxxRevisionMacro(vtkPVLODPartDisplayInformation, "1.3");

//----------------------------------------------------------------------------
vtkPVLODPartDisplayInformation::vtkPVLODPartDisplayInformation()
{
}

//----------------------------------------------------------------------------
vtkPVLODPartDisplayInformation::~vtkPVLODPartDisplayInformation()
{
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplayInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "GeometryMemorySize: " << this->GeometryMemorySize << "\n";
  os << indent << "LODGeometryMemorySize: "
     << this->LODGeometryMemorySize << "\n";
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplayInformation::CopyFromObject(vtkObject* obj)
{
  this->GeometryMemorySize = 0;
  this->LODGeometryMemorySize = 0;
  vtkQuadricClustering* deci = vtkQuadricClustering::SafeDownCast(obj);
  if(!deci)
    {
    vtkErrorMacro("Could not downcast decimation filter.");
    return;
    }

  // Get the data object form the decimate filter.  This is a bit of a
  // hack. Maybe we should have a PVPart object on all processes.
  // Sanity checks to avoid slim chance of segfault.
  vtkDataObject** inputs = deci->GetInputs();
  vtkDataObject** outputs = deci->GetOutputs();
  if(!outputs || !outputs[0])
    {
    vtkErrorMacro("Could not get deci output.");
    return;
    }
  if(!inputs || !inputs[0])
    {
    vtkErrorMacro("Could not get deci input.");
    return;
    }
  vtkDataObject* geoData = inputs[0];
  vtkDataObject* deciData = outputs[0];

  this->GeometryMemorySize = geoData->GetActualMemorySize();
  this->LODGeometryMemorySize = deciData->GetActualMemorySize();
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplayInformation::AddInformation(vtkPVInformation* info)
{
  vtkPVLODPartDisplayInformation* pdInfo =
    vtkPVLODPartDisplayInformation::SafeDownCast(info);
  if(!pdInfo)
    {
    vtkErrorMacro("Cannot downcast to LODPartDisplay information.");
    return;
    }
  this->GeometryMemorySize += pdInfo->GetGeometryMemorySize();
  this->LODGeometryMemorySize += pdInfo->GetLODGeometryMemorySize();
}

//----------------------------------------------------------------------------
void
vtkPVLODPartDisplayInformation::CopyToStream(vtkClientServerStream* css) const
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->GeometryMemorySize << this->LODGeometryMemorySize;
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void
vtkPVLODPartDisplayInformation
::CopyFromStream(const vtkClientServerStream* css)
{
  if(!css->GetArgument(0, 0, &this->GeometryMemorySize))
    {
    vtkErrorMacro("Error parsing geometry memory size from message.");
    return;
    }
  if(!css->GetArgument(0, 1, &this->LODGeometryMemorySize))
    {
    vtkErrorMacro("Error parsing LOD geometry memory size from message.");
    return;
    }
}
