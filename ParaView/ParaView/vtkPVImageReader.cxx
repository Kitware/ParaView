/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVImageReader.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVImageReader, "1.24.2.1");
vtkStandardNewMacro(vtkPVImageReader);

//----------------------------------------------------------------------------
vtkPVImageReader::vtkPVImageReader()
{
  delete[] this->FilePattern;
  this->FilePattern = new char[strlen("%s") + 1];
  strcpy (this->FilePattern, "%s");

  this->FileDimensionality = 3;
}

//----------------------------------------------------------------------------
void vtkPVImageReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

