/*=========================================================================

  Module:    vtkXMLKWUserInterfaceManagerReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLKWUserInterfaceManagerReader.h"

#include "vtkKWUserInterfaceManager.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkXMLKWUserInterfaceManagerReader);
vtkCxxRevisionMacro(vtkXMLKWUserInterfaceManagerReader, "1.2");

//----------------------------------------------------------------------------
char* vtkXMLKWUserInterfaceManagerReader::GetRootElementName()
{
  return "KWUserInterfaceManager";
}
