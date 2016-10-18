/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCacheSizeInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCacheSizeInformation.h"

#include "vtkCacheSizeKeeper.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkPVCacheSizeInformation);
//-----------------------------------------------------------------------------
vtkPVCacheSizeInformation::vtkPVCacheSizeInformation()
{
  this->CacheSize = 0;
}

//-----------------------------------------------------------------------------
vtkPVCacheSizeInformation::~vtkPVCacheSizeInformation()
{
}

//-----------------------------------------------------------------------------
void vtkPVCacheSizeInformation::CopyFromObject(vtkObject* obj)
{
  vtkCacheSizeKeeper* csk = vtkCacheSizeKeeper::SafeDownCast(obj);
#ifdef FIXME
  vtkProcessModule* pm = vtkProcessModule::SafeDownCast(obj);
  if (pm)
  {
    csk = pm->GetCacheSizeKeeper();
  }
#endif
  if (!csk)
  {
    vtkErrorMacro("vtkPVCacheSizeInformation requires vtkCacheSizeKeeper to gather info.");
    return;
  }
  this->CacheSize = csk->GetCacheSize();
}

//-----------------------------------------------------------------------------
void vtkPVCacheSizeInformation::CopyToStream(vtkClientServerStream* stream)
{
  stream->Reset();
  *stream << vtkClientServerStream::Reply << this->CacheSize << vtkClientServerStream::End;
}

//-----------------------------------------------------------------------------
void vtkPVCacheSizeInformation::CopyFromStream(const vtkClientServerStream* stream)
{
  this->CacheSize = 0;
  if (!stream->GetArgument(0, 0, &this->CacheSize))
  {
    vtkErrorMacro("Error parsing CacheSize.");
  }
}

//-----------------------------------------------------------------------------
void vtkPVCacheSizeInformation::AddInformation(vtkPVInformation* info)
{
  vtkPVCacheSizeInformation* cinfo = vtkPVCacheSizeInformation::SafeDownCast(info);
  if (!cinfo)
  {
    vtkErrorMacro("AddInformation needs vtkPVCacheSizeInformation.");
    return;
  }
  this->CacheSize = (cinfo->CacheSize > this->CacheSize) ? cinfo->CacheSize : this->CacheSize;
}

//-----------------------------------------------------------------------------
void vtkPVCacheSizeInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CacheSize: " << this->CacheSize << endl;
}
