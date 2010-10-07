/*=========================================================================

  Program:   ParaView
  Module:    vtkStreamAlgorithm.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreamAlgorithm.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkStreamAlgorithm);

class Internals
{
public:
  Internals(vtkStreamAlgorithm *owner)
  {
    this->Owner = owner;
  }
  ~Internals()
  {
  }
  vtkStreamAlgorithm *Owner;
};

//----------------------------------------------------------------------------
vtkStreamAlgorithm::vtkStreamAlgorithm()
{
  this->Internal = new Internals(this);
}

//----------------------------------------------------------------------------
vtkStreamAlgorithm::~vtkStreamAlgorithm()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkStreamAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
