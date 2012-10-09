/*=========================================================================

  Program:   ParaView
  Module:    vtkCPHelperScripts.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPHelperScripts.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
// String that map the file content of CoProcessing/CoProcessor/cp_helper.py
//----------------------------------------------------------------------------
extern const char* cp_helper_py;
//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkCPHelperScripts);
//----------------------------------------------------------------------------
vtkCPHelperScripts::vtkCPHelperScripts()
{
}

//----------------------------------------------------------------------------
vtkCPHelperScripts::~vtkCPHelperScripts()
{
}

//----------------------------------------------------------------------------
void vtkCPHelperScripts::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
const char* vtkCPHelperScripts::GetPythonHelperScript()
{
  return cp_helper_py;
}
