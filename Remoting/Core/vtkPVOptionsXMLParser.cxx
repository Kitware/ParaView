// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

// Hide PARAVIEW_DEPRECATED_IN_5_12_0() warnings for this class.
#define PARAVIEW_DEPRECATION_LEVEL 0

#include "vtkPVOptionsXMLParser.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptions.h"

#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVOptionsXMLParser);

//----------------------------------------------------------------------------
void vtkPVOptionsXMLParser::SetProcessType(const char* ptype)
{
  if (!ptype)
  {
    this->SetProcessTypeInt(vtkCommandOptions::EVERYBODY);
    return;
  }

  std::string type = vtksys::SystemTools::LowerCase((ptype ? ptype : ""));
  if (type == "client")
  {
    this->SetProcessTypeInt(vtkPVOptions::PVCLIENT);
    return;
  }
  if (type == "server")
  {
    this->SetProcessTypeInt(vtkPVOptions::PVSERVER);
    return;
  }
  if (type == "render-server" || type == "renderserver")
  {
    this->SetProcessTypeInt(vtkPVOptions::PVRENDER_SERVER);
    return;
  }

  if (type == "data-server" || type == "dataserver")
  {
    this->SetProcessTypeInt(vtkPVOptions::PVDATA_SERVER);
    return;
  }

  if (type == "paraview")
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
