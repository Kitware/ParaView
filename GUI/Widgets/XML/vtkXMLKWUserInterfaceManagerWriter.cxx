/*=========================================================================

  Module:    vtkXMLKWUserInterfaceManagerWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLKWUserInterfaceManagerWriter.h"

#include "vtkKWUserInterfaceManager.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkXMLKWUserInterfaceManagerWriter);
vtkCxxRevisionMacro(vtkXMLKWUserInterfaceManagerWriter, "1.2");

//----------------------------------------------------------------------------
char* vtkXMLKWUserInterfaceManagerWriter::GetRootElementName()
{
  return "KWUserInterfaceManager";
}
