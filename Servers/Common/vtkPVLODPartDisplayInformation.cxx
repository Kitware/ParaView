/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODPartDisplayInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
  if (obj == 0)
    {
    return;
    }
  vtkQuadricClustering* deci = vtkQuadricClustering::SafeDownCast(obj);
  if(!deci)
    {
    vtkErrorMacro("Could not downcast decimation filter.");
    return;
    }

  // Get the data object form the decimate filter.  This is a bit of a
  // hack. Maybe we should have a PVPart object on all processes.
  // Sanity checks to avoid slim chance of segfault.
  vtkDataObject* geoData = deci->GetInput();
  vtkDataObject* deciData = deci->GetOutput();

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
