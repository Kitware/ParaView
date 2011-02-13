/*=========================================================================

  Module:    vtkPVOptionsXMLParser.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVOptionsXMLParser.h"
#include "vtkPVOptions.h"
#include "vtkObjectFactory.h"
#include "vtkStdString.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVOptionsXMLParser);

//----------------------------------------------------------------------------
void vtkPVOptionsXMLParser::SetProcessType(const char* ptype)
{
  if(!ptype)
    {
    this->SetProcessTypeInt(vtkCommandOptions::EVERYBODY);
    return;
    }
  vtkstd::string type = ptype;
  if(type == "client")
    {
    this->SetProcessTypeInt(vtkPVOptions::PVCLIENT);
    return;
    }
  if(type == "server")
    {
    this->SetProcessTypeInt(vtkPVOptions::PVSERVER);
    return;
    }
  if(type == "render-server")
    {
    this->SetProcessTypeInt(vtkPVOptions::PVRENDER_SERVER);
    return;
    }
  if(type == "data-server")
    {
    this->SetProcessTypeInt(vtkPVOptions::PVDATA_SERVER);
    return;
    }
  if(type == "paraview")
    {
    this->SetProcessTypeInt(vtkPVOptions::PARAVIEW);
    return;
    }

  this->Superclass::SetProcessType(ptype);
}

//----------------------------------------------------------------------------
void vtkPVOptionsXMLParser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
