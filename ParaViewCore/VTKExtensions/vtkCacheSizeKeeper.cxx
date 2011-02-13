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


vtkStandardNewMacro(vtkCacheSizeKeeper);
//-----------------------------------------------------------------------------
vtkCacheSizeKeeper::vtkCacheSizeKeeper()
{
  this->CacheSize = 0;
  this->CacheFull = 0;
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
}
