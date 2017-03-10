/*=========================================================================

  Program:   ParaView
  Module:    vtkLoadStateOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLoadStateOptions.h"

#include "vtkObjectFactory.h"
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkLoadStateOptions);
//----------------------------------------------------------------------------
vtkLoadStateOptions::vtkLoadStateOptions()
{
}

//----------------------------------------------------------------------------
vtkLoadStateOptions::~vtkLoadStateOptions()
{
}

//----------------------------------------------------------------------------
void vtkLoadStateOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
std::string vtkLoadStateOptions::LocateFileInDirectory(const std::string& filepath)
{
  std::string result = "";
  vtksys::SystemTools::LocateFileInDir(filepath.c_str(), this->DataDirectory.c_str(), result);
  return result;
}
