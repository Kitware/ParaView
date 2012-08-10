/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCSVExporterProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCSVExporterProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSpreadSheetView.h"
#include "vtkSMViewProxy.h"
#include "vtkCSVExporter.h"

vtkStandardNewMacro(vtkSMCSVExporterProxy);
//----------------------------------------------------------------------------
vtkSMCSVExporterProxy::vtkSMCSVExporterProxy()
{
}

//----------------------------------------------------------------------------
vtkSMCSVExporterProxy::~vtkSMCSVExporterProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMCSVExporterProxy::CanExport(vtkSMProxy* proxy)
{
  return (proxy && proxy->GetXMLName() &&
    strcmp(proxy->GetXMLName(), "SpreadSheetView") == 0);
}

//----------------------------------------------------------------------------
void vtkSMCSVExporterProxy::Write()
{
  this->CreateVTKObjects();


  vtkCSVExporter* exporter = vtkCSVExporter::SafeDownCast(this->GetClientSideObject());
  if (!exporter)
    {
    vtkErrorMacro("No vtkCSVExporter.");
    return;
    }

  vtkSpreadSheetView* view = vtkSpreadSheetView::SafeDownCast(
    this->View->GetClientSideObject());
  if (!view)
    {
    vtkErrorMacro("Failed to locate vtkSpreadSheetView.");
    return;
    }
  view->Export(exporter);
}

//----------------------------------------------------------------------------
void vtkSMCSVExporterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


