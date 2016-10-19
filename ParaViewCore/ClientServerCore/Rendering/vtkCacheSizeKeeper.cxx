/*=========================================================================

  Program:   ParaView
  Module:    vtkCacheSizeKeeper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCacheSizeKeeper.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

//----------------------------------------------------------------------------
// Can't use vtkStandardNewMacro since it adds the instantiator function which
// does not compile since vtkClientServerInterpreterInitializer::New() is
// protected.
vtkCacheSizeKeeper* vtkCacheSizeKeeper::New()
{
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCacheSizeKeeper");
  if (ret)
  {
    return static_cast<vtkCacheSizeKeeper*>(ret);
  }
  vtkCacheSizeKeeper* o = new vtkCacheSizeKeeper;
  o->InitializeObjectBase();
  return o;
}

//-----------------------------------------------------------------------------
vtkCacheSizeKeeper* vtkCacheSizeKeeper::GetInstance()
{
  static vtkSmartPointer<vtkCacheSizeKeeper> Singleton;
  if (Singleton.GetPointer() == NULL)
  {
    Singleton.TakeReference(vtkCacheSizeKeeper::New());
  }
  return Singleton.GetPointer();
}

//-----------------------------------------------------------------------------
vtkCacheSizeKeeper::vtkCacheSizeKeeper()
{
  this->CacheSize = 0;
  this->CacheFull = 0;
  this->CacheLimit = 100 * 1024; // 100 MBs.
}

//-----------------------------------------------------------------------------
vtkCacheSizeKeeper::~vtkCacheSizeKeeper()
{
}

//-----------------------------------------------------------------------------
void vtkCacheSizeKeeper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CacheSize: " << this->CacheSize << endl;
  os << indent << "CacheFull: " << this->CacheFull << endl;
  os << indent << "CacheLimit: " << this->CacheLimit << endl;
}
