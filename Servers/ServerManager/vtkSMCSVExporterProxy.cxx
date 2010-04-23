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

#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSpreadSheetRepresentationProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkCSVExporter.h"
#include "vtkTable.h"
#include <math.h>

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

  // Locate first visible representation.
  vtkSMPropertyHelper helper(this->View, "Representations");

  vtkSMSpreadSheetRepresentationProxy* activeRepr = 0;
  unsigned int numReprs = helper.GetNumberOfElements();
  for (unsigned int cc=0; cc < numReprs; cc++)
    {
    vtkSMSpreadSheetRepresentationProxy* repr = 
      vtkSMSpreadSheetRepresentationProxy::SafeDownCast(
        helper.GetAsProxy(cc));
    if (repr && repr->GetVisibility())
      {
      activeRepr = repr;
      break;
      }
    }

  if (!activeRepr)
    {
    vtkWarningMacro("Nothing to write.");
    return;
    }

  vtkCSVExporter* exporter = vtkCSVExporter::SafeDownCast(this->GetClientSideObject());
  if (!exporter || !exporter->Open())
    {
    vtkErrorMacro("No vtkCSVExporter.");
    return;
    }

  bool initialized = false;
  vtkIdType numBlocks = activeRepr->GetNumberOfRequiredBlocks();
  for (vtkIdType blockNo=0; blockNo < numBlocks; blockNo++)
    {
    vtkTable* table = vtkTable::SafeDownCast(activeRepr->GetOutput(blockNo));
    if (table)
      {
      if (!initialized)
        {
        initialized = true;
        exporter->WriteHeader(table->GetRowData());
        }
      exporter->WriteData(table->GetRowData());
      }
    }
  exporter->Close();
}

//----------------------------------------------------------------------------
void vtkSMCSVExporterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


