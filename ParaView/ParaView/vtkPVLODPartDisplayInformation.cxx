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
#include "vtkObjectFactory.h"
#include "vtkDataObject.h"
#include "vtkQuadricClustering.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLODPartDisplayInformation);
vtkCxxRevisionMacro(vtkPVLODPartDisplayInformation, "1.1");

//----------------------------------------------------------------------------
void vtkPVLODPartDisplayInformation::CopyFromObject(vtkObject* object)
{
  vtkDataObject** dataObjects;
  vtkDataObject* geoData;
  vtkDataObject* deciData;
  vtkSource* deci;

  deci = vtkQuadricClustering::SafeDownCast(object);
  if (deci == NULL)
    {
    vtkErrorMacro("Could not downcast decimation filter.");
    return;
    }

  // Get the data object form the decimate filter.
  // This is a bit of a hack. Maybe we should have a PVPart object
  // on all processes.
  // Sanity checks to avoid slim chance of segfault.
  dataObjects = deci->GetOutputs(); 
  if (dataObjects == NULL || dataObjects[0] == NULL)
    {
    vtkErrorMacro("Could not get deci output.");
    return;
    }
  deciData = dataObjects[0];
  dataObjects = deci->GetInputs(); 
  if (dataObjects == NULL || dataObjects[0] == NULL)
    {
    vtkErrorMacro("Could not get deci input.");
    return;
    }
  geoData = dataObjects[0];

  this->GeometryMemorySize = geoData->GetActualMemorySize();
  this->LODGeometryMemorySize = deciData->GetActualMemorySize();
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplayInformation::CopyFromMessage(unsigned char* msg)
{
  memcpy((unsigned char*)&this->GeometryMemorySize, msg, sizeof(unsigned long));
  msg += sizeof(unsigned long);
  memcpy((unsigned char*)&this->LODGeometryMemorySize, msg, sizeof(unsigned long));  
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplayInformation::AddInformation(vtkPVInformation* info)
{
  vtkPVLODPartDisplayInformation* pdInfo;

  pdInfo = vtkPVLODPartDisplayInformation::SafeDownCast(info);
  this->GeometryMemorySize += pdInfo->GetGeometryMemorySize();
  this->LODGeometryMemorySize += pdInfo->GetLODGeometryMemorySize();
}

//----------------------------------------------------------------------------
int vtkPVLODPartDisplayInformation::GetMessageLength()
{
  return 2 * static_cast<int>(sizeof(unsigned long));
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplayInformation::WriteMessage(unsigned char* msg)
{
  memcpy(msg, (unsigned char*)&this->GeometryMemorySize, sizeof(unsigned long));
  msg += sizeof(unsigned long);
  memcpy(msg, (unsigned char*)&this->LODGeometryMemorySize, sizeof(unsigned long));  
}

//----------------------------------------------------------------------------
void vtkPVLODPartDisplayInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "GeometryMemorySize: " << this->GeometryMemorySize << endl;
  os << indent << "LODGeometryMemorySize: " 
     << this->LODGeometryMemorySize << endl;
}

  



